#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ZonefallWeatherTypes.h"
#include "ZonefallWeatherController.generated.h"

class UDirectionalLightComponent;
class UExponentialHeightFogComponent;
class UMaterialInstanceDynamic;
class UMaterialInterface;
class UMaterialParameterCollection;
class UMaterialParameterCollectionInstance;
class UNiagaraComponent;
class UNiagaraSystem;
class UPointLightComponent;
class USoundBase;
class UAudioComponent;
class USceneComponent;
class USkyAtmosphereComponent;
class USkyLightComponent;
class UStaticMesh;
class UVolumetricCloudComponent;
class UZonefallWeatherPreset;
class AZonefallLightningBolt;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FZonefallWeatherTimeEvent, float, TimeOfDayHours);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FZonefallWeatherStateEvent, EZonefallWeatherType, WeatherType);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FZonefallWeatherPrecipitationEvent, float, RainIntensity, float, SnowIntensity);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FZonefallWeatherLightningEvent, float, LightningStrength);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FZonefallWindGustEvent, float, GustStrength, FVector2D, GustDirection);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FZonefallWetnessChangedEvent, float, Wetness, float, SnowAmount);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FZonefallForecastAdvancedEvent, FZonefallForecastEntry, NextEntry);

UCLASS(Blueprintable)
class ZONEFALLWEATHER_API AZonefallWeatherController : public AActor
{
	GENERATED_BODY()

public:
	AZonefallWeatherController();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Zonefall Weather|Components")
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Zonefall Weather|Components")
	TObjectPtr<UDirectionalLightComponent> SunLight;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Zonefall Weather|Components")
	TObjectPtr<UDirectionalLightComponent> MoonLight;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Zonefall Weather|Components")
	TObjectPtr<USkyAtmosphereComponent> SkyAtmosphere;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Zonefall Weather|Components")
	TObjectPtr<USkyLightComponent> SkyLight;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Zonefall Weather|Components")
	TObjectPtr<UExponentialHeightFogComponent> HeightFog;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Zonefall Weather|Components")
	TObjectPtr<UVolumetricCloudComponent> VolumetricClouds;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Zonefall Weather|Components")
	TObjectPtr<UNiagaraComponent> RainEffectComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Zonefall Weather|Components")
	TObjectPtr<UNiagaraComponent> SnowEffectComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Zonefall Weather|Components")
	TObjectPtr<UPointLightComponent> LightningFlashLight;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Zonefall Weather|Components")
	TObjectPtr<UAudioComponent> ThunderAudioComponent;

	UPROPERTY(BlueprintAssignable, Category = "Zonefall Weather|Events")
	FZonefallWeatherTimeEvent OnTimeOfDayChanged;

	UPROPERTY(BlueprintAssignable, Category = "Zonefall Weather|Events")
	FZonefallWeatherStateEvent OnWeatherChanged;

	UPROPERTY(BlueprintAssignable, Category = "Zonefall Weather|Events")
	FZonefallWeatherPrecipitationEvent OnPrecipitationChanged;

	UPROPERTY(BlueprintAssignable, Category = "Zonefall Weather|Events")
	FZonefallWeatherLightningEvent OnLightningStrike;

	UPROPERTY(BlueprintAssignable, Category = "Zonefall Weather|Events")
	FZonefallWindGustEvent OnWindGust;

	UPROPERTY(BlueprintAssignable, Category = "Zonefall Weather|Events")
	FZonefallWetnessChangedEvent OnWetnessChanged;

	UPROPERTY(BlueprintAssignable, Category = "Zonefall Weather|Events")
	FZonefallForecastAdvancedEvent OnForecastAdvanced;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zonefall Weather|Time", meta = (ClampMin = "0.0", ClampMax = "24.0"))
	float TimeOfDayHours = 8.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zonefall Weather|Time", meta = (ClampMin = "0.1"))
	float DayLengthMinutes = 45.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zonefall Weather|Time")
	bool bRunDayNightCycle = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zonefall Weather|Time")
	float SunYaw = -35.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zonefall Weather|Time")
	float MoonYawOffset = 180.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zonefall Weather|Lighting")
	bool bAutoPrioritizeSunAndMoon = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zonefall Weather|Lighting")
	int32 DominantLightForwardPriority = 10;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zonefall Weather|Lighting")
	int32 SecondaryLightForwardPriority = 5;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zonefall Weather|Weather")
	FZonefallWeatherState DefaultWeatherState;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zonefall Weather|Weather")
	TObjectPtr<UZonefallWeatherPreset> InitialWeatherPreset;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zonefall Weather|Weather")
	bool bUseBuiltInStartupWeather = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zonefall Weather|Weather")
	EZonefallWeatherType StartupWeatherType = EZonefallWeatherType::Clear;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zonefall Weather|Weather")
	bool bForceClearSkyWhenNoPreset = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zonefall Weather|Weather")
	FZonefallCloudMaterialParameters CloudMaterialParameters;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zonefall Weather|Weather", meta = (ClampMin = "0.0"))
	float DefaultWeatherBlendTime = 12.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zonefall Weather|Auto Weather")
	FZonefallAutoWeatherSettings AutoWeatherSettings;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zonefall Weather|Clouds")
	TArray<FZonefallCloudLayer> CloudLayers;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zonefall Weather|Clouds")
	bool bAutoPopulateDefaultCloudLayers = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zonefall Weather|Wind")
	FZonefallWindState WindSettings;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zonefall Weather|Wetness")
	FZonefallWetnessSettings WetnessSettings;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zonefall Weather|Sky")
	FZonefallSkyExtras SkyExtras;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zonefall Weather|MPC")
	FZonefallMPCParameters MaterialParameters;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zonefall Weather|Precipitation")
	bool bPrecipitationFollowsCamera = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zonefall Weather|Precipitation")
	float PrecipitationCameraLeadDistance = 600.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zonefall Weather|Lightning")
	TSubclassOf<AZonefallLightningBolt> LightningBoltClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zonefall Weather|Lightning")
	float LightningSpawnAltitude = 18000.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zonefall Weather|Lightning")
	float LightningGroundOffsetXY = 6000.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zonefall Weather|Lightning")
	bool bSpawnVisibleLightningBolts = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zonefall Weather|Forecast")
	TArray<FZonefallForecastEntry> Forecast;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zonefall Weather|Forecast")
	bool bForecastSuspendsAutoWeather = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zonefall Weather|Effects")
	TObjectPtr<UNiagaraSystem> RainNiagaraSystem;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zonefall Weather|Effects")
	TObjectPtr<UNiagaraSystem> SnowNiagaraSystem;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zonefall Weather|Effects")
	TObjectPtr<USoundBase> ThunderSound;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zonefall Weather|Effects")
	TObjectPtr<UMaterialInterface> RainMaterial;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zonefall Weather|Effects")
	TObjectPtr<UMaterialInterface> SnowMaterial;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zonefall Weather|Effects", meta = (ClampMin = "100.0"))
	float PrecipitationFollowHeight = 1200.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zonefall Weather|Effects", meta = (ClampMin = "0.0"))
	float LightningFlashIntensity = 50000.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zonefall Weather|Effects", meta = (ClampMin = "0.02"))
	float LightningFlashDuration = 0.18f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zonefall Weather|Effects", meta = (ClampMin = "1.0"))
	float MinLightningIntervalSeconds = 5.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zonefall Weather|Effects", meta = (ClampMin = "1.0"))
	float MaxLightningIntervalSeconds = 18.0f;

	// Lighting / MPC / precipitation writes (cheap) are throttled to this interval.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zonefall Weather|Performance", meta = (ClampMin = "0.0", ClampMax = "0.5"))
	float MaterialUpdateInterval = 0.05f;

	// Heavy render-state writes (sky atmosphere / fog / cloud component) run on this slower
	// cadence AND only when values actually changed, so MarkRenderStateDirty rarely fires.
	// This is the main FPS fix — rebuilding those proxies every frame is very expensive.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zonefall Weather|Performance", meta = (ClampMin = "0.05", ClampMax = "1.0"))
	float HeavyUpdateInterval = 0.2f;

	// Angular diameter of the sun disk in the sky (degrees). Bigger = larger sun.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zonefall Weather|Sky", meta = (ClampMin = "0.1", ClampMax = "8.0"))
	float SunDiscSizeDegrees = 1.2f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zonefall Weather|Sky", meta = (ClampMin = "0.1", ClampMax = "8.0"))
	float MoonDiscSizeDegrees = 1.4f;

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void Tick(float DeltaSeconds) override;
#if WITH_EDITOR
	virtual void OnConstruction(const FTransform& Transform) override;
#endif

	UFUNCTION(BlueprintCallable, Category = "Zonefall Weather|Time")
	void SetTimeOfDay(float NewTimeOfDayHours);

	UFUNCTION(BlueprintCallable, Category = "Zonefall Weather|Time")
	void SetDayNightCycleEnabled(bool bEnabled);

	UFUNCTION(BlueprintCallable, Category = "Zonefall Weather|Weather")
	void ApplyWeatherState(const FZonefallWeatherState& NewState, float BlendTime = 0.0f);

	UFUNCTION(BlueprintCallable, Category = "Zonefall Weather|Weather")
	void ApplyWeatherPreset(UZonefallWeatherPreset* Preset, float BlendTime = -1.0f);

	UFUNCTION(BlueprintCallable, Category = "Zonefall Weather|Weather")
	void ApplyBuiltInWeather(EZonefallWeatherType WeatherType, float BlendTime = -1.0f);

	UFUNCTION(BlueprintCallable, Category = "Zonefall Weather|Weather")
	void GenerateNextWeather(bool bApplyTransition = true);

	UFUNCTION(BlueprintCallable, Category = "Zonefall Weather|Auto Weather")
	void SetAutoWeatherEnabled(bool bEnabled);

	UFUNCTION(BlueprintPure, Category = "Zonefall Weather|Weather")
	FZonefallWeatherState GetBuiltInWeatherState(EZonefallWeatherType WeatherType) const;

	UFUNCTION(BlueprintPure, Category = "Zonefall Weather|Weather")
	FZonefallWeatherState GetCurrentWeatherState() const;

	UFUNCTION(BlueprintPure, Category = "Zonefall Weather|Time")
	float GetNormalizedTimeOfDay() const;

	UFUNCTION(BlueprintPure, Category = "Zonefall Weather|Effects")
	UMaterialInstanceDynamic* GetRainMaterialInstance() const;

	UFUNCTION(BlueprintPure, Category = "Zonefall Weather|Effects")
	UMaterialInstanceDynamic* GetSnowMaterialInstance() const;

	UFUNCTION(BlueprintCallable, Category = "Zonefall Weather|Forecast")
	void QueueForecastEntry(FZonefallForecastEntry Entry);

	UFUNCTION(BlueprintCallable, Category = "Zonefall Weather|Forecast")
	void ClearForecast();

	UFUNCTION(BlueprintPure, Category = "Zonefall Weather|Wind")
	FVector GetCurrentWind3D() const;

	UFUNCTION(BlueprintPure, Category = "Zonefall Weather|Wind")
	float GetCurrentWindSpeed() const;

	UFUNCTION(BlueprintPure, Category = "Zonefall Weather|Wetness")
	float GetCurrentWetness() const;

	UFUNCTION(BlueprintPure, Category = "Zonefall Weather|Wetness")
	float GetCurrentSnowAmount() const;

	UFUNCTION(BlueprintCallable, Category = "Zonefall Weather|Lightning")
	AZonefallLightningBolt* SpawnLightningBolt(float Strength);

	UFUNCTION(BlueprintCallable, Category = "Zonefall Weather|Sky")
	void SetAuroraIntensity(float NewIntensity);

private:
	UPROPERTY(Transient)
	float MaterialUpdateAccumulator = 0.0f;

	UPROPERTY(Transient)
	float HeavyUpdateAccumulator = 0.0f;

	// Cached last-applied values so heavy render-state writes only fire on real change.
	UPROPERTY(Transient) float LastAppliedFogDensity = -1.0f;
	UPROPERTY(Transient) float LastAppliedFogFalloff = -1.0f;
	UPROPERTY(Transient) float LastAppliedFogOpacity = -1.0f;
	UPROPERTY(Transient) FLinearColor LastAppliedFogColor = FLinearColor(-1, -1, -1, -1);
	UPROPERTY(Transient) float LastAppliedMieScatter = -1.0f;
	UPROPERTY(Transient) float LastAppliedMieAbsorb = -1.0f;
	UPROPERTY(Transient) float LastAppliedCloudCoverage = -1.0f;
	UPROPERTY(Transient) float LastAppliedCloudDensity = -1.0f;
	UPROPERTY(Transient) float LastAppliedCloudAltitude = -1.0f;
	UPROPERTY(Transient) float LastAppliedCloudHeight = -1.0f;
	UPROPERTY(Transient) bool bLastCloudsVisible = true;

	UPROPERTY(Transient)
	FZonefallWeatherState CurrentWeatherState;

	UPROPERTY(Transient)
	FZonefallWeatherState SourceWeatherState;

	UPROPERTY(Transient)
	FZonefallWeatherState TargetWeatherState;

	UPROPERTY(Transient)
	float WeatherBlendAlpha = 1.0f;

	UPROPERTY(Transient)
	float WeatherBlendDuration = 0.0f;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> DynamicCloudMaterial;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> DynamicRainMaterial;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> DynamicSnowMaterial;

	UPROPERTY(Transient)
	float AutoWeatherElapsedSeconds = 0.0f;

	UPROPERTY(Transient)
	float NextAutoWeatherChangeSeconds = 0.0f;

	UPROPERTY(Transient)
	float NextLightningSeconds = 0.0f;

	UPROPERTY(Transient)
	float LightningRemainingSeconds = 0.0f;

	UPROPERTY(Transient)
	FRandomStream WeatherRandomStream;

	UPROPERTY(Transient)
	float WindNoiseTime = 0.0f;

	UPROPERTY(Transient)
	bool bRegisteredWithSubsystem = false;

	UPROPERTY(Transient)
	float ForecastNextEntryTimer = 0.0f;

	UPROPERTY(Transient)
	int32 ForecastIndex = 0;

	UPROPERTY(Transient)
	bool bForecastActive = false;

	UPROPERTY(Transient)
	float ForecastHoldRemaining = 0.0f;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialParameterCollectionInstance> ResolvedMPCInstance;

	UPROPERTY(Transient)
	float LastGustEmitStrength = 0.0f;

	void AdvanceTime(float DeltaSeconds);
	void AdvanceWeatherBlend(float DeltaSeconds);
	void AdvanceAutomaticWeather(float DeltaSeconds);
	void AdvanceLightning(float DeltaSeconds);
	void AdvanceWind(float DeltaSeconds);
	void AdvanceCloudLayers(float DeltaSeconds);
	void AdvanceWetness(float DeltaSeconds);
	void AdvanceForecast(float DeltaSeconds);
	void ApplyLighting();
	void ApplyAtmosphere();
	void ApplyFog();
	void ApplyClouds();
	void ApplyPrecipitationEffects();
	void ApplyMPC();
	FZonefallWeatherState MakeClearSkyState() const;
	FZonefallWeatherState ResolveStartupWeatherState() const;
	FZonefallWeatherState MakeWeatherState(EZonefallWeatherType WeatherType, float TemperatureCelsius, float Humidity) const;
	EZonefallWeatherType PickRealisticWeatherType(float TemperatureCelsius, float Humidity);
	void ResetAutoWeatherTimer();
	void SetupEffectAssets();
	void SetupDefaultCloudLayersIfEmpty();
	void EnsureMPCInstance();
	void EnsureDynamicCloudMaterial(UMaterialInterface* Material);
	FZonefallWeatherState BlendWeatherStates(const FZonefallWeatherState& A, const FZonefallWeatherState& B, float Alpha) const;
	float GetSunElevationAlpha() const;
	FVector GetObserverLocation() const;
	void RegisterWithSubsystem();
	void UnregisterFromSubsystem();
};
