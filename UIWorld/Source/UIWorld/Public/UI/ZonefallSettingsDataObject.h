#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "ZonefallSettingsDataObject.generated.h"

UENUM(BlueprintType)
enum class EZonefallGraphicsPreset : uint8
{
	Competitive UMETA(DisplayName = "Competitive"),
	Balanced UMETA(DisplayName = "Balanced"),
	Quality UMETA(DisplayName = "Quality"),
	Ultra UMETA(DisplayName = "Ultra"),
	AutoDetect UMETA(DisplayName = "Auto Detect")
};

UCLASS(BlueprintType, Blueprintable)
class ZONEFALL_API UZonefallSettingsDataObject : public UObject
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zonefall|Settings")
	FString DisplayMode;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zonefall|Settings")
	FString OverallQuality;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zonefall|Settings")
	FString ResolutionScale;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zonefall|Settings")
	FString VSync;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zonefall|Settings")
	FString FPSLimit;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zonefall|Settings")
	FString Lumen;

	// "DirectX 12" / "DirectX 11" — applied on next launch (RHI can't switch at runtime).
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zonefall|Settings")
	FString DirectXVersion;

	// "On" / "Off" — hardware ray tracing (RTX). Runtime RT features toggle immediately;
	// fully enabling/disabling RT applies on next launch.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zonefall|Settings")
	FString RayTracing;

	// "Off" / "Low" / "High" / "Epic" — volumetric clouds quality (applies live via CVars).
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zonefall|Settings")
	FString VolumetricClouds;

	// Per-group quality overrides (group name -> "Low"/"Medium"/"High"/"Epic").
	// Groups: ViewDistance, Shadows, GlobalIllumination, Reflections, PostProcess,
	// Textures, Effects, Foliage, Shading, AntiAliasing.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zonefall|Settings|Advanced")
	TMap<FName, FString> AdvancedQuality;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zonefall|Settings")
	FString DLSSMode;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zonefall|Settings")
	FString FrameGeneration;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zonefall|Settings")
	FString FSRMode;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zonefall|Settings")
	FString FSRFrameGeneration;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Zonefall|Settings")
	bool bDLSSSupported;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Zonefall|Settings")
	bool bFrameGenerationSupported;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Zonefall|Settings")
	bool bFSRSupported;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Zonefall|Settings")
	bool bFSRFrameGenerationSupported;

	// Selected screen resolution in format "1920x1080".
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zonefall|Settings")
	FString ScreenResolution;

	// Brightness (gamma), 0.5..2.0. 1.0 == default.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zonefall|Settings", meta = (ClampMin = "0.5", ClampMax = "2.0"))
	float Brightness = 1.0f;

	// Field of view in degrees for the player camera. Game must apply it from a delegate.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zonefall|Settings", meta = (ClampMin = "60.0", ClampMax = "120.0"))
	float FieldOfView = 90.0f;

	// Master / SFX / Music / Voice volumes (0..1).
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zonefall|Settings|Audio", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float MasterVolume = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zonefall|Settings|Audio", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float SfxVolume = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zonefall|Settings|Audio", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float MusicVolume = 0.8f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zonefall|Settings|Audio", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float VoiceVolume = 1.0f;

	UFUNCTION(BlueprintCallable, Category = "Zonefall|Settings")
	void LoadFromSystem();

	UFUNCTION(BlueprintCallable, Category = "Zonefall|Settings")
	void ApplyToSystem(UObject* WorldContextObject);

	// Lightweight apply for display mode + resolution only.
	UFUNCTION(BlueprintCallable, Category = "Zonefall|Settings")
	void ApplyDisplayModeAndResolution(bool bSaveSettings = false);

	UFUNCTION(BlueprintCallable, Category = "Zonefall|Settings")
	void SetDefaults();

	// Fill combo options with available fullscreen resolutions.
	UFUNCTION(BlueprintCallable, Category = "Zonefall|Settings")
	void GetAvailableScreenResolutions(TArray<FString>& OutResolutions, bool bOnly16by9 = false);

	UFUNCTION(BlueprintPure, Category = "Zonefall|Settings")
	FString GetCurrentScreenResolutionString() const;

	// Accepts "WidthxHeight", e.g. "1920x1080".
	UFUNCTION(BlueprintCallable, Category = "Zonefall|Settings")
	bool SetScreenResolutionFromString(const FString& ResolutionString, bool bApplyNow = false);

	// Normalizes values loaded from UI/save before any apply.
	UFUNCTION(BlueprintCallable, Category = "Zonefall|Settings")
	void SanitizeSettings();

	// Applies a tuned profile and keeps values valid for current hardware support.
	UFUNCTION(BlueprintCallable, Category = "Zonefall|Settings")
	void ApplyGraphicsPreset(EZonefallGraphicsPreset Preset);

	// Inspects the current GPU/CPU/RAM and picks the most appropriate preset.
	UFUNCTION(BlueprintCallable, Category = "Zonefall|Settings")
	EZonefallGraphicsPreset DetectRecommendedPreset() const;

	/**
	 * Loads persisted upscaler settings (FSR/DLSS/FrameGen) from per-user config (GameUserSettings.ini).
	 * This exists because upscaler CVars are not guaranteed to be serialized by UGameUserSettings.
	 */
	UFUNCTION(BlueprintCallable, Category = "Zonefall|Settings")
	void LoadUpscalerSettingsFromConfig();

	/** Persists current upscaler settings (FSR/DLSS/FrameGen) to per-user config (GameUserSettings.ini). */
	UFUNCTION(BlueprintCallable, Category = "Zonefall|Settings")
	void SaveUpscalerSettingsToConfig() const;

	/**
	 * Applies ONLY upscaler-related settings (FSR/DLSS/FrameGen) to runtime CVars/libraries.
	 * Safe to call on startup to restore FSR without overriding other game user settings.
	 */
	UFUNCTION(BlueprintCallable, Category = "Zonefall|Settings")
	void ApplyUpscalerSettingsOnly(UObject* WorldContextObject);

private:
	static FString NormalizeDisplayModeValue(const FString& InValue);
	static FString NormalizeQualityValue(const FString& InValue);
	static FString NormalizeResolutionScaleValue(const FString& InValue);
	static FString NormalizeVSyncValue(const FString& InValue);
	static FString NormalizeFPSLimitValue(const FString& InValue);
	static FString NormalizeLumenValue(const FString& InValue);
	static FString NormalizeDLSSValue(const FString& InValue, bool bSupported);
	static FString NormalizeFrameGenerationValue(const FString& InValue, bool bSupported);
	static FString NormalizeFSRValue(const FString& InValue, bool bSupported);
	static FString NormalizeFSRFrameGenerationValue(const FString& InValue, bool bSupported);
	void ResolveUpscalerConflicts();
};

