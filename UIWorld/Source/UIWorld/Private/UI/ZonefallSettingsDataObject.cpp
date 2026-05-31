#include "ZonefallSettingsDataObject.h"

#include "AudioDevice.h"
#include "Camera/CameraComponent.h"
#include "Character/ZonefallPlayerCharacter.h"
#include "DLSSLibrary.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameFramework/GameUserSettings.h"
#include "GameFramework/PlayerController.h"
#include "GenericPlatform/GenericPlatformMisc.h"
#include "HAL/IConsoleManager.h"
#include "HAL/PlatformMemory.h"
#include "HAL/PlatformMisc.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Logging/LogMacros.h"
#include "Misc/ConfigCacheIni.h"
#include "RHI.h"
#include "StreamlineLibraryDLSSG.h"

namespace ZonefallSettingsParse
{
	static const TCHAR* UpscalerConfigSection()
	{
		return TEXT("/Script/Zonefall.ZonefallUpscalerSettings");
	}

	static const TCHAR* AudioDisplayConfigSection()
	{
		return TEXT("/Script/Zonefall.ZonefallAudioDisplaySettings");
	}

	static FString CanonicalizeSpacesAndCase(const FString& InValue)
	{
		FString Value = InValue.TrimStartAndEnd();
		Value = Value.Replace(TEXT("_"), TEXT(" "));
		while (Value.Contains(TEXT("  ")))
		{
			Value = Value.Replace(TEXT("  "), TEXT(" "));
		}
		return Value.ToLower();
	}

	static int32 ParseOverallQualityLevel(const FString& InQuality)
	{
		const FString Normalized = CanonicalizeSpacesAndCase(InQuality);
		if (Normalized == TEXT("low")) { return 0; }
		if (Normalized == TEXT("medium")) { return 1; }
		if (Normalized == TEXT("high")) { return 2; }
		if (Normalized == TEXT("epic")) { return 3; }
		if (Normalized == TEXT("cinematic")) { return 4; }
		return 2;
	}

	static FString GroupLevelToString(int32 Level)
	{
		switch (FMath::Clamp(Level, 0, 4))
		{
		case 0: return TEXT("Low");
		case 1: return TEXT("Medium");
		case 2: return TEXT("High");
		default: return TEXT("Epic"); // 3 (Epic) and 4 (Cinematic) both shown as Epic in the UI.
		}
	}

	static bool IsDLSSSupported()
	{
		return UDLSSLibrary::QueryDLSSSupport() == UDLSSSupport::Supported;
	}

	static bool IsFrameGenerationSupported()
	{
		return UStreamlineLibraryDLSSG::QueryDLSSGSupport() == EStreamlineFeatureSupport::Supported;
	}

	static bool IsFSRSupported()
	{
		return IConsoleManager::Get().FindConsoleVariable(TEXT("r.FidelityFX.FSR.Enabled")) != nullptr;
	}

	static bool IsFSRFrameGenerationSupported()
	{
		// Force-unavailable: FSR3 frame interpolation crashes in this plugin build
		// (FFXD3D12Backend::UpdateSwapChain assert). Re-enable only once the plugin is stable.
		return false;
	}

	static FString DetectDLSSModeString(bool bSupported)
	{
		if (!bSupported)
		{
			return TEXT("Unavailable");
		}
		if (!UDLSSLibrary::IsDLSSEnabled())
		{
			return TEXT("Off");
		}

		switch (UDLSSLibrary::GetDLSSMode())
		{
		case UDLSSMode::DLAA: return TEXT("DLAA");
		case UDLSSMode::Balanced: return TEXT("Balanced");
		case UDLSSMode::Performance: return TEXT("Performance");
		case UDLSSMode::UltraPerformance: return TEXT("Ultra Performance");
		case UDLSSMode::UltraQuality: return TEXT("Quality");
		case UDLSSMode::Quality: return TEXT("Quality");
		default: return TEXT("Quality");
		}
	}

	static FString DetectFrameGenerationString(bool bSupported)
	{
		if (!bSupported)
		{
			return TEXT("Unavailable");
		}
		const IConsoleVariable* FGVar = IConsoleManager::Get().FindConsoleVariable(TEXT("r.Streamline.DLSSG.Enable"));
		return (FGVar && FGVar->GetInt() != 0) ? TEXT("On") : TEXT("Off");
	}

	static FString DetectFSRModeString(bool bSupported)
	{
		if (!bSupported)
		{
			return TEXT("Unavailable");
		}

		const IConsoleVariable* EnableVar = IConsoleManager::Get().FindConsoleVariable(TEXT("r.FidelityFX.FSR.Enabled"));
		const IConsoleVariable* QualityVar = IConsoleManager::Get().FindConsoleVariable(TEXT("r.FidelityFX.FSR.QualityMode"));
		const int32 bEnabled = EnableVar ? EnableVar->GetInt() : 0;
		if (bEnabled == 0)
		{
			return TEXT("Off");
		}

		const int32 Quality = QualityVar ? QualityVar->GetInt() : 1;
		switch (Quality)
		{
		case 0: return TEXT("Native AA");
		case 1: return TEXT("Quality");
		case 2: return TEXT("Balanced");
		case 3: return TEXT("Performance");
		case 4: return TEXT("Ultra Performance");
		default: return TEXT("Quality");
		}
	}

	static FString DetectFSRFrameGenerationString(bool bSupported)
	{
		if (!bSupported)
		{
			return TEXT("Unavailable");
		}

		const IConsoleVariable* FGVar = IConsoleManager::Get().FindConsoleVariable(TEXT("r.FidelityFX.FI.Enabled"));
		return (FGVar && FGVar->GetInt() != 0) ? TEXT("On") : TEXT("Off");
	}

	static FString DetectLumenStateString()
	{
		const IConsoleVariable* GIMethod = IConsoleManager::Get().FindConsoleVariable(TEXT("r.DynamicGlobalIlluminationMethod"));
		const IConsoleVariable* ReflectionMethod = IConsoleManager::Get().FindConsoleVariable(TEXT("r.ReflectionMethod"));
		const IConsoleVariable* LumenDiffuseAllowed = IConsoleManager::Get().FindConsoleVariable(TEXT("r.Lumen.DiffuseIndirect.Allow"));
		const IConsoleVariable* LumenReflectionsAllowed = IConsoleManager::Get().FindConsoleVariable(TEXT("r.Lumen.Reflections.Allow"));

		// Treat as enabled only when both are explicitly on Lumen (1).
		const int32 GIValue = GIMethod ? GIMethod->GetInt() : 0;
		const int32 ReflectionValue = ReflectionMethod ? ReflectionMethod->GetInt() : 0;
		const bool bMethodSaysOn = (GIValue == 1 && ReflectionValue == 1);

		// Runtime allow flags are additional verification that Lumen is really active.
		const bool bDiffuseAllowed = !LumenDiffuseAllowed || LumenDiffuseAllowed->GetInt() != 0;
		const bool bReflectionsAllowed = !LumenReflectionsAllowed || LumenReflectionsAllowed->GetInt() != 0;

		return (bMethodSaysOn && bDiffuseAllowed && bReflectionsAllowed) ? TEXT("On") : TEXT("Off");
	}

	static void ApplyLumenState(UObject* WorldContextObject, bool bEnableLumen)
	{
		if (!WorldContextObject)
		{
			return;
		}

		IConsoleVariable* GIMethod = IConsoleManager::Get().FindConsoleVariable(TEXT("r.DynamicGlobalIlluminationMethod"));
		IConsoleVariable* ReflectionMethod = IConsoleManager::Get().FindConsoleVariable(TEXT("r.ReflectionMethod"));
		IConsoleVariable* LumenDiffuseAllowed = IConsoleManager::Get().FindConsoleVariable(TEXT("r.Lumen.DiffuseIndirect.Allow"));
		IConsoleVariable* LumenReflectionsAllowed = IConsoleManager::Get().FindConsoleVariable(TEXT("r.Lumen.Reflections.Allow"));

		const int32 GIValue = bEnableLumen ? 1 : 0;
		const int32 ReflectionValue = bEnableLumen ? 1 : 0;
		const int32 AllowValue = bEnableLumen ? 1 : 0;

		// Apply with direct CVar writes first (most reliable path).
		if (GIMethod) { GIMethod->Set(GIValue, ECVF_SetByGameSetting); }
		if (ReflectionMethod) { ReflectionMethod->Set(ReflectionValue, ECVF_SetByGameSetting); }
		if (LumenDiffuseAllowed) { LumenDiffuseAllowed->Set(AllowValue, ECVF_SetByGameSetting); }
		if (LumenReflectionsAllowed) { LumenReflectionsAllowed->Set(AllowValue, ECVF_SetByGameSetting); }

		// Keep console-command fallback for compatibility across plugin/platform overrides.
		if (bEnableLumen)
		{
			UKismetSystemLibrary::ExecuteConsoleCommand(WorldContextObject, TEXT("r.DynamicGlobalIlluminationMethod 1"));
			UKismetSystemLibrary::ExecuteConsoleCommand(WorldContextObject, TEXT("r.ReflectionMethod 1"));
			UKismetSystemLibrary::ExecuteConsoleCommand(WorldContextObject, TEXT("r.Lumen.DiffuseIndirect.Allow 1"));
			UKismetSystemLibrary::ExecuteConsoleCommand(WorldContextObject, TEXT("r.Lumen.Reflections.Allow 1"));
		}
		else
		{
			UKismetSystemLibrary::ExecuteConsoleCommand(WorldContextObject, TEXT("r.DynamicGlobalIlluminationMethod 0"));
			UKismetSystemLibrary::ExecuteConsoleCommand(WorldContextObject, TEXT("r.ReflectionMethod 0"));
			UKismetSystemLibrary::ExecuteConsoleCommand(WorldContextObject, TEXT("r.Lumen.DiffuseIndirect.Allow 0"));
			UKismetSystemLibrary::ExecuteConsoleCommand(WorldContextObject, TEXT("r.Lumen.Reflections.Allow 0"));
		}
	}

	static void SetIntCVar(const TCHAR* Name, int32 Value)
	{
		if (IConsoleVariable* CVar = IConsoleManager::Get().FindConsoleVariable(Name))
		{
			CVar->Set(Value, ECVF_SetByGameSetting);
		}
	}

	static int32 GetIntCVar(const TCHAR* Name, int32 DefaultValue = 0)
	{
		if (const IConsoleVariable* CVar = IConsoleManager::Get().FindConsoleVariable(Name))
		{
			return CVar->GetInt();
		}
		return DefaultValue;
	}

	static int32 ResolveFSRQualityModeValue(const FString& InFSRMode)
	{
		if (InFSRMode == TEXT("Native AA")) { return 0; }
		if (InFSRMode == TEXT("Quality")) { return 1; }
		if (InFSRMode == TEXT("Balanced")) { return 2; }
		if (InFSRMode == TEXT("Performance")) { return 3; }
		if (InFSRMode == TEXT("Ultra Performance")) { return 4; }
		return 1;
	}

	static bool TryParseResolution(const FString& InValue, FIntPoint& OutResolution)
	{
		FString Left;
		FString Right;
		if (!InValue.Split(TEXT("x"), &Left, &Right))
		{
			return false;
		}

		const int32 Width = FCString::Atoi(*Left);
		const int32 Height = FCString::Atoi(*Right);
		if (Width <= 0 || Height <= 0)
		{
			return false;
		}

		OutResolution = FIntPoint(Width, Height);
		return true;
	}
}

void UZonefallSettingsDataObject::SetDefaults()
{
	// Detect hardware support first so defaults are normalized correctly.
	bDLSSSupported = ZonefallSettingsParse::IsDLSSSupported();
	bFrameGenerationSupported = ZonefallSettingsParse::IsFrameGenerationSupported();
	bFSRSupported = ZonefallSettingsParse::IsFSRSupported();
	bFSRFrameGenerationSupported = ZonefallSettingsParse::IsFSRFrameGenerationSupported();

	DisplayMode = TEXT("Windowed Fullscreen");
	OverallQuality = TEXT("High");
	ResolutionScale = TEXT("100%");
	VSync = TEXT("On");
	FPSLimit = TEXT("120");
	Lumen = TEXT("On");
	DirectXVersion = TEXT("DirectX 12");
	RayTracing = TEXT("Off");
	VolumetricClouds = TEXT("High");
	DLSSMode = TEXT("Off");
	FrameGeneration = TEXT("Off");
	FSRMode = TEXT("Off");
	FSRFrameGeneration = TEXT("Off");
	ScreenResolution = TEXT("1920x1080");
	SanitizeSettings();
}

void UZonefallSettingsDataObject::LoadUpscalerSettingsFromConfig()
{
	// Per-user persistence: GameUserSettings.ini (Saved/Config/<Platform>/GameUserSettings.ini in packaged).
	FString SavedDLSS;
	FString SavedFG;
	FString SavedFSR;
	FString SavedFSRFG;

	if (GConfig)
	{
		GConfig->GetString(ZonefallSettingsParse::UpscalerConfigSection(), TEXT("DLSSMode"), SavedDLSS, GGameUserSettingsIni);
		GConfig->GetString(ZonefallSettingsParse::UpscalerConfigSection(), TEXT("FrameGeneration"), SavedFG, GGameUserSettingsIni);
		GConfig->GetString(ZonefallSettingsParse::UpscalerConfigSection(), TEXT("FSRMode"), SavedFSR, GGameUserSettingsIni);
		GConfig->GetString(ZonefallSettingsParse::UpscalerConfigSection(), TEXT("FSRFrameGeneration"), SavedFSRFG, GGameUserSettingsIni);
	}

	// Only override if we actually have something persisted.
	if (!SavedDLSS.IsEmpty()) { DLSSMode = SavedDLSS; }
	if (!SavedFG.IsEmpty()) { FrameGeneration = SavedFG; }
	if (!SavedFSR.IsEmpty()) { FSRMode = SavedFSR; }
	if (!SavedFSRFG.IsEmpty()) { FSRFrameGeneration = SavedFSRFG; }

	// Ensure values are valid for current runtime support.
	bDLSSSupported = ZonefallSettingsParse::IsDLSSSupported();
	bFrameGenerationSupported = ZonefallSettingsParse::IsFrameGenerationSupported();
	bFSRSupported = ZonefallSettingsParse::IsFSRSupported();
	bFSRFrameGenerationSupported = ZonefallSettingsParse::IsFSRFrameGenerationSupported();
	SanitizeSettings();
}

void UZonefallSettingsDataObject::SaveUpscalerSettingsToConfig() const
{
	if (!GConfig)
	{
		return;
	}

	GConfig->SetString(ZonefallSettingsParse::UpscalerConfigSection(), TEXT("DLSSMode"), *DLSSMode, GGameUserSettingsIni);
	GConfig->SetString(ZonefallSettingsParse::UpscalerConfigSection(), TEXT("FrameGeneration"), *FrameGeneration, GGameUserSettingsIni);
	GConfig->SetString(ZonefallSettingsParse::UpscalerConfigSection(), TEXT("FSRMode"), *FSRMode, GGameUserSettingsIni);
	GConfig->SetString(ZonefallSettingsParse::UpscalerConfigSection(), TEXT("FSRFrameGeneration"), *FSRFrameGeneration, GGameUserSettingsIni);
	GConfig->Flush(false, GGameUserSettingsIni);
}

void UZonefallSettingsDataObject::LoadFromSystem()
{
	UGameUserSettings* Settings = UGameUserSettings::GetGameUserSettings();
	if (!Settings)
	{
		SetDefaults();
		return;
	}

	const EWindowMode::Type Mode = Settings->GetFullscreenMode();
	if (Mode == EWindowMode::Fullscreen)
	{
		DisplayMode = TEXT("Fullscreen");
	}
	else if (Mode == EWindowMode::Windowed)
	{
		DisplayMode = TEXT("Windowed");
	}
	else
	{
		DisplayMode = TEXT("Windowed Fullscreen");
	}

	const int32 Quality = Settings->GetOverallScalabilityLevel();
	int32 EffectiveQuality = Quality;
	if (EffectiveQuality < 0)
	{
		// Custom/non-uniform scalability can report -1. Derive a stable UI level from current groups.
		const auto& Q = Settings->ScalabilityQuality;
		const int32 Sum =
			Q.ViewDistanceQuality +
			Q.AntiAliasingQuality +
			Q.ShadowQuality +
			Q.GlobalIlluminationQuality +
			Q.ReflectionQuality +
			Q.PostProcessQuality +
			Q.TextureQuality +
			Q.EffectsQuality +
			Q.FoliageQuality +
			Q.ShadingQuality;
		EffectiveQuality = FMath::Clamp(FMath::RoundToInt(static_cast<float>(Sum) / 10.0f), 0, 4);
	}

	switch (EffectiveQuality)
	{
	case 0: OverallQuality = TEXT("Low"); break;
	case 1: OverallQuality = TEXT("Medium"); break;
	case 2: OverallQuality = TEXT("High"); break;
	case 3: OverallQuality = TEXT("Epic"); break;
	default: OverallQuality = TEXT("Cinematic"); break;
	}

	ResolutionScale = FString::Printf(TEXT("%.0f%%"), Settings->GetResolutionScaleNormalized() * 100.0f);
	VSync = Settings->IsVSyncEnabled() ? TEXT("On") : TEXT("Off");

	const float Limit = Settings->GetFrameRateLimit();
	if (Limit <= 0.0f)
	{
		FPSLimit = TEXT("Unlimited");
	}
	else
	{
		FPSLimit = FString::Printf(TEXT("%.0f"), Limit);
	}

	Lumen = ZonefallSettingsParse::DetectLumenStateString();
	bDLSSSupported = ZonefallSettingsParse::IsDLSSSupported();
	bFrameGenerationSupported = ZonefallSettingsParse::IsFrameGenerationSupported();
	bFSRSupported = ZonefallSettingsParse::IsFSRSupported();
	bFSRFrameGenerationSupported = ZonefallSettingsParse::IsFSRFrameGenerationSupported();
	DLSSMode = ZonefallSettingsParse::DetectDLSSModeString(bDLSSSupported);
	FrameGeneration = ZonefallSettingsParse::DetectFrameGenerationString(bFrameGenerationSupported);
	FSRMode = ZonefallSettingsParse::DetectFSRModeString(bFSRSupported);
	FSRFrameGeneration = ZonefallSettingsParse::DetectFSRFrameGenerationString(bFSRFrameGenerationSupported);
	// If we persisted upscaler preferences, prefer showing them in UI.
	// The actual runtime application is done on startup by the game instance (or after Apply).
	LoadUpscalerSettingsFromConfig();

	// Restore persisted audio/display extras.
	if (GConfig)
	{
		const TCHAR* Section = ZonefallSettingsParse::AudioDisplayConfigSection();
		float V = 0.0f;
		if (GConfig->GetFloat(Section, TEXT("MasterVolume"), V, GGameUserSettingsIni)) { MasterVolume = FMath::Clamp(V, 0.0f, 1.0f); }
		if (GConfig->GetFloat(Section, TEXT("SfxVolume"), V, GGameUserSettingsIni)) { SfxVolume = FMath::Clamp(V, 0.0f, 1.0f); }
		if (GConfig->GetFloat(Section, TEXT("MusicVolume"), V, GGameUserSettingsIni)) { MusicVolume = FMath::Clamp(V, 0.0f, 1.0f); }
		if (GConfig->GetFloat(Section, TEXT("VoiceVolume"), V, GGameUserSettingsIni)) { VoiceVolume = FMath::Clamp(V, 0.0f, 1.0f); }
		if (GConfig->GetFloat(Section, TEXT("Brightness"), V, GGameUserSettingsIni)) { Brightness = FMath::Clamp(V, 0.5f, 2.0f); }
		if (GConfig->GetFloat(Section, TEXT("FieldOfView"), V, GGameUserSettingsIni)) { FieldOfView = FMath::Clamp(V, 60.0f, 120.0f); }
	}

	// Detect the currently-running graphics RHI for the DirectX combo.
	{
		const FString RHIName = GDynamicRHI ? FString(GDynamicRHI->GetName()) : FString();
		DirectXVersion = RHIName.Contains(TEXT("D3D11")) ? TEXT("DirectX 11") : TEXT("DirectX 12");
	}

	// Read the persisted ray tracing master switch.
	if (GConfig)
	{
		bool bRT = false;
		GConfig->GetBool(TEXT("/Script/Engine.RendererSettings"), TEXT("r.RayTracing"), bRT, GEngineIni);
		RayTracing = bRT ? TEXT("On") : TEXT("Off");
	}

	// Volumetric clouds: reflect the live CVar (on/off); default High when enabled.
	if (IConsoleVariable* CloudCVar = IConsoleManager::Get().FindConsoleVariable(TEXT("r.VolumetricCloud")))
	{
		if (CloudCVar->GetInt() == 0)
		{
			VolumetricClouds = TEXT("Off");
		}
		else if (VolumetricClouds.IsEmpty() || VolumetricClouds == TEXT("Off"))
		{
			VolumetricClouds = TEXT("High");
		}
	}

	// Populate advanced per-group quality from the current scalability state.
	{
		AdvancedQuality.Add(TEXT("ViewDistance"), ZonefallSettingsParse::GroupLevelToString(Settings->ScalabilityQuality.ViewDistanceQuality));
		AdvancedQuality.Add(TEXT("Shadows"), ZonefallSettingsParse::GroupLevelToString(Settings->ScalabilityQuality.ShadowQuality));
		AdvancedQuality.Add(TEXT("GlobalIllumination"), ZonefallSettingsParse::GroupLevelToString(Settings->ScalabilityQuality.GlobalIlluminationQuality));
		AdvancedQuality.Add(TEXT("Reflections"), ZonefallSettingsParse::GroupLevelToString(Settings->ScalabilityQuality.ReflectionQuality));
		AdvancedQuality.Add(TEXT("PostProcess"), ZonefallSettingsParse::GroupLevelToString(Settings->ScalabilityQuality.PostProcessQuality));
		AdvancedQuality.Add(TEXT("Textures"), ZonefallSettingsParse::GroupLevelToString(Settings->ScalabilityQuality.TextureQuality));
		AdvancedQuality.Add(TEXT("Effects"), ZonefallSettingsParse::GroupLevelToString(Settings->ScalabilityQuality.EffectsQuality));
		AdvancedQuality.Add(TEXT("Foliage"), ZonefallSettingsParse::GroupLevelToString(Settings->ScalabilityQuality.FoliageQuality));
		AdvancedQuality.Add(TEXT("Shading"), ZonefallSettingsParse::GroupLevelToString(Settings->ScalabilityQuality.ShadingQuality));
		AdvancedQuality.Add(TEXT("AntiAliasing"), ZonefallSettingsParse::GroupLevelToString(Settings->ScalabilityQuality.AntiAliasingQuality));
	}

	const FIntPoint Resolution = Settings->GetScreenResolution();
	ScreenResolution = FString::Printf(TEXT("%dx%d"), Resolution.X, Resolution.Y);
	SanitizeSettings();

	UE_LOG(LogTemp, Log, TEXT("[SettingsDebug] Overall=%d DisplayMode=%s ScreenRes=%s VSync=%s FPS=%s Lumen=%s DLSS=%s FG=%s FSR=%s FSRFG=%s"),
		Settings->GetOverallScalabilityLevel(),
		*DisplayMode,
		*ScreenResolution,
		*VSync,
		*FPSLimit,
		*Lumen,
		*DLSSMode,
		*FrameGeneration,
		*FSRMode,
		*FSRFrameGeneration);

	UE_LOG(LogTemp, Log, TEXT("[SettingsDebug][Groups] ViewDistance=%d AntiAliasing=%d Shadow=%d GI=%d Reflection=%d PostProcess=%d Texture=%d Effects=%d Foliage=%d Shading=%d"),
		Settings->ScalabilityQuality.ViewDistanceQuality,
		Settings->ScalabilityQuality.AntiAliasingQuality,
		Settings->ScalabilityQuality.ShadowQuality,
		Settings->ScalabilityQuality.GlobalIlluminationQuality,
		Settings->ScalabilityQuality.ReflectionQuality,
		Settings->ScalabilityQuality.PostProcessQuality,
		Settings->ScalabilityQuality.TextureQuality,
		Settings->ScalabilityQuality.EffectsQuality,
		Settings->ScalabilityQuality.FoliageQuality,
		Settings->ScalabilityQuality.ShadingQuality);
}

void UZonefallSettingsDataObject::ApplyToSystem(UObject* WorldContextObject)
{
	UGameUserSettings* Settings = UGameUserSettings::GetGameUserSettings();
	if (!Settings)
	{
		return;
	}

	// Refresh capability flags before sanitizing target values.
	bDLSSSupported = ZonefallSettingsParse::IsDLSSSupported();
	bFrameGenerationSupported = ZonefallSettingsParse::IsFrameGenerationSupported();
	bFSRSupported = ZonefallSettingsParse::IsFSRSupported();
	bFSRFrameGenerationSupported = ZonefallSettingsParse::IsFSRFrameGenerationSupported();
	SanitizeSettings();

	const FString RequestedLumenState = Lumen;

	if (DisplayMode == TEXT("Fullscreen"))
	{
		Settings->SetFullscreenMode(EWindowMode::Fullscreen);
	}
	else if (DisplayMode == TEXT("Windowed"))
	{
		Settings->SetFullscreenMode(EWindowMode::Windowed);
	}
	else
	{
		Settings->SetFullscreenMode(EWindowMode::WindowedFullscreen);
	}

	FIntPoint ParsedResolution;
	if (ZonefallSettingsParse::TryParseResolution(ScreenResolution, ParsedResolution))
	{
		Settings->SetScreenResolution(ParsedResolution);
	}

	const int32 TargetQualityLevel = ZonefallSettingsParse::ParseOverallQualityLevel(OverallQuality);
	Settings->SetOverallScalabilityLevel(TargetQualityLevel);
	Settings->ScalabilityQuality.SetFromSingleQualityLevel(TargetQualityLevel);
	Settings->ScalabilityQuality.ViewDistanceQuality = TargetQualityLevel;
	Settings->ScalabilityQuality.AntiAliasingQuality = TargetQualityLevel;
	Settings->ScalabilityQuality.ShadowQuality = TargetQualityLevel;
	Settings->ScalabilityQuality.GlobalIlluminationQuality = TargetQualityLevel;
	Settings->ScalabilityQuality.ReflectionQuality = TargetQualityLevel;
	Settings->ScalabilityQuality.PostProcessQuality = TargetQualityLevel;
	Settings->ScalabilityQuality.TextureQuality = TargetQualityLevel;
	Settings->ScalabilityQuality.EffectsQuality = TargetQualityLevel;
	Settings->ScalabilityQuality.FoliageQuality = TargetQualityLevel;
	Settings->ScalabilityQuality.ShadingQuality = TargetQualityLevel;

	// Per-group overrides on top of the overall preset (advanced graphics).
	{
		auto ApplyGroup = [&](const TCHAR* Group, int32& Field)
		{
			if (const FString* V = AdvancedQuality.Find(FName(Group)))
			{
				if (!V->IsEmpty())
				{
					Field = ZonefallSettingsParse::ParseOverallQualityLevel(*V);
				}
			}
		};
		ApplyGroup(TEXT("ViewDistance"), Settings->ScalabilityQuality.ViewDistanceQuality);
		ApplyGroup(TEXT("Shadows"), Settings->ScalabilityQuality.ShadowQuality);
		ApplyGroup(TEXT("GlobalIllumination"), Settings->ScalabilityQuality.GlobalIlluminationQuality);
		ApplyGroup(TEXT("Reflections"), Settings->ScalabilityQuality.ReflectionQuality);
		ApplyGroup(TEXT("PostProcess"), Settings->ScalabilityQuality.PostProcessQuality);
		ApplyGroup(TEXT("Textures"), Settings->ScalabilityQuality.TextureQuality);
		ApplyGroup(TEXT("Effects"), Settings->ScalabilityQuality.EffectsQuality);
		ApplyGroup(TEXT("Foliage"), Settings->ScalabilityQuality.FoliageQuality);
		ApplyGroup(TEXT("Shading"), Settings->ScalabilityQuality.ShadingQuality);
		ApplyGroup(TEXT("AntiAliasing"), Settings->ScalabilityQuality.AntiAliasingQuality);
	}

	const FString ScaleNumeric = ResolutionScale.Replace(TEXT("%"), TEXT(""));
	const float Scale = FMath::Clamp(FCString::Atof(*ScaleNumeric), 50.0f, 100.0f);
	Settings->SetResolutionScaleValueEx(Scale);

	Settings->SetVSyncEnabled(VSync == TEXT("On"));

	if (FPSLimit == TEXT("Unlimited"))
	{
		Settings->SetFrameRateLimit(0.0f);
	}
	else
	{
		Settings->SetFrameRateLimit(FMath::Max(30.0f, FCString::Atof(*FPSLimit)));
	}

	Settings->ApplyNonResolutionSettings();
	Settings->ApplySettings(false);
	Settings->SaveSettings();

	// Lumen toggle via runtime CVars (with direct CVar write + verification path).
	if (WorldContextObject)
	{
		// Avoid conflicts with NVIDIA NIS primary spatial upscaler when using DLSS/FSR.
		UKismetSystemLibrary::ExecuteConsoleCommand(WorldContextObject, TEXT("r.NIS.Enable 0"));
		UKismetSystemLibrary::ExecuteConsoleCommand(WorldContextObject, TEXT("r.NIS.Upscaling 0"));
		UKismetSystemLibrary::ExecuteConsoleCommand(WorldContextObject, TEXT("r.TemporalAA.Upscaler 1"));

		const bool bEnableLumen = (Lumen == TEXT("On"));
		ZonefallSettingsParse::ApplyLumenState(WorldContextObject, bEnableLumen);
	}

	const bool bRequestDLSS = (DLSSMode != TEXT("Off") && DLSSMode != TEXT("Unavailable")) && bDLSSSupported;
	const bool bRequestFSR = (FSRMode != TEXT("Off") && FSRMode != TEXT("Unavailable")) && bFSRSupported && !bRequestDLSS;
	if (WorldContextObject)
	{
		// Reset to known neutral state first for stable plugin behavior.
		if (bDLSSSupported && UDLSSLibrary::IsDLSSEnabled())
		{
			UDLSSLibrary::EnableDLSS(false);
		}
		ZonefallSettingsParse::SetIntCVar(TEXT("r.FidelityFX.FSR.Enabled"), 0);
		UKismetSystemLibrary::ExecuteConsoleCommand(WorldContextObject, TEXT("r.FidelityFX.FSR.Enabled 0"));

		if (bRequestDLSS)
		{
			UDLSSMode Mode = UDLSSMode::Quality;
			if (DLSSMode == TEXT("Balanced")) { Mode = UDLSSMode::Balanced; }
			else if (DLSSMode == TEXT("Performance")) { Mode = UDLSSMode::Performance; }
			else if (DLSSMode == TEXT("Ultra Performance")) { Mode = UDLSSMode::UltraPerformance; }
			else if (DLSSMode == TEXT("DLAA")) { Mode = UDLSSMode::DLAA; }

			if (bDLSSSupported)
			{
				UDLSSLibrary::SetDLSSMode(WorldContextObject, Mode);
				UDLSSLibrary::EnableDLSS(true);
			}
		}
		else if (bRequestFSR)
		{
			const int32 FSRQualityModeValue = ZonefallSettingsParse::ResolveFSRQualityModeValue(FSRMode);
			ZonefallSettingsParse::SetIntCVar(TEXT("r.FidelityFX.FSR.QualityMode"), FSRQualityModeValue);
			ZonefallSettingsParse::SetIntCVar(TEXT("r.FidelityFX.FSR.Enabled"), 1);
			UKismetSystemLibrary::ExecuteConsoleCommand(WorldContextObject, TEXT("r.FidelityFX.FSR.Enabled 1"));
			UKismetSystemLibrary::ExecuteConsoleCommand(
				WorldContextObject,
				FString::Printf(TEXT("r.FidelityFX.FSR.QualityMode %d"), FSRQualityModeValue)
			);
		}
	}

	if (WorldContextObject && bFrameGenerationSupported)
	{
		if (FSRFrameGeneration == TEXT("On"))
		{
			FrameGeneration = TEXT("Off");
		}

		const EStreamlineDLSSGMode FGMode = (FrameGeneration == TEXT("On")) ? EStreamlineDLSSGMode::On2X : EStreamlineDLSSGMode::Off;
		UWorld* World = WorldContextObject->GetWorld();
		const bool bAllowSetFG = World && World->WorldType == EWorldType::Game;
		const bool bFeatureStillSupported = UStreamlineLibraryDLSSG::QueryDLSSGSupport() == EStreamlineFeatureSupport::Supported;
		if (bAllowSetFG && bFeatureStillSupported)
		{
			UStreamlineLibraryDLSSG::SetDLSSGMode(FGMode);
		}
	}

	// FSR Frame Generation (frame interpolation) is force-disabled: it crashes in this FSR
	// plugin build (FFXD3D12Backend::UpdateSwapChain assert during Present). Keep it OFF so the
	// game stays stable. DLSS/FSR upscaling are unaffected.
	if (WorldContextObject)
	{
		FSRFrameGeneration = TEXT("Off");
		UKismetSystemLibrary::ExecuteConsoleCommand(WorldContextObject, TEXT("r.FidelityFX.FI.Enabled 0"));
	}

	// Persist upscaler preferences separately (CVars may not serialize through UGameUserSettings).
	SaveUpscalerSettingsToConfig();

	// Read back real runtime state (in case platform/project overrides CVars).
	Lumen = ZonefallSettingsParse::DetectLumenStateString();
	bDLSSSupported = ZonefallSettingsParse::IsDLSSSupported();
	bFrameGenerationSupported = ZonefallSettingsParse::IsFrameGenerationSupported();
	bFSRSupported = ZonefallSettingsParse::IsFSRSupported();
	bFSRFrameGenerationSupported = ZonefallSettingsParse::IsFSRFrameGenerationSupported();
	DLSSMode = ZonefallSettingsParse::DetectDLSSModeString(bDLSSSupported);
	FrameGeneration = ZonefallSettingsParse::DetectFrameGenerationString(bFrameGenerationSupported);
	FSRMode = ZonefallSettingsParse::DetectFSRModeString(bFSRSupported);
	FSRFrameGeneration = ZonefallSettingsParse::DetectFSRFrameGenerationString(bFSRFrameGenerationSupported);

	const int32 RuntimeFSREnabled = ZonefallSettingsParse::GetIntCVar(TEXT("r.FidelityFX.FSR.Enabled"), -1);
	const int32 RuntimeFSRQuality = ZonefallSettingsParse::GetIntCVar(TEXT("r.FidelityFX.FSR.QualityMode"), -1);
	UE_LOG(
		LogTemp,
		Log,
		TEXT("[SettingsDebug][Upscaler] RequestDLSS=%d RequestFSR=%d RuntimeDLSS=%s RuntimeFSREnabled=%d RuntimeFSRQuality=%d"),
		bRequestDLSS ? 1 : 0,
		bRequestFSR ? 1 : 0,
		*DLSSMode,
		RuntimeFSREnabled,
		RuntimeFSRQuality
	);
	UE_LOG(LogTemp, Log, TEXT("[SettingsDebug][Lumen] Requested=%s Actual=%s"), *RequestedLumenState, *ZonefallSettingsParse::DetectLumenStateString());

	// --- Ray Tracing (RTX) ---
	{
		const bool bRT = (RayTracing == TEXT("On"));
		if (WorldContextObject)
		{
			// These take effect at runtime when ray tracing was enabled at startup.
			UKismetSystemLibrary::ExecuteConsoleCommand(WorldContextObject, FString::Printf(TEXT("r.Lumen.HardwareRayTracing %d"), bRT ? 1 : 0));
			UKismetSystemLibrary::ExecuteConsoleCommand(WorldContextObject, FString::Printf(TEXT("r.RayTracing.Shadows %d"), bRT ? 1 : 0));
			UKismetSystemLibrary::ExecuteConsoleCommand(WorldContextObject, FString::Printf(TEXT("r.RayTracing.Reflections %d"), bRT ? 1 : 0));
			UKismetSystemLibrary::ExecuteConsoleCommand(WorldContextObject, FString::Printf(TEXT("r.RayTracing.AmbientOcclusion %d"), bRT ? 1 : 0));
		}
		// Persist the master switch (read at startup — RT can only be enabled before init).
		if (GConfig)
		{
			GConfig->SetBool(TEXT("/Script/Engine.RendererSettings"), TEXT("r.RayTracing"), bRT, GEngineIni);
			GConfig->Flush(false, GEngineIni);
		}
	}

	// --- Volumetric clouds (applies live via CVars) ---
	if (WorldContextObject)
	{
		const bool bClouds = (VolumetricClouds != TEXT("Off"));
		UKismetSystemLibrary::ExecuteConsoleCommand(WorldContextObject, FString::Printf(TEXT("r.VolumetricCloud %d"), bClouds ? 1 : 0));
		if (bClouds)
		{
			int32 ViewSamples = 512;   // High
			int32 ReflSamples = 40;
			if (VolumetricClouds == TEXT("Low"))  { ViewSamples = 256; ReflSamples = 24; }
			else if (VolumetricClouds == TEXT("Epic")) { ViewSamples = 1024; ReflSamples = 80; }
			UKismetSystemLibrary::ExecuteConsoleCommand(WorldContextObject, FString::Printf(TEXT("r.VolumetricCloud.ViewRaySampleMaxCount %d"), ViewSamples));
			UKismetSystemLibrary::ExecuteConsoleCommand(WorldContextObject, FString::Printf(TEXT("r.VolumetricCloud.ReflectionRaySampleMaxCount %d"), ReflSamples));
		}
		if (GConfig)
		{
			GConfig->SetBool(TEXT("/Script/Engine.RendererSettings"), TEXT("r.VolumetricCloud"), bClouds, GEngineIni);
			GConfig->Flush(false, GEngineIni);
		}
	}

	// --- DirectX / RHI preference (applies on next launch) ---
	if (GConfig && !DirectXVersion.IsEmpty())
	{
		const FString RHIValue = (DirectXVersion == TEXT("DirectX 11"))
			? TEXT("DefaultGraphicsRHI_DX11")
			: TEXT("DefaultGraphicsRHI_DX12");
		GConfig->SetString(TEXT("/Script/WindowsTargetPlatform.WindowsTargetSettings"), TEXT("DefaultGraphicsRHI"), *RHIValue, GEngineIni);
		GConfig->Flush(false, GEngineIni);
	}

	// --- Brightness (display gamma): higher Brightness -> lower gamma -> brighter image. ---
	if (GEngine)
	{
		GEngine->DisplayGamma = FMath::Clamp(2.2f / FMath::Max(0.05f, Brightness), 0.5f, 6.0f);
	}

	// --- Audio (master volume) + FOV: applied to the live world / local player. ---
	if (WorldContextObject)
	{
		if (UWorld* World = WorldContextObject->GetWorld())
		{
			if (FAudioDeviceHandle AudioDevice = World->GetAudioDevice())
			{
				// Master volume without needing a SoundMix/SoundClass asset.
				AudioDevice->SetTransientPrimaryVolume(FMath::Clamp(MasterVolume, 0.0f, 1.0f));
			}

			if (APlayerController* PC = UGameplayStatics::GetPlayerController(World, 0))
			{
				if (AZonefallPlayerCharacter* ZChar = Cast<AZonefallPlayerCharacter>(PC->GetPawn()))
				{
					const float ClampedFOV = FMath::Clamp(FieldOfView, 60.0f, 120.0f);
					if (ZChar->ThirdPersonCamera) { ZChar->ThirdPersonCamera->SetFieldOfView(ClampedFOV); }
					if (ZChar->FirstPersonCamera) { ZChar->FirstPersonCamera->SetFieldOfView(ClampedFOV); }
				}
			}
		}
	}

	// --- Persist audio/display extras (UGameUserSettings doesn't serialize these). ---
	if (GConfig)
	{
		const TCHAR* Section = ZonefallSettingsParse::AudioDisplayConfigSection();
		GConfig->SetFloat(Section, TEXT("MasterVolume"), MasterVolume, GGameUserSettingsIni);
		GConfig->SetFloat(Section, TEXT("SfxVolume"), SfxVolume, GGameUserSettingsIni);
		GConfig->SetFloat(Section, TEXT("MusicVolume"), MusicVolume, GGameUserSettingsIni);
		GConfig->SetFloat(Section, TEXT("VoiceVolume"), VoiceVolume, GGameUserSettingsIni);
		GConfig->SetFloat(Section, TEXT("Brightness"), Brightness, GGameUserSettingsIni);
		GConfig->SetFloat(Section, TEXT("FieldOfView"), FieldOfView, GGameUserSettingsIni);
		GConfig->Flush(false, GGameUserSettingsIni);
	}
}

void UZonefallSettingsDataObject::ApplyUpscalerSettingsOnly(UObject* WorldContextObject)
{
	// Refresh capability flags before sanitizing.
	bDLSSSupported = ZonefallSettingsParse::IsDLSSSupported();
	bFrameGenerationSupported = ZonefallSettingsParse::IsFrameGenerationSupported();
	bFSRSupported = ZonefallSettingsParse::IsFSRSupported();
	bFSRFrameGenerationSupported = ZonefallSettingsParse::IsFSRFrameGenerationSupported();
	SanitizeSettings();

	if (!WorldContextObject)
	{
		return;
	}

	const bool bRequestDLSS = (DLSSMode != TEXT("Off") && DLSSMode != TEXT("Unavailable")) && bDLSSSupported;
	const bool bRequestFSR = (FSRMode != TEXT("Off") && FSRMode != TEXT("Unavailable")) && bFSRSupported && !bRequestDLSS;

	// Neutral baseline for stable behavior.
	if (bDLSSSupported && UDLSSLibrary::IsDLSSEnabled())
	{
		UDLSSLibrary::EnableDLSS(false);
	}
	ZonefallSettingsParse::SetIntCVar(TEXT("r.FidelityFX.FSR.Enabled"), 0);
	UKismetSystemLibrary::ExecuteConsoleCommand(WorldContextObject, TEXT("r.FidelityFX.FSR.Enabled 0"));

	if (bRequestDLSS)
	{
		UDLSSMode Mode = UDLSSMode::Quality;
		if (DLSSMode == TEXT("Balanced")) { Mode = UDLSSMode::Balanced; }
		else if (DLSSMode == TEXT("Performance")) { Mode = UDLSSMode::Performance; }
		else if (DLSSMode == TEXT("Ultra Performance")) { Mode = UDLSSMode::UltraPerformance; }
		else if (DLSSMode == TEXT("DLAA")) { Mode = UDLSSMode::DLAA; }

		UDLSSLibrary::SetDLSSMode(WorldContextObject, Mode);
		UDLSSLibrary::EnableDLSS(true);
	}
	else if (bRequestFSR)
	{
		const int32 FSRQualityModeValue = ZonefallSettingsParse::ResolveFSRQualityModeValue(FSRMode);
		ZonefallSettingsParse::SetIntCVar(TEXT("r.FidelityFX.FSR.QualityMode"), FSRQualityModeValue);
		ZonefallSettingsParse::SetIntCVar(TEXT("r.FidelityFX.FSR.Enabled"), 1);
		UKismetSystemLibrary::ExecuteConsoleCommand(WorldContextObject, TEXT("r.FidelityFX.FSR.Enabled 1"));
		UKismetSystemLibrary::ExecuteConsoleCommand(
			WorldContextObject,
			FString::Printf(TEXT("r.FidelityFX.FSR.QualityMode %d"), FSRQualityModeValue)
		);
	}

	if (bFrameGenerationSupported)
	{
		if (FSRFrameGeneration == TEXT("On"))
		{
			FrameGeneration = TEXT("Off");
		}

		const EStreamlineDLSSGMode FGMode = (FrameGeneration == TEXT("On")) ? EStreamlineDLSSGMode::On2X : EStreamlineDLSSGMode::Off;
		UWorld* World = WorldContextObject->GetWorld();
		const bool bAllowSetFG = World && World->WorldType == EWorldType::Game;
		const bool bFeatureStillSupported = UStreamlineLibraryDLSSG::QueryDLSSGSupport() == EStreamlineFeatureSupport::Supported;
		if (bAllowSetFG && bFeatureStillSupported)
		{
			UStreamlineLibraryDLSSG::SetDLSSGMode(FGMode);
		}
	}

	// Keep FSR Frame Generation OFF on startup too (crash-prone in this plugin build).
	if (WorldContextObject)
	{
		FSRFrameGeneration = TEXT("Off");
		UKismetSystemLibrary::ExecuteConsoleCommand(WorldContextObject, TEXT("r.FidelityFX.FI.Enabled 0"));
	}
}

void UZonefallSettingsDataObject::ApplyDisplayModeAndResolution(bool bSaveSettings)
{
	UGameUserSettings* Settings = UGameUserSettings::GetGameUserSettings();
	if (!Settings)
	{
		return;
	}

	EWindowMode::Type ModeToApply = EWindowMode::WindowedFullscreen;
	if (DisplayMode == TEXT("Fullscreen"))
	{
		ModeToApply = EWindowMode::Fullscreen;
	}
	else if (DisplayMode == TEXT("Windowed"))
	{
		ModeToApply = EWindowMode::Windowed;
	}

	FIntPoint TargetResolution = Settings->GetScreenResolution();
	FIntPoint ParsedResolution;
	if (ZonefallSettingsParse::TryParseResolution(ScreenResolution, ParsedResolution))
	{
		TargetResolution = ParsedResolution;
	}

	Settings->RequestResolutionChange(TargetResolution.X, TargetResolution.Y, ModeToApply, false);
	Settings->ConfirmVideoMode();
	Settings->SetFullscreenMode(ModeToApply);
	Settings->SetScreenResolution(TargetResolution);
	Settings->ApplyResolutionSettings(false);

	if (bSaveSettings)
	{
		Settings->SaveSettings();
	}
}

void UZonefallSettingsDataObject::GetAvailableScreenResolutions(TArray<FString>& OutResolutions, bool bOnly16by9)
{
	OutResolutions.Reset();

	TArray<FIntPoint> Resolutions;
	if (!UKismetSystemLibrary::GetSupportedFullscreenResolutions(Resolutions))
	{
		OutResolutions = { TEXT("1280x720"), TEXT("1600x900"), TEXT("1920x1080"), TEXT("2560x1440") };
		return;
	}

	for (const FIntPoint& Resolution : Resolutions)
	{
		if (Resolution.X <= 0 || Resolution.Y <= 0)
		{
			continue;
		}

		if (bOnly16by9)
		{
			const float Aspect = static_cast<float>(Resolution.X) / static_cast<float>(Resolution.Y);
			if (!FMath::IsNearlyEqual(Aspect, 16.0f / 9.0f, 0.02f))
			{
				continue;
			}
		}

		OutResolutions.AddUnique(FString::Printf(TEXT("%dx%d"), Resolution.X, Resolution.Y));
	}

	OutResolutions.Sort([](const FString& A, const FString& B)
	{
		FIntPoint RA(0, 0);
		FIntPoint RB(0, 0);
		const bool bValidA = ZonefallSettingsParse::TryParseResolution(A, RA);
		const bool bValidB = ZonefallSettingsParse::TryParseResolution(B, RB);
		if (!bValidA || !bValidB)
		{
			return A < B;
		}

		const int32 PixelsA = RA.X * RA.Y;
		const int32 PixelsB = RB.X * RB.Y;
		return PixelsA < PixelsB;
	});
}

FString UZonefallSettingsDataObject::GetCurrentScreenResolutionString() const
{
	if (!ScreenResolution.IsEmpty())
	{
		return ScreenResolution;
	}

	UGameUserSettings* Settings = UGameUserSettings::GetGameUserSettings();
	if (!Settings)
	{
		return TEXT("1920x1080");
	}

	const FIntPoint Resolution = Settings->GetScreenResolution();
	return FString::Printf(TEXT("%dx%d"), Resolution.X, Resolution.Y);
}

bool UZonefallSettingsDataObject::SetScreenResolutionFromString(const FString& ResolutionString, bool bApplyNow)
{
	FIntPoint ParsedResolution;
	if (!ZonefallSettingsParse::TryParseResolution(ResolutionString, ParsedResolution))
	{
		return false;
	}

	ScreenResolution = ResolutionString;

	UGameUserSettings* Settings = UGameUserSettings::GetGameUserSettings();
	if (!Settings)
	{
		return false;
	}

	Settings->SetScreenResolution(ParsedResolution);
	if (bApplyNow)
	{
		Settings->ApplySettings(false);
		Settings->SaveSettings();
	}

	return true;
}

void UZonefallSettingsDataObject::SanitizeSettings()
{
	DisplayMode = NormalizeDisplayModeValue(DisplayMode);
	OverallQuality = NormalizeQualityValue(OverallQuality);
	ResolutionScale = NormalizeResolutionScaleValue(ResolutionScale);
	VSync = NormalizeVSyncValue(VSync);
	FPSLimit = NormalizeFPSLimitValue(FPSLimit);
	Lumen = NormalizeLumenValue(Lumen);
	DLSSMode = NormalizeDLSSValue(DLSSMode, bDLSSSupported);
	FrameGeneration = NormalizeFrameGenerationValue(FrameGeneration, bFrameGenerationSupported);
	FSRMode = NormalizeFSRValue(FSRMode, bFSRSupported);
	FSRFrameGeneration = NormalizeFSRFrameGenerationValue(FSRFrameGeneration, bFSRFrameGenerationSupported);

	FIntPoint ParsedResolution;
	if (!ZonefallSettingsParse::TryParseResolution(ScreenResolution, ParsedResolution))
	{
		ScreenResolution = TEXT("1920x1080");
	}

	ResolveUpscalerConflicts();
}

void UZonefallSettingsDataObject::ApplyGraphicsPreset(EZonefallGraphicsPreset Preset)
{
	// Refresh support flags so preset logic is hardware-aware.
	bDLSSSupported = ZonefallSettingsParse::IsDLSSSupported();
	bFrameGenerationSupported = ZonefallSettingsParse::IsFrameGenerationSupported();
	bFSRSupported = ZonefallSettingsParse::IsFSRSupported();
	bFSRFrameGenerationSupported = ZonefallSettingsParse::IsFSRFrameGenerationSupported();

	switch (Preset)
	{
	case EZonefallGraphicsPreset::Competitive:
		DisplayMode = TEXT("Windowed Fullscreen");
		OverallQuality = TEXT("Medium");
		ResolutionScale = TEXT("80%");
		VSync = TEXT("Off");
		FPSLimit = TEXT("120");
		Lumen = TEXT("Off");
		DLSSMode = TEXT("Off");
		FrameGeneration = TEXT("Off");
		FSRMode = bFSRSupported ? TEXT("Quality") : TEXT("Off");
		FSRFrameGeneration = TEXT("Off");
		break;

	case EZonefallGraphicsPreset::Balanced:
		DisplayMode = TEXT("Windowed Fullscreen");
		OverallQuality = TEXT("High");
		ResolutionScale = TEXT("100%");
		VSync = TEXT("On");
		FPSLimit = TEXT("120");
		Lumen = TEXT("On");
		if (bDLSSSupported)
		{
			DLSSMode = TEXT("Quality");
			FSRMode = TEXT("Off");
		}
		else if (bFSRSupported)
		{
			DLSSMode = TEXT("Off");
			FSRMode = TEXT("Quality");
		}
		else
		{
			DLSSMode = TEXT("Off");
			FSRMode = TEXT("Off");
		}
		FrameGeneration = TEXT("Off");
		FSRFrameGeneration = TEXT("Off");
		break;

	case EZonefallGraphicsPreset::Quality:
		DisplayMode = TEXT("Fullscreen");
		OverallQuality = TEXT("Epic");
		ResolutionScale = TEXT("100%");
		VSync = TEXT("On");
		FPSLimit = TEXT("60");
		Lumen = TEXT("On");
		if (bDLSSSupported)
		{
			DLSSMode = TEXT("DLAA");
			FrameGeneration = bFrameGenerationSupported ? TEXT("On") : TEXT("Off");
			FSRMode = TEXT("Off");
			FSRFrameGeneration = TEXT("Off");
		}
		else
		{
			DLSSMode = TEXT("Off");
			FrameGeneration = TEXT("Off");
			FSRMode = bFSRSupported ? TEXT("Native AA") : TEXT("Off");
			FSRFrameGeneration = bFSRFrameGenerationSupported ? TEXT("On") : TEXT("Off");
		}
		break;

	case EZonefallGraphicsPreset::Ultra:
		// Maximum visual fidelity. Targets high-end GPUs.
		DisplayMode = TEXT("Fullscreen");
		OverallQuality = TEXT("Cinematic");
		ResolutionScale = TEXT("100%");
		VSync = TEXT("On");
		FPSLimit = TEXT("Unlimited");
		Lumen = TEXT("On");
		if (bDLSSSupported)
		{
			DLSSMode = TEXT("DLAA");
			FrameGeneration = bFrameGenerationSupported ? TEXT("On") : TEXT("Off");
			FSRMode = TEXT("Off");
			FSRFrameGeneration = TEXT("Off");
		}
		else
		{
			DLSSMode = TEXT("Off");
			FrameGeneration = TEXT("Off");
			FSRMode = bFSRSupported ? TEXT("Native AA") : TEXT("Off");
			FSRFrameGeneration = bFSRFrameGenerationSupported ? TEXT("On") : TEXT("Off");
		}
		break;

	case EZonefallGraphicsPreset::AutoDetect:
	default:
		ApplyGraphicsPreset(DetectRecommendedPreset());
		return;
	}

	SanitizeSettings();
}

EZonefallGraphicsPreset UZonefallSettingsDataObject::DetectRecommendedPreset() const
{
	const FPlatformMemoryStats MemStats = FPlatformMemory::GetStats();
	const uint64 TotalPhysicalMB = MemStats.TotalPhysical / (1024ull * 1024ull);
	const int32 NumCores = FPlatformMisc::NumberOfCores();
	const int32 NumLogicalCores = FPlatformMisc::NumberOfCoresIncludingHyperthreads();

	// Sniff GPU class via brand string (rough heuristic; works for NVIDIA/AMD/Intel desktops).
	const FString GpuBrand = FPlatformMisc::GetPrimaryGPUBrand();
	const FString GpuLower = GpuBrand.ToLower();

	const bool bHighEnd =
		GpuLower.Contains(TEXT("rtx 40")) ||
		GpuLower.Contains(TEXT("rtx 50")) ||
		GpuLower.Contains(TEXT("rtx 4080")) ||
		GpuLower.Contains(TEXT("rtx 4090")) ||
		GpuLower.Contains(TEXT("rx 7900")) ||
		GpuLower.Contains(TEXT("rx 9070")) ||
		GpuLower.Contains(TEXT("rx 9080")) ||
		GpuLower.Contains(TEXT("arc b770"));

	const bool bMidHigh =
		GpuLower.Contains(TEXT("rtx 30")) ||
		GpuLower.Contains(TEXT("rtx 4070")) ||
		GpuLower.Contains(TEXT("rtx 4060")) ||
		GpuLower.Contains(TEXT("rx 6800")) ||
		GpuLower.Contains(TEXT("rx 7700")) ||
		GpuLower.Contains(TEXT("rx 7800"));

	const bool bMid =
		GpuLower.Contains(TEXT("rtx 20")) ||
		GpuLower.Contains(TEXT("gtx 16")) ||
		GpuLower.Contains(TEXT("rx 6600")) ||
		GpuLower.Contains(TEXT("rx 6700"));

	UE_LOG(LogTemp, Log, TEXT("[SettingsDebug][AutoDetect] GPU='%s' RAM=%lluMB Cores=%d/%d -> highend=%d midhigh=%d mid=%d"),
		*GpuBrand, TotalPhysicalMB, NumCores, NumLogicalCores, bHighEnd ? 1 : 0, bMidHigh ? 1 : 0, bMid ? 1 : 0);

	if (bHighEnd && TotalPhysicalMB >= 24000 && NumLogicalCores >= 12)
	{
		return EZonefallGraphicsPreset::Ultra;
	}
	if (bMidHigh && TotalPhysicalMB >= 16000)
	{
		return EZonefallGraphicsPreset::Quality;
	}
	if (bMid && TotalPhysicalMB >= 8000)
	{
		return EZonefallGraphicsPreset::Balanced;
	}
	return EZonefallGraphicsPreset::Competitive;
}

FString UZonefallSettingsDataObject::NormalizeDisplayModeValue(const FString& InValue)
{
	const FString Normalized = ZonefallSettingsParse::CanonicalizeSpacesAndCase(InValue);
	if (Normalized == TEXT("fullscreen")) { return TEXT("Fullscreen"); }
	if (Normalized == TEXT("windowed")) { return TEXT("Windowed"); }
	if (Normalized == TEXT("windowed fullscreen") || Normalized == TEXT("borderless") || Normalized == TEXT("borderless fullscreen"))
	{
		return TEXT("Windowed Fullscreen");
	}
	return TEXT("Windowed Fullscreen");
}

FString UZonefallSettingsDataObject::NormalizeQualityValue(const FString& InValue)
{
	const FString Normalized = ZonefallSettingsParse::CanonicalizeSpacesAndCase(InValue);
	if (Normalized == TEXT("low")) { return TEXT("Low"); }
	if (Normalized == TEXT("medium")) { return TEXT("Medium"); }
	if (Normalized == TEXT("high")) { return TEXT("High"); }
	if (Normalized == TEXT("epic")) { return TEXT("Epic"); }
	if (Normalized == TEXT("cinematic")) { return TEXT("Cinematic"); }
	return TEXT("High");
}

FString UZonefallSettingsDataObject::NormalizeResolutionScaleValue(const FString& InValue)
{
	FString Raw = InValue.TrimStartAndEnd();
	Raw = Raw.Replace(TEXT("%"), TEXT(""));
	const float Value = FMath::Clamp(FCString::Atof(*Raw), 50.0f, 100.0f);
	// Keep UI and runtime values aligned to menu-supported steps (50, 60, ... 100).
	const int32 QuantizedValue = FMath::Clamp(FMath::RoundToInt(Value / 10.0f) * 10, 50, 100);
	return FString::Printf(TEXT("%d%%"), QuantizedValue);
}

FString UZonefallSettingsDataObject::NormalizeVSyncValue(const FString& InValue)
{
	const FString Normalized = ZonefallSettingsParse::CanonicalizeSpacesAndCase(InValue);
	return (Normalized == TEXT("on") || Normalized == TEXT("true") || Normalized == TEXT("1")) ? TEXT("On") : TEXT("Off");
}

FString UZonefallSettingsDataObject::NormalizeFPSLimitValue(const FString& InValue)
{
	const FString Normalized = ZonefallSettingsParse::CanonicalizeSpacesAndCase(InValue);
	if (Normalized == TEXT("unlimited") || Normalized == TEXT("0") || Normalized == TEXT("uncapped"))
	{
		return TEXT("Unlimited");
	}

	const int32 FPS = FMath::Clamp(FMath::RoundToInt(FCString::Atof(*Normalized)), 30, 360);
	return FString::FromInt(FPS);
}

FString UZonefallSettingsDataObject::NormalizeLumenValue(const FString& InValue)
{
	const FString Normalized = ZonefallSettingsParse::CanonicalizeSpacesAndCase(InValue);
	return (Normalized == TEXT("on") || Normalized == TEXT("true") || Normalized == TEXT("1")) ? TEXT("On") : TEXT("Off");
}

FString UZonefallSettingsDataObject::NormalizeDLSSValue(const FString& InValue, bool bSupported)
{
	if (!bSupported)
	{
		return TEXT("Unavailable");
	}

	const FString Normalized = ZonefallSettingsParse::CanonicalizeSpacesAndCase(InValue);
	if (Normalized == TEXT("off")) { return TEXT("Off"); }
	if (Normalized == TEXT("quality") || Normalized == TEXT("ultra quality")) { return TEXT("Quality"); }
	if (Normalized == TEXT("balanced")) { return TEXT("Balanced"); }
	if (Normalized == TEXT("performance")) { return TEXT("Performance"); }
	if (Normalized == TEXT("ultra performance")) { return TEXT("Ultra Performance"); }
	if (Normalized == TEXT("dlaa")) { return TEXT("DLAA"); }
	return TEXT("Off");
}

FString UZonefallSettingsDataObject::NormalizeFrameGenerationValue(const FString& InValue, bool bSupported)
{
	if (!bSupported)
	{
		return TEXT("Unavailable");
	}

	const FString Normalized = ZonefallSettingsParse::CanonicalizeSpacesAndCase(InValue);
	return (Normalized == TEXT("on") || Normalized == TEXT("true") || Normalized == TEXT("1")) ? TEXT("On") : TEXT("Off");
}

FString UZonefallSettingsDataObject::NormalizeFSRValue(const FString& InValue, bool bSupported)
{
	if (!bSupported)
	{
		return TEXT("Unavailable");
	}

	const FString Normalized = ZonefallSettingsParse::CanonicalizeSpacesAndCase(InValue);
	if (Normalized == TEXT("off")) { return TEXT("Off"); }
	if (Normalized == TEXT("native aa") || Normalized == TEXT("native")) { return TEXT("Native AA"); }
	if (Normalized == TEXT("quality")) { return TEXT("Quality"); }
	if (Normalized == TEXT("balanced")) { return TEXT("Balanced"); }
	if (Normalized == TEXT("performance")) { return TEXT("Performance"); }
	if (Normalized == TEXT("ultra performance")) { return TEXT("Ultra Performance"); }
	return TEXT("Off");
}

FString UZonefallSettingsDataObject::NormalizeFSRFrameGenerationValue(const FString& InValue, bool bSupported)
{
	if (!bSupported)
	{
		return TEXT("Unavailable");
	}

	const FString Normalized = ZonefallSettingsParse::CanonicalizeSpacesAndCase(InValue);
	return (Normalized == TEXT("on") || Normalized == TEXT("true") || Normalized == TEXT("1")) ? TEXT("On") : TEXT("Off");
}

void UZonefallSettingsDataObject::ResolveUpscalerConflicts()
{
	const bool bDLSSActive = (DLSSMode != TEXT("Off") && DLSSMode != TEXT("Unavailable"));
	const bool bFSRActive = (FSRMode != TEXT("Off") && FSRMode != TEXT("Unavailable"));

	// Spatial upscalers are mutually exclusive.
	if (bDLSSActive && bFSRActive)
	{
		FSRMode = TEXT("Off");
	}

	// DLSS FG and FSR FG cannot be active at once.
	if (FrameGeneration == TEXT("On") && FSRFrameGeneration == TEXT("On"))
	{
		FSRFrameGeneration = TEXT("Off");
	}

	// FSR FG with DLSS FG active is invalid.
	if (FrameGeneration == TEXT("On"))
	{
		FSRFrameGeneration = TEXT("Off");
	}
}

