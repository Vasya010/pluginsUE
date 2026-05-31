#include "UIWorldMenuGameInstance.h"

#include "Blueprint/UserWidget.h"
#include "Engine/Engine.h"
#include "GameFramework/GameModeBase.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerState.h"
#include "Interfaces/OnlineIdentityInterface.h"
#include "Interfaces/OnlineSessionInterface.h"
#include "Interfaces/OnlineIdentityInterface.h"
#include "Interfaces/OnlineAchievementsInterface.h"
#include "OnlineStats.h"
#include "OnlineSubsystemTypes.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Misc/EngineVersion.h"
#include "MoviePlayer.h"
#include "Save/UIWorldSaveManager.h"
#include "Save/UIWorldSaveGame.h"
#include "Character/ZonefallPlayerCharacter.h"
#include "Localization/ZonefallLocalizationSubsystem.h"
#include "UI/ZonefallBootLogosWidget.h"
#include "UI/ZonefallPressStartWidget.h"
#include "UI/ZonefallSaveToastWidget.h"
#include "Online/OnlineSessionNames.h"
#include "OnlineSessionSettings.h"
#include "OnlineSubsystem.h"
#include "SocketSubsystem.h"
#include "UI/ZonefallLoadingScreenWidget.h"
#include "UI/ZonefallMasterSettingsWidget.h"
#include "UI/ZonefallOnlineLobbyWidget.h"
#include "UI/ZonefallPauseMenuWidget.h"
#include "UI/ZonefallSettingsDataObject.h"
#include "UI/ZonefallShaderLoadingWidget.h"
#include "ZonefallShaderCacheSubsystem.h"
#include "HAL/FileManager.h"
#include "HAL/PlatformFileManager.h"
#include "Misc/FileHelper.h"
#include "Misc/ConfigCacheIni.h"
#include "HAL/PlatformProcess.h"
#include "Misc/PackageName.h"
#include "Misc/Paths.h"
#include "Misc/App.h"
#include "UObject/Package.h"
#include "ShaderCompiler.h"
#include "Engine/NetDriver.h"
#include "Engine/GameEngine.h"
#include "Engine/World.h"
#include "GameFramework/PlayerStart.h"
#include "UObject/ConstructorHelpers.h"

DEFINE_LOG_CATEGORY_STATIC(LogUIWorldStartupFlow, Log, All);

UUIWorldMenuGameInstance::UUIWorldMenuGameInstance()
	: bShowStartupIntro(true)
	, StartupIntroDuration(3.0f)
	, bAutoShowMenuOnStart(true)
	, bShowShaderLoadingOnStartup(true)
	, StartupShaderLoadingDuration(3.5f)
	, bUseAdaptiveStartupShaderDelay(true)
	, StartupShaderMaxDuration(45.0f)   // give PSO precompile time to finish before the safety cap
	, bCacheWidgetsByScreen(true)
	, MenuZOrder(100)
	, MainMenuLevelName(TEXT("Menu"))
	// Host into the actual gameplay level — NOT the menu. If the host stays on the
	// "Menu" map, a joining client also lands on a map literally named "Menu", and the
	// abort/safety logic mistakes the successful join for "never left the menu" and
	// kicks the client back. Hosting a real level is what makes online actually play.
	, OnlineHostMapName(TEXT("Levelgames"))
	, OnlineServerName(TEXT("UIWorld Server"))
	, LoadingScreenDelayBeforeOpenLevel(0.12f)
	, bUseAdaptiveLoadingDelay(true)
	, AdaptiveDelayMinSeconds(0.12f)
	, AdaptiveDelayMaxSeconds(1.25f)
	, bUseMapComplexityDelay(true)
	, MapDelayPer100MB(1.25f)
	, MapDelayMaxSeconds(8.0f)
	, bUseMoviePlayerLoadingScreen(true)
	, CurrentMenuScreen(EUIWorldMenuScreen::MainMenu)
	, LastNonSettingsMenuScreen(EUIWorldMenuScreen::MainMenu)
	, PinnedMenuWidget(nullptr)
	, ActiveLoadingScreenWidget(nullptr)
	, ActiveStartupShaderWidget(nullptr)
	, bStartupShaderPhaseActive(false)
	, bStartupShaderPhaseCompleted(false)
	, bApplyGameFocusOnNextMapLoad(false)
	, PendingLevelNameToLoad(NAME_None)
	, bPendingLevelAbsolute(true)
	, StartupShaderPhaseStartSeconds(0.0f)
	, StartupInitialShaderJobs(0)
{
	// Character creator opens on the menu pawn by default (no studio travel / no loading screen).
	// Set CharacterCreatorLevelName in the GI Blueprint only if you want a dedicated preview map.
	CharacterCreatorLevelName = NAME_None;
	GameplayLevelName = TEXT("GameMap");

	static ConstructorHelpers::FClassFinder<AZonefallPlayerCharacter> GameCharecterFinder(
		TEXT("/Game/charecters/gamecharecter"));
	if (GameCharecterFinder.Succeeded())
	{
		CharacterCreatorPawnClass = GameCharecterFinder.Class;
	}
}

namespace
{
	static TSharedPtr<const FUniqueNetId> UIWorldGetOrCreateLanUserId(UWorld* World, bool bLAN);

	/** LAN = local network only (NULL). Steam = internet (never mix the two). */
	static IOnlineSubsystem* UIWorldGetOnlineSubsystem(bool bPreferLAN)
	{
		if (bPreferLAN)
		{
			return IOnlineSubsystem::Get(FName(TEXT("NULL")));
		}
		return IOnlineSubsystem::Get(FName(TEXT("STEAM")));
	}

	static IOnlineSessionPtr UIWorldGetSessionInterface(bool bPreferLAN = false)
	{
		if (IOnlineSubsystem* OSS = UIWorldGetOnlineSubsystem(bPreferLAN))
		{
			return OSS->GetSessionInterface();
		}
		return nullptr;
	}

	/** Both host and client must use LanGameNetDriver (Ip) — not SteamSockets + Ip mixed. */
	static FString UIWorldAppendLanNetDriverOption(FString Url)
	{
		if (Url.Contains(TEXT("NetDriver="), ESearchCase::IgnoreCase))
		{
			return Url;
		}

		// ClientTravel "host:port?Opt" mis-parses the port; engine then uses GameNetDriver (SteamSockets) and crashes/fails in PIE.
		if (!Url.Contains(TEXT("/")) && Url.Contains(TEXT(":")))
		{
			const int32 QueryIdx = Url.Find(TEXT("?"));
			if (QueryIdx != INDEX_NONE)
			{
				Url = Url.Left(QueryIdx) + TEXT("/") + Url.Mid(QueryIdx);
			}
			else
			{
				Url += TEXT("/");
			}
		}

		Url += Url.Contains(TEXT("?")) ? TEXT("&NetDriver=LanGameNetDriver") : TEXT("?NetDriver=LanGameNetDriver");
		return Url;
	}

	static void UIWorldShutdownWorldNetDriver(UWorld* World)
	{
		if (!World || !GEngine)
		{
			return;
		}
		if (UNetDriver* Driver = World->GetNetDriver())
		{
			GEngine->DestroyNamedNetDriver(World, Driver->NetDriverName);
		}
	}

	/** Tear down Steam listen driver in menu before LAN host/join uses Ip only. */
	static void UIWorldActivateLanNetworking(UWorld* World)
	{
		UIWorldShutdownWorldNetDriver(World);
	}

	static void UIWorldActivateSteamNetworking(UWorld* World)
	{
		UIWorldShutdownWorldNetDriver(World);
	}

	/** PIE often loads ?listen maps as Standalone — force a listen socket so LAN/NULL beacons work. */
	static bool UIWorldTryStartListenServer(UWorld* World, bool bLAN, int32 LanPort)
	{
		if (!World || !GEngine)
		{
			return false;
		}

		const ENetMode NetMode = World->GetNetMode();
		if (NetMode == NM_ListenServer || NetMode == NM_DedicatedServer)
		{
			return true;
		}
		if (NetMode != NM_Standalone)
		{
			return false;
		}

		if (World->GetNetDriver())
		{
			return true;
		}

		if (bLAN)
		{
			UIWorldActivateLanNetworking(World);
		}
		else
		{
			UIWorldActivateSteamNetworking(World);
		}

		FURL ListenURL;
		ListenURL.Port = FMath::Clamp(LanPort, 1, 65535);
		ListenURL.AddOption(TEXT("Listen"));
		ListenURL.AddOption(bLAN ? TEXT("NetDriver=LanGameNetDriver") : TEXT("NetDriver=GameNetDriver"));

		const bool bListenOk = World->Listen(ListenURL);
		UE_LOG(
			LogUIWorldStartupFlow,
			Log,
			TEXT("[Online] World->Listen port=%d LAN=%d -> %s (NetMode=%d, driver=%s)"),
			ListenURL.Port,
			bLAN ? 1 : 0,
			bListenOk ? TEXT("OK") : TEXT("FAIL"),
			(int32)World->GetNetMode(),
			World->GetNetDriver() ? *World->GetNetDriver()->GetClass()->GetName() : TEXT("none"));
		return bListenOk && World->GetNetDriver() != nullptr;
	}

	static TSharedPtr<const FUniqueNetId> UIWorldGetLocalUserId(UWorld* World, bool bPreferLAN = false)
	{
		if (!World)
		{
			return nullptr;
		}

		if (bPreferLAN)
		{
			return UIWorldGetOrCreateLanUserId(World, true);
		}

		if (IOnlineSubsystem* OSS = IOnlineSubsystem::Get(FName(TEXT("STEAM"))))
		{
			if (IOnlineIdentityPtr Identity = OSS->GetIdentityInterface())
			{
				if (TSharedPtr<const FUniqueNetId> NetId = Identity->GetUniquePlayerId(0); NetId.IsValid())
				{
					return NetId;
				}
			}
		}

		if (const ULocalPlayer* LocalPlayer = World->GetFirstLocalPlayerFromController())
		{
			const FUniqueNetIdRepl& PreferredId = LocalPlayer->GetPreferredUniqueNetId();
			if (PreferredId.IsValid())
			{
				return PreferredId.GetUniqueNetId();
			}
		}

		// Fallback: PlayerState id (often empty in the main menu before OSS login).
		if (APlayerController* PC = UGameplayStatics::GetPlayerController(World, 0))
		{
			if (PC->PlayerState)
			{
				const FUniqueNetIdRepl& Repl = PC->PlayerState->GetUniqueId();
				if (Repl.IsValid())
				{
					return Repl.GetUniqueNetId();
				}
			}
		}

		return nullptr;
	}

	static FString UIWorldDescribeActiveOnlineSubsystem(bool bPreferLAN = false)
	{
		if (const IOnlineSubsystem* OSS = UIWorldGetOnlineSubsystem(bPreferLAN))
		{
			return OSS->GetSubsystemName().ToString();
		}
		return TEXT("None");
	}

	static FString UIWorldDescribeJoinResult(EOnJoinSessionCompleteResult::Type JoinResult)
	{
		switch (JoinResult)
		{
		case EOnJoinSessionCompleteResult::Success:
			return FString();
		case EOnJoinSessionCompleteResult::SessionIsFull:
			return TEXT("session is full");
		case EOnJoinSessionCompleteResult::SessionDoesNotExist:
			return TEXT("session no longer exists");
		case EOnJoinSessionCompleteResult::CouldNotRetrieveAddress:
			return TEXT("host address not ready — wait until the host is in the map");
		case EOnJoinSessionCompleteResult::AlreadyInSession:
			return TEXT("already in a session — press Leave, then Join again");
		case EOnJoinSessionCompleteResult::UnknownError:
		default:
			return TEXT("join rejected (stale session? press Leave, then Join)");
		}
	}

	static FString UIWorldResolveLocalLanIp()
	{
		ISocketSubsystem* SocketSubsystem = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM);
		if (!SocketSubsystem)
		{
			return TEXT("Unknown");
		}

		bool bCanBindAll = false;
		TSharedRef<FInternetAddr> LocalAddr = SocketSubsystem->GetLocalHostAddr(*GLog, bCanBindAll);
		return LocalAddr->IsValid() ? LocalAddr->ToString(false) : TEXT("Unknown");
	}

	static FString UIWorldStripPieMapPrefix(FString MapName)
	{
		TArray<FString> Parts;
		MapName.ParseIntoArray(Parts, TEXT("_"), true);
		if (Parts.Num() >= 3 && Parts[0].Equals(TEXT("UEDPIE"), ESearchCase::IgnoreCase))
		{
			Parts.RemoveAt(0, 2);
			MapName = FString::Join(Parts, TEXT("_"));
		}
		return MapName;
	}

	static bool UIWorldBuildLanConnectString(
		const UUIWorldMenuGameInstance* GI,
		const FOnlineSessionSearchResult* SearchResult,
		FString& OutConnectString)
	{
		if (!GI)
		{
			return false;
		}

		int32 Port = FMath::Clamp(GI->OnlineLanPort, 1, 65535);
		FString HostIp = TEXT("127.0.0.1");

		if (SearchResult)
		{
			int32 SessionPort = 0;
			if (SearchResult->Session.SessionSettings.Get(FName(TEXT("LAN_PORT")), SessionPort) && SessionPort > 0)
			{
				Port = SessionPort;
			}

			FString SessionHost;
			if (SearchResult->Session.SessionSettings.Get(FName(TEXT("LAN_HOST")), SessionHost) && !SessionHost.IsEmpty())
			{
				HostIp = SessionHost;
			}
		}

		// Same-machine / PIE: always loopback — Wi-Fi IP often fails between two editor instances.
		if (GIsEditor || FPlatformProperties::RequiresCookedData() == false)
		{
			HostIp = TEXT("127.0.0.1");
		}
		else if (HostIp.Equals(TEXT("Unknown")))
		{
			const FString LanIp = UIWorldResolveLocalLanIp();
			HostIp = LanIp.Equals(TEXT("Unknown")) ? TEXT("127.0.0.1") : LanIp;
		}

		// ClientTravel expects "host:port" — the listen server assigns the map after connect.
		OutConnectString = FString::Printf(TEXT("%s:%d"), *HostIp, Port);
		return true;
	}

	static TSharedPtr<const FUniqueNetId> UIWorldGetOrCreateLanUserId(UWorld* World, bool bLAN)
	{
		if (!World || !bLAN)
		{
			return nullptr;
		}

		if (IOnlineSubsystem* OSS = UIWorldGetOnlineSubsystem(true))
		{
			if (IOnlineIdentityPtr Identity = OSS->GetIdentityInterface())
			{
				if (TSharedPtr<const FUniqueNetId> Id = Identity->GetUniquePlayerId(0); Id.IsValid())
				{
					return Id;
				}
				return Identity->CreateUniquePlayerId(TEXT("UIWorldLAN"));
			}
		}

		return nullptr;
	}

	static bool UIWorldIsLanJoinForIndex(
		const TSharedPtr<FOnlineSessionSearch>& Search,
		int32 ResultIndex,
		bool bLastQueryWasLAN)
	{
		if (bLastQueryWasLAN)
		{
			return true;
		}
		if (!Search.IsValid())
		{
			return false;
		}
		if (Search->bIsLanQuery)
		{
			return true;
		}
		if (Search->SearchResults.IsValidIndex(ResultIndex))
		{
			return Search->SearchResults[ResultIndex].Session.SessionSettings.bIsLANMatch;
		}
		return false;
	}

	static void UIWorldNormalizeConnectStringForPIE(FString& ConnectString)
	{
		if (!GIsEditor || ConnectString.IsEmpty())
		{
			return;
		}

		// steam:// and 127.0.0.1 are fine; LAN beacon often advertises Wi-Fi IP (192.168.x.x) which
		// fails between two PIE windows on the same PC (see Zonefallprotocol_2.log ConnectionTimeout).
		if (ConnectString.StartsWith(TEXT("steam.")))
		{
			return;
		}

		FString HostPart;
		FString PathPart;
		ConnectString.Split(TEXT("/"), &HostPart, &PathPart);
		if (HostPart.IsEmpty())
		{
			HostPart = ConnectString;
		}

		FString Address;
		FString PortStr;
		if (!HostPart.Split(TEXT(":"), &Address, &PortStr))
		{
			return;
		}

		if (Address.StartsWith(TEXT("127.")) || Address.Equals(TEXT("localhost"), ESearchCase::IgnoreCase))
		{
			return;
		}

		const int32 Port = PortStr.IsEmpty() ? 7777 : FCString::Atoi(*PortStr);
		ConnectString = FString::Printf(TEXT("127.0.0.1:%d"), FMath::Clamp(Port, 1, 65535));
		UE_LOG(
			LogUIWorldStartupFlow,
			Log,
			TEXT("[Online] PIE: rewrote LAN connect %s:%s -> %s"),
			*Address,
			*PortStr,
			*ConnectString);
	}

	static bool UIWorldResolveSessionConnectString(
		const UUIWorldMenuGameInstance* GI,
		const IOnlineSessionPtr& Sessions,
		FName SessionName,
		const FOnlineSessionSearchResult* SearchResult,
		bool bPreferLAN,
		FString& OutConnectString)
	{
		const bool bLanSession = bPreferLAN || (SearchResult && SearchResult->Session.SessionSettings.bIsLANMatch);

		// LAN / same-PC: never trust OSS beacon IP (192.168.x.x) before our loopback builder.
		if (bLanSession)
		{
			if (UIWorldBuildLanConnectString(GI, SearchResult, OutConnectString))
			{
				UIWorldNormalizeConnectStringForPIE(OutConnectString);
				return true;
			}
		}

		if (Sessions.IsValid())
		{
			if (Sessions->GetResolvedConnectString(SessionName, OutConnectString) && !OutConnectString.IsEmpty())
			{
				UIWorldNormalizeConnectStringForPIE(OutConnectString);
				return true;
			}

			if (SearchResult && Sessions->GetResolvedConnectString(*SearchResult, NAME_GamePort, OutConnectString) && !OutConnectString.IsEmpty())
			{
				UIWorldNormalizeConnectStringForPIE(OutConnectString);
				return true;
			}
		}

		return false;
	}

	static FString UIWorldGetProjectBuildId(const UUIWorldMenuGameInstance* GI)
	{
		if (GI && !GI->OnlineGameBuildId.IsEmpty())
		{
			return GI->OnlineGameBuildId;
		}
		return TEXT("1");
	}

	static FString UIWorldShortMapLabel(const FString& MapPath)
	{
		if (MapPath.IsEmpty())
		{
			return TEXT("Unknown map");
		}
		FString Label = MapPath;
		Label.ReplaceInline(TEXT("/Game/"), TEXT(""));
		Label.ReplaceInline(TEXT("Levels/"), TEXT(""));
		Label.ReplaceInline(TEXT("levels/"), TEXT(""));
		const int32 Slash = Label.Find(TEXT("/"), ESearchCase::IgnoreCase, ESearchDir::FromEnd);
		if (Slash != INDEX_NONE)
		{
			Label = Label.Mid(Slash + 1);
		}
		return Label;
	}

	static FUIWorldOnlineSessionResult UIWorldConvertSearchResult(
		const UUIWorldMenuGameInstance* GI,
		const FOnlineSessionSearchResult& Result,
		int32 SearchIndex)
	{
		FUIWorldOnlineSessionResult Out;
		Out.SearchResultIndex = SearchIndex;
		Out.SessionId = Result.GetSessionIdStr();
		Out.OwningUserName = Result.Session.OwningUserName;
		Out.PingMs = Result.PingInMs;
		Out.bIsLAN = Result.Session.SessionSettings.bIsLANMatch;

		// NumOpenPublicConnections = free slots; NumPublicConnections = max seats (Steam may send 0).
		const int32 OpenPublic = Result.Session.NumOpenPublicConnections;
		Out.MaxPlayers = FMath::Max(1, Result.Session.SessionSettings.NumPublicConnections);
		if (Out.MaxPlayers <= 1 && OpenPublic > 0)
		{
			Out.MaxPlayers = OpenPublic + 1;
		}
		Out.CurrentPlayers = (OpenPublic >= 0)
			? FMath::Clamp(Out.MaxPlayers - OpenPublic, 0, Out.MaxPlayers)
			: 1;
		Out.bIsFull = OpenPublic <= 0;

		FString ServerName;
		if (Result.Session.SessionSettings.Get(FName(TEXT("SERVER_NAME")), ServerName))
		{
			Out.ServerName = ServerName;
		}
		else
		{
			Out.ServerName = Out.OwningUserName.IsEmpty() ? TEXT("Session") : Out.OwningUserName;
		}

		FString MapName;
		if (Result.Session.SessionSettings.Get(FName(TEXT("MAP_NAME")), MapName))
		{
			Out.MapDisplayName = UIWorldShortMapLabel(MapName);
		}

		FString SessionPassword;
		if (Result.Session.SessionSettings.Get(FName(TEXT("SESSION_PASSWORD")), SessionPassword)
			&& !SessionPassword.IsEmpty())
		{
			Out.bPasswordProtected = true;
		}

		FString HostBuild;
		if (Result.Session.SessionSettings.Get(FName(TEXT("BUILD_ID")), HostBuild))
		{
			Out.BuildId = HostBuild;
			Out.bBuildCompatible = HostBuild.Equals(UIWorldGetProjectBuildId(GI), ESearchCase::IgnoreCase);
		}
		else
		{
			Out.bBuildCompatible = true;
		}

		return Out;
	}

	static bool UIWorldValidateSessionJoin(
		const UUIWorldMenuGameInstance* GI,
		const FOnlineSessionSearchResult* SearchResult,
		FString& OutError)
	{
		if (!SearchResult)
		{
			OutError = TEXT("Join failed: invalid session");
			return false;
		}

		const FUIWorldOnlineSessionResult View = UIWorldConvertSearchResult(GI, *SearchResult, 0);
		if (View.bIsFull)
		{
			OutError = TEXT("Join failed: session is full");
			return false;
		}
		if (!View.bBuildCompatible)
		{
			OutError = FString::Printf(
				TEXT("Join failed: game version mismatch (host %s, you %s)"),
				*View.BuildId,
				*UIWorldGetProjectBuildId(GI));
			return false;
		}

		FString SessionPassword;
		if (SearchResult->Session.SessionSettings.Get(FName(TEXT("SESSION_PASSWORD")), SessionPassword)
			&& !SessionPassword.IsEmpty())
		{
			const FString Attempt = GI ? GI->PendingJoinPassword : FString();
			if (!SessionPassword.Equals(Attempt, ESearchCase::CaseSensitive))
			{
				OutError = TEXT("Join failed: wrong session password");
				return false;
			}
		}

		return true;
	}

	/** Short names like "1" or "Menu" work in PIE but often fail in packaged builds; resolve to a full /Game/... map package when possible. */
	static FName UIWorldResolveLevelNameForOpen(UWorld* World, FName LevelName)
	{
		if (LevelName.IsNone())
		{
			return NAME_None;
		}

		FString Raw = LevelName.ToString();
		if (Raw.IsEmpty())
		{
			return NAME_None;
		}

		{
			const int32 DotIdx = Raw.Find(TEXT("."), ESearchCase::CaseSensitive, ESearchDir::FromStart);
			if (DotIdx != INDEX_NONE)
			{
				const FString MaybePackage = Raw.Left(DotIdx);
				if (FPackageName::DoesPackageExist(MaybePackage))
				{
					return FName(*MaybePackage);
				}
			}
		}

		if (Raw.StartsWith(TEXT("/")))
		{
			if (FPackageName::DoesPackageExist(Raw))
			{
				return LevelName;
			}
			const FString AsPackage = FPackageName::ObjectPathToPackageName(Raw);
			if (AsPackage != Raw && FPackageName::DoesPackageExist(AsPackage))
			{
				return FName(*AsPackage);
			}
			UE_LOG(
				LogUIWorldStartupFlow,
				Warning,
				TEXT("[LevelLoad] Package not found for '%s'. Ensure the map is cooked (Project Settings > Packaging / Asset Manager)."),
				*Raw
			);
			return LevelName;
		}

		static const TCHAR* Prefixes[] = {
			TEXT("/Game/"),
			TEXT("/Game/Maps/"),
			TEXT("/Game/Levels/"),
			TEXT("/Game/levels/"),
			TEXT("/Game/ThirdPerson/"),
		};

		for (const TCHAR* Prefix : Prefixes)
		{
			const FString Candidate = FString::Printf(TEXT("%s%s"), Prefix, *Raw);
			if (FPackageName::DoesPackageExist(Candidate))
			{
				UE_LOG(LogUIWorldStartupFlow, Log, TEXT("[LevelLoad] Resolved '%s' -> '%s'"), *Raw, *Candidate);
				return FName(*Candidate);
			}
		}

		if (World && World->GetOutermost())
		{
			const FString OuterName = World->GetOutermost()->GetName();
			const FString Dir = FPackageName::GetLongPackagePath(OuterName);
			if (!Dir.IsEmpty())
			{
				const FString SameFolder = FPaths::Combine(Dir, Raw);
				if (FPackageName::DoesPackageExist(SameFolder))
				{
					UE_LOG(LogUIWorldStartupFlow, Log, TEXT("[LevelLoad] Resolved '%s' -> '%s' (next to %s)"), *Raw, *SameFolder, *OuterName);
					return FName(*SameFolder);
				}
			}
		}

		UE_LOG(
			LogUIWorldStartupFlow,
			Warning,
			TEXT("[LevelLoad] Could not resolve short map name '%s'. Use a full path (e.g. /Game/Levels/MyMap) and include the map in packaging."),
			*Raw
		);
		return LevelName;
	}
}

void UUIWorldMenuGameInstance::Init()
{
	Super::Init();
	FCoreUObjectDelegates::PostLoadMapWithWorld.AddUObject(this, &UUIWorldMenuGameInstance::HandlePostLoadMap);

	// Recover from failed online travel/connection instead of leaving a loading screen up forever.
	if (GEngine)
	{
		TravelFailureDelegateHandle = GEngine->OnTravelFailure().AddUObject(this, &UUIWorldMenuGameInstance::HandleTravelFailure);
		NetworkFailureDelegateHandle = GEngine->OnNetworkFailure().AddUObject(this, &UUIWorldMenuGameInstance::HandleNetworkFailure);
	}

	LoadLocalAchievements();
	EnsureOnlineAutoLogin();

	// Live language switching: when the player changes language, rebuild the active menu
	// so its self-assembling text reappears in the new language immediately.
	if (UZonefallLocalizationSubsystem* Loc = GetSubsystem<UZonefallLocalizationSubsystem>())
	{
		Loc->OnLanguageChanged.AddDynamic(this, &UUIWorldMenuGameInstance::HandleLanguageChanged);
	}
}

void UUIWorldMenuGameInstance::HandleLanguageChanged(EZonefallLanguage /*NewLanguage*/)
{
	if (!PinnedMenuWidget)
	{
		return;
	}

	// Defer to next tick: this fires from inside a combo-box selection callback on the very
	// widget we're about to destroy/rebuild, so rebuilding synchronously would be a
	// use-after-free. Next tick the Slate callback has fully unwound.
	if (UWorld* World = GetWorld())
	{
		const EUIWorldMenuScreen ScreenToRebuild = CurrentMenuScreen;
		World->GetTimerManager().SetTimerForNextTick([WeakThis = TWeakObjectPtr<UUIWorldMenuGameInstance>(this), ScreenToRebuild]()
		{
			if (UUIWorldMenuGameInstance* Self = WeakThis.Get())
			{
				Self->MenuWidgetCache.Empty();
				Self->PinnedMenuWidget = nullptr;
				Self->ShowMenuFromList(ScreenToRebuild, /*bForceRebuild*/ true);
			}
		});
	}
}

void UUIWorldMenuGameInstance::OnStart()
{
	Super::OnStart();

	bStartupShaderPhaseActive = false;
	bStartupShaderPhaseCompleted = false;

	// Restore persisted upscaler settings early (FSR/DLSS/FG CVars often reset between runs).
	// This is especially important in packaged builds where CVars fall back to defaults on launch.
	{
		UZonefallSettingsDataObject* StartupSettings = NewObject<UZonefallSettingsDataObject>(this);
		StartupSettings->SetDefaults();
		StartupSettings->LoadUpscalerSettingsFromConfig();
		StartupSettings->ApplyUpscalerSettingsOnly(this);
	}

	UE_LOG(
		LogUIWorldStartupFlow,
		Log,
		TEXT("OnStart: bShowShaderLoadingOnStartup=%d ShaderLoadingWidgetClass=%s bAutoShowMenuOnStart=%d"),
		bShowShaderLoadingOnStartup,
		*GetNameSafe(ShaderLoadingWidgetClass.Get()),
		bAutoShowMenuOnStart
	);

	// Animated game-title splash plays first, then chains into the shader phase / menu.
	if (bShowStartupIntro && StartupIntroWidgetClass)
	{
		BeginStartupIntroPhase();
		return;
	}

	if (bShowShaderLoadingOnStartup && ShaderLoadingWidgetClass)
	{
		BeginStartupShaderPhase();
		return;
	}

	if (bAutoShowMenuOnStart)
	{
		ShowMenuFromList(EUIWorldMenuScreen::MainMenu, false);
	}
}

void UUIWorldMenuGameInstance::BeginStartupIntroPhase()
{
	UWorld* World = GetWorld();
	if (!World || !StartupIntroWidgetClass)
	{
		BeginStartupShaderPhase();
		return;
	}

	APlayerController* PC = UGameplayStatics::GetPlayerController(World, 0);
	if (!PC)
	{
		BeginStartupShaderPhase();
		return;
	}

	ActiveStartupIntroWidget = CreateWidget<UUserWidget>(PC, StartupIntroWidgetClass);
	if (!ActiveStartupIntroWidget)
	{
		BeginStartupShaderPhase();
		return;
	}

	ActiveStartupIntroWidget->AddToViewport(MenuZOrder + 1300);
	UE_LOG(LogUIWorldStartupFlow, Log, TEXT("BeginStartupIntroPhase: showing title splash for %.1fs"), StartupIntroDuration);

	World->GetTimerManager().ClearTimer(StartupIntroTimerHandle);
	World->GetTimerManager().SetTimer(
		StartupIntroTimerHandle, this, &UUIWorldMenuGameInstance::FinishStartupIntroPhase,
		FMath::Clamp(StartupIntroDuration, 0.5f, 15.0f), false);
}

void UUIWorldMenuGameInstance::FinishStartupIntroPhase()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(StartupIntroTimerHandle);
	}
	if (ActiveStartupIntroWidget)
	{
		ActiveStartupIntroWidget->RemoveFromParent();
		ActiveStartupIntroWidget = nullptr;
	}

	// Continue the normal startup chain.
	if (bShowShaderLoadingOnStartup && ShaderLoadingWidgetClass)
	{
		BeginStartupShaderPhase();
		return;
	}
	if (bAutoShowMenuOnStart)
	{
		ShowMenuFromList(EUIWorldMenuScreen::MainMenu, false);
	}
}

void UUIWorldMenuGameInstance::Shutdown()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(StartupShaderTimerHandle);
		World->GetTimerManager().ClearTimer(StartupIntroTimerHandle);
	}
	if (ActiveStartupShaderWidget)
	{
		ActiveStartupShaderWidget->RemoveFromParent();
		ActiveStartupShaderWidget = nullptr;
	}
	if (ActiveStartupIntroWidget)
	{
		ActiveStartupIntroWidget->RemoveFromParent();
		ActiveStartupIntroWidget = nullptr;
	}

	if (IOnlineSubsystem* OSS = IOnlineSubsystem::Get())
	{
		if (IOnlineIdentityPtr Identity = OSS->GetIdentityInterface())
		{
			if (OnlineLoginDelegateHandle.IsValid())
			{
				Identity->ClearOnLoginCompleteDelegate_Handle(0, OnlineLoginDelegateHandle);
				OnlineLoginDelegateHandle.Reset();
			}
		}
	}

	// Unbind any outstanding online-session listeners so they can't fire into a torn-down instance.
	if (IOnlineSessionPtr Sessions = UIWorldGetSessionInterface())
	{
		if (CreateSessionDelegateHandle.IsValid())
		{
			Sessions->ClearOnCreateSessionCompleteDelegate_Handle(CreateSessionDelegateHandle);
			CreateSessionDelegateHandle.Reset();
		}
		if (FindSessionsDelegateHandle.IsValid())
		{
			Sessions->ClearOnFindSessionsCompleteDelegate_Handle(FindSessionsDelegateHandle);
			FindSessionsDelegateHandle.Reset();
		}
		if (JoinSessionDelegateHandle.IsValid())
		{
			Sessions->ClearOnJoinSessionCompleteDelegate_Handle(JoinSessionDelegateHandle);
			JoinSessionDelegateHandle.Reset();
		}
		if (DestroySessionDelegateHandle.IsValid())
		{
			Sessions->ClearOnDestroySessionCompleteDelegate_Handle(DestroySessionDelegateHandle);
			DestroySessionDelegateHandle.Reset();
		}
	}

	FCoreUObjectDelegates::PostLoadMapWithWorld.RemoveAll(this);

	if (GEngine)
	{
		if (TravelFailureDelegateHandle.IsValid())
		{
			GEngine->OnTravelFailure().Remove(TravelFailureDelegateHandle);
			TravelFailureDelegateHandle.Reset();
		}
		if (NetworkFailureDelegateHandle.IsValid())
		{
			GEngine->OnNetworkFailure().Remove(NetworkFailureDelegateHandle);
			NetworkFailureDelegateHandle.Reset();
		}
	}
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(OnlineJoinTimeoutHandle);
		World->GetTimerManager().ClearTimer(HostedSessionRefreshTimerHandle);
		World->GetTimerManager().ClearTimer(JoinConnectRetryTimerHandle);
	}

	Super::Shutdown();
}

TSubclassOf<UUserWidget> UUIWorldMenuGameInstance::ResolveMenuClass(EUIWorldMenuScreen MenuScreen) const
{
	switch (MenuScreen)
	{
	case EUIWorldMenuScreen::MainMenu:
		UE_LOG(LogUIWorldStartupFlow, Verbose, TEXT("[MenuFlow] ResolveMenuClass -> MainMenu (%s)"), *GetNameSafe(MainMenuWidgetClass.Get()));
		return MainMenuWidgetClass;
	case EUIWorldMenuScreen::OnlineMenu:
		return OnlineMenuWidgetClass
			? OnlineMenuWidgetClass
			: TSubclassOf<UUserWidget>(UZonefallOnlineLobbyWidget::StaticClass());
	case EUIWorldMenuScreen::PauseMenu:
		if (PauseMenuWidgetClass)
		{
			UE_LOG(LogUIWorldStartupFlow, Verbose, TEXT("[MenuFlow] ResolveMenuClass -> PauseMenu (%s)"), *GetNameSafe(PauseMenuWidgetClass.Get()));
			return PauseMenuWidgetClass;
		}
		UE_LOG(LogUIWorldStartupFlow, Warning, TEXT("ResolveMenuClass(PauseMenu): PauseMenuWidgetClass is null, using UZonefallPauseMenuWidget fallback."));
		return UZonefallPauseMenuWidget::StaticClass();
	case EUIWorldMenuScreen::SettingsMenu:
		// Same full settings whether opened from the main menu or from pause.
		// Prefer pause-specific class (from pause), then the general settings class,
		// then the built-in master settings widget so it always works out of the box.
		if (LastNonSettingsMenuScreen == EUIWorldMenuScreen::PauseMenu && PauseSettingsMenuWidgetClass)
		{
			return PauseSettingsMenuWidgetClass;
		}
		if (SettingsMenuWidgetClass)
		{
			return SettingsMenuWidgetClass;
		}
		return TSubclassOf<UUserWidget>(UZonefallMasterSettingsWidget::StaticClass());
	default:
		UE_LOG(LogUIWorldStartupFlow, Warning, TEXT("[MenuFlow] ResolveMenuClass -> Default fallback to MainMenu (%s)"), *GetNameSafe(MainMenuWidgetClass.Get()));
		return MainMenuWidgetClass;
	}
}

UUserWidget* UUIWorldMenuGameInstance::ResolveOrCreateWidget(APlayerController* PlayerController, TSubclassOf<UUserWidget> WidgetClass, bool bForceRebuild)
{
	if (!PlayerController || !WidgetClass)
	{
		UE_LOG(
			LogUIWorldStartupFlow,
			Warning,
			TEXT("[MenuFlow] ResolveOrCreateWidget aborted. PlayerController=%d WidgetClass=%s"),
			PlayerController != nullptr,
			*GetNameSafe(WidgetClass.Get())
		);
		return nullptr;
	}

	UUserWidget* CachedWidget = nullptr;
	if (bCacheWidgetsByScreen)
	{
		if (TObjectPtr<UUserWidget>* FoundWidget = MenuWidgetCache.Find(CurrentMenuScreen))
		{
			CachedWidget = FoundWidget->Get();
		}
	}

	const bool bNeedNewWidget = bForceRebuild || !CachedWidget || !CachedWidget->IsA(WidgetClass);
	UUserWidget* FinalWidget = CachedWidget;
	if (bNeedNewWidget)
	{
		UE_LOG(
			LogUIWorldStartupFlow,
			Log,
			TEXT("[MenuFlow] Creating widget. Screen=%d Class=%s Force=%d CachedValid=%d"),
			static_cast<int32>(CurrentMenuScreen),
			*GetNameSafe(WidgetClass.Get()),
			bForceRebuild ? 1 : 0,
			CachedWidget != nullptr
		);
		FinalWidget = CreateWidget<UUserWidget>(PlayerController, WidgetClass);
		if (!FinalWidget)
		{
			UE_LOG(LogUIWorldStartupFlow, Error, TEXT("[MenuFlow] CreateWidget failed for class=%s"), *GetNameSafe(WidgetClass.Get()));
			return nullptr;
		}
		if (bCacheWidgetsByScreen)
		{
			MenuWidgetCache.Add(CurrentMenuScreen, FinalWidget);
		}
	}
	else
	{
		UE_LOG(
			LogUIWorldStartupFlow,
			Verbose,
			TEXT("[MenuFlow] Reusing cached widget. Screen=%d Widget=%s"),
			static_cast<int32>(CurrentMenuScreen),
			*GetNameSafe(FinalWidget)
		);
	}
	return FinalWidget;
}

void UUIWorldMenuGameInstance::ApplyMenuInputMode(APlayerController* PlayerController, UUserWidget* WidgetToFocus) const
{
	if (!PlayerController || !WidgetToFocus)
	{
		return;
	}

	FInputModeUIOnly UiOnlyInputMode;
	if (WidgetToFocus->IsFocusable())
	{
		UiOnlyInputMode.SetWidgetToFocus(WidgetToFocus->TakeWidget());
	}
	UiOnlyInputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);

	PlayerController->SetInputMode(UiOnlyInputMode);
	PlayerController->SetShowMouseCursor(true);
	PlayerController->SetIgnoreMoveInput(true);
	PlayerController->SetIgnoreLookInput(true);
}

void UUIWorldMenuGameInstance::HideCurrentMenuWidget()
{
	if (PinnedMenuWidget && PinnedMenuWidget->IsInViewport())
	{
		PinnedMenuWidget->RemoveFromParent();
	}
}

UUserWidget* UUIWorldMenuGameInstance::ShowMenuFromList(EUIWorldMenuScreen MenuScreen, bool bForceRebuild)
{
	UE_LOG(
		LogUIWorldStartupFlow,
		Log,
		TEXT("[MenuFlow] ShowMenuFromList start. Screen=%d Force=%d StartupActive=%d StartupDone=%d"),
		static_cast<int32>(MenuScreen),
		bForceRebuild ? 1 : 0,
		bStartupShaderPhaseActive ? 1 : 0,
		bStartupShaderPhaseCompleted ? 1 : 0
	);
	if (bShowShaderLoadingOnStartup && ShaderLoadingWidgetClass && !bStartupShaderPhaseCompleted)
	{
		if (!bStartupShaderPhaseActive)
		{
			BeginStartupShaderPhase();
		}
		return nullptr;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return nullptr;
	}

	APlayerController* PlayerController = UGameplayStatics::GetPlayerController(World, 0);
	if (!PlayerController)
	{
		UE_LOG(LogUIWorldStartupFlow, Warning, TEXT("[MenuFlow] ShowMenuFromList failed: PlayerController is null."));
		return nullptr;
	}

	if (MenuScreen == EUIWorldMenuScreen::SettingsMenu)
	{
		// Keep track of where settings were opened from for reliable Back behavior.
		if (CurrentMenuScreen != EUIWorldMenuScreen::SettingsMenu)
		{
			LastNonSettingsMenuScreen = CurrentMenuScreen;
		}
	}
	else
	{
		LastNonSettingsMenuScreen = MenuScreen;
	}

	CurrentMenuScreen = MenuScreen;
	TSubclassOf<UUserWidget> SelectedWidgetClass = ResolveMenuClass(MenuScreen);
	UUserWidget* WorkingWidget = ResolveOrCreateWidget(PlayerController, SelectedWidgetClass, bForceRebuild);
	if (!WorkingWidget)
	{
		UE_LOG(LogUIWorldStartupFlow, Error, TEXT("[MenuFlow] ShowMenuFromList failed: ResolveOrCreateWidget returned null."));
		return nullptr;
	}

	HideCurrentMenuWidget();
	if (!WorkingWidget->IsInViewport())
	{
		WorkingWidget->AddToViewport(MenuZOrder);
		UE_LOG(LogUIWorldStartupFlow, Log, TEXT("[MenuFlow] Widget added to viewport. Widget=%s Z=%d"), *GetNameSafe(WorkingWidget), MenuZOrder);
	}
	else
	{
		UE_LOG(LogUIWorldStartupFlow, Verbose, TEXT("[MenuFlow] Widget already in viewport. Widget=%s"), *GetNameSafe(WorkingWidget));
	}

	ApplyMenuInputMode(PlayerController, WorkingWidget);
	PinnedMenuWidget = WorkingWidget;
	UE_LOG(
		LogUIWorldStartupFlow,
		Log,
		TEXT("[MenuFlow] ShowMenuFromList success. Screen=%d Widget=%s"),
		static_cast<int32>(CurrentMenuScreen),
		*GetNameSafe(PinnedMenuWidget)
	);
	OnMenuWidgetChanged.Broadcast(WorkingWidget);
	OnMenuScreenChanged.Broadcast(CurrentMenuScreen);
	return WorkingWidget;
}

void UUIWorldMenuGameInstance::CloseMenuUI(bool bRemoveFromParentOnly)
{
	HideCurrentMenuWidget();
	if (!bRemoveFromParentOnly)
	{
		PinnedMenuWidget = nullptr;
		MenuWidgetCache.Empty();
	}

	if (UWorld* World = GetWorld())
	{
		if (APlayerController* PlayerController = UGameplayStatics::GetPlayerController(World, 0))
		{
			PlayerController->ResetIgnoreMoveInput();
			PlayerController->ResetIgnoreLookInput();
			FInputModeGameOnly GameOnlyInputMode;
			PlayerController->SetInputMode(GameOnlyInputMode);
			PlayerController->SetShowMouseCursor(false);
		}
	}
	OnMenuWidgetChanged.Broadcast(nullptr);
}

bool UUIWorldMenuGameInstance::LoadLevelAndFocusGame(FName LevelName, bool bAbsolute)
{
	if (LevelName.IsNone())
	{
		return false;
	}
	UWorld* World = GetWorld();
	if (!World)
	{
		return false;
	}

	CloseMenuUI(false);
	bApplyGameFocusOnNextMapLoad = true;
	const FName Resolved = UIWorldResolveLevelNameForOpen(World, LevelName);
	UGameplayStatics::OpenLevel(World, Resolved, bAbsolute);
	return true;
}

bool UUIWorldMenuGameInstance::LoadLevelWithLoadingScreen(FName LevelName, bool bAbsolute)
{
	if (LevelName.IsNone())
	{
		return false;
	}
	UWorld* World = GetWorld();
	if (!World)
	{
		return false;
	}

	ShowSimpleTravelLoadingScreen();
	CloseMenuUI(false);
	bApplyGameFocusOnNextMapLoad = true;
	PendingLevelNameToLoad = UIWorldResolveLevelNameForOpen(World, LevelName);
	bPendingLevelAbsolute = bAbsolute;

	World->GetTimerManager().ClearTimer(PendingLoadLevelTimerHandle);
	float Delay = bUseAdaptiveLoadingDelay
		? EstimateAdaptiveLoadingDelay()
		: FMath::Clamp(LoadingScreenDelayBeforeOpenLevel, 0.0f, 2.0f);
	if (bUseMapComplexityDelay)
	{
		Delay += EstimateMapComplexityDelay(PendingLevelNameToLoad);
	}
	Delay = FMath::Clamp(Delay, 0.0f, 30.0f);
	if (Delay <= KINDA_SMALL_NUMBER)
	{
		World->GetTimerManager().SetTimerForNextTick(this, &UUIWorldMenuGameInstance::ExecutePendingLevelLoad);
	}
	else
	{
		World->GetTimerManager().SetTimer(
			PendingLoadLevelTimerHandle,
			this,
			&UUIWorldMenuGameInstance::ExecutePendingLevelLoad,
			Delay,
			false
		);
	}
	return true;
}

void UUIWorldMenuGameInstance::ExecutePendingLevelLoad()
{
	UWorld* World = GetWorld();
	if (!World || PendingLevelNameToLoad.IsNone())
	{
		return;
	}

	const FName LevelToOpen = UIWorldResolveLevelNameForOpen(World, PendingLevelNameToLoad);
	const bool bAbsolute = bPendingLevelAbsolute;
	PendingLevelNameToLoad = NAME_None;
	if (UZonefallLoadingScreenWidget* TypedLoading = Cast<UZonefallLoadingScreenWidget>(ActiveLoadingScreenWidget))
	{
		TypedLoading->EnterFinalizingPhase(82.0f);
	}
	if (bUseMoviePlayerLoadingScreen)
	{
		SetupMoviePlayerLoadingScreen();
	}
	UGameplayStatics::OpenLevel(World, LevelToOpen, bAbsolute);
}

float UUIWorldMenuGameInstance::EstimateAdaptiveLoadingDelay() const
{
	const int32 Cores = FPlatformMisc::NumberOfCoresIncludingHyperthreads();
	const uint64 TotalPhysicalBytes = FPlatformMemory::GetConstants().TotalPhysical;
	const float TotalRamGB = static_cast<float>(TotalPhysicalBytes) / (1024.0f * 1024.0f * 1024.0f);

	float CpuFactor = 0.35f;
	if (Cores <= 4)
	{
		CpuFactor = 1.0f;
	}
	else if (Cores <= 8)
	{
		CpuFactor = 0.72f;
	}
	else if (Cores <= 12)
	{
		CpuFactor = 0.52f;
	}

	float RamFactor = 0.35f;
	if (TotalRamGB < 8.0f)
	{
		RamFactor = 1.0f;
	}
	else if (TotalRamGB < 16.0f)
	{
		RamFactor = 0.72f;
	}
	else if (TotalRamGB < 24.0f)
	{
		RamFactor = 0.52f;
	}

	const float HardwareFactor = FMath::Max(CpuFactor, RamFactor);
	const float Base = FMath::Clamp(LoadingScreenDelayBeforeOpenLevel, 0.0f, 2.0f);
	const float Adaptive = Base + (HardwareFactor * 0.55f);
	const float MinDelay = FMath::Clamp(AdaptiveDelayMinSeconds, 0.0f, 5.0f);
	const float MaxDelay = FMath::Clamp(AdaptiveDelayMaxSeconds, MinDelay + 0.01f, 10.0f);
	return FMath::Clamp(Adaptive, MinDelay, MaxDelay);
}

float UUIWorldMenuGameInstance::EstimateMapComplexityDelay(FName LevelName) const
{
	const FString RawLevel = LevelName.ToString();
	if (RawLevel.IsEmpty())
	{
		return 0.0f;
	}

	auto ResolveMapPathAndSizeMB = [&RawLevel]() -> float
	{
		FString Normalized = RawLevel;
		if (Normalized.StartsWith(TEXT("/Game/")))
		{
			Normalized = Normalized.RightChop(6); // after /Game/
		}
		const int32 DotIndex = Normalized.Find(TEXT("."), ESearchCase::IgnoreCase, ESearchDir::FromStart);
		if (DotIndex != INDEX_NONE)
		{
			Normalized = Normalized.Left(DotIndex);
		}

		FString DirectMapPath = FPaths::Combine(FPaths::ProjectContentDir(), Normalized + TEXT(".umap"));
		int64 SizeBytes = IFileManager::Get().FileSize(*DirectMapPath);
		if (SizeBytes <= 0)
		{
			// Fallback: search by short map name.
			const FString MapFileName = FPaths::GetBaseFilename(Normalized) + TEXT(".umap");
			TArray<FString> FoundFiles;
			IFileManager::Get().FindFilesRecursive(FoundFiles, *FPaths::ProjectContentDir(), *MapFileName, true, false, false);
			if (FoundFiles.Num() > 0)
			{
				SizeBytes = IFileManager::Get().FileSize(*FoundFiles[0]);
			}
		}

		if (SizeBytes <= 0)
		{
			return 0.0f;
		}

		return static_cast<float>(SizeBytes) / (1024.0f * 1024.0f);
	};

	const float MapSizeMB = ResolveMapPathAndSizeMB();
	if (MapSizeMB <= 0.0f)
	{
		return 0.0f;
	}

	const float Per100 = FMath::Max(0.0f, MapDelayPer100MB);
	const float RawDelay = (MapSizeMB / 100.0f) * Per100;
	return FMath::Clamp(RawDelay, 0.0f, FMath::Max(0.0f, MapDelayMaxSeconds));
}

void UUIWorldMenuGameInstance::ContinueGame(bool bUnpauseGame)
{
	CloseMenuUI(false);
	if (bUnpauseGame)
	{
		if (UWorld* World = GetWorld())
		{
			UGameplayStatics::SetGamePaused(World, false);
			ApplyGameFocusInput(World);
		}
	}
}

UUserWidget* UUIWorldMenuGameInstance::OpenPauseSettingsMenu(bool bForceRebuild)
{
	if (UWorld* World = GetWorld())
	{
		UGameplayStatics::SetGamePaused(World, true);
	}
	LastNonSettingsMenuScreen = EUIWorldMenuScreen::PauseMenu;
	UE_LOG(LogUIWorldStartupFlow, Log, TEXT("[PauseFlow] OpenPauseSettingsMenu called. Force=%d"), bForceRebuild ? 1 : 0);
	return ShowMenuFromList(EUIWorldMenuScreen::SettingsMenu, bForceRebuild);
}

UUserWidget* UUIWorldMenuGameInstance::OpenSettingsPauseMenu(bool bForceRebuild)
{
	UE_LOG(LogUIWorldStartupFlow, Log, TEXT("[PauseFlow] OpenSettingsPauseMenu alias called. Force=%d"), bForceRebuild ? 1 : 0);
	return OpenPauseSettingsMenu(bForceRebuild);
}

UUserWidget* UUIWorldMenuGameInstance::OpenSettingsMainMenu(bool bForceRebuild)
{
	if (UWorld* World = GetWorld())
	{
		UGameplayStatics::SetGamePaused(World, false);
	}
	LastNonSettingsMenuScreen = EUIWorldMenuScreen::MainMenu;
	UE_LOG(LogUIWorldStartupFlow, Log, TEXT("[MenuFlow] OpenSettingsMainMenu called. Force=%d"), bForceRebuild ? 1 : 0);
	return ShowMenuFromList(EUIWorldMenuScreen::SettingsMenu, bForceRebuild);
}

UUserWidget* UUIWorldMenuGameInstance::BackMenuPause(bool bForceRebuild)
{
	if (UWorld* World = GetWorld())
	{
		UGameplayStatics::SetGamePaused(World, true);
	}
	UE_LOG(LogUIWorldStartupFlow, Log, TEXT("[PauseFlow] BackMenuPause called. Force=%d"), bForceRebuild ? 1 : 0);
	return ShowMenuFromList(EUIWorldMenuScreen::PauseMenu, bForceRebuild);
}

UUserWidget* UUIWorldMenuGameInstance::BackFromSettingsPauseMenu(bool bForceRebuild)
{
	UE_LOG(LogUIWorldStartupFlow, Log, TEXT("[PauseFlow] BackFromSettingsPauseMenu alias called. Force=%d"), bForceRebuild ? 1 : 0);
	return BackMenuPause(bForceRebuild);
}

UUserWidget* UUIWorldMenuGameInstance::BackFromSettingsMainMenu(bool bForceRebuild)
{
	if (UWorld* World = GetWorld())
	{
		UGameplayStatics::SetGamePaused(World, false);
	}
	LastNonSettingsMenuScreen = EUIWorldMenuScreen::MainMenu;
	UE_LOG(LogUIWorldStartupFlow, Log, TEXT("[MenuFlow] BackFromSettingsMainMenu called. Force=%d"), bForceRebuild ? 1 : 0);
	return ShowMenuFromList(EUIWorldMenuScreen::MainMenu, bForceRebuild);
}

UUserWidget* UUIWorldMenuGameInstance::BackFromSettingsMenuSmart(bool bForceRebuild)
{
	EUIWorldMenuScreen ReturnScreen = LastNonSettingsMenuScreen;
	if (ReturnScreen == EUIWorldMenuScreen::SettingsMenu)
	{
		ReturnScreen = EUIWorldMenuScreen::MainMenu;
	}

	if (ReturnScreen == EUIWorldMenuScreen::PauseMenu)
	{
		UE_LOG(LogUIWorldStartupFlow, Log, TEXT("[PauseFlow] BackFromSettingsMenuSmart -> PauseMenu. Force=%d"), bForceRebuild ? 1 : 0);
		if (UWorld* World = GetWorld())
		{
			UGameplayStatics::SetGamePaused(World, true);
		}
		return ShowMenuFromList(EUIWorldMenuScreen::PauseMenu, bForceRebuild);
	}

	UE_LOG(LogUIWorldStartupFlow, Log, TEXT("[MenuFlow] BackFromSettingsMenuSmart -> MainMenu. Force=%d"), bForceRebuild ? 1 : 0);
	if (UWorld* World = GetWorld())
	{
		UGameplayStatics::SetGamePaused(World, false);
	}
	return ShowMenuFromList(EUIWorldMenuScreen::MainMenu, bForceRebuild);
}

bool UUIWorldMenuGameInstance::LoadMainMenuLevel(bool bAbsolute)
{
	if (MainMenuLevelName.IsNone())
	{
		return false;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return false;
	}

	// Main menu should return to UI focus flow (visible cursor), not gameplay focus.
	UGameplayStatics::SetGamePaused(World, false);
	CloseMenuUI(false);
	bApplyGameFocusOnNextMapLoad = false;
	const FName ResolvedMainMenu = UIWorldResolveLevelNameForOpen(World, MainMenuLevelName);
	UGameplayStatics::OpenLevel(World, ResolvedMainMenu, bAbsolute);
	return true;
}

void UUIWorldMenuGameInstance::QuitGameNow(bool bIgnorePlatformRestrictions)
{
	if (UWorld* World = GetWorld())
	{
		APlayerController* PlayerController = UGameplayStatics::GetPlayerController(World, 0);
		UKismetSystemLibrary::QuitGame(World, PlayerController, EQuitPreference::Quit, bIgnorePlatformRestrictions);
	}
}

FString UUIWorldMenuGameInstance::GetEngineVersionString() const
{
	return FString::Printf(TEXT("Unreal Engine %s"), *FEngineVersion::Current().ToString(EVersionComponent::Patch));
}

bool UUIWorldMenuGameInstance::SaveGame()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return false;
	}

	UUIWorldSaveManager* SaveManager = NewObject<UUIWorldSaveManager>(this);
	const FString LevelName = UWorld::RemovePIEPrefix(World->GetMapName());

	UUIWorldSaveGame* Save = SaveManager->CreateEmptySave();
	if (!Save)
	{
		ShowSaveToast(TEXT("Save failed"));
		return false;
	}

	Save->SavedLevelName = LevelName;
	Save->SavedAtUtc = FDateTime::UtcNow();

	// Capture the local player's full snapshot (health, weapons + ammo, picked-up items, position).
	// Authority is required to read the server-authoritative components — on a listen host /
	// standalone that's the local player; pure clients fall back to a level-only save.
	if (APawn* Pawn = UGameplayStatics::GetPlayerPawn(World, 0))
	{
		if (AZonefallPlayerCharacter* Player = Cast<AZonefallPlayerCharacter>(Pawn))
		{
			if (Player->HasAuthority())
			{
				Player->CaptureToSaveGame(Save);
			}
		}
	}

	const bool bSaved = SaveManager->WriteSave(Save);

	ShowSaveToast(bSaved
		? FString::Printf(TEXT("%s"), *LevelName)
		: TEXT("Save failed"));
	return bSaved;
}

bool UUIWorldMenuGameInstance::HasSavedGame() const
{
	const UUIWorldSaveManager* SaveManager = NewObject<UUIWorldSaveManager>(const_cast<UUIWorldMenuGameInstance*>(this));
	return SaveManager && SaveManager->HasSave();
}

bool UUIWorldMenuGameInstance::ContinueSavedGame()
{
	UUIWorldSaveManager* SaveManager = NewObject<UUIWorldSaveManager>(this);

	FString SavedLevel;
	if (!SaveManager->LoadSavedLevelName(SavedLevel) || SavedLevel.IsEmpty())
	{
		// No save: behave like a normal resume of whatever is loaded.
		ContinueGame(true);
		return false;
	}

	// Flag the restore so the player spawned on the loaded level re-applies its snapshot.
	bPendingSaveRestore = true;
	return LoadLevelWithLoadingScreen(FName(*SavedLevel), true);
}

bool UUIWorldMenuGameInstance::ConsumeSaveRestoreRequest()
{
	const bool bWasPending = bPendingSaveRestore;
	bPendingSaveRestore = false;
	return bWasPending;
}

bool UUIWorldMenuGameInstance::OpenCharacterCreator()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return false;
	}

	// Hide the main menu overlay — character setup opens on the same map (no extra menu level).
	CloseMenuUI(false);

	auto TryOpenOnPawn = [this](AZonefallPlayerCharacter* Player) -> bool
	{
		if (!Player)
		{
			return false;
		}

		if (HasCurrentAppearance())
		{
			Player->ApplyAppearance(GetCurrentAppearance());
		}

		bPendingShowCharacterCreator = false;
		Player->OpenCharacterCreatorUI();
		return true;
	};

	// Preferred path: open immediately on the menu pawn (no loading screen, no studio travel).
	if (IsMenuMapWorld(World))
	{
		if (AZonefallPlayerCharacter* Player = EnsureCharacterCreatorPawn())
		{
			return TryOpenOnPawn(Player);
		}

		bPendingShowCharacterCreator = true;
		return true;
	}

	// Optional dedicated creator studio — only when explicitly configured and not on the menu map.
	const bool bAlreadyOnCreatorLevel = !CharacterCreatorLevelName.IsNone()
		&& World->GetMapName().Contains(CharacterCreatorLevelName.ToString());
	if (!CharacterCreatorLevelName.IsNone() && !bAlreadyOnCreatorLevel)
	{
		bPendingShowCharacterCreator = true;
		return LoadLevelWithLoadingScreen(CharacterCreatorLevelName, true);
	}

	if (AZonefallPlayerCharacter* Player = Cast<AZonefallPlayerCharacter>(UGameplayStatics::GetPlayerPawn(World, 0)))
	{
		return TryOpenOnPawn(Player);
	}

	bPendingShowCharacterCreator = true;
	return true;
}

bool UUIWorldMenuGameInstance::ConsumeShowCharacterCreatorRequest()
{
	const bool bWasPending = bPendingShowCharacterCreator;
	bPendingShowCharacterCreator = false;
	return bWasPending;
}

void UUIWorldMenuGameInstance::SetCurrentAppearance(const FZonefallCharacterAppearance& InAppearance)
{
	CurrentAppearance = InAppearance;
	bHasCurrentAppearance = true;
}

bool UUIWorldMenuGameInstance::StartGameWithCreatedCharacter(const FZonefallCharacterAppearance& InAppearance)
{
	if (UWorld* World = GetWorld())
	{
		if (AZonefallPlayerCharacter* Player = Cast<AZonefallPlayerCharacter>(UGameplayStatics::GetPlayerPawn(World, 0)))
		{
			Player->CloseCharacterCreatorUI();
		}
	}

	FZonefallCharacterAppearance Appearance = InAppearance;
	Appearance.bCreated = true;
	SetCurrentAppearance(Appearance);

	// Persist the look so it survives a full restart (Continue restores it as well).
	UUIWorldSaveManager* SaveManager = NewObject<UUIWorldSaveManager>(this);
	UUIWorldSaveGame* Save = SaveManager->LoadSave();
	if (!Save)
	{
		Save = SaveManager->CreateEmptySave();
	}
	if (Save)
	{
		Save->bHasAppearance = true;
		Save->Appearance = Appearance;
		Save->SavedAtUtc = FDateTime::UtcNow();
		SaveManager->WriteSave(Save);
	}

	bPendingShowCharacterCreator = false;
	const FName Target = !GameplayLevelName.IsNone() ? GameplayLevelName : FName(TEXT("GameMap"));
	return LoadLevelWithLoadingScreen(Target, true);
}

void UUIWorldMenuGameInstance::ShowSaveToast(const FString& Message)
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}
	APlayerController* PC = UGameplayStatics::GetPlayerController(World, 0);
	if (!PC)
	{
		return;
	}

	const TSubclassOf<UUserWidget> ToastClass = SaveToastWidgetClass
		? SaveToastWidgetClass
		: TSubclassOf<UUserWidget>(UZonefallSaveToastWidget::StaticClass());

	if (UUserWidget* W = CreateWidget<UUserWidget>(PC, ToastClass))
	{
		W->AddToViewport(MenuZOrder + 500);
		if (UZonefallSaveToastWidget* Toast = Cast<UZonefallSaveToastWidget>(W))
		{
			Toast->ShowToast(FText::GetEmpty(), FText::FromString(Message));
		}
	}
}

void UUIWorldMenuGameInstance::ShowSimpleTravelLoadingScreen()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	if (APlayerController* PlayerController = UGameplayStatics::GetPlayerController(World, 0))
	{
		if (!ActiveLoadingScreenWidget && LoadingScreenWidgetClass)
		{
			ActiveLoadingScreenWidget = CreateWidget<UUserWidget>(PlayerController, LoadingScreenWidgetClass);
		}
		if (ActiveLoadingScreenWidget && !ActiveLoadingScreenWidget->IsInViewport())
		{
			ActiveLoadingScreenWidget->AddToViewport(MenuZOrder + 900);
		}
		if (UZonefallLoadingScreenWidget* TypedLoading = Cast<UZonefallLoadingScreenWidget>(ActiveLoadingScreenWidget))
		{
			TypedLoading->StartLoading();
		}

		FInputModeUIOnly UiOnlyInputMode;
		PlayerController->SetInputMode(UiOnlyInputMode);
		PlayerController->SetIgnoreMoveInput(true);
		PlayerController->SetIgnoreLookInput(true);
		PlayerController->SetShowMouseCursor(false);
	}
}

void UUIWorldMenuGameInstance::HideSimpleTravelLoadingScreen()
{
	if (UZonefallLoadingScreenWidget* TypedLoading = Cast<UZonefallLoadingScreenWidget>(ActiveLoadingScreenWidget))
	{
		TypedLoading->CompleteLoading();
	}

	if (ActiveLoadingScreenWidget)
	{
		ActiveLoadingScreenWidget->RemoveFromParent();
		ActiveLoadingScreenWidget = nullptr;
	}
}

void UUIWorldMenuGameInstance::BeginOnlineTravel()
{
	BeginOnlineTravelWithPhase(EZonefallOnlineTravelPhase::Joining, FText::GetEmpty());
}

void UUIWorldMenuGameInstance::BeginOnlineTravelWithPhase(EZonefallOnlineTravelPhase Phase, const FText& StatusHint)
{
	bOnlineTravelInProgress = true;
	bOnlineJoinReachedGameMap = false;
	bOnlineLoadingOverlayActive = true;
	ActiveOnlineTravelPhase = Phase;

	if (UWorld* World = GetWorld())
	{
		OnlineTravelStartSeconds = World->GetTimeSeconds();
		World->GetTimerManager().ClearTimer(OnlineAbortDeferHandle);
		World->GetTimerManager().ClearTimer(OnlineJoinTimeoutHandle);
		World->GetTimerManager().ClearTimer(OnlineLoadingStatusTimerHandle);
		World->GetTimerManager().SetTimer(
			OnlineJoinTimeoutHandle,
			this,
			&UUIWorldMenuGameInstance::HandleOnlineJoinTimeout,
			FMath::Clamp(OnlineJoinTimeoutSeconds, 15.0f, 120.0f),
			false);
		World->GetTimerManager().SetTimer(
			OnlineLoadingStatusTimerHandle,
			this,
			&UUIWorldMenuGameInstance::HandleOnlineLoadingStatusTick,
			2.2f,
			true);
	}

	ShowOnlineTravelLoadingScreen(Phase, StatusHint);
}

void UUIWorldMenuGameInstance::ShowOnlineTravelLoadingScreen(EZonefallOnlineTravelPhase Phase, const FText& StatusHint)
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	if (APlayerController* PlayerController = UGameplayStatics::GetPlayerController(World, 0))
	{
		if (!ActiveLoadingScreenWidget && LoadingScreenWidgetClass)
		{
			ActiveLoadingScreenWidget = CreateWidget<UUserWidget>(PlayerController, LoadingScreenWidgetClass);
		}
		if (ActiveLoadingScreenWidget && !ActiveLoadingScreenWidget->IsInViewport())
		{
			ActiveLoadingScreenWidget->AddToViewport(MenuZOrder + 900);
		}
		if (UZonefallLoadingScreenWidget* TypedLoading = Cast<UZonefallLoadingScreenWidget>(ActiveLoadingScreenWidget))
		{
			TypedLoading->ConfigureOnlineTravelLoading(Phase, StatusHint);
		}

		// Do not steal game input during ClientTravel — only show the overlay.
		FInputModeGameOnly InputMode;
		PlayerController->SetInputMode(InputMode);
		PlayerController->SetShowMouseCursor(false);
	}
}

void UUIWorldMenuGameInstance::UpdateOnlineTravelLoadingStatus(const FText& Status)
{
	if (UZonefallLoadingScreenWidget* TypedLoading = Cast<UZonefallLoadingScreenWidget>(ActiveLoadingScreenWidget))
	{
		TypedLoading->SetOnlineTravelStatus(Status);
	}
}

void UUIWorldMenuGameInstance::HandleOnlineLoadingStatusTick()
{
	if (!bOnlineTravelInProgress)
	{
		return;
	}

	static const FText HostPhrases[] = {
		NSLOCTEXT("ZonefallUI", "OnlineHost1", "Creating your public session..."),
		NSLOCTEXT("ZonefallUI", "OnlineHost2", "Opening player slots..."),
		NSLOCTEXT("ZonefallUI", "OnlineHost3", "Loading the shared world..."),
		NSLOCTEXT("ZonefallUI", "OnlineHost4", "Waiting for players to join...")
	};
	static const FText JoinPhrases[] = {
		NSLOCTEXT("ZonefallUI", "OnlineJoin1", "Finding the best host..."),
		NSLOCTEXT("ZonefallUI", "OnlineJoin2", "Connecting to the session..."),
		NSLOCTEXT("ZonefallUI", "OnlineJoin3", "Loading the open world..."),
		NSLOCTEXT("ZonefallUI", "OnlineJoin4", "Spawning into the match...")
	};

	const FText* Phrases = (ActiveOnlineTravelPhase == EZonefallOnlineTravelPhase::Hosting) ? HostPhrases : JoinPhrases;
	const int32 Count = (ActiveOnlineTravelPhase == EZonefallOnlineTravelPhase::Hosting)
		? UE_ARRAY_COUNT(HostPhrases)
		: UE_ARRAY_COUNT(JoinPhrases);
	OnlineLoadingStatusPhraseIndex = (OnlineLoadingStatusPhraseIndex + 1) % Count;
	UpdateOnlineTravelLoadingStatus(Phrases[OnlineLoadingStatusPhraseIndex]);
}

void UUIWorldMenuGameInstance::HideOnlineTravelLoadingScreen()
{
	bOnlineLoadingOverlayActive = false;
	HideSimpleTravelLoadingScreen();
}

void UUIWorldMenuGameInstance::CancelOnlineTravelLoading()
{
	bOnlineTravelInProgress = false;
	bOnlineLoadingOverlayActive = false;
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(OnlineJoinTimeoutHandle);
		World->GetTimerManager().ClearTimer(OnlineAbortDeferHandle);
		World->GetTimerManager().ClearTimer(OnlineLoadingStatusTimerHandle);
	}
	HideOnlineTravelLoadingScreen();
}

void UUIWorldMenuGameInstance::RegisterLocalPlayerInActiveSession(UWorld* World)
{
	if (!World)
	{
		return;
	}

	IOnlineSessionPtr Sessions = UIWorldGetSessionInterface(bLastOnlineQueryWasLAN);
	if (!Sessions.IsValid())
	{
		return;
	}

	if (Sessions->GetNamedSession(NAME_GameSession) == nullptr)
	{
		return;
	}

	if (const TSharedPtr<const FUniqueNetId> UserId = UIWorldGetOrCreateLanUserId(World, bLastOnlineQueryWasLAN))
	{
		Sessions->RegisterPlayer(NAME_GameSession, *UserId, false);
	}
}

void UUIWorldMenuGameInstance::FinalizeOnlineTravelSuccess(UWorld* LoadedWorld)
{
	if (!LoadedWorld)
	{
		return;
	}

	bOnlineJoinReachedGameMap = true;
	bOnlineTravelInProgress = false;
	bOnlineLoadingOverlayActive = false;

	LoadedWorld->GetTimerManager().ClearTimer(OnlineJoinTimeoutHandle);
	LoadedWorld->GetTimerManager().ClearTimer(OnlineAbortDeferHandle);

	HideOnlineTravelLoadingScreen();
	CloseMenuUI(true);

	RegisterLocalPlayerInActiveSession(LoadedWorld);
	PublishHostedOnlineSession(LoadedWorld);
	ScheduleHostedOnlineSessionRefresh(LoadedWorld);

	OnOnlineMatchReady.Broadcast(LoadedWorld);

	if (bApplyGameFocusOnNextMapLoad)
	{
		ApplyGameFocusInput(LoadedWorld);
		bApplyGameFocusOnNextMapLoad = false;
	}

	UE_LOG(
		LogUIWorldStartupFlow,
		Log,
		TEXT("[Online] Match ready on %s (NetMode=%d LAN=%d)"),
		*LoadedWorld->GetMapName(),
		(int32)LoadedWorld->GetNetMode(),
		bLastOnlineQueryWasLAN ? 1 : 0);
}

void UUIWorldMenuGameInstance::EnsureOnlineAutoLogin()
{
	IOnlineSubsystem* OSS = IOnlineSubsystem::Get();
	if (!OSS)
	{
		return;
	}

	IOnlineIdentityPtr Identity = OSS->GetIdentityInterface();
	if (!Identity.IsValid())
	{
		return;
	}

	if (Identity->GetLoginStatus(0) == ELoginStatus::LoggedIn)
	{
		return;
	}

	if (!OnlineLoginDelegateHandle.IsValid())
	{
		OnlineLoginDelegateHandle = Identity->AddOnLoginCompleteDelegate_Handle(
			0, FOnLoginCompleteDelegate::CreateUObject(this, &UUIWorldMenuGameInstance::HandleOnlineIdentityLoginComplete));
	}

	if (!Identity->AutoLogin(0))
	{
		UE_LOG(LogUIWorldStartupFlow, Warning, TEXT("[Online] AutoLogin did not start (OSS=%s). Is Steam running?"), *UIWorldDescribeActiveOnlineSubsystem());
	}
}

void UUIWorldMenuGameInstance::HandleOnlineIdentityLoginComplete(
	int32 LocalUserNum, bool bWasSuccessful, const FUniqueNetId& UserId, const FString& Error)
{
	if (IOnlineSubsystem* OSS = IOnlineSubsystem::Get())
	{
		if (IOnlineIdentityPtr Identity = OSS->GetIdentityInterface())
		{
			if (OnlineLoginDelegateHandle.IsValid())
			{
				Identity->ClearOnLoginCompleteDelegate_Handle(LocalUserNum, OnlineLoginDelegateHandle);
				OnlineLoginDelegateHandle.Reset();
			}
		}
	}

	if (bWasSuccessful)
	{
		UE_LOG(LogUIWorldStartupFlow, Log, TEXT("[Online] Logged in via %s"), *UIWorldDescribeActiveOnlineSubsystem());
		LastOnlineDiagnostic.Reset();
		FlushPendingFindOnlineSessions();
	}
	else
	{
		LastOnlineDiagnostic = FString::Printf(
			TEXT("Online login failed (%s): %s"),
			*UIWorldDescribeActiveOnlineSubsystem(),
			Error.IsEmpty() ? TEXT("unknown error") : *Error);
		UE_LOG(LogUIWorldStartupFlow, Warning, TEXT("[Online] %s"), *LastOnlineDiagnostic);
		if (bPendingFindOnlineSessions)
		{
			bPendingFindOnlineSessions = false;
			OnSessionsFound.Broadcast(TArray<FUIWorldOnlineSessionResult>());
		}
	}
}

void UUIWorldMenuGameInstance::FlushPendingFindOnlineSessions()
{
	if (!bPendingFindOnlineSessions)
	{
		return;
	}
	const int32 MaxResults = PendingFindMaxResults;
	const bool bLAN = bPendingFindLAN;
	bPendingFindOnlineSessions = false;
	FindOnlineSessions(MaxResults, bLAN);
}

bool UUIWorldMenuGameInstance::TravelToListenHostMap(UWorld* World, const FString& MapPackagePath, bool bLAN)
{
	if (!IsValid(World) || MapPackagePath.IsEmpty())
	{
		return false;
	}

	if (World->GetNetMode() == NM_Client)
	{
		LastOnlineDiagnostic = TEXT("Host from Player 1 window (Listen Server), not the client PIE window");
		UE_LOG(LogUIWorldStartupFlow, Warning, TEXT("[Online] %s"), *LastOnlineDiagnostic);
		return false;
	}

	if (bLAN)
	{
		UIWorldActivateLanNetworking(World);
	}
	else
	{
		UIWorldActivateSteamNetworking(World);
	}

	const FString TravelOptions = bLAN
		? TEXT("listen&NetDriver=LanGameNetDriver")
		: TEXT("listen&NetDriver=GameNetDriver");

	if (AGameModeBase* GameMode = World->GetAuthGameMode())
	{
		GameMode->bUseSeamlessTravel = false;
	}

	const FName MapPackage = FName(*MapPackagePath);
	if (World->GetNetMode() == NM_Standalone)
	{
		UE_LOG(
			LogUIWorldStartupFlow,
			Log,
			TEXT("[Online] Host OpenLevel -> %s?%s (PIE standalone -> listen)"),
			*MapPackagePath,
			*TravelOptions);
		UGameplayStatics::OpenLevel(World, MapPackage, true, TravelOptions);
		return true;
	}

	const FString ListenURL = FString::Printf(TEXT("%s?%s"), *MapPackagePath, *TravelOptions);
	UE_LOG(
		LogUIWorldStartupFlow,
		Log,
		TEXT("[Online] Host ServerTravel -> %s (mode=%s OSS=%s)"),
		*ListenURL,
		bLAN ? TEXT("LAN/NULL") : TEXT("Steam"),
		*UIWorldDescribeActiveOnlineSubsystem(bLAN));
	World->ServerTravel(ListenURL, true);
	return true;
}

void UUIWorldMenuGameInstance::TryActivateListenServerAfterHostLoad(UWorld* LoadedWorld)
{
	if (!IsValid(LoadedWorld) || !bOnlineTravelInProgress
		|| ActiveOnlineTravelPhase != EZonefallOnlineTravelPhase::Hosting)
	{
		return;
	}

	if (IsMenuMapWorld(LoadedWorld))
	{
		return;
	}

	if (!UIWorldTryStartListenServer(LoadedWorld, bLastOnlineQueryWasLAN, OnlineLanPort))
	{
		return;
	}

	if (bLastOnlineQueryWasLAN)
	{
		if (IOnlineSessionPtr LanSessions = UIWorldGetSessionInterface(true))
		{
			if (LanSessions->GetNamedSession(NAME_GameSession))
			{
				LanSessions->StartSession(NAME_GameSession);
			}
		}
	}

	if (LoadedWorld->GetNetMode() == NM_ListenServer || LoadedWorld->GetNetDriver())
	{
		FinalizeOnlineTravelSuccess(LoadedWorld);
	}
}

void UUIWorldMenuGameInstance::OnHostCreateSessionComplete(FName /*SessionName*/, bool bWasSuccessful)
{
	const bool bLAN = bLastOnlineQueryWasLAN;
	if (IOnlineSessionPtr LocalSessions = UIWorldGetSessionInterface(bLAN))
	{
		if (CreateSessionDelegateHandle.IsValid())
		{
			LocalSessions->ClearOnCreateSessionCompleteDelegate_Handle(CreateSessionDelegateHandle);
			CreateSessionDelegateHandle.Reset();
		}
	}

	if (!IsValid(this))
	{
		return;
	}

	if (!bWasSuccessful)
	{
		CancelOnlineTravelLoading();
		OnHostCompleted.Broadcast(false, TEXT("CreateSession failed"));
		return;
	}

	UWorld* World = GetWorld();
	if (!IsValid(World))
	{
		CancelOnlineTravelLoading();
		OnHostCompleted.Broadcast(false, TEXT("Host aborted: world no longer available"));
		return;
	}

	const FName ResolvedHostMap = UIWorldResolveLevelNameForOpen(
		World,
		OnlineHostMapName.IsNone() ? FName(TEXT("Menu")) : OnlineHostMapName);
	const FString MapToOpen = ResolvedHostMap.IsNone() ? TEXT("Menu") : ResolvedHostMap.ToString();

	if (IOnlineSessionPtr LocalSessions = UIWorldGetSessionInterface(bLAN))
	{
		if (FNamedOnlineSession* ActiveSession = LocalSessions->GetNamedSession(NAME_GameSession))
		{
				ActiveSession->SessionSettings.Set(
					FName(TEXT("MAP_NAME")), MapToOpen, EOnlineDataAdvertisementType::ViaOnlineServiceAndPing);
				ActiveSession->SessionSettings.Set(
					FName(TEXT("BUILD_ID")),
					UIWorldGetProjectBuildId(this),
					EOnlineDataAdvertisementType::ViaOnlineServiceAndPing);
				if (!PendingHostPassword.IsEmpty())
				{
					ActiveSession->SessionSettings.Set(
						FName(TEXT("SESSION_PASSWORD")),
						PendingHostPassword,
						EOnlineDataAdvertisementType::ViaOnlineServiceAndPing);
				}
				if (bLAN)
				{
					ActiveSession->SessionSettings.Set(
						FName(TEXT("LAN_HOST")),
						FString(TEXT("127.0.0.1")),
						EOnlineDataAdvertisementType::ViaOnlineServiceAndPing);
					ActiveSession->SessionSettings.Set(
						FName(TEXT("LAN_PORT")),
						FMath::Clamp(OnlineLanPort, 1, 65535),
						EOnlineDataAdvertisementType::ViaOnlineServiceAndPing);
					LocalSessions->UpdateSession(NAME_GameSession, ActiveSession->SessionSettings, true);
				}
				else
				{
					LocalSessions->UpdateSession(NAME_GameSession, ActiveSession->SessionSettings, true);
				}
		}
	}

	if (!TravelToListenHostMap(World, MapToOpen, bLAN))
	{
		CancelOnlineTravelLoading();
		OnHostCompleted.Broadcast(false, LastOnlineDiagnostic.IsEmpty() ? TEXT("Host travel failed") : LastOnlineDiagnostic);
		return;
	}

	bApplyGameFocusOnNextMapLoad = true;
	CloseMenuUI(false);
	OnHostCompleted.Broadcast(
		true,
		bLAN
			? FString::Printf(TEXT("LAN hosted | %s | connect 127.0.0.1:%d"), *MapToOpen, OnlineLanPort)
			: FString::Printf(TEXT("Steam hosted | %s"), *MapToOpen));
}

void UUIWorldMenuGameInstance::OnDestroySessionBeforeHost(FName /*SessionName*/, bool bWasDestroyed)
{
	const bool bLAN = bLastOnlineQueryWasLAN;
	bPendingHostAfterDestroy = false;
	const int32 MaxPlayers = PendingHostMaxPlayers;
	PendingHostMaxPlayers = 4;

	if (IOnlineSessionPtr LocalSessions = UIWorldGetSessionInterface(bLAN))
	{
		if (DestroySessionDelegateHandle.IsValid())
		{
			LocalSessions->ClearOnDestroySessionCompleteDelegate_Handle(DestroySessionDelegateHandle);
			DestroySessionDelegateHandle.Reset();
		}
	}

	if (!IsValid(this))
	{
		return;
	}

	if (!bWasDestroyed)
	{
		CancelOnlineTravelLoading();
		OnHostCompleted.Broadcast(false, TEXT("Host failed: could not clear old session"));
		return;
	}

	BeginHostCreateSession(MaxPlayers, bLAN);
}

bool UUIWorldMenuGameInstance::BeginHostCreateSession(int32 MaxPlayers, bool bLAN)
{
	UWorld* World = GetWorld();
	if (!IsValid(World))
	{
		LastOnlineDiagnostic = TEXT("World is null");
		OnHostCompleted.Broadcast(false, LastOnlineDiagnostic);
		return false;
	}

	IOnlineSessionPtr Sessions = UIWorldGetSessionInterface(bLAN);
	if (!Sessions.IsValid())
	{
		LastOnlineDiagnostic = FString::Printf(
			TEXT("SessionInterface missing (LAN=%d OSS=%s)"),
			bLAN ? 1 : 0,
			*UIWorldDescribeActiveOnlineSubsystem(bLAN));
		OnHostCompleted.Broadcast(false, LastOnlineDiagnostic);
		return false;
	}

	const TSharedPtr<const FUniqueNetId> UserId = bLAN
		? UIWorldGetOrCreateLanUserId(World, true)
		: UIWorldGetLocalUserId(World, false);
	if (!UserId.IsValid())
	{
		if (!bLAN)
		{
			EnsureOnlineAutoLogin();
			LastOnlineDiagnostic = FString::Printf(
				TEXT("Not logged in yet (%s). Start Steam, wait a moment, then try Host again."),
				*UIWorldDescribeActiveOnlineSubsystem());
		}
		else
		{
			LastOnlineDiagnostic = TEXT("LAN host: could not create local player id");
		}
		OnHostCompleted.Broadcast(false, LastOnlineDiagnostic);
		return false;
	}

	FOnlineSessionSettings Settings;
	Settings.bIsLANMatch = bLAN;
	Settings.bUsesPresence = !bLAN;
	Settings.NumPublicConnections = FMath::Clamp(MaxPlayers, 1, 64);
	Settings.bAllowJoinInProgress = true;
	Settings.bAllowJoinViaPresence = !bLAN;
	Settings.bShouldAdvertise = true;
	Settings.bUseLobbiesIfAvailable = !bLAN;
	Settings.Set(FName(TEXT("SERVER_NAME")), OnlineServerName, EOnlineDataAdvertisementType::ViaOnlineServiceAndPing);
	Settings.Set(
		FName(TEXT("BUILD_ID")),
		UIWorldGetProjectBuildId(this),
		EOnlineDataAdvertisementType::ViaOnlineServiceAndPing);

	const int32 Privacy = FMath::Clamp(PendingHostPrivacy, 0, 2);
	if (!bLAN)
	{
		Settings.bShouldAdvertise = Privacy == 0;
		Settings.bAllowJoinViaPresence = Privacy <= 1;
		Settings.bUsesPresence = Privacy <= 1;
	}
	if (!PendingHostPassword.IsEmpty())
	{
		Settings.Set(
			FName(TEXT("SESSION_PASSWORD")),
			PendingHostPassword,
			EOnlineDataAdvertisementType::ViaOnlineServiceAndPing);
	}
	Settings.Set(FName(TEXT("SESSION_PRIVACY")), Privacy, EOnlineDataAdvertisementType::ViaOnlineServiceAndPing);

	if (CreateSessionDelegateHandle.IsValid())
	{
		Sessions->ClearOnCreateSessionCompleteDelegate_Handle(CreateSessionDelegateHandle);
		CreateSessionDelegateHandle.Reset();
	}

	FOnCreateSessionCompleteDelegate CreateDelegate;
	CreateDelegate.BindUObject(this, &UUIWorldMenuGameInstance::OnHostCreateSessionComplete);
	CreateSessionDelegateHandle = Sessions->AddOnCreateSessionCompleteDelegate_Handle(CreateDelegate);

	BeginOnlineTravelWithPhase(
		EZonefallOnlineTravelPhase::Hosting,
		NSLOCTEXT("ZonefallUI", "OnlineHostStart", "Creating session and loading map..."));

	const bool bStarted = Sessions->CreateSession(*UserId, NAME_GameSession, Settings);
	if (!bStarted)
	{
		if (CreateSessionDelegateHandle.IsValid())
		{
			Sessions->ClearOnCreateSessionCompleteDelegate_Handle(CreateSessionDelegateHandle);
			CreateSessionDelegateHandle.Reset();
		}
		CancelOnlineTravelLoading();
		OnHostCompleted.Broadcast(false, TEXT("CreateSession did not start"));
	}
	return bStarted;
}

bool UUIWorldMenuGameInstance::HostOnlineSession(int32 MaxPlayers, bool bLAN)
{
	bLastOnlineQueryWasLAN = bLAN;
	LastOnlineDiagnostic.Reset();
	PendingHostMaxPlayers = FMath::Clamp(MaxPlayers, 1, 64);

	UWorld* World = GetWorld();
	if (!IsValid(World))
	{
		LastOnlineDiagnostic = TEXT("World is null");
		OnHostCompleted.Broadcast(false, LastOnlineDiagnostic);
		return false;
	}

	IOnlineSessionPtr Sessions = UIWorldGetSessionInterface(bLAN);
	if (!Sessions.IsValid())
	{
		LastOnlineDiagnostic = FString::Printf(
			TEXT("SessionInterface missing (LAN=%d OSS=%s)"),
			bLAN ? 1 : 0,
			*UIWorldDescribeActiveOnlineSubsystem(bLAN));
		OnHostCompleted.Broadcast(false, LastOnlineDiagnostic);
		return false;
	}

	const FName SessionName(NAME_GameSession);
	if (Sessions->GetNamedSession(SessionName) != nullptr)
	{
		if (DestroySessionDelegateHandle.IsValid())
		{
			Sessions->ClearOnDestroySessionCompleteDelegate_Handle(DestroySessionDelegateHandle);
			DestroySessionDelegateHandle.Reset();
		}

		FOnDestroySessionCompleteDelegate DestroyDelegate;
		DestroyDelegate.BindUObject(this, &UUIWorldMenuGameInstance::OnDestroySessionBeforeHost);
		DestroySessionDelegateHandle = Sessions->AddOnDestroySessionCompleteDelegate_Handle(DestroyDelegate);
		bPendingHostAfterDestroy = true;

		const bool bDestroyStarted = Sessions->DestroySession(SessionName);
		if (!bDestroyStarted)
		{
			bPendingHostAfterDestroy = false;
			if (DestroySessionDelegateHandle.IsValid())
			{
				Sessions->ClearOnDestroySessionCompleteDelegate_Handle(DestroySessionDelegateHandle);
				DestroySessionDelegateHandle.Reset();
			}
			return BeginHostCreateSession(PendingHostMaxPlayers, bLAN);
		}
		return true;
	}

	return BeginHostCreateSession(PendingHostMaxPlayers, bLAN);
}

void UUIWorldMenuGameInstance::SortAndPublishFoundSessions(bool bWasSuccessful)
{
	if (bWasSuccessful && LastSessionSearchNative.IsValid())
	{
		LastFoundSessions.Reset();
		for (int32 Index = 0; Index < LastSessionSearchNative->SearchResults.Num(); ++Index)
		{
			LastFoundSessions.Add(
				UIWorldConvertSearchResult(this, LastSessionSearchNative->SearchResults[Index], Index));
		}

		LastFoundSessions.Sort([](const FUIWorldOnlineSessionResult& A, const FUIWorldOnlineSessionResult& B)
		{
			if (A.bIsFull != B.bIsFull)
			{
				return !A.bIsFull;
			}
			if (A.bBuildCompatible != B.bBuildCompatible)
			{
				return A.bBuildCompatible;
			}
			const int32 PingA = A.PingMs >= 0 ? A.PingMs : 99999;
			const int32 PingB = B.PingMs >= 0 ? B.PingMs : 99999;
			return PingA < PingB;
		});
	}
	else if (!bWasSuccessful)
	{
		LastFoundSessions.Reset();
	}
}

int32 UUIWorldMenuGameInstance::FindBestQuickJoinIndex() const
{
	int32 BestIndex = INDEX_NONE;
	int32 BestPing = MAX_int32;

	for (const FUIWorldOnlineSessionResult& Session : LastFoundSessions)
	{
		if (Session.bIsFull || !Session.bBuildCompatible || Session.bPasswordProtected)
		{
			continue;
		}

		const int32 Ping = Session.PingMs >= 0 ? Session.PingMs : 9999;
		if (Ping < BestPing && Session.SearchResultIndex != INDEX_NONE)
		{
			BestPing = Ping;
			BestIndex = Session.SearchResultIndex;
		}
	}
	return BestIndex;
}

bool UUIWorldMenuGameInstance::TryExecuteQuickJoin()
{
	bPendingQuickJoin = false;
	const int32 BestIndex = FindBestQuickJoinIndex();
	if (BestIndex == INDEX_NONE)
	{
		LastOnlineDiagnostic = TEXT("Quick Join: no open compatible sessions. Host a match or try REFRESH.");
		OnJoinCompleted.Broadcast(false, LastOnlineDiagnostic);
		return false;
	}

	for (const FUIWorldOnlineSessionResult& Session : LastFoundSessions)
	{
		if (Session.SearchResultIndex == BestIndex)
		{
			UE_LOG(
				LogUIWorldStartupFlow,
				Log,
				TEXT("[Online] Quick Join -> \"%s\" ping=%d players=%d/%d"),
				*Session.ServerName,
				Session.PingMs,
				Session.CurrentPlayers,
				Session.MaxPlayers);
			break;
		}
	}

	BeginOnlineTravelWithPhase(
		EZonefallOnlineTravelPhase::Joining,
		NSLOCTEXT("ZonefallUI", "OnlineQuickJoin", "Finding the best session..."));
	return JoinOnlineSessionByIndex(BestIndex);
}

bool UUIWorldMenuGameInstance::QuickJoinOnlineSession(bool bLAN)
{
	bLastOnlineQueryWasLAN = bLAN;
	bPendingQuickJoin = true;
	LastOnlineDiagnostic.Reset();

	if (LastSessionSearchNative.IsValid() && LastFoundSessions.Num() > 0)
	{
		return TryExecuteQuickJoin();
	}

	return FindOnlineSessions(100, bLAN);
}

bool UUIWorldMenuGameInstance::FindOnlineSessions(int32 MaxResults, bool bLAN)
{
	bLastOnlineQueryWasLAN = bLAN;
	LastOnlineDiagnostic.Reset();
	UWorld* World = GetWorld();
	if (!World)
	{
		LastOnlineDiagnostic = TEXT("World is null");
		OnSessionsFound.Broadcast(TArray<FUIWorldOnlineSessionResult>());
		return false;
	}

	IOnlineSessionPtr Sessions = UIWorldGetSessionInterface(bLAN);
	if (!Sessions.IsValid())
	{
		LastOnlineDiagnostic = FString::Printf(
			TEXT("SessionInterface missing (LAN=%d OSS=%s)"),
			bLAN ? 1 : 0,
			*UIWorldDescribeActiveOnlineSubsystem());
		OnSessionsFound.Broadcast(TArray<FUIWorldOnlineSessionResult>());
		return false;
	}

	const TSharedPtr<const FUniqueNetId> UserId = bLAN
		? UIWorldGetOrCreateLanUserId(World, true)
		: UIWorldGetLocalUserId(World, false);
	if (!UserId.IsValid() && !bLAN)
	{
		bPendingFindOnlineSessions = true;
		PendingFindMaxResults = MaxResults;
		bPendingFindLAN = bLAN;
		EnsureOnlineAutoLogin();
		LastOnlineDiagnostic = FString::Printf(
			TEXT("Waiting for %s login... (Steam must be running; LAN mode must match the host)"),
			*UIWorldDescribeActiveOnlineSubsystem());
		UE_LOG(LogUIWorldStartupFlow, Log, TEXT("[Online] %s"), *LastOnlineDiagnostic);
		// Do not broadcast an empty result yet — login completion will retry FindSessions.
		return false;
	}
	LastSessionSearchNative = MakeShared<FOnlineSessionSearch>();
	LastSessionSearchNative->bIsLanQuery = bLAN;
	LastSessionSearchNative->MaxSearchResults = FMath::Clamp(MaxResults, 1, 500);
	// Keep searches snappy — LAN resolves in well under a second; cap Steam so it never hangs.
	LastSessionSearchNative->TimeoutInSeconds = bLAN ? 3.0f : 8.0f;
	LastSessionSearchNative->PingBucketSize = 50;

	// Steam lobby search needs an explicit lobby query when not on LAN.
	if (!bLAN)
	{
		LastSessionSearchNative->QuerySettings.Set(SEARCH_LOBBIES, true, EOnlineComparisonOp::Equals);
	}

	if (FindSessionsDelegateHandle.IsValid())
	{
		Sessions->ClearOnFindSessionsCompleteDelegate_Handle(FindSessionsDelegateHandle);
		FindSessionsDelegateHandle.Reset();
	}

	FOnFindSessionsCompleteDelegate FindDelegate;
	FindDelegate.BindWeakLambda(this, [this](bool bWasSuccessful)
	{
		if (IOnlineSessionPtr LocalSessions = UIWorldGetSessionInterface(bLastOnlineQueryWasLAN))
		{
			if (FindSessionsDelegateHandle.IsValid())
			{
				LocalSessions->ClearOnFindSessionsCompleteDelegate_Handle(FindSessionsDelegateHandle);
				FindSessionsDelegateHandle.Reset();
			}
		}

		SortAndPublishFoundSessions(bWasSuccessful);
		if (!bWasSuccessful)
		{
			LastOnlineDiagnostic = bLastOnlineQueryWasLAN
				? FString::Printf(
					TEXT("LAN search failed (OSS=%s). Enable LAN on host and client."),
					*UIWorldDescribeActiveOnlineSubsystem(true))
				: FString::Printf(
					TEXT("Steam search failed (OSS=%s). Is Steam running? Same BuildId on both PCs."),
					*UIWorldDescribeActiveOnlineSubsystem(false));
		}
		else if (LastFoundSessions.Num() == 0)
		{
			LastOnlineDiagnostic = bLastOnlineQueryWasLAN
				? FString::Printf(
					TEXT("No LAN sessions (OSS=%s). PIE: 2 players, Player 1 HOST, wait for map, then REFRESH on Player 2. Or CONNECT 127.0.0.1:%d"),
					*UIWorldDescribeActiveOnlineSubsystem(true),
					FMath::Clamp(OnlineLanPort, 1, 65535))
				: FString::Printf(
					TEXT("No Steam sessions (OSS=%s). Host with LAN off; both need Steam."),
					*UIWorldDescribeActiveOnlineSubsystem(false));
		}
		UE_LOG(
			LogUIWorldStartupFlow,
			Log,
			TEXT("[Online] FindSessions complete: success=%d count=%d mode=%s OSS=%s"),
			bWasSuccessful,
			LastFoundSessions.Num(),
			bLastOnlineQueryWasLAN ? TEXT("LAN") : TEXT("Steam"),
			*UIWorldDescribeActiveOnlineSubsystem(bLastOnlineQueryWasLAN));

		if (bPendingQuickJoin)
		{
			if (!TryExecuteQuickJoin())
			{
				OnSessionsFound.Broadcast(LastFoundSessions);
			}
			return;
		}

		OnSessionsFound.Broadcast(LastFoundSessions);
	});

	FindSessionsDelegateHandle = Sessions->AddOnFindSessionsCompleteDelegate_Handle(FindDelegate);
	const bool bStarted = Sessions->FindSessions(*UserId, LastSessionSearchNative.ToSharedRef());
	if (!bStarted)
	{
		LastOnlineDiagnostic = TEXT("FindSessions did not start");
		OnSessionsFound.Broadcast(TArray<FUIWorldOnlineSessionResult>());
	}
	return bStarted;
}

bool UUIWorldMenuGameInstance::DestroyExistingSessionThenJoin(int32 ResultIndex)
{
	const bool bLan = UIWorldIsLanJoinForIndex(LastSessionSearchNative, ResultIndex, bLastOnlineQueryWasLAN);
	bLastOnlineQueryWasLAN = bLan;
	IOnlineSessionPtr Sessions = UIWorldGetSessionInterface(bLan);
	if (!Sessions.IsValid())
	{
		OnJoinCompleted.Broadcast(false, TEXT("SessionInterface missing"));
		return false;
	}

	PendingJoinSessionIndex = ResultIndex;

	if (DestroySessionDelegateHandle.IsValid())
	{
		Sessions->ClearOnDestroySessionCompleteDelegate_Handle(DestroySessionDelegateHandle);
		DestroySessionDelegateHandle.Reset();
	}

	FOnDestroySessionCompleteDelegate DestroyDelegate;
	DestroyDelegate.BindWeakLambda(this, [this](FName /*Name*/, bool bDestroyed)
	{
		const bool bLanDestroy = bLastOnlineQueryWasLAN;
		if (IOnlineSessionPtr LocalSessions = UIWorldGetSessionInterface(bLanDestroy))
		{
			if (DestroySessionDelegateHandle.IsValid())
			{
				LocalSessions->ClearOnDestroySessionCompleteDelegate_Handle(DestroySessionDelegateHandle);
				DestroySessionDelegateHandle.Reset();
			}
		}

		const int32 IndexToJoin = PendingJoinSessionIndex;
		PendingJoinSessionIndex = INDEX_NONE;

		if (!bDestroyed || IndexToJoin == INDEX_NONE)
		{
			CancelOnlineTravelLoading();
			OnJoinCompleted.Broadcast(false, TEXT("Join failed: could not clear old session"));
			return;
		}

		if (bLanDestroy)
		{
			JoinLanSessionDirect(IndexToJoin);
		}
		else
		{
			ExecuteJoinOnlineSession(IndexToJoin);
		}
	});
	DestroySessionDelegateHandle = Sessions->AddOnDestroySessionCompleteDelegate_Handle(DestroyDelegate);

	const bool bStarted = Sessions->DestroySession(NAME_GameSession);
	if (!bStarted)
	{
		if (DestroySessionDelegateHandle.IsValid())
		{
			Sessions->ClearOnDestroySessionCompleteDelegate_Handle(DestroySessionDelegateHandle);
			DestroySessionDelegateHandle.Reset();
		}
		PendingJoinSessionIndex = INDEX_NONE;
		return bLastOnlineQueryWasLAN
			? JoinLanSessionDirect(ResultIndex)
			: ExecuteJoinOnlineSession(ResultIndex);
	}
	return true;
}

bool UUIWorldMenuGameInstance::ExecuteJoinOnlineSession(int32 ResultIndex)
{
	UWorld* World = GetWorld();
	if (!World)
	{
		OnJoinCompleted.Broadcast(false, TEXT("World is null"));
		return false;
	}

	if (!LastSessionSearchNative.IsValid() || !LastSessionSearchNative->SearchResults.IsValidIndex(ResultIndex))
	{
		OnJoinCompleted.Broadcast(false, TEXT("Join failed: invalid result index"));
		return false;
	}

	const bool bLan = UIWorldIsLanJoinForIndex(LastSessionSearchNative, ResultIndex, bLastOnlineQueryWasLAN);
	bLastOnlineQueryWasLAN = bLan;

	// LAN always uses direct IP ClientTravel — JoinSession returns Wi-Fi beacon IP and times out in PIE.
	if (bLan)
	{
		return JoinLanSessionDirect(ResultIndex);
	}

	const FOnlineSessionSearchResult& ChosenResult = LastSessionSearchNative->SearchResults[ResultIndex];
	if (ChosenResult.Session.NumOpenPublicConnections == 0
		&& ChosenResult.Session.SessionSettings.NumPublicConnections > 0)
	{
		OnJoinCompleted.Broadcast(false, TEXT("Join failed: session is full"));
		return false;
	}

	UIWorldActivateSteamNetworking(World);

	IOnlineSessionPtr Sessions = UIWorldGetSessionInterface(false);
	if (!Sessions.IsValid())
	{
		OnJoinCompleted.Broadcast(false, TEXT("Steam session interface missing — is Steam running?"));
		return false;
	}

	const TSharedPtr<const FUniqueNetId> UserId = UIWorldGetLocalUserId(World, false);
	if (!UserId.IsValid())
	{
		EnsureOnlineAutoLogin();
		OnJoinCompleted.Broadcast(false, TEXT("Not logged in to Steam yet"));
		return false;
	}

	if (JoinSessionDelegateHandle.IsValid())
	{
		Sessions->ClearOnJoinSessionCompleteDelegate_Handle(JoinSessionDelegateHandle);
		JoinSessionDelegateHandle.Reset();
	}

	PendingJoinConnectSearchIndex = ResultIndex;
	JoinConnectRetryAttempts = 0;

	FOnJoinSessionCompleteDelegate JoinDelegate;
	JoinDelegate.BindWeakLambda(this, [this, World, ResultIndex, bLan](FName Name, EOnJoinSessionCompleteResult::Type JoinResult)
	{
		IOnlineSessionPtr LocalSessions = UIWorldGetSessionInterface(bLan);
		if (LocalSessions.IsValid() && JoinSessionDelegateHandle.IsValid())
		{
			LocalSessions->ClearOnJoinSessionCompleteDelegate_Handle(JoinSessionDelegateHandle);
			JoinSessionDelegateHandle.Reset();
		}

		if (JoinResult != EOnJoinSessionCompleteResult::Success)
		{
			PendingJoinConnectSearchIndex = INDEX_NONE;
			if (UWorld* TimerWorld = GetWorld())
			{
				TimerWorld->GetTimerManager().ClearTimer(JoinConnectRetryTimerHandle);
			}
			CancelOnlineTravelLoading();
			const FString FailReason = UIWorldDescribeJoinResult(JoinResult);
			OnJoinCompleted.Broadcast(false, FString::Printf(TEXT("Join failed: %s"), *FailReason));
			return;
		}

		if (TryCompleteJoinTravel(World, ResultIndex))
		{
			return;
		}

		// Host may still be loading the map / publishing lobby P2P keys — retry briefly.
		if (UWorld* TimerWorld = GetWorld())
		{
			TimerWorld->GetTimerManager().ClearTimer(JoinConnectRetryTimerHandle);
			TimerWorld->GetTimerManager().SetTimer(
				JoinConnectRetryTimerHandle,
				this,
				&UUIWorldMenuGameInstance::HandleJoinConnectRetry,
				0.5f,
				true);
		}
	});

	JoinSessionDelegateHandle = Sessions->AddOnJoinSessionCompleteDelegate_Handle(JoinDelegate);
	BeginOnlineTravelWithPhase(
		EZonefallOnlineTravelPhase::Joining,
		NSLOCTEXT("ZonefallUI", "OnlineJoinStart", "Joining online session..."));

	const bool bStarted = Sessions->JoinSession(*UserId, NAME_GameSession, LastSessionSearchNative->SearchResults[ResultIndex]);
	if (!bStarted)
	{
		if (JoinSessionDelegateHandle.IsValid())
		{
			Sessions->ClearOnJoinSessionCompleteDelegate_Handle(JoinSessionDelegateHandle);
			JoinSessionDelegateHandle.Reset();
		}
		CancelOnlineTravelLoading();
		OnJoinCompleted.Broadcast(false, TEXT("JoinSession did not start"));
	}
	return bStarted;
}

bool UUIWorldMenuGameInstance::JoinOnlineSessionByIndex(int32 ResultIndex)
{
	if (!LastSessionSearchNative.IsValid() || !LastSessionSearchNative->SearchResults.IsValidIndex(ResultIndex))
	{
		OnJoinCompleted.Broadcast(false, TEXT("Join failed: session list expired — press REFRESH"));
		return false;
	}

	const FOnlineSessionSearchResult& TargetResult = LastSessionSearchNative->SearchResults[ResultIndex];
	FString JoinError;
	if (!UIWorldValidateSessionJoin(this, &TargetResult, JoinError))
	{
		OnJoinCompleted.Broadcast(false, JoinError);
		return false;
	}

	const bool bLan = UIWorldIsLanJoinForIndex(LastSessionSearchNative, ResultIndex, bLastOnlineQueryWasLAN);
	bLastOnlineQueryWasLAN = bLan;
	IOnlineSessionPtr Sessions = UIWorldGetSessionInterface(bLan);
	if (!Sessions.IsValid())
	{
		OnJoinCompleted.Broadcast(false, TEXT("SessionInterface missing"));
		return false;
	}

	// DestroySession is async — JoinSession fails with "already exists" if we don't wait.
	if (Sessions->GetNamedSession(NAME_GameSession) != nullptr)
	{
		return DestroyExistingSessionThenJoin(ResultIndex);
	}

	if (bLan)
	{
		return JoinLanSessionDirect(ResultIndex);
	}

	return ExecuteJoinOnlineSession(ResultIndex);
}

bool UUIWorldMenuGameInstance::JoinOnlineByAddress(const FString& Address)
{
	UWorld* World = GetWorld();
	if (!World)
	{
		OnJoinCompleted.Broadcast(false, TEXT("World is null"));
		return false;
	}

	FString Clean = Address.TrimStartAndEnd();
	if (Clean.IsEmpty())
	{
		OnJoinCompleted.Broadcast(false, TEXT("Enter an address or ID to connect"));
		return false;
	}

	// Bare IP/hostname (no port, not a steam URL) -> append the default LAN port.
	const bool bIsSteamUrl = Clean.StartsWith(TEXT("steam."));
	if (!bIsSteamUrl && !Clean.Contains(TEXT(":")))
	{
		Clean = FString::Printf(TEXT("%s:%d"), *Clean, FMath::Clamp(OnlineLanPort, 1, 65535));
	}

	UIWorldNormalizeConnectStringForPIE(Clean);

	APlayerController* PC = UGameplayStatics::GetPlayerController(World, 0);
	if (!PC)
	{
		OnJoinCompleted.Broadcast(false, TEXT("Join failed: no player controller"));
		return false;
	}

	bLastOnlineQueryWasLAN = !bIsSteamUrl;
	PendingJoinConnectSearchIndex = INDEX_NONE;
	JoinConnectRetryAttempts = 0;
	if (bIsSteamUrl)
	{
		UIWorldActivateSteamNetworking(World);
	}
	else
	{
		UIWorldActivateLanNetworking(World);
	}
	BeginOnlineTravelWithPhase(
		EZonefallOnlineTravelPhase::Joining,
		NSLOCTEXT("ZonefallUI", "OnlineDirectStart", "Connecting to server..."));

	FString TravelUrl = bIsSteamUrl
		? FString::Printf(TEXT("%s?NetDriver=GameNetDriver"), *Clean)
		: UIWorldAppendLanNetDriverOption(Clean);
	UE_LOG(LogUIWorldStartupFlow, Log, TEXT("[Online] Direct connect -> ClientTravel %s"), *TravelUrl);
	bApplyGameFocusOnNextMapLoad = true;
	CloseMenuUI(false);
	OnJoinCompleted.Broadcast(true, FString::Printf(TEXT("Connecting to %s"), *Clean));
	PC->ClientTravel(TravelUrl, TRAVEL_Absolute);
	return true;
}

bool UUIWorldMenuGameInstance::JoinLanSessionDirect(int32 ResultIndex)
{
	UWorld* World = GetWorld();
	if (!World)
	{
		OnJoinCompleted.Broadcast(false, TEXT("World is null"));
		return false;
	}

	if (!LastSessionSearchNative.IsValid() || !LastSessionSearchNative->SearchResults.IsValidIndex(ResultIndex))
	{
		OnJoinCompleted.Broadcast(false, TEXT("Join failed: invalid result index"));
		return false;
	}

	bLastOnlineQueryWasLAN = true;
	const FOnlineSessionSearchResult& ChosenResult = LastSessionSearchNative->SearchResults[ResultIndex];
	if (ChosenResult.Session.NumOpenPublicConnections == 0
		&& ChosenResult.Session.SessionSettings.NumPublicConnections > 0)
	{
		OnJoinCompleted.Broadcast(false, TEXT("Join failed: session is full"));
		return false;
	}

	IOnlineSessionPtr Sessions = UIWorldGetSessionInterface(true);
	FString ConnectString;
	const FOnlineSessionSearchResult* SearchResult = &LastSessionSearchNative->SearchResults[ResultIndex];
	if (!UIWorldResolveSessionConnectString(this, Sessions, NAME_GameSession, SearchResult, true, ConnectString))
	{
		OnJoinCompleted.Broadcast(
			false,
			FString::Printf(
				TEXT("Join failed: no LAN address (host must be in-game; default port %d)"),
				OnlineLanPort));
		return false;
	}

	PendingJoinConnectSearchIndex = INDEX_NONE;
	JoinConnectRetryAttempts = 0;
	UIWorldActivateLanNetworking(World);
	BeginOnlineTravelWithPhase(
		EZonefallOnlineTravelPhase::Joining,
		NSLOCTEXT("ZonefallUI", "OnlineLanJoinStart", "Connecting to LAN host..."));

	const FString TravelUrl = UIWorldAppendLanNetDriverOption(ConnectString);
	UE_LOG(LogUIWorldStartupFlow, Log, TEXT("[Online] LAN direct join -> ClientTravel %s"), *TravelUrl);
	if (APlayerController* PC = UGameplayStatics::GetPlayerController(World, 0))
	{
		bApplyGameFocusOnNextMapLoad = true;
		CloseMenuUI(false);
		OnJoinCompleted.Broadcast(true, FString::Printf(TEXT("Connecting %s"), *ConnectString));
		PC->ClientTravel(TravelUrl, TRAVEL_Absolute);
		return true;
	}

	CancelOnlineTravelLoading();
	OnJoinCompleted.Broadcast(false, TEXT("Join failed: no player controller"));
	return false;
}

void UUIWorldMenuGameInstance::PublishHostedOnlineSession(UWorld* LoadedWorld)
{
	if (!LoadedWorld || LoadedWorld->GetNetMode() != NM_ListenServer)
	{
		return;
	}

	if (bLastOnlineQueryWasLAN)
	{
		IOnlineSessionPtr LanSessions = UIWorldGetSessionInterface(true);
		if (LanSessions.IsValid())
		{
			if (FNamedOnlineSession* Session = LanSessions->GetNamedSession(NAME_GameSession))
			{
				int32 AdvertisedPort = FMath::Clamp(OnlineLanPort, 1, 65535);
				if (UNetDriver* NetDriver = LoadedWorld->GetNetDriver())
				{
					if (TSharedPtr<const FInternetAddr> LocalAddr = NetDriver->GetLocalAddr())
					{
						const int32 BoundPort = LocalAddr->GetPort();
						if (BoundPort > 0)
						{
							AdvertisedPort = BoundPort;
						}
					}
				}

				Session->SessionSettings.Set(
					FName(TEXT("LAN_HOST")),
					FString(TEXT("127.0.0.1")),
					EOnlineDataAdvertisementType::ViaOnlineServiceAndPing);
				Session->SessionSettings.Set(
					FName(TEXT("LAN_PORT")), AdvertisedPort, EOnlineDataAdvertisementType::ViaOnlineServiceAndPing);
				LanSessions->StartSession(NAME_GameSession);
				LanSessions->UpdateSession(NAME_GameSession, Session->SessionSettings, true);
				FString DriverName = TEXT("unknown");
				if (UNetDriver* NetDriver = LoadedWorld->GetNetDriver())
				{
					DriverName = NetDriver->GetClass()->GetName();
				}
				if (!DriverName.Contains(TEXT("IpNet")))
				{
					UE_LOG(
						LogUIWorldStartupFlow,
						Warning,
						TEXT("[Online] LAN host is NOT on IpNetDriver (%s) — clients will timeout"),
						*DriverName);
				}
				UE_LOG(
					LogUIWorldStartupFlow,
					Log,
					TEXT("[Online] LAN listen server ready — connect 127.0.0.1:%d (map %s, driver %s)"),
					AdvertisedPort,
					*LoadedWorld->GetMapName(),
					*DriverName);
				return;
			}
		}

		UE_LOG(
			LogUIWorldStartupFlow,
			Log,
			TEXT("[Online] LAN listen server on %s (no session to update)"),
			*LoadedWorld->GetMapName());
		return;
	}

	IOnlineSessionPtr Sessions = UIWorldGetSessionInterface(false);
	if (!Sessions.IsValid())
	{
		return;
	}

	FNamedOnlineSession* Session = Sessions->GetNamedSession(NAME_GameSession);
	if (!Session)
	{
		return;
	}

	Sessions->StartSession(NAME_GameSession);

	if (const TSharedPtr<const FUniqueNetId> UserId = UIWorldGetLocalUserId(LoadedWorld, bLastOnlineQueryWasLAN))
	{
		Sessions->RegisterPlayer(NAME_GameSession, *UserId, false);
	}

	// Push P2P keys into the Steam lobby so remote clients can build a connect URL.
	Sessions->UpdateSession(NAME_GameSession, Session->SessionSettings, true);
	UE_LOG(LogUIWorldStartupFlow, Log, TEXT("[Online] Published Steam lobby (listen server on %s)"), *LoadedWorld->GetMapName());
}

void UUIWorldMenuGameInstance::ScheduleHostedOnlineSessionRefresh(UWorld* LoadedWorld)
{
	if (!LoadedWorld || LoadedWorld->GetNetMode() != NM_ListenServer)
	{
		return;
	}

	HostedSessionPublishAttempts = 0;
	PublishHostedOnlineSession(LoadedWorld);

	LoadedWorld->GetTimerManager().ClearTimer(HostedSessionRefreshTimerHandle);
	LoadedWorld->GetTimerManager().SetTimer(
		HostedSessionRefreshTimerHandle,
		this,
		&UUIWorldMenuGameInstance::HandleHostedSessionRefreshTick,
		1.0f,
		true);
}

void UUIWorldMenuGameInstance::HandleHostedSessionRefreshTick()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	if (++HostedSessionPublishAttempts > 5)
	{
		World->GetTimerManager().ClearTimer(HostedSessionRefreshTimerHandle);
		return;
	}

	PublishHostedOnlineSession(World);
}

bool UUIWorldMenuGameInstance::TryCompleteJoinTravel(UWorld* World, int32 SearchIndex)
{
	if (!World)
	{
		return false;
	}

	const bool bLan = UIWorldIsLanJoinForIndex(LastSessionSearchNative, SearchIndex, bLastOnlineQueryWasLAN);
	bLastOnlineQueryWasLAN = bLan;
	IOnlineSessionPtr Sessions = UIWorldGetSessionInterface(bLan);
	if (!Sessions.IsValid())
	{
		return false;
	}

	// Steam JoinSession path must not run for LAN — use loopback direct connect in PIE.
	if (bLan)
	{
		return JoinLanSessionDirect(SearchIndex);
	}

	const FOnlineSessionSearchResult* SearchResult = nullptr;
	if (LastSessionSearchNative.IsValid() && LastSessionSearchNative->SearchResults.IsValidIndex(SearchIndex))
	{
		SearchResult = &LastSessionSearchNative->SearchResults[SearchIndex];
	}

	FString ConnectString;
	if (!UIWorldResolveSessionConnectString(this, Sessions, NAME_GameSession, SearchResult, bLan, ConnectString))
	{
		return false;
	}

	UIWorldNormalizeConnectStringForPIE(ConnectString);

	if (UWorld* TimerWorld = GetWorld())
	{
		TimerWorld->GetTimerManager().ClearTimer(JoinConnectRetryTimerHandle);
		TimerWorld->GetTimerManager().ClearTimer(OnlineAbortDeferHandle);
	}

	PendingJoinConnectSearchIndex = INDEX_NONE;
	JoinConnectRetryAttempts = 0;

	if (!bOnlineTravelInProgress)
	{
		BeginOnlineTravelWithPhase(
			EZonefallOnlineTravelPhase::Joining,
			NSLOCTEXT("ZonefallUI", "OnlineJoinStart", "Joining online session..."));
	}

	UIWorldActivateSteamNetworking(World);
	if (!ConnectString.Contains(TEXT("NetDriver="), ESearchCase::IgnoreCase))
	{
		ConnectString += ConnectString.Contains(TEXT("?")) ? TEXT("&NetDriver=GameNetDriver") : TEXT("?NetDriver=GameNetDriver");
	}

	UE_LOG(LogUIWorldStartupFlow, Log, TEXT("[Online] Steam join -> ClientTravel %s"), *ConnectString);
	if (APlayerController* PC = UGameplayStatics::GetPlayerController(World, 0))
	{
		bApplyGameFocusOnNextMapLoad = true;
		CloseMenuUI(false);
		OnJoinCompleted.Broadcast(true, FString::Printf(TEXT("Connecting %s"), *ConnectString));
		PC->ClientTravel(ConnectString, TRAVEL_Absolute);
		return true;
	}

	CancelOnlineTravelLoading();
	OnJoinCompleted.Broadcast(false, TEXT("Join failed: no player controller"));
	return false;
}

void UUIWorldMenuGameInstance::HandleJoinConnectRetry()
{
	UWorld* World = GetWorld();
	if (!World || !bOnlineTravelInProgress)
	{
		if (World)
		{
			World->GetTimerManager().ClearTimer(JoinConnectRetryTimerHandle);
		}
		return;
	}

	++JoinConnectRetryAttempts;
	const int32 SearchIndex = PendingJoinConnectSearchIndex;

	if (TryCompleteJoinTravel(World, SearchIndex))
	{
		return;
	}

	constexpr int32 MaxAttempts = 24;
	if (JoinConnectRetryAttempts >= MaxAttempts)
	{
		World->GetTimerManager().ClearTimer(JoinConnectRetryTimerHandle);
		PendingJoinConnectSearchIndex = INDEX_NONE;
		CancelOnlineTravelLoading();
		const FString FailMsg = bLastOnlineQueryWasLAN
			? FString::Printf(
				TEXT("Join failed: no LAN address — both must use LAN, host in-game on port %d, then REFRESH + JOIN"),
				OnlineLanPort)
			: TEXT(
				"Join failed: no Steam URL — on one PC enable LAN on both; for Steam use 2 accounts + host in-game first");
		OnJoinCompleted.Broadcast(false, FailMsg);
	}
}

bool UUIWorldMenuGameInstance::LeaveOnlineSessionAndReturnToMenu()
{
	IOnlineSessionPtr Sessions = UIWorldGetSessionInterface(bLastOnlineQueryWasLAN);
	if (!Sessions.IsValid())
	{
		OnLeaveCompleted.Broadcast(false, TEXT("SessionInterface missing"));
		return false;
	}

	// No active session: nothing to destroy, just bounce back to the menu level.
	if (Sessions->GetNamedSession(NAME_GameSession) == nullptr)
	{
		OnLeaveCompleted.Broadcast(true, TEXT("No active session"));
		if (!MainMenuLevelName.IsNone())
		{
			LoadLevelAndFocusGame(MainMenuLevelName, true);
		}
		return true;
	}

	// Clear any previously bound handler so repeat Leave calls don't accumulate listeners.
	if (DestroySessionDelegateHandle.IsValid())
	{
		Sessions->ClearOnDestroySessionCompleteDelegate_Handle(DestroySessionDelegateHandle);
		DestroySessionDelegateHandle.Reset();
	}

	FOnDestroySessionCompleteDelegate DestroyDelegate;
	DestroyDelegate.BindWeakLambda(this, [this](FName Name, bool bOk)
	{
		if (IOnlineSessionPtr LocalSessions = UIWorldGetSessionInterface())
		{
			if (DestroySessionDelegateHandle.IsValid())
			{
				LocalSessions->ClearOnDestroySessionCompleteDelegate_Handle(DestroySessionDelegateHandle);
				DestroySessionDelegateHandle.Reset();
			}
		}

		OnLeaveCompleted.Broadcast(bOk, bOk ? TEXT("Left session") : TEXT("DestroySession failed"));

		// Only return to the menu once the session has actually been torn down,
		// so we don't ClientTravel out from underneath an in-flight destroy.
		if (!MainMenuLevelName.IsNone())
		{
			LoadLevelAndFocusGame(MainMenuLevelName, true);
		}
	});
	DestroySessionDelegateHandle = Sessions->AddOnDestroySessionCompleteDelegate_Handle(DestroyDelegate);

	const bool bStarted = Sessions->DestroySession(NAME_GameSession);
	if (!bStarted)
	{
		if (DestroySessionDelegateHandle.IsValid())
		{
			Sessions->ClearOnDestroySessionCompleteDelegate_Handle(DestroySessionDelegateHandle);
			DestroySessionDelegateHandle.Reset();
		}
		OnLeaveCompleted.Broadcast(true, TEXT("No active session"));
		if (!MainMenuLevelName.IsNone())
		{
			LoadLevelAndFocusGame(MainMenuLevelName, true);
		}
	}
	return true;
}

// ---------------------------------------------------------------------------
// Online account / status (Steam persona, login state) — drives the menu indicator.
// ---------------------------------------------------------------------------
bool UUIWorldMenuGameInstance::IsOnlineAvailable() const
{
	return IOnlineSubsystem::Get() != nullptr;
}

bool UUIWorldMenuGameInstance::IsOnlineLoggedIn() const
{
	if (IOnlineSubsystem* OSS = IOnlineSubsystem::Get())
	{
		if (IOnlineIdentityPtr Identity = OSS->GetIdentityInterface())
		{
			return Identity->GetLoginStatus(0) == ELoginStatus::LoggedIn;
		}
	}
	return false;
}

FString UUIWorldMenuGameInstance::GetOnlineServiceName() const
{
	if (const IOnlineSubsystem* OSS = IOnlineSubsystem::Get())
	{
		return OSS->GetSubsystemName().ToString();
	}
	return TEXT("None");
}

FString UUIWorldMenuGameInstance::GetOnlinePlayerNickname() const
{
	if (IOnlineSubsystem* OSS = IOnlineSubsystem::Get())
	{
		if (IOnlineIdentityPtr Identity = OSS->GetIdentityInterface())
		{
			const FString PlayerNick = Identity->GetPlayerNickname(0);
			if (!PlayerNick.IsEmpty())
			{
				return PlayerNick;
			}
		}
	}

	// Local fallback (offline / Steam not running).
	if (UWorld* World = GetWorld())
	{
		if (APlayerController* PC = UGameplayStatics::GetPlayerController(World, 0))
		{
			if (PC->PlayerState && !PC->PlayerState->GetPlayerName().IsEmpty())
			{
				return PC->PlayerState->GetPlayerName();
			}
		}
	}
	return FString(FPlatformProcess::ComputerName());
}

int32 UUIWorldMenuGameInstance::GetCurrentSessionPlayerCount() const
{
	if (IOnlineSessionPtr Sessions = UIWorldGetSessionInterface(bLastOnlineQueryWasLAN))
	{
		if (FNamedOnlineSession* Session = Sessions->GetNamedSession(NAME_GameSession))
		{
			const int32 Max = FMath::Max(1, Session->SessionSettings.NumPublicConnections);
			const int32 Open = FMath::Max(0, Session->NumOpenPublicConnections);
			return FMath::Clamp(Max - Open, 1, Max);
		}
	}
	return 0;
}

void UUIWorldMenuGameInstance::RequestOnlineLogin()
{
	EnsureOnlineAutoLogin();
}

// ---------------------------------------------------------------------------
// Achievements (Steam-backed, with an offline local fallback)
// ---------------------------------------------------------------------------
void UUIWorldMenuGameInstance::LoadLocalAchievements()
{
	UnlockedAchievementIds.Reset();
	TArray<FString> Saved;
	GConfig->GetArray(TEXT("ZonefallAchievements"), TEXT("Unlocked"), Saved, GGameUserSettingsIni);
	for (const FString& S : Saved)
	{
		if (!S.IsEmpty())
		{
			UnlockedAchievementIds.AddUnique(FName(*S));
		}
	}
}

void UUIWorldMenuGameInstance::SaveLocalAchievements() const
{
	TArray<FString> ToSave;
	ToSave.Reserve(UnlockedAchievementIds.Num());
	for (const FName& Id : UnlockedAchievementIds)
	{
		ToSave.Add(Id.ToString());
	}
	GConfig->SetArray(TEXT("ZonefallAchievements"), TEXT("Unlocked"), ToSave, GGameUserSettingsIni);
	GConfig->Flush(false, GGameUserSettingsIni);
}

void UUIWorldMenuGameInstance::WriteAchievementToOnlineService(FName AchievementId, float PercentComplete)
{
	IOnlineSubsystem* OSS = IOnlineSubsystem::Get();
	if (!OSS)
	{
		return;
	}

	IOnlineAchievementsPtr Achievements = OSS->GetAchievementsInterface();
	if (!Achievements.IsValid())
	{
		return;
	}

	const TSharedPtr<const FUniqueNetId> UserId = UIWorldGetLocalUserId(GetWorld());
	if (!UserId.IsValid())
	{
		return;
	}

	FOnlineAchievementsWriteRef WriteObject = MakeShared<FOnlineAchievementsWrite, ESPMode::ThreadSafe>();
	WriteObject->SetFloatStat(AchievementId.ToString(), FMath::Clamp(PercentComplete, 0.0f, 100.0f));

	Achievements->WriteAchievements(*UserId, WriteObject,
		FOnAchievementsWrittenDelegate::CreateWeakLambda(this, [AchievementId](const FUniqueNetId&, bool bSuccess)
		{
			UE_LOG(LogUIWorldStartupFlow, Log, TEXT("[Achievements] Steam write '%s' -> %s"),
				*AchievementId.ToString(), bSuccess ? TEXT("OK") : TEXT("failed (define it in Steamworks first)"));
		}));
}

void UUIWorldMenuGameInstance::UnlockAchievement(FName AchievementId, float PercentComplete)
{
	if (AchievementId.IsNone())
	{
		return;
	}

	const bool bComplete = PercentComplete >= 100.0f;
	if (bComplete && !UnlockedAchievementIds.Contains(AchievementId))
	{
		UnlockedAchievementIds.AddUnique(AchievementId);
		SaveLocalAchievements();
		ShowSaveToast(FString::Printf(TEXT("Achievement unlocked: %s"), *AchievementId.ToString()));
		UE_LOG(LogUIWorldStartupFlow, Log, TEXT("[Achievements] Unlocked '%s'"), *AchievementId.ToString());
	}

	WriteAchievementToOnlineService(AchievementId, PercentComplete);
}

bool UUIWorldMenuGameInstance::IsAchievementUnlocked(FName AchievementId) const
{
	return UnlockedAchievementIds.Contains(AchievementId);
}

AZonefallPlayerCharacter* UUIWorldMenuGameInstance::EnsureCharacterCreatorPawn()
{
	UWorld* World = GetWorld();
	APlayerController* PC = World ? UGameplayStatics::GetPlayerController(World, 0) : nullptr;
	if (!World || !PC)
	{
		return nullptr;
	}

	TSubclassOf<AZonefallPlayerCharacter> DesiredClass = CharacterCreatorPawnClass;
	if (!DesiredClass)
	{
		DesiredClass = AZonefallPlayerCharacter::StaticClass();
	}

	if (AZonefallPlayerCharacter* Existing = Cast<AZonefallPlayerCharacter>(PC->GetPawn()))
	{
		if (Existing->GetClass() == DesiredClass)
		{
			return Existing;
		}
	}

	FVector SpawnLocation = FVector::ZeroVector;
	FRotator SpawnRotation = FRotator(0.0f, 180.0f, 0.0f);
	if (const APawn* OldPawn = PC->GetPawn())
	{
		SpawnLocation = OldPawn->GetActorLocation();
		SpawnRotation = OldPawn->GetActorRotation();
	}
	else if (const AActor* Start = UGameplayStatics::GetActorOfClass(World, APlayerStart::StaticClass()))
	{
		SpawnLocation = Start->GetActorLocation();
		SpawnRotation = Start->GetActorRotation();
	}

	if (APawn* OldPawn = PC->GetPawn())
	{
		PC->UnPossess();
		OldPawn->Destroy();
	}

	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
	AZonefallPlayerCharacter* NewPawn = World->SpawnActor<AZonefallPlayerCharacter>(
		DesiredClass, SpawnLocation, SpawnRotation, Params);
	if (NewPawn)
	{
		PC->Possess(NewPawn);
	}

	return NewPawn;
}

bool UUIWorldMenuGameInstance::IsMenuMapWorld(const UWorld* World) const
{
	if (!World)
	{
		return false;
	}

	const FString MapToken = UIWorldStripPieMapPrefix(World->GetMapName());
	if (MapToken.IsEmpty())
	{
		return false;
	}

	const FString MenuShort = MainMenuLevelName.IsNone() ? TEXT("Menu") : MainMenuLevelName.ToString();
	if (MapToken.Equals(MenuShort, ESearchCase::IgnoreCase))
	{
		return true;
	}

	const FName ResolvedMenu = UIWorldResolveLevelNameForOpen(const_cast<UWorld*>(World), MainMenuLevelName);
	if (!ResolvedMenu.IsNone())
	{
		const FString ResolvedShort = FPackageName::GetShortName(ResolvedMenu.ToString());
		if (MapToken.Equals(ResolvedShort, ESearchCase::IgnoreCase))
		{
			return true;
		}
	}

	return false;
}

void UUIWorldMenuGameInstance::HandlePostLoadMap(UWorld* LoadedWorld)
{
	if (LoadedWorld)
	{
		if (bOnlineTravelInProgress)
		{
			if (ActiveOnlineTravelPhase == EZonefallOnlineTravelPhase::Hosting
				&& !IsMenuMapWorld(LoadedWorld))
			{
				TryActivateListenServerAfterHostLoad(LoadedWorld);
				if (bOnlineTravelInProgress
					&& LoadedWorld->GetNetMode() == NM_Standalone
					&& !LoadedWorld->GetNetDriver())
				{
					LoadedWorld->GetTimerManager().SetTimerForNextTick(
						FTimerDelegate::CreateUObject(
							this,
							&UUIWorldMenuGameInstance::TryActivateListenServerAfterHostLoad,
							LoadedWorld));
				}
			}

			if (bOnlineTravelInProgress)
			{
				const ENetMode LoadedNetMode = LoadedWorld->GetNetMode();
				const bool bClientReady = (LoadedNetMode == NM_Client);
				const bool bHostReady =
					(LoadedNetMode == NM_ListenServer || LoadedNetMode == NM_DedicatedServer
						|| (LoadedWorld->GetNetDriver() != nullptr))
					&& !IsMenuMapWorld(LoadedWorld);

				if (bClientReady || bHostReady)
				{
					FinalizeOnlineTravelSuccess(LoadedWorld);
				}
			}
		}
		else
		{
			bOnlineJoinReachedGameMap = false;
			StopMoviePlayerLoadingScreen();
			HideSimpleTravelLoadingScreen();

			if (bApplyGameFocusOnNextMapLoad)
			{
				ApplyGameFocusInput(LoadedWorld);
				bApplyGameFocusOnNextMapLoad = false;
			}
		}
	}
	else
	{
		CancelOnlineTravelLoading();
	}
}

void UUIWorldMenuGameInstance::HandleTravelFailure(UWorld* World, ETravelFailure::Type FailureType, const FString& ErrorString)
{
	UE_LOG(LogUIWorldStartupFlow, Warning, TEXT("[Online] TravelFailure: %s | %s"),
		ETravelFailure::ToString(FailureType), *ErrorString);

	if (bOnlineJoinReachedGameMap || (World && !IsMenuMapWorld(World)))
	{
		return;
	}

	if (!bOnlineTravelInProgress)
	{
		return;
	}

	ScheduleDeferredOnlineAbort(ErrorString.IsEmpty() ? TEXT("Travel failed") : ErrorString);
}

void UUIWorldMenuGameInstance::HandleNetworkFailure(UWorld* World, UNetDriver* /*NetDriver*/, ENetworkFailure::Type FailureType, const FString& ErrorString)
{
	UE_LOG(LogUIWorldStartupFlow, Warning, TEXT("[Online] NetworkFailure: %s | %s"),
		ENetworkFailure::ToString(FailureType), *ErrorString);

	if (bOnlineJoinReachedGameMap || (World && !IsMenuMapWorld(World)))
	{
		return;
	}

	if (!bOnlineTravelInProgress)
	{
		return;
	}

	// Engine already Browse "?closed" back to Menu on hard net failures — don't double-abort instantly.
	if (FailureType == ENetworkFailure::ConnectionTimeout
		|| FailureType == ENetworkFailure::PendingConnectionFailure
		|| FailureType == ENetworkFailure::FailureReceived)
	{
		if (World && IsMenuMapWorld(World))
		{
			CancelOnlineTravelLoading();
			OnJoinCompleted.Broadcast(
				false,
				TEXT("Connection failed — use LAN on both windows; for one PC host must finish loading first"));
			return;
		}

		if (World)
		{
			const float Elapsed = World->GetTimeSeconds() - OnlineTravelStartSeconds;
			if (Elapsed < 15.0f)
			{
				return;
			}
		}
	}

	ScheduleDeferredOnlineAbort(ErrorString.IsEmpty() ? TEXT("Connection failed") : ErrorString);
}

void UUIWorldMenuGameInstance::ScheduleDeferredOnlineAbort(const FString& Reason)
{
	PendingOnlineAbortReason = Reason;
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(OnlineAbortDeferHandle);
		World->GetTimerManager().SetTimer(
			OnlineAbortDeferHandle,
			this,
			&UUIWorldMenuGameInstance::ExecuteDeferredOnlineAbort,
			FMath::Clamp(OnlineAbortGraceSeconds, 0.5f, 10.0f),
			false);
	}
}

void UUIWorldMenuGameInstance::ExecuteDeferredOnlineAbort()
{
	if (bOnlineJoinReachedGameMap)
	{
		return;
	}

	if (UWorld* World = GetWorld())
	{
		// If we hold an open connection to a server, the join genuinely succeeded — do not
		// bounce back to the menu. This must NOT be gated on the map name: a host that
		// happens to use the menu map would otherwise make every client self-evict.
		const bool bClientConnected =
			(World->GetNetMode() == NM_Client) ||
			(World->GetNetDriver()
				&& World->GetNetDriver()->ServerConnection
				&& World->GetNetDriver()->ServerConnection->GetConnectionState() == USOCK_Open);

		if (bClientConnected)
		{
			FinalizeOnlineTravelSuccess(World);
			return;
		}
	}

	if (!bOnlineTravelInProgress)
	{
		return;
	}

	AbortOnlineTravelToMenu(PendingOnlineAbortReason.IsEmpty() ? TEXT("Connection failed") : PendingOnlineAbortReason);
}

void UUIWorldMenuGameInstance::HandleOnlineJoinTimeout()
{
	if (!bOnlineTravelInProgress)
	{
		return;
	}
	const bool bHosting = ActiveOnlineTravelPhase == EZonefallOnlineTravelPhase::Hosting;
	UE_LOG(
		LogUIWorldStartupFlow,
		Warning,
		TEXT("[Online] %s timed out after %.1fs"),
		bHosting ? TEXT("Host") : TEXT("Join"),
		OnlineJoinTimeoutSeconds);
	AbortOnlineTravelToMenu(
		bHosting
			? TEXT("Host timed out — listen server did not start (PIE: use 2 players, Play As Listen Server)")
			: TEXT("Join timed out — host unreachable"));
}

void UUIWorldMenuGameInstance::AbortOnlineTravelToMenu(const FString& Reason)
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(OnlineJoinTimeoutHandle);
		World->GetTimerManager().ClearTimer(JoinConnectRetryTimerHandle);
		World->GetTimerManager().ClearTimer(OnlineAbortDeferHandle);
	}

	const bool bWasTravelling = bOnlineTravelInProgress;
	PendingJoinConnectSearchIndex = INDEX_NONE;
	JoinConnectRetryAttempts = 0;

	CancelOnlineTravelLoading();
	bOnlineJoinReachedGameMap = false;

	// Tear down any half-formed session so the next attempt is clean.
	if (IOnlineSessionPtr Sessions = UIWorldGetSessionInterface(bLastOnlineQueryWasLAN))
	{
		if (Sessions->GetNamedSession(NAME_GameSession) != nullptr)
		{
			Sessions->DestroySession(NAME_GameSession);
		}
	}

	OnJoinCompleted.Broadcast(false, FString::Printf(TEXT("Join failed: %s"), *Reason));

	// Bounce back to the main menu so the player isn't stranded on a dead loading screen.
	if (bWasTravelling && !MainMenuLevelName.IsNone())
	{
		UWorld* World = GetWorld();
		if (!World || !IsMenuMapWorld(World))
		{
			UE_LOG(LogUIWorldStartupFlow, Warning, TEXT("[Online] Abort -> main menu (%s)"), *Reason);
			LoadMainMenuLevel(true);
		}
		else
		{
			ShowMenuFromList(EUIWorldMenuScreen::MainMenu, false);
		}
	}
}

void UUIWorldMenuGameInstance::ApplyGameFocusInput(UWorld* World) const
{
	if (!World)
	{
		return;
	}

	if (APlayerController* PlayerController = UGameplayStatics::GetPlayerController(World, 0))
	{
		PlayerController->ResetIgnoreMoveInput();
		PlayerController->ResetIgnoreLookInput();
		FInputModeGameOnly GameOnlyInputMode;
		PlayerController->SetInputMode(GameOnlyInputMode);
		PlayerController->SetIgnoreMoveInput(false);
		PlayerController->SetIgnoreLookInput(false);
		PlayerController->SetShowMouseCursor(false);
	}
}

void UUIWorldMenuGameInstance::SetupMoviePlayerLoadingScreen() const
{
	IGameMoviePlayer* MoviePlayer = GetMoviePlayer();
	if (!MoviePlayer || !ActiveLoadingScreenWidget)
	{
		return;
	}

	FLoadingScreenAttributes LoadingScreen;
	LoadingScreen.bAutoCompleteWhenLoadingCompletes = true;
	LoadingScreen.bWaitForManualStop = false;
	LoadingScreen.bAllowEngineTick = true;
	LoadingScreen.MinimumLoadingScreenDisplayTime = FMath::Max(0.25f, LoadingScreenDelayBeforeOpenLevel);
	LoadingScreen.WidgetLoadingScreen = ActiveLoadingScreenWidget->TakeWidget();

	MoviePlayer->SetupLoadingScreen(LoadingScreen);
}

void UUIWorldMenuGameInstance::StopMoviePlayerLoadingScreen() const
{
	IGameMoviePlayer* MoviePlayer = GetMoviePlayer();
	if (MoviePlayer && MoviePlayer->IsMovieCurrentlyPlaying())
	{
		MoviePlayer->StopMovie();
	}
}

void UUIWorldMenuGameInstance::BeginStartupShaderPhase()
{
	UE_LOG(LogUIWorldStartupFlow, Log, TEXT("BeginStartupShaderPhase: entered"));

	if (bStartupShaderPhaseActive)
	{
		UE_LOG(LogUIWorldStartupFlow, Log, TEXT("BeginStartupShaderPhase: already active, skip"));
		return;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		UE_LOG(LogUIWorldStartupFlow, Warning, TEXT("BeginStartupShaderPhase: World is null"));
		return;
	}

	APlayerController* PlayerController = UGameplayStatics::GetPlayerController(World, 0);
	if (!PlayerController)
	{
		UE_LOG(LogUIWorldStartupFlow, Warning, TEXT("BeginStartupShaderPhase: PlayerController missing, retry next tick"));
		World->GetTimerManager().SetTimerForNextTick(this, &UUIWorldMenuGameInstance::BeginStartupShaderPhase);
		return;
	}

	if (!ShaderLoadingWidgetClass)
	{
		UE_LOG(LogUIWorldStartupFlow, Warning, TEXT("BeginStartupShaderPhase: ShaderLoadingWidgetClass is null, finishing immediately"));
		bStartupShaderPhaseCompleted = true;
		FinishStartupShaderPhase();
		return;
	}

	StartupInitialShaderJobs = (GShaderCompilingManager != nullptr)
		? FMath::Max(0, GShaderCompilingManager->GetNumRemainingJobs())
		: 0;
	if (StartupInitialShaderJobs <= 0 && HasValidShaderWarmupCache())
	{
		UE_LOG(LogUIWorldStartupFlow, Log, TEXT("BeginStartupShaderPhase: no shader jobs and warmup cache exists, skipping screen"));
		bStartupShaderPhaseCompleted = true;
		FinishStartupShaderPhase();
		return;
	}

	if (ActiveStartupShaderWidget)
	{
		ActiveStartupShaderWidget->RemoveFromParent();
		ActiveStartupShaderWidget = nullptr;
	}

	ActiveStartupShaderWidget = CreateWidget<UZonefallShaderLoadingWidget>(PlayerController, ShaderLoadingWidgetClass);
	if (!ActiveStartupShaderWidget)
	{
		UE_LOG(
			LogUIWorldStartupFlow,
			Error,
			TEXT("BeginStartupShaderPhase: CreateWidget failed for class=%s"),
			*GetNameSafe(ShaderLoadingWidgetClass.Get())
		);
		bStartupShaderPhaseCompleted = true;
		FinishStartupShaderPhase();
		return;
	}

	bStartupShaderPhaseActive = true;
	ActiveStartupShaderWidget->AddToViewport(MenuZOrder + 1000);
	UE_LOG(
		LogUIWorldStartupFlow,
		Log,
		TEXT("BeginStartupShaderPhase: startup widget added to viewport (%s), z=%d"),
		*GetNameSafe(ActiveStartupShaderWidget),
		MenuZOrder + 1000
	);
	ActiveStartupShaderWidget->SetShaderCompileProgress(0.0f);
	ActiveStartupShaderWidget->SetInitialShaderJobCount(FMath::Max(1, StartupInitialShaderJobs));

	FInputModeUIOnly UiOnlyInputMode;
	PlayerController->SetInputMode(UiOnlyInputMode);
	PlayerController->SetIgnoreMoveInput(true);
	PlayerController->SetIgnoreLookInput(true);
	PlayerController->SetShowMouseCursor(false);

	float MinDuration = FMath::Clamp(StartupShaderLoadingDuration, 0.1f, 30.0f);
	if (bUseAdaptiveStartupShaderDelay)
	{
		const int32 Cores = FPlatformMisc::NumberOfCoresIncludingHyperthreads();
		const float TotalRamGB = static_cast<float>(FPlatformMemory::GetConstants().TotalPhysical) / (1024.0f * 1024.0f * 1024.0f);
		const float HardwarePenalty =
			(Cores <= 6 ? 0.8f : (Cores <= 12 ? 0.35f : 0.0f)) +
			(TotalRamGB < 16.0f ? 0.8f : (TotalRamGB < 24.0f ? 0.35f : 0.0f));
		MinDuration += HardwarePenalty;
	}

	const float MaxDuration = FMath::Max(MinDuration + 0.1f, FMath::Clamp(StartupShaderMaxDuration, 0.5f, 60.0f));
	StartupShaderPhaseStartSeconds = World->GetRealTimeSeconds();

	World->GetTimerManager().ClearTimer(StartupShaderTimerHandle);
	World->GetTimerManager().SetTimer(
		StartupShaderTimerHandle,
		this,
		&UUIWorldMenuGameInstance::PollStartupShaderPhase,
		0.15f,
		true
	);
	UE_LOG(LogUIWorldStartupFlow, Log, TEXT("BeginStartupShaderPhase: adaptive shader phase started (Min=%.2fs Max=%.2fs)"), MinDuration, MaxDuration);
}

void UUIWorldMenuGameInstance::PollStartupShaderPhase()
{
	UWorld* World = GetWorld();
	if (!World || !bStartupShaderPhaseActive)
	{
		return;
	}

	float MinDuration = FMath::Clamp(StartupShaderLoadingDuration, 0.1f, 30.0f);
	if (bUseAdaptiveStartupShaderDelay)
	{
		const int32 Cores = FPlatformMisc::NumberOfCoresIncludingHyperthreads();
		const float TotalRamGB = static_cast<float>(FPlatformMemory::GetConstants().TotalPhysical) / (1024.0f * 1024.0f * 1024.0f);
		const float HardwarePenalty =
			(Cores <= 6 ? 0.8f : (Cores <= 12 ? 0.35f : 0.0f)) +
			(TotalRamGB < 16.0f ? 0.8f : (TotalRamGB < 24.0f ? 0.35f : 0.0f));
		MinDuration += HardwarePenalty;
	}

	const float MaxDuration = FMath::Max(MinDuration + 0.1f, FMath::Clamp(StartupShaderMaxDuration, 0.5f, 60.0f));
	const float Elapsed = World->GetRealTimeSeconds() - StartupShaderPhaseStartSeconds;
	const bool bReachedMinDuration = Elapsed >= MinDuration;
	const bool bReachedMaxDuration = Elapsed >= MaxDuration;
	const int32 RemainingJobs = GShaderCompilingManager
		? FMath::Max(0, GShaderCompilingManager->GetNumRemainingJobs())
		: 0;
	const bool bCompilerReportsDone = (GShaderCompilingManager == nullptr) || (RemainingJobs <= 0);

	// --- Also wait for the PSO (render pipeline) precache to drain. PSOs are the #1 cause of
	//     in-game shader hitches, so we MUST hold the loading screen until they're compiled. ---
	int32 RemainingPSO = 0;
	float PSOProgress = 1.0f;
	if (GEngine)
	{
		if (UZonefallShaderCacheSubsystem* ShaderCache = GEngine->GetEngineSubsystem<UZonefallShaderCacheSubsystem>())
		{
			RemainingPSO = ShaderCache->GetNumPSOsRemaining();
			PSOProgress = ShaderCache->GetPrecompileProgress();
		}
	}
	const bool bPSODone = (RemainingPSO <= 0);

	// Combined progress driven to the widget: the lower of shader-map and pipeline progress.
	const float ShaderProgress = (StartupInitialShaderJobs > 0)
		? FMath::Clamp(1.0f - (static_cast<float>(RemainingJobs) / static_cast<float>(StartupInitialShaderJobs)), 0.0f, 1.0f)
		: 1.0f;
	const float CombinedProgress = FMath::Min(ShaderProgress, PSOProgress);
	if (ActiveStartupShaderWidget)
	{
		ActiveStartupShaderWidget->SetShaderCompileProgress(CombinedProgress * 100.0f);
		ActiveStartupShaderWidget->SetPipelinesRemaining(RemainingPSO);
	}

	// Done only when BOTH the shader compiler AND the PSO precache have finished.
	const bool bShaderDone = bCompilerReportsDone && bPSODone;
	if (bShaderDone && ActiveStartupShaderWidget)
	{
		ActiveStartupShaderWidget->SetShaderCompileProgress(100.0f);
	}

	if ((bReachedMinDuration && bShaderDone) || bReachedMaxDuration)
	{
		if (bShaderDone)
		{
			SaveShaderWarmupCache();
		}
		UE_LOG(
			LogUIWorldStartupFlow,
			Log,
			TEXT("PollStartupShaderPhase: finishing (Elapsed=%.2f, Min=%.2f, Max=%.2f, Jobs=%d, PSO=%d, ShaderDone=%d, MaxHit=%d)"),
			Elapsed, MinDuration, MaxDuration, RemainingJobs, RemainingPSO, bShaderDone, bReachedMaxDuration
		);
		FinishStartupShaderPhase();
	}
}

bool UUIWorldMenuGameInstance::HasValidShaderWarmupCache() const
{
	const FString CachePath = FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("UIWorld/ShaderWarmup.cache"));
	FString CachedSignature;
	if (!FPaths::FileExists(CachePath) || !FFileHelper::LoadFileToString(CachedSignature, *CachePath))
	{
		return false;
	}

	return CachedSignature.Equals(BuildShaderWarmupSignature(), ESearchCase::CaseSensitive);
}

void UUIWorldMenuGameInstance::SaveShaderWarmupCache() const
{
	const FString CacheDir = FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("UIWorld"));
	IFileManager::Get().MakeDirectory(*CacheDir, true);
	const FString CachePath = FPaths::Combine(CacheDir, TEXT("ShaderWarmup.cache"));
	const FString Signature = BuildShaderWarmupSignature();
	FFileHelper::SaveStringToFile(Signature, *CachePath);
}

FString UUIWorldMenuGameInstance::BuildShaderWarmupSignature() const
{
	return FString(FApp::GetProjectName())
		+ TEXT("|")
		+ FPlatformMisc::GetPrimaryGPUBrand()
		+ TEXT("|")
		+ FApp::GetBuildVersion();
}

void UUIWorldMenuGameInstance::FinishStartupShaderPhase()
{
	UE_LOG(LogUIWorldStartupFlow, Log, TEXT("FinishStartupShaderPhase: entered"));

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(StartupShaderTimerHandle);
	}

	bStartupShaderPhaseActive = false;
	bStartupShaderPhaseCompleted = true;

	if (ActiveStartupShaderWidget)
	{
		UE_LOG(LogUIWorldStartupFlow, Log, TEXT("FinishStartupShaderPhase: removing startup widget %s"), *GetNameSafe(ActiveStartupShaderWidget));
		ActiveStartupShaderWidget->RemoveFromParent();
		ActiveStartupShaderWidget = nullptr;
	}

	// AAA boot logos (engine + studio) play here, then chain into the menu.
	if (bShowBootLogos)
	{
		BeginBootLogosPhase();
		return;
	}

	if (bAutoShowMenuOnStart)
	{
		ShowMenuFromList(EUIWorldMenuScreen::MainMenu, false);
	}
}

void UUIWorldMenuGameInstance::BeginBootLogosPhase()
{
	UWorld* World = GetWorld();
	APlayerController* PC = World ? UGameplayStatics::GetPlayerController(World, 0) : nullptr;
	const TSubclassOf<UUserWidget> WidgetClass = BootLogosWidgetClass
		? BootLogosWidgetClass
		: TSubclassOf<UUserWidget>(UZonefallBootLogosWidget::StaticClass());

	ActiveBootLogosWidget = PC
		? CreateWidget<UUserWidget>(PC, WidgetClass)
		: CreateWidget<UUserWidget>(this, WidgetClass);

	if (!ActiveBootLogosWidget)
	{
		HandleBootLogosFinished();
		return;
	}

	if (UZonefallBootLogosWidget* Boot = Cast<UZonefallBootLogosWidget>(ActiveBootLogosWidget))
	{
		Boot->SetEngineVersionString(GetEngineVersionString());
		Boot->OnFinished.AddDynamic(this, &UUIWorldMenuGameInstance::HandleBootLogosFinished);
	}
	ActiveBootLogosWidget->AddToViewport(20000);
}

void UUIWorldMenuGameInstance::HandleBootLogosFinished()
{
	if (ActiveBootLogosWidget)
	{
		ActiveBootLogosWidget->RemoveFromParent();
		ActiveBootLogosWidget = nullptr;
	}

	// Then the "press any key to continue" screen over the scene, then the menu.
	if (bShowPressStart)
	{
		BeginPressStartPhase();
		return;
	}

	if (bAutoShowMenuOnStart)
	{
		ShowMenuFromList(EUIWorldMenuScreen::MainMenu, false);
	}
}

void UUIWorldMenuGameInstance::BeginPressStartPhase()
{
	UWorld* World = GetWorld();
	APlayerController* PC = World ? UGameplayStatics::GetPlayerController(World, 0) : nullptr;
	const TSubclassOf<UUserWidget> WidgetClass = PressStartWidgetClass
		? PressStartWidgetClass
		: TSubclassOf<UUserWidget>(UZonefallPressStartWidget::StaticClass());

	ActivePressStartWidget = PC
		? CreateWidget<UUserWidget>(PC, WidgetClass)
		: CreateWidget<UUserWidget>(this, WidgetClass);

	if (!ActivePressStartWidget)
	{
		HandlePressStartContinue();
		return;
	}

	if (UZonefallPressStartWidget* Press = Cast<UZonefallPressStartWidget>(ActivePressStartWidget))
	{
		Press->OnContinue.AddDynamic(this, &UUIWorldMenuGameInstance::HandlePressStartContinue);
	}
	ActivePressStartWidget->AddToViewport(19000);

	// Make sure keyboard/gamepad reaches the prompt.
	if (PC)
	{
		FInputModeUIOnly Mode;
		Mode.SetWidgetToFocus(ActivePressStartWidget->TakeWidget());
		PC->SetInputMode(Mode);
		PC->SetShowMouseCursor(true);
	}
}

void UUIWorldMenuGameInstance::HandlePressStartContinue()
{
	if (ActivePressStartWidget)
	{
		ActivePressStartWidget->RemoveFromParent();
		ActivePressStartWidget = nullptr;
	}

	if (bAutoShowMenuOnStart)
	{
		ShowMenuFromList(EUIWorldMenuScreen::MainMenu, false);
	}
}

