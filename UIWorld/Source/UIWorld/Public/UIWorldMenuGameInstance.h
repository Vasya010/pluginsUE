#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "Engine/EngineBaseTypes.h"
#include "Localization/ZonefallLocalizationSubsystem.h"
#include "Character/ZonefallCharacterAppearance.h"
#include "UIWorldMenuGameInstance.generated.h"

class UUserWidget;
class UZonefallShaderLoadingWidget;
class UWorld;
class UNetDriver;
class FOnlineSessionSearch;
// Core reusable game instance for UIWorld plugin.

UENUM(BlueprintType)
enum class EUIWorldMenuScreen : uint8
{
	MainMenu UMETA(DisplayName = "Main Menu"),
	OnlineMenu UMETA(DisplayName = "Online Menu"),
	PauseMenu UMETA(DisplayName = "Pause Menu"),
	SettingsMenu UMETA(DisplayName = "Settings Menu")
};

/** Host vs client online travel — drives loading-screen copy and pacing. */
UENUM(BlueprintType)
enum class EZonefallOnlineTravelPhase : uint8
{
	Joining UMETA(DisplayName = "Joining"),
	Hosting UMETA(DisplayName = "Hosting"),
	Syncing UMETA(DisplayName = "Syncing")
};

USTRUCT(BlueprintType)
struct FUIWorldOnlineSessionResult
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "UIWorld|Online")
	FString SessionId;

	UPROPERTY(BlueprintReadOnly, Category = "UIWorld|Online")
	FString OwningUserName;

	UPROPERTY(BlueprintReadOnly, Category = "UIWorld|Online")
	FString ServerName;

	UPROPERTY(BlueprintReadOnly, Category = "UIWorld|Online")
	int32 PingMs = -1;

	UPROPERTY(BlueprintReadOnly, Category = "UIWorld|Online")
	int32 MaxPlayers = 0;

	UPROPERTY(BlueprintReadOnly, Category = "UIWorld|Online")
	int32 CurrentPlayers = 0;

	UPROPERTY(BlueprintReadOnly, Category = "UIWorld|Online")
	bool bIsLAN = true;

	/** Index into the last OSS session search (used after UI filtering/sorting). */
	UPROPERTY(BlueprintReadOnly, Category = "UIWorld|Online")
	int32 SearchResultIndex = INDEX_NONE;

	UPROPERTY(BlueprintReadOnly, Category = "UIWorld|Online")
	FString MapDisplayName;

	UPROPERTY(BlueprintReadOnly, Category = "UIWorld|Online")
	bool bIsFull = false;

	UPROPERTY(BlueprintReadOnly, Category = "UIWorld|Online")
	bool bPasswordProtected = false;

	UPROPERTY(BlueprintReadOnly, Category = "UIWorld|Online")
	FString BuildId;

	/** False when host build id does not match this client (Steam / packaged builds). */
	UPROPERTY(BlueprintReadOnly, Category = "UIWorld|Online")
	bool bBuildCompatible = true;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnUIWorldOnlineOpCompleted, bool, bSuccess, const FString&, Message);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnUIWorldSessionsFound, const TArray<FUIWorldOnlineSessionResult>&, Results);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnUIWorldOnlineMatchReady, UWorld*, World);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnUIWorldMenuWidgetChanged, UUserWidget*, ActiveWidget);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnUIWorldMenuScreenChanged, EUIWorldMenuScreen, MenuScreen);

UCLASS(BlueprintType, Blueprintable)
class UIWORLD_API UUIWorldMenuGameInstance : public UGameInstance
{
	GENERATED_BODY()

public:
	UUIWorldMenuGameInstance();

	virtual void Init() override;
	virtual void OnStart() override;
	virtual void Shutdown() override;

	UFUNCTION(BlueprintCallable, Category = "UIWorld|UI")
	UUserWidget* ShowMenuFromList(EUIWorldMenuScreen MenuScreen, bool bForceRebuild = false);

	UFUNCTION(BlueprintCallable, Category = "UIWorld|UI")
	void CloseMenuUI(bool bRemoveFromParentOnly = true);

	UFUNCTION(BlueprintCallable, Category = "UIWorld|Flow")
	bool LoadLevelAndFocusGame(FName LevelName, bool bAbsolute = true);

	// Shows loading screen first, then opens target level.
	UFUNCTION(BlueprintCallable, Category = "UIWorld|Flow")
	bool LoadLevelWithLoadingScreen(FName LevelName, bool bAbsolute = true);

	// Compatibility helpers for copied Zonefall UI widgets.
	UFUNCTION(BlueprintCallable, Category = "UIWorld|Flow")
	void ContinueGame(bool bUnpauseGame = true);

	UFUNCTION(BlueprintCallable, Category = "UIWorld|Flow")
	UUserWidget* OpenPauseSettingsMenu(bool bForceRebuild = false);

	UFUNCTION(BlueprintCallable, Category = "UIWorld|Flow")
	UUserWidget* BackMenuPause(bool bForceRebuild = false);

	// Explicit alias for pause -> settings navigation (kept for clearer Blueprint naming).
	UFUNCTION(BlueprintCallable, Category = "UIWorld|Flow")
	UUserWidget* OpenSettingsPauseMenu(bool bForceRebuild = false);

	// Explicit open settings from main-menu flow.
	UFUNCTION(BlueprintCallable, Category = "UIWorld|Flow")
	UUserWidget* OpenSettingsMainMenu(bool bForceRebuild = false);

	// Explicit alias for settings -> pause back navigation.
	UFUNCTION(BlueprintCallable, Category = "UIWorld|Flow")
	UUserWidget* BackFromSettingsPauseMenu(bool bForceRebuild = false);

	// Explicit back route from settings to main menu.
	UFUNCTION(BlueprintCallable, Category = "UIWorld|Flow")
	UUserWidget* BackFromSettingsMainMenu(bool bForceRebuild = false);

	// Smart return from settings: routes to the previous menu context (main or pause).
	UFUNCTION(BlueprintCallable, Category = "UIWorld|Flow")
	UUserWidget* BackFromSettingsMenuSmart(bool bForceRebuild = false);

	UFUNCTION(BlueprintCallable, Category = "UIWorld|Flow")
	bool LoadMainMenuLevel(bool bAbsolute = true);

	UFUNCTION(BlueprintCallable, Category = "UIWorld|Flow")
	void QuitGameNow(bool bIgnorePlatformRestrictions = false);

	// Returns a friendly engine identifier, e.g. "Unreal Engine 5.7.0".
	UFUNCTION(BlueprintPure, Category = "UIWorld|Info")
	FString GetEngineVersionString() const;

	// Optional widget class for the slide-in "saved" toast (falls back to the built-in one).
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "UIWorld|Save")
	TSubclassOf<UUserWidget> SaveToastWidgetClass;

	// Pops a GTA-style "saved" toast from the side. Safe to call anytime in-game.
	UFUNCTION(BlueprintCallable, Category = "UIWorld|Save")
	void ShowSaveToast(const FString& Message);

	// Saves current level progress and pops the "saved" toast. Returns success.
	UFUNCTION(BlueprintCallable, Category = "UIWorld|Save")
	bool SaveGame();

	// True if a real save exists on disk (drives whether the main-menu "Continue" is usable).
	UFUNCTION(BlueprintPure, Category = "UIWorld|Save")
	bool HasSavedGame() const;

	// Main-menu "Continue": loads the saved level (with loading screen) and flags the spawned
	// player to restore its full snapshot (health, weapons, ammo, picked-up items, position).
	UFUNCTION(BlueprintCallable, Category = "UIWorld|Save")
	bool ContinueSavedGame();

	// Called by the freshly-spawned player on the loaded level; returns true once (then clears
	// the flag) so the player knows to apply the saved snapshot to itself.
	UFUNCTION(BlueprintCallable, Category = "UIWorld|Save")
	bool ConsumeSaveRestoreRequest();

	// --- Character creation ---

	// Opens the character creator: hides the main menu and shows the creator widget on the
	// menu pawn immediately (no loading screen). After CREATE & START, loads GameplayLevelName.
	UFUNCTION(BlueprintCallable, Category = "UIWorld|Character")
	bool OpenCharacterCreator();

	// Consumed once by the spawned player to know it should open the creator UI on itself.
	UFUNCTION(BlueprintCallable, Category = "UIWorld|Character")
	bool ConsumeShowCharacterCreatorRequest();

	// Stores the chosen look so it survives level travel and gets re-applied in gameplay.
	UFUNCTION(BlueprintCallable, Category = "UIWorld|Character")
	void SetCurrentAppearance(const FZonefallCharacterAppearance& InAppearance);

	UFUNCTION(BlueprintPure, Category = "UIWorld|Character")
	bool HasCurrentAppearance() const { return bHasCurrentAppearance; }

	UFUNCTION(BlueprintPure, Category = "UIWorld|Character")
	FZonefallCharacterAppearance GetCurrentAppearance() const { return CurrentAppearance; }

	// Commits the created character (saves it) and starts a new game on GameplayLevelName.
	UFUNCTION(BlueprintCallable, Category = "UIWorld|Character")
	bool StartGameWithCreatedCharacter(const FZonefallCharacterAppearance& InAppearance);

	// Level used to edit the character (its player pawn is the live preview). If None, the
	// creator opens on GameplayLevelName instead.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UIWorld|Character")
	FName CharacterCreatorLevelName;

	// Level a freshly-created character starts the game on.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UIWorld|Character")
	FName GameplayLevelName;

	// Optional widget class for the creator (the character also has its own default).
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "UIWorld|Character")
	TSubclassOf<UUserWidget> CharacterCreatorWidgetClass;

	// Blueprint pawn shown in the creator preview (e.g. /Game/charecters/gamecharecter).
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UIWorld|Character")
	TSubclassOf<class AZonefallPlayerCharacter> CharacterCreatorPawnClass;

	UFUNCTION(BlueprintCallable, Category = "UIWorld|Online")
	bool HostOnlineSession(int32 MaxPlayers = 4, bool bLAN = false);

	/** Find the best open session (lowest ping, same build) and join automatically. */
	UFUNCTION(BlueprintCallable, Category = "UIWorld|Online")
	bool QuickJoinOnlineSession(bool bLAN = false);

	UFUNCTION(BlueprintCallable, Category = "UIWorld|Online")
	bool FindOnlineSessions(int32 MaxResults = 50, bool bLAN = true);

	UFUNCTION(BlueprintCallable, Category = "UIWorld|Online")
	bool JoinOnlineSessionByIndex(int32 ResultIndex);

	/** Direct-connect by a typed address / ID (e.g. "192.168.0.10:7777", "steam.<id>", or a bare IP). */
	UFUNCTION(BlueprintCallable, Category = "UIWorld|Online")
	bool JoinOnlineByAddress(const FString& Address);

	UFUNCTION(BlueprintCallable, Category = "UIWorld|Online")
	bool LeaveOnlineSessionAndReturnToMenu();

	UFUNCTION(BlueprintPure, Category = "UIWorld|Online")
	const TArray<FUIWorldOnlineSessionResult>& GetLastFoundSessions() const { return LastFoundSessions; }

	/** Human-readable reason when Host/Find/Join failed (empty if last op succeeded). */
	UFUNCTION(BlueprintPure, Category = "UIWorld|Online")
	FString GetLastOnlineDiagnostic() const { return LastOnlineDiagnostic; }

	// --- Online account / status (drives the main-menu online indicator) ---

	/** True if an online subsystem (Steam / Null) is present and initialised. */
	UFUNCTION(BlueprintPure, Category = "UIWorld|Online")
	bool IsOnlineAvailable() const;

	/** True once the identity interface reports a logged-in user (Steam persona signed in). */
	UFUNCTION(BlueprintPure, Category = "UIWorld|Online")
	bool IsOnlineLoggedIn() const;

	/** Active subsystem name, e.g. "STEAM", "NULL", or "None". */
	UFUNCTION(BlueprintPure, Category = "UIWorld|Online")
	FString GetOnlineServiceName() const;

	/** Logged-in player's display name (Steam persona, or a local fallback). */
	UFUNCTION(BlueprintPure, Category = "UIWorld|Online")
	FString GetOnlinePlayerNickname() const;

	/** Players currently in the active named session (0 if none / not in a session). */
	UFUNCTION(BlueprintPure, Category = "UIWorld|Online")
	int32 GetCurrentSessionPlayerCount() const;

	/** Kicks off Steam AutoLogin if not already signed in (safe to call repeatedly). */
	UFUNCTION(BlueprintCallable, Category = "UIWorld|Online")
	void RequestOnlineLogin();

	// --- Achievements (Steam-backed, with an offline local fallback) ---

	/** Unlock (or set progress on) an achievement. Writes to Steam when available and always records locally. */
	UFUNCTION(BlueprintCallable, Category = "UIWorld|Achievements")
	void UnlockAchievement(FName AchievementId, float PercentComplete = 100.0f);

	/** True if the achievement has been unlocked (locally recorded). */
	UFUNCTION(BlueprintPure, Category = "UIWorld|Achievements")
	bool IsAchievementUnlocked(FName AchievementId) const;

	/** All achievement ids unlocked so far (local record). */
	UFUNCTION(BlueprintPure, Category = "UIWorld|Achievements")
	TArray<FName> GetUnlockedAchievements() const { return UnlockedAchievementIds; }

	UPROPERTY(BlueprintAssignable, Category = "UIWorld|Online")
	FOnUIWorldOnlineOpCompleted OnHostCompleted;

	UPROPERTY(BlueprintAssignable, Category = "UIWorld|Online")
	FOnUIWorldSessionsFound OnSessionsFound;

	UPROPERTY(BlueprintAssignable, Category = "UIWorld|Online")
	FOnUIWorldOnlineOpCompleted OnJoinCompleted;

	UPROPERTY(BlueprintAssignable, Category = "UIWorld|Online")
	FOnUIWorldOnlineOpCompleted OnLeaveCompleted;

	/** Fired when host/join travel finishes and gameplay should start (HUD, input, weapons). */
	UPROPERTY(BlueprintAssignable, Category = "UIWorld|Online")
	FOnUIWorldOnlineMatchReady OnOnlineMatchReady;

	UFUNCTION(BlueprintPure, Category = "UIWorld|Online")
	bool IsOnlineTravelInProgress() const { return bOnlineTravelInProgress; }

	UPROPERTY(BlueprintAssignable, Category = "UIWorld|UI")
	FOnUIWorldMenuWidgetChanged OnMenuWidgetChanged;

	UPROPERTY(BlueprintAssignable, Category = "UIWorld|UI")
	FOnUIWorldMenuScreenChanged OnMenuScreenChanged;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "UIWorld|UI")
	TSubclassOf<UUserWidget> MainMenuWidgetClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "UIWorld|UI")
	TSubclassOf<UUserWidget> OnlineMenuWidgetClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "UIWorld|UI")
	TSubclassOf<UUserWidget> PauseMenuWidgetClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "UIWorld|UI")
	TSubclassOf<UUserWidget> SettingsMenuWidgetClass;

	// Optional dedicated settings widget for pause flow.
	// If set, OpenSettingsPauseMenu will use this class instead of SettingsMenuWidgetClass.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "UIWorld|UI")
	TSubclassOf<UUserWidget> PauseSettingsMenuWidgetClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "UIWorld|Flow")
	TSubclassOf<UUserWidget> LoadingScreenWidgetClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "UIWorld|UI|Startup")
	TSubclassOf<UZonefallShaderLoadingWidget> ShaderLoadingWidgetClass;

	// Optional animated game-title splash shown BEFORE shader compilation.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "UIWorld|UI|Startup")
	TSubclassOf<UUserWidget> StartupIntroWidgetClass;

	// AAA boot-logo sequence shown AFTER shader compilation, before the main menu
	// (e.g. "Made with Unreal Engine" then the studio). Falls back to the built-in one.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "UIWorld|UI|Startup")
	TSubclassOf<UUserWidget> BootLogosWidgetClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UIWorld|UI|Startup")
	bool bShowBootLogos = true;

	// "Press any key to continue" screen shown over the scene after the boot logos.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "UIWorld|UI|Startup")
	TSubclassOf<UUserWidget> PressStartWidgetClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UIWorld|UI|Startup")
	bool bShowPressStart = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UIWorld|UI|Startup")
	bool bShowStartupIntro;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UIWorld|UI|Startup", meta = (ClampMin = "0.5", ClampMax = "15.0"))
	float StartupIntroDuration;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UIWorld|UI|Startup")
	bool bAutoShowMenuOnStart;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UIWorld|UI|Startup")
	bool bShowShaderLoadingOnStartup;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UIWorld|UI|Startup", meta = (ClampMin = "0.1", ClampMax = "30.0"))
	float StartupShaderLoadingDuration;

	// Scales startup shader screen minimum time by hardware class.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UIWorld|UI|Startup")
	bool bUseAdaptiveStartupShaderDelay;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UIWorld|UI|Startup", meta = (ClampMin = "0.1", ClampMax = "20.0"))
	float StartupShaderMaxDuration;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UIWorld|UI")
	bool bCacheWidgetsByScreen;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UIWorld|UI")
	int32 MenuZOrder;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UIWorld|Online")
	FName MainMenuLevelName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UIWorld|Online")
	FName OnlineHostMapName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UIWorld|Online")
	FString OnlineServerName;

	/** Port for LAN ?listen / ClientTravel when using Null (same-PC testing). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UIWorld|Online", meta = (ClampMin = "1", ClampMax = "65535"))
	int32 OnlineLanPort = 7777;

	/** Advertised with each session so clients can filter mismatched builds (see DefaultEngine BuildIdOverride). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UIWorld|Online")
	FString OnlineGameBuildId = TEXT("1");

	/** Set by lobby UI before Host — empty = open session. */
	UPROPERTY(Transient, BlueprintReadWrite, Category = "UIWorld|Online")
	FString PendingHostPassword;

	/** 0 = public, 1 = friends/presence only, 2 = invite-only (hidden from browse). */
	UPROPERTY(Transient, BlueprintReadWrite, Category = "UIWorld|Online")
	int32 PendingHostPrivacy = 0;

	/** Set by lobby UI before Join — required when session has SESSION_PASSWORD. */
	UPROPERTY(Transient, BlueprintReadWrite, Category = "UIWorld|Online")
	FString PendingJoinPassword;

	// Small delay to ensure loading widget is drawn before blocking OpenLevel.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UIWorld|Flow|Loading", meta = (ClampMin = "0.0", ClampMax = "2.0"))
	float LoadingScreenDelayBeforeOpenLevel;

	// If enabled, pre-open delay is adjusted by CPU/RAM (weaker PC => longer delay).
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UIWorld|Flow|Loading|Adaptive")
	bool bUseAdaptiveLoadingDelay;

	// Minimum adaptive delay.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UIWorld|Flow|Loading|Adaptive", meta = (ClampMin = "0.0", ClampMax = "5.0"))
	float AdaptiveDelayMinSeconds;

	// Maximum adaptive delay.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UIWorld|Flow|Loading|Adaptive", meta = (ClampMin = "0.1", ClampMax = "10.0"))
	float AdaptiveDelayMaxSeconds;

	// Adds extra delay based on target map package size (.umap).
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UIWorld|Flow|Loading|Adaptive")
	bool bUseMapComplexityDelay;

	// Added seconds per 100MB map size.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UIWorld|Flow|Loading|Adaptive", meta = (ClampMin = "0.0", ClampMax = "10.0"))
	float MapDelayPer100MB;

	// Safety clamp for map-based delay component.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UIWorld|Flow|Loading|Adaptive", meta = (ClampMin = "0.0", ClampMax = "30.0"))
	float MapDelayMaxSeconds;

	// Uses MoviePlayer Slate loading (animated and non-freezing while OpenLevel blocks).
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UIWorld|Flow|Loading")
	bool bUseMoviePlayerLoadingScreen;

private:
	// Rebuilds the currently-shown menu in the newly-selected language (live switch).
	UFUNCTION()
	void HandleLanguageChanged(EZonefallLanguage NewLanguage);

	void HandlePostLoadMap(UWorld* LoadedWorld);
	void ApplyGameFocusInput(UWorld* World) const;
	TSubclassOf<UUserWidget> ResolveMenuClass(EUIWorldMenuScreen MenuScreen) const;
	UUserWidget* ResolveOrCreateWidget(class APlayerController* PlayerController, TSubclassOf<UUserWidget> WidgetClass, bool bForceRebuild);
	void ApplyMenuInputMode(class APlayerController* PlayerController, UUserWidget* WidgetToFocus) const;
	void HideCurrentMenuWidget();
	void ShowSimpleTravelLoadingScreen();
	void HideSimpleTravelLoadingScreen();
	void BeginOnlineTravel();
	void BeginOnlineTravelWithPhase(EZonefallOnlineTravelPhase Phase, const FText& StatusHint);
	void FinalizeOnlineTravelSuccess(UWorld* LoadedWorld);
	void CancelOnlineTravelLoading();
	void ShowOnlineTravelLoadingScreen(EZonefallOnlineTravelPhase Phase, const FText& StatusHint);
	void HideOnlineTravelLoadingScreen();
	void UpdateOnlineTravelLoadingStatus(const FText& Status);
	void HandleOnlineLoadingStatusTick();
	void RegisterLocalPlayerInActiveSession(UWorld* World);
	void ExecutePendingLevelLoad();
	float EstimateAdaptiveLoadingDelay() const;
	float EstimateMapComplexityDelay(FName LevelName) const;
	void SetupMoviePlayerLoadingScreen() const;
	void StopMoviePlayerLoadingScreen() const;
	void BeginStartupIntroPhase();
	void FinishStartupIntroPhase();
	void BeginBootLogosPhase();

	UFUNCTION()
	void HandleBootLogosFinished();

	void BeginPressStartPhase();

	UFUNCTION()
	void HandlePressStartContinue();
	void BeginStartupShaderPhase();
	void PollStartupShaderPhase();
	void FinishStartupShaderPhase();
	bool HasValidShaderWarmupCache() const;
	void SaveShaderWarmupCache() const;
	FString BuildShaderWarmupSignature() const;

	// Online travel safety: recover gracefully from failed joins instead of an endless loading screen.
	void HandleTravelFailure(UWorld* World, ETravelFailure::Type FailureType, const FString& ErrorString);
	void HandleNetworkFailure(UWorld* World, UNetDriver* NetDriver, ENetworkFailure::Type FailureType, const FString& ErrorString);
	void HandleOnlineJoinTimeout();
	void AbortOnlineTravelToMenu(const FString& Reason);
	void ScheduleDeferredOnlineAbort(const FString& Reason);
	void ExecuteDeferredOnlineAbort();
	void HandleOnlineIdentityLoginComplete(int32 LocalUserNum, bool bWasSuccessful, const FUniqueNetId& UserId, const FString& Error);
	void FlushPendingFindOnlineSessions();
	void EnsureOnlineAutoLogin();
	bool ExecuteJoinOnlineSession(int32 ResultIndex);
	bool JoinLanSessionDirect(int32 ResultIndex);
	bool DestroyExistingSessionThenJoin(int32 ResultIndex);
	bool BeginHostCreateSession(int32 MaxPlayers, bool bLAN);
	void OnHostCreateSessionComplete(FName SessionName, bool bWasSuccessful);
	void OnDestroySessionBeforeHost(FName SessionName, bool bWasDestroyed);
	bool TravelToListenHostMap(UWorld* World, const FString& MapPackagePath, bool bLAN);
	void TryActivateListenServerAfterHostLoad(UWorld* LoadedWorld);
	void SortAndPublishFoundSessions(bool bWasSuccessful);
	int32 FindBestQuickJoinIndex() const;
	bool TryExecuteQuickJoin();
	void PublishHostedOnlineSession(UWorld* LoadedWorld);
	void ScheduleHostedOnlineSessionRefresh(UWorld* LoadedWorld);
	bool TryCompleteJoinTravel(UWorld* World, int32 SearchIndex);
	void HandleJoinConnectRetry();
	void HandleHostedSessionRefreshTick();
	bool IsMenuMapWorld(const UWorld* World) const;
	AZonefallPlayerCharacter* EnsureCharacterCreatorPawn();

	UPROPERTY(Transient)
	EUIWorldMenuScreen CurrentMenuScreen;

	UPROPERTY(Transient)
	EUIWorldMenuScreen LastNonSettingsMenuScreen;

	UPROPERTY(Transient)
	TMap<EUIWorldMenuScreen, TObjectPtr<UUserWidget>> MenuWidgetCache;

	UPROPERTY(Transient)
	TObjectPtr<UUserWidget> PinnedMenuWidget;

	UPROPERTY(Transient)
	TObjectPtr<UUserWidget> ActiveLoadingScreenWidget;

	UPROPERTY(Transient)
	TObjectPtr<UZonefallShaderLoadingWidget> ActiveStartupShaderWidget;

	UPROPERTY(Transient)
	TObjectPtr<UUserWidget> ActiveStartupIntroWidget;

	UPROPERTY(Transient)
	TObjectPtr<UUserWidget> ActiveBootLogosWidget;

	UPROPERTY(Transient)
	TObjectPtr<UUserWidget> ActivePressStartWidget;

	FTimerHandle StartupIntroTimerHandle;

	UPROPERTY(Transient)
	bool bStartupShaderPhaseActive;

	UPROPERTY(Transient)
	bool bStartupShaderPhaseCompleted;

	UPROPERTY(Transient)
	bool bApplyGameFocusOnNextMapLoad;

	UPROPERTY(Transient)
	FName PendingLevelNameToLoad;

	UPROPERTY(Transient)
	bool bPendingLevelAbsolute;

	UPROPERTY(Transient)
	TArray<FUIWorldOnlineSessionResult> LastFoundSessions;

	UPROPERTY(Transient)
	FString LastOnlineDiagnostic;

	UPROPERTY(Transient)
	bool bPendingFindOnlineSessions = false;

	UPROPERTY(Transient)
	int32 PendingFindMaxResults = 50;

	UPROPERTY(Transient)
	bool bPendingFindLAN = false;

	UPROPERTY(Transient)
	int32 PendingHostMaxPlayers = 4;

	UPROPERTY(Transient)
	bool bPendingHostAfterDestroy = false;

	UPROPERTY(Transient)
	bool bPendingQuickJoin = false;

	UPROPERTY(Transient)
	int32 PendingJoinSessionIndex = INDEX_NONE;

	UPROPERTY(Transient)
	int32 PendingJoinConnectSearchIndex = INDEX_NONE;

	UPROPERTY(Transient)
	int32 JoinConnectRetryAttempts = 0;

	UPROPERTY(Transient)
	int32 HostedSessionPublishAttempts = 0;

	/** Matches the last Host/Find call (LAN => Null OSS, otherwise Steam). */
	UPROPERTY(Transient)
	bool bLastOnlineQueryWasLAN = false;

	TSharedPtr<FOnlineSessionSearch> LastSessionSearchNative;

	// Stored delegate handles so we can unbind before re-binding (prevents lambda accumulation across calls).
	FDelegateHandle CreateSessionDelegateHandle;
	FDelegateHandle FindSessionsDelegateHandle;
	FDelegateHandle JoinSessionDelegateHandle;
	FDelegateHandle DestroySessionDelegateHandle;

	FTimerHandle PendingLoadLevelTimerHandle;
	FTimerHandle StartupShaderTimerHandle;
	FTimerHandle OnlineJoinTimeoutHandle;
	FTimerHandle HostedSessionRefreshTimerHandle;
	FTimerHandle JoinConnectRetryTimerHandle;
	FTimerHandle OnlineAbortDeferHandle;
	FTimerHandle OnlineLoadingStatusTimerHandle;
	FString PendingOnlineAbortReason;
	EZonefallOnlineTravelPhase ActiveOnlineTravelPhase = EZonefallOnlineTravelPhase::Joining;
	int32 OnlineLoadingStatusPhraseIndex = 0;
	float StartupShaderPhaseStartSeconds;
	int32 StartupInitialShaderJobs;

	FDelegateHandle TravelFailureDelegateHandle;
	FDelegateHandle NetworkFailureDelegateHandle;
	FDelegateHandle OnlineLoginDelegateHandle;

	// Locally-recorded unlocked achievements (mirrors Steam, and persists offline).
	UPROPERTY(Transient)
	TArray<FName> UnlockedAchievementIds;

	void LoadLocalAchievements();
	void SaveLocalAchievements() const;
	void WriteAchievementToOnlineService(FName AchievementId, float PercentComplete);

	// Set by ContinueSavedGame, consumed by the spawned player to restore its saved snapshot.
	UPROPERTY(Transient)
	bool bPendingSaveRestore = false;

	// Set by OpenCharacterCreator, consumed by the spawned player to show the creator UI.
	UPROPERTY(Transient)
	bool bPendingShowCharacterCreator = false;

	// The chosen look, carried across level travel and applied to the gameplay character.
	UPROPERTY(Transient)
	bool bHasCurrentAppearance = false;

	UPROPERTY(Transient)
	FZonefallCharacterAppearance CurrentAppearance;

	// True while a join/connect is in flight, so failure handlers know to react.
	UPROPERTY(Transient)
	bool bOnlineTravelInProgress = false;

	/** Set when a join ClientTravel successfully lands on a non-menu map. */
	UPROPERTY(Transient)
	bool bOnlineJoinReachedGameMap = false;

	UPROPERTY(Transient)
	bool bOnlineLoadingOverlayActive = false;

	UPROPERTY(Transient)
	float OnlineTravelStartSeconds = 0.0f;

	// Safety timeout for joining a session before we bail back to the menu.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UIWorld|Online", meta = (ClampMin = "3.0", ClampMax = "60.0", AllowPrivateAccess = "true"))
	float OnlineJoinTimeoutSeconds = 45.0f;

	/** Wait before treating travel/network errors as a failed join (avoids false kicks while the map loads). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UIWorld|Online", meta = (ClampMin = "0.5", ClampMax = "10.0", AllowPrivateAccess = "true"))
	float OnlineAbortGraceSeconds = 5.0f;
};

