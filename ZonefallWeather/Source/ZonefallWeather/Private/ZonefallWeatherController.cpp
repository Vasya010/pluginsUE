#include "ZonefallWeatherController.h"

#include "Components/DirectionalLightComponent.h"
#include "Components/ExponentialHeightFogComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/SkyAtmosphereComponent.h"
#include "Components/SkyLightComponent.h"
#include "Components/VolumetricCloudComponent.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialParameterCollection.h"
#include "Materials/MaterialParameterCollectionInstance.h"
#include "NiagaraComponent.h"
#include "NiagaraSystem.h"
#include "Components/AudioComponent.h"
#include "ZonefallLightningBolt.h"
#include "ZonefallWeatherPreset.h"
#include "ZonefallWeatherSubsystem.h"

AZonefallWeatherController::AZonefallWeatherController()
{
	PrimaryActorTick.bCanEverTick = true;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);

	SunLight = CreateDefaultSubobject<UDirectionalLightComponent>(TEXT("SunLight"));
	SunLight->SetupAttachment(SceneRoot);
	SunLight->SetMobility(EComponentMobility::Movable);
	SunLight->SetAtmosphereSunLight(true);
	SunLight->SetAtmosphereSunLightIndex(0);
	SunLight->SetForwardShadingPriority(DominantLightForwardPriority);
	SunLight->bCastCloudShadows = true;
	// Bigger, softer sun disc + softer shadows.
	SunLight->LightSourceAngle = 1.2f;

	MoonLight = CreateDefaultSubobject<UDirectionalLightComponent>(TEXT("MoonLight"));
	MoonLight->SetupAttachment(SceneRoot);
	MoonLight->SetMobility(EComponentMobility::Movable);
	MoonLight->SetAtmosphereSunLight(true);
	MoonLight->SetAtmosphereSunLightIndex(1);
	MoonLight->SetForwardShadingPriority(SecondaryLightForwardPriority);
	MoonLight->bCastCloudShadows = false;
	MoonLight->LightSourceAngle = 1.4f;

	SkyAtmosphere = CreateDefaultSubobject<USkyAtmosphereComponent>(TEXT("SkyAtmosphere"));
	SkyAtmosphere->SetupAttachment(SceneRoot);
	SkyAtmosphere->SetMobility(EComponentMobility::Movable);

	SkyLight = CreateDefaultSubobject<USkyLightComponent>(TEXT("SkyLight"));
	SkyLight->SetupAttachment(SceneRoot);
	SkyLight->SetMobility(EComponentMobility::Movable);
	SkyLight->bRealTimeCapture = true;

	HeightFog = CreateDefaultSubobject<UExponentialHeightFogComponent>(TEXT("ExponentialHeightFog"));
	HeightFog->SetupAttachment(SceneRoot);
	HeightFog->SetMobility(EComponentMobility::Movable);
	HeightFog->bEnableVolumetricFog = true;
	// GTA-style realistic volumetric fog: soft forward-scattering haze, sane defaults.
	HeightFog->VolumetricFogScatteringDistribution = 0.55f;   // forward-ish scattering
	HeightFog->VolumetricFogAlbedo = FColor(190, 205, 225);    // cool daylight haze
	HeightFog->VolumetricFogExtinctionScale = 1.0f;
	HeightFog->VolumetricFogDistance = 40000.0f;               // far view depth
	HeightFog->SecondFogData.FogDensity = 0.0f;

	VolumetricClouds = CreateDefaultSubobject<UVolumetricCloudComponent>(TEXT("VolumetricClouds"));
	VolumetricClouds->SetupAttachment(SceneRoot);
	VolumetricClouds->SetMobility(EComponentMobility::Movable);

	RainEffectComponent = CreateDefaultSubobject<UNiagaraComponent>(TEXT("RainEffect"));
	RainEffectComponent->SetupAttachment(SceneRoot);
	RainEffectComponent->SetAutoActivate(false);

	SnowEffectComponent = CreateDefaultSubobject<UNiagaraComponent>(TEXT("SnowEffect"));
	SnowEffectComponent->SetupAttachment(SceneRoot);
	SnowEffectComponent->SetAutoActivate(false);

	LightningFlashLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("LightningFlashLight"));
	LightningFlashLight->SetupAttachment(SceneRoot);
	LightningFlashLight->SetMobility(EComponentMobility::Movable);
	LightningFlashLight->SetIntensity(0.0f);
	LightningFlashLight->SetAttenuationRadius(12000.0f);
	LightningFlashLight->SetLightColor(FLinearColor(0.55f, 0.72f, 1.0f, 1.0f));
	LightningFlashLight->bUseInverseSquaredFalloff = false;

	ThunderAudioComponent = CreateDefaultSubobject<UAudioComponent>(TEXT("ThunderAudio"));
	ThunderAudioComponent->SetupAttachment(SceneRoot);
	ThunderAudioComponent->bAutoActivate = false;

	DefaultWeatherState.WeatherType = EZonefallWeatherType::Clear;
	DefaultWeatherState = MakeClearSkyState();
	CurrentWeatherState = DefaultWeatherState;
	SourceWeatherState = DefaultWeatherState;
	TargetWeatherState = DefaultWeatherState;
}

void AZonefallWeatherController::BeginPlay()
{
	Super::BeginPlay();

	WeatherRandomStream.Initialize(AutoWeatherSettings.RandomSeed);
	ResetAutoWeatherTimer();
	SetupDefaultCloudLayersIfEmpty();
	SetupEffectAssets();
	EnsureMPCInstance();
	WindSettings.CurrentDirection = WindSettings.BaseDirection.GetSafeNormal();
	WindSettings.CurrentSpeed = WindSettings.BaseSpeed;

	CurrentWeatherState = ResolveStartupWeatherState();
	SourceWeatherState = CurrentWeatherState;
	TargetWeatherState = CurrentWeatherState;
	if (InitialWeatherPreset)
	{
		EnsureDynamicCloudMaterial(InitialWeatherPreset->CloudMaterial);
	}

	RegisterWithSubsystem();

	// Apply the configurable sun/moon disc sizes (bigger, AAA-looking sun).
	if (SunLight) { SunLight->LightSourceAngle = SunDiscSizeDegrees; }
	if (MoonLight) { MoonLight->LightSourceAngle = MoonDiscSizeDegrees; }

	ApplyLighting();
	ApplyAtmosphere();
	ApplyFog();
	ApplyClouds();
	ApplyPrecipitationEffects();
	ApplyMPC();
}

void AZonefallWeatherController::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	UnregisterFromSubsystem();
	Super::EndPlay(EndPlayReason);
}

void AZonefallWeatherController::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	// Cheap per-frame state integration.
	AdvanceTime(DeltaSeconds);
	AdvanceForecast(DeltaSeconds);
	AdvanceWeatherBlend(DeltaSeconds);
	AdvanceAutomaticWeather(DeltaSeconds);
	AdvanceWind(DeltaSeconds);
	AdvanceCloudLayers(DeltaSeconds);
	AdvanceWetness(DeltaSeconds);
	AdvanceLightning(DeltaSeconds);

	// Cheap writes (light rotation/intensity, MPC, precipitation follow) at ~20Hz.
	MaterialUpdateAccumulator += DeltaSeconds;
	if (MaterialUpdateAccumulator >= MaterialUpdateInterval)
	{
		MaterialUpdateAccumulator = 0.0f;
		ApplyLighting();
		ApplyPrecipitationEffects();
		ApplyMPC();
	}

	// Heavy render-state writes (atmosphere/fog/cloud component) on a slower cadence, and the
	// Apply* functions below early-out when nothing changed — so MarkRenderStateDirty (which
	// rebuilds those render proxies) almost never fires while the weather is steady.
	HeavyUpdateAccumulator += DeltaSeconds;
	if (HeavyUpdateAccumulator >= HeavyUpdateInterval)
	{
		HeavyUpdateAccumulator = 0.0f;
		ApplyAtmosphere();
		ApplyFog();
		ApplyClouds();
	}
}

#if WITH_EDITOR
void AZonefallWeatherController::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	CurrentWeatherState = ResolveStartupWeatherState();
	ApplyLighting();
	ApplyAtmosphere();
	ApplyFog();
	ApplyClouds();
}
#endif

void AZonefallWeatherController::SetTimeOfDay(float NewTimeOfDayHours)
{
	TimeOfDayHours = FMath::Fmod(FMath::Max(NewTimeOfDayHours, 0.0f), 24.0f);
	OnTimeOfDayChanged.Broadcast(TimeOfDayHours);
}

void AZonefallWeatherController::SetDayNightCycleEnabled(bool bEnabled)
{
	bRunDayNightCycle = bEnabled;
}

void AZonefallWeatherController::ApplyWeatherState(const FZonefallWeatherState& NewState, float BlendTime)
{
	SourceWeatherState = CurrentWeatherState;
	TargetWeatherState = NewState;
	WeatherBlendDuration = FMath::Max(BlendTime, 0.0f);
	WeatherBlendAlpha = WeatherBlendDuration > 0.0f ? 0.0f : 1.0f;

	if (WeatherBlendDuration <= 0.0f)
	{
		CurrentWeatherState = TargetWeatherState;
		OnWeatherChanged.Broadcast(CurrentWeatherState.WeatherType);
	}
}

void AZonefallWeatherController::ApplyWeatherPreset(UZonefallWeatherPreset* Preset, float BlendTime)
{
	if (!Preset)
	{
		return;
	}

	EnsureDynamicCloudMaterial(Preset->CloudMaterial);
	ApplyWeatherState(Preset->State, BlendTime >= 0.0f ? BlendTime : DefaultWeatherBlendTime);
}

void AZonefallWeatherController::ApplyBuiltInWeather(EZonefallWeatherType WeatherType, float BlendTime)
{
	ApplyWeatherState(GetBuiltInWeatherState(WeatherType), BlendTime >= 0.0f ? BlendTime : DefaultWeatherBlendTime);
}

void AZonefallWeatherController::GenerateNextWeather(bool bApplyTransition)
{
	const float SeasonTemperatureBias =
		AutoWeatherSettings.Season == EZonefallWeatherSeason::Winter ? -18.0f :
		AutoWeatherSettings.Season == EZonefallWeatherSeason::Summer ? 7.0f :
		AutoWeatherSettings.Season == EZonefallWeatherSeason::Autumn ? -3.0f : 1.0f;

	// Time-of-day bias — cooler/more humid at night, warmer/drier in afternoon
	const float TimeAlpha = GetNormalizedTimeOfDay();
	const float SunVisibility = GetSunElevationAlpha();
	const float NightAlpha = 1.0f - SunVisibility;
	const float TODTemperatureBias = -AutoWeatherSettings.NightTemperatureBiasCelsius * (SunVisibility - 0.5f) * 2.0f;
	const float TODHumidityBias = AutoWeatherSettings.NightHumidityBias * NightAlpha;

	// Pressure-system bias from subsystem
	float PressureTempBias = 0.0f;
	float PressureHumidityBias = 0.0f;
	float PressureStormBoost = 0.0f;
	if (AutoWeatherSettings.bUsePressureSimulation)
	{
		if (UWorld* World = GetWorld())
		{
			if (UZonefallWeatherSubsystem* Sub = World->GetSubsystem<UZonefallWeatherSubsystem>())
			{
				const FVector Loc = GetActorLocation();
				Sub->SamplePressureAt(FVector2D(Loc.X, Loc.Y), PressureTempBias, PressureHumidityBias, PressureStormBoost);
			}
		}
	}

	const float Temperature = AutoWeatherSettings.BaseTemperatureCelsius + SeasonTemperatureBias + TODTemperatureBias + PressureTempBias + WeatherRandomStream.FRandRange(-4.0f, 4.0f);
	const float Humidity = FMath::Clamp(AutoWeatherSettings.BaseHumidity + TODHumidityBias + PressureHumidityBias + WeatherRandomStream.FRandRange(-0.18f, 0.22f), 0.0f, 1.0f);

	EZonefallWeatherType WeatherType = PickRealisticWeatherType(Temperature, Humidity);

	// Dawn/dusk fog bias
	const bool bIsDawn = TimeAlpha > 0.18f && TimeAlpha < 0.32f;
	if (bIsDawn && Humidity > 0.55f && WeatherRandomStream.FRand() < AutoWeatherSettings.DawnFogChance)
	{
		WeatherType = EZonefallWeatherType::Foggy;
	}

	// Pressure boost storm chance
	if (PressureStormBoost > 0.25f && WeatherType == EZonefallWeatherType::Rain && WeatherRandomStream.FRand() < PressureStormBoost)
	{
		WeatherType = EZonefallWeatherType::Storm;
	}

	const FZonefallWeatherState GeneratedState = MakeWeatherState(WeatherType, Temperature, Humidity);

	if (bApplyTransition)
	{
		ApplyWeatherState(GeneratedState, DefaultWeatherBlendTime);
	}
	else
	{
		CurrentWeatherState = GeneratedState;
		SourceWeatherState = GeneratedState;
		TargetWeatherState = GeneratedState;
		WeatherBlendAlpha = 1.0f;
		OnWeatherChanged.Broadcast(CurrentWeatherState.WeatherType);
	}

	ResetAutoWeatherTimer();
}

void AZonefallWeatherController::SetAutoWeatherEnabled(bool bEnabled)
{
	AutoWeatherSettings.bEnableAutoWeather = bEnabled;
	ResetAutoWeatherTimer();
}

FZonefallWeatherState AZonefallWeatherController::GetBuiltInWeatherState(EZonefallWeatherType WeatherType) const
{
	const float Temperature =
		WeatherType == EZonefallWeatherType::Snow ? -4.0f :
		AutoWeatherSettings.Season == EZonefallWeatherSeason::Winter ? 1.0f : 18.0f;
	const float Humidity =
		WeatherType == EZonefallWeatherType::Clear ? 0.25f :
		WeatherType == EZonefallWeatherType::Cloudy ? 0.48f :
		WeatherType == EZonefallWeatherType::Overcast ? 0.72f :
		WeatherType == EZonefallWeatherType::Foggy ? 0.82f :
		WeatherType == EZonefallWeatherType::Rain ? 0.88f :
		WeatherType == EZonefallWeatherType::Snow ? 0.86f :
		WeatherType == EZonefallWeatherType::Storm ? 0.94f : AutoWeatherSettings.BaseHumidity;
	return MakeWeatherState(WeatherType, Temperature, Humidity);
}

FZonefallWeatherState AZonefallWeatherController::GetCurrentWeatherState() const
{
	return CurrentWeatherState;
}

float AZonefallWeatherController::GetNormalizedTimeOfDay() const
{
	return TimeOfDayHours / 24.0f;
}

UMaterialInstanceDynamic* AZonefallWeatherController::GetRainMaterialInstance() const
{
	return DynamicRainMaterial;
}

UMaterialInstanceDynamic* AZonefallWeatherController::GetSnowMaterialInstance() const
{
	return DynamicSnowMaterial;
}

void AZonefallWeatherController::AdvanceTime(float DeltaSeconds)
{
	if (!bRunDayNightCycle || DayLengthMinutes <= 0.0f)
	{
		return;
	}

	const float HoursPerSecond = 24.0f / (DayLengthMinutes * 60.0f);
	SetTimeOfDay(TimeOfDayHours + DeltaSeconds * HoursPerSecond);
}

void AZonefallWeatherController::AdvanceWeatherBlend(float DeltaSeconds)
{
	if (WeatherBlendAlpha >= 1.0f || WeatherBlendDuration <= 0.0f)
	{
		return;
	}

	WeatherBlendAlpha = FMath::Clamp(WeatherBlendAlpha + DeltaSeconds / WeatherBlendDuration, 0.0f, 1.0f);
	CurrentWeatherState = BlendWeatherStates(SourceWeatherState, TargetWeatherState, FMath::InterpEaseInOut(0.0f, 1.0f, WeatherBlendAlpha, 2.0f));
	if (WeatherBlendAlpha >= 1.0f)
	{
		CurrentWeatherState = TargetWeatherState;
		OnWeatherChanged.Broadcast(CurrentWeatherState.WeatherType);
	}
}

void AZonefallWeatherController::AdvanceAutomaticWeather(float DeltaSeconds)
{
	if (!AutoWeatherSettings.bEnableAutoWeather || WeatherBlendAlpha < 1.0f)
	{
		return;
	}

	if (bForecastActive && bForecastSuspendsAutoWeather)
	{
		return;
	}

	AutoWeatherElapsedSeconds += DeltaSeconds;
	if (AutoWeatherElapsedSeconds >= NextAutoWeatherChangeSeconds)
	{
		GenerateNextWeather(true);
	}
}

void AZonefallWeatherController::AdvanceLightning(float DeltaSeconds)
{
	if (LightningRemainingSeconds > 0.0f)
	{
		LightningRemainingSeconds = FMath::Max(0.0f, LightningRemainingSeconds - DeltaSeconds);
		const float FlashAlpha = LightningRemainingSeconds / FMath::Max(LightningFlashDuration, 0.01f);
		LightningFlashLight->SetIntensity(LightningFlashIntensity * FlashAlpha);
		return;
	}

	LightningFlashLight->SetIntensity(0.0f);
	if (CurrentWeatherState.LightningIntensity <= 0.01f || WeatherBlendAlpha < 1.0f)
	{
		return;
	}

	NextLightningSeconds -= DeltaSeconds;
	if (NextLightningSeconds <= 0.0f)
	{
		const float LightningStrength = FMath::Clamp(CurrentWeatherState.LightningIntensity * WeatherRandomStream.FRandRange(0.65f, 1.35f), 0.0f, 1.0f);
		LightningRemainingSeconds = LightningFlashDuration;
		LightningFlashLight->SetIntensity(LightningFlashIntensity * LightningStrength);
		if (ThunderSound)
		{
			ThunderAudioComponent->SetSound(ThunderSound);
			ThunderAudioComponent->Play();
		}

		// Spawn a real bolt actor if class is configured — provides visible discharge + 3D positioned thunder
		SpawnLightningBolt(LightningStrength);

		OnLightningStrike.Broadcast(LightningStrength);
		NextLightningSeconds = WeatherRandomStream.FRandRange(MinLightningIntervalSeconds, MaxLightningIntervalSeconds);
	}
}

void AZonefallWeatherController::ApplyLighting()
{
	const float Time = GetNormalizedTimeOfDay();
	const float SunPitch = 90.0f - Time * 360.0f;
	const float MoonPitch = SunPitch + 180.0f;
	const float SunVisibility = GetSunElevationAlpha();   // 0.5 = horizon, 1 = noon, 0 = midnight
	const float MoonVisibility = 1.0f - SunVisibility;

	// Plateau the daytime brightness: FULL while the sun is comfortably up, only ramping down
	// as it crosses the horizon (real sunset). So midday is never dark, and it only gets dark
	// once the sun actually sets and the moon takes over.
	const float DayBrightness = FMath::SmoothStep(0.42f, 0.60f, SunVisibility);

	if (bAutoPrioritizeSunAndMoon)
	{
		const bool bSunIsDominant = SunVisibility >= MoonVisibility;
		SunLight->SetForwardShadingPriority(bSunIsDominant ? DominantLightForwardPriority : SecondaryLightForwardPriority);
		MoonLight->SetForwardShadingPriority(bSunIsDominant ? SecondaryLightForwardPriority : DominantLightForwardPriority);
	}

	SunLight->SetWorldRotation(FRotator(SunPitch, SunYaw, 0.0f));

	// Dawn/dusk warmth — boost warmth when sun is near horizon
	FLinearColor EffectiveSunColor = CurrentWeatherState.SunColor;
	if (SkyExtras.DawnDuskWarmth > 0.0f)
	{
		const float HorizonProximity = 1.0f - FMath::Abs(SunVisibility * 2.0f - 1.0f);
		const float WarmthAlpha = FMath::Clamp(HorizonProximity * SkyExtras.DawnDuskWarmth, 0.0f, 1.0f);
		const FLinearColor& TargetWarm = (GetNormalizedTimeOfDay() < 0.5f) ? SkyExtras.DawnSunColor : SkyExtras.DuskSunColor;
		EffectiveSunColor = FLinearColor::LerpUsingHSV(CurrentWeatherState.SunColor, TargetWarm, WarmthAlpha);
	}

	SunLight->SetIntensity(CurrentWeatherState.SunIntensity * DayBrightness);
	SunLight->SetLightColor(EffectiveSunColor);
	SunLight->CloudShadowStrength = CurrentWeatherState.CloudShadowStrength;
	SunLight->CloudShadowOnAtmosphereStrength = CurrentWeatherState.CloudShadowStrength;
	SunLight->CloudShadowOnSurfaceStrength = CurrentWeatherState.CloudShadowStrength;

	MoonLight->SetWorldRotation(FRotator(MoonPitch, SunYaw + MoonYawOffset, 0.0f));
	MoonLight->SetIntensity(CurrentWeatherState.MoonIntensity * MoonVisibility);
	MoonLight->SetLightColor(CurrentWeatherState.MoonColor);

	// Night ambient boost via aurora (only when sun is below horizon)
	float NightAmbientBoost = 0.0f;
	if (MoonVisibility > 0.3f && SkyExtras.AuroraIntensity > 0.0f)
	{
		const bool bColdEnough = !SkyExtras.bAuroraOnlyInCold || CurrentWeatherState.TemperatureCelsius <= SkyExtras.AuroraColdTemperatureCelsius;
		if (bColdEnough)
		{
			NightAmbientBoost = SkyExtras.AuroraIntensity * 0.35f * MoonVisibility;
		}
	}

	const FLinearColor SkyLightFinalColor = FLinearColor::LerpUsingHSV(
		CurrentWeatherState.SkyLightColor,
		SkyExtras.AuroraColor,
		FMath::Clamp(NightAmbientBoost * 0.5f, 0.0f, 0.6f));

	SkyLight->SetIntensity(CurrentWeatherState.SkyLightIntensity * FMath::Lerp(0.35f, 1.0f, DayBrightness) + NightAmbientBoost);
	SkyLight->SetLightColor(SkyLightFinalColor);
}

void AZonefallWeatherController::ApplyAtmosphere()
{
	const float MieScatter = FMath::Lerp(0.15f, 2.4f, CurrentWeatherState.CloudCoverage);
	const float MieAbsorb = FMath::Lerp(0.05f, 0.9f, FMath::Clamp(CurrentWeatherState.FogDensity * 35.0f, 0.0f, 1.0f));

	// Skip the (expensive) render-state rebuild when nothing meaningfully changed.
	if (FMath::IsNearlyEqual(MieScatter, LastAppliedMieScatter, 0.002f) &&
		FMath::IsNearlyEqual(MieAbsorb, LastAppliedMieAbsorb, 0.002f))
	{
		return;
	}
	LastAppliedMieScatter = MieScatter;
	LastAppliedMieAbsorb = MieAbsorb;

	SkyAtmosphere->MieScatteringScale = MieScatter;
	SkyAtmosphere->MieAbsorptionScale = MieAbsorb;
	SkyAtmosphere->SkyLuminanceFactor = FLinearColor(0.65f, 0.82f, 1.25f, 1.0f);
	SkyAtmosphere->SkyAndAerialPerspectiveLuminanceFactor = FLinearColor(0.65f, 0.82f, 1.15f, 1.0f);
	SkyAtmosphere->HeightFogContribution = 1.0f;
	SkyAtmosphere->MarkRenderStateDirty();
}

void AZonefallWeatherController::ApplyFog()
{
	const float D = CurrentWeatherState.FogDensity;
	const float F = CurrentWeatherState.FogHeightFalloff;
	const float O = CurrentWeatherState.FogMaxOpacity;
	const FLinearColor C = CurrentWeatherState.FogColor;

	// Only touch the fog component (each Set* rebuilds its render state) when values changed.
	if (FMath::IsNearlyEqual(D, LastAppliedFogDensity, 0.00005f) &&
		FMath::IsNearlyEqual(F, LastAppliedFogFalloff, 0.002f) &&
		FMath::IsNearlyEqual(O, LastAppliedFogOpacity, 0.002f) &&
		C.Equals(LastAppliedFogColor, 0.004f))
	{
		return;
	}
	LastAppliedFogDensity = D;
	LastAppliedFogFalloff = F;
	LastAppliedFogOpacity = O;
	LastAppliedFogColor = C;

	HeightFog->SetFogDensity(D);
	HeightFog->SetFogHeightFalloff(F);
	HeightFog->SetFogInscatteringColor(C);
	HeightFog->SetFogMaxOpacity(O);
	HeightFog->VolumetricFogExtinctionScale = FMath::Lerp(0.5f, 3.0f, FMath::Clamp(D * 50.0f, 0.0f, 1.0f));
}

void AZonefallWeatherController::ApplyClouds()
{
	// Aggregate per-layer values so multi-layer system drives the single volumetric component
	float MergedCoverage = CurrentWeatherState.CloudCoverage;
	float MergedDensity = CurrentWeatherState.CloudDensity;
	float MergedBottomAltitude = CurrentWeatherState.CloudLayerBottomAltitude;
	float MergedHeight = CurrentWeatherState.CloudLayerHeight;
	FVector2D MergedOffset = FVector2D::ZeroVector;

	if (CloudLayers.Num() > 0)
	{
		float SumCoverage = 0.0f;
		float SumDensity = 0.0f;
		float SumAltitude = 0.0f;
		float SumHeight = 0.0f;
		FVector2D SumOffset = FVector2D::ZeroVector;
		float Weight = 0.0f;

		for (const FZonefallCloudLayer& Layer : CloudLayers)
		{
			// Weight middle layers (Cumulus) more — they're the most visible
			const float LayerWeight = Layer.Kind == EZonefallCloudLayerKind::Cumulus ? 1.6f : 1.0f;
			SumCoverage += Layer.Coverage * LayerWeight;
			SumDensity += Layer.Density * LayerWeight;
			SumAltitude += Layer.Altitude * LayerWeight;
			SumHeight += Layer.Height * LayerWeight;
			SumOffset += Layer.AccumulatedOffset * LayerWeight;
			Weight += LayerWeight;
		}

		if (Weight > 0.0f)
		{
			const float ProceduralCoverage = FMath::Clamp(SumCoverage / Weight, 0.0f, 1.0f);
			MergedCoverage = FMath::Max(CurrentWeatherState.CloudCoverage, ProceduralCoverage);
			MergedDensity = FMath::Max(CurrentWeatherState.CloudDensity, SumDensity / Weight);
			MergedBottomAltitude = SumAltitude / Weight;
			MergedHeight = FMath::Max(0.5f, SumHeight / Weight);
			MergedOffset = SumOffset / Weight;
		}
	}

	const bool bCloudsVisible = MergedCoverage > 0.05f && MergedDensity > 0.04f;

	// The VolumetricCloud component Set* calls rebuild render state — only touch them when the
	// cloud shape actually changed. (Cloud DRIFT goes through the material params below, which
	// are cheap, so clouds still move smoothly without rebuilding the component every frame.)
	const bool bCloudShapeChanged =
		bCloudsVisible != bLastCloudsVisible ||
		!FMath::IsNearlyEqual(MergedCoverage, LastAppliedCloudCoverage, 0.004f) ||
		!FMath::IsNearlyEqual(MergedDensity, LastAppliedCloudDensity, 0.004f) ||
		!FMath::IsNearlyEqual(MergedBottomAltitude, LastAppliedCloudAltitude, 0.05f) ||
		!FMath::IsNearlyEqual(MergedHeight, LastAppliedCloudHeight, 0.05f);

	if (bCloudShapeChanged)
	{
		bLastCloudsVisible = bCloudsVisible;
		LastAppliedCloudCoverage = MergedCoverage;
		LastAppliedCloudDensity = MergedDensity;
		LastAppliedCloudAltitude = MergedBottomAltitude;
		LastAppliedCloudHeight = MergedHeight;

		VolumetricClouds->SetVisibility(bCloudsVisible, true);
		VolumetricClouds->SetHiddenInGame(!bCloudsVisible);
		VolumetricClouds->SetLayerBottomAltitude(MergedBottomAltitude);
		VolumetricClouds->SetLayerHeight(MergedHeight);
		VolumetricClouds->SetTracingMaxDistance(CurrentWeatherState.CloudTracingMaxDistance);
		VolumetricClouds->SetSkyLightCloudBottomOcclusion(MergedCoverage);
		VolumetricClouds->SetGroundAlbedo(CurrentWeatherState.SkyLightColor.ToFColor(true));
		VolumetricClouds->SetShadowViewSampleCountScale(FMath::Lerp(0.5f, 2.0f, MergedCoverage));
	}

	if (DynamicCloudMaterial)
	{
		DynamicCloudMaterial->SetScalarParameterValue(CloudMaterialParameters.CoverageParameterName, MergedCoverage);
		DynamicCloudMaterial->SetScalarParameterValue(CloudMaterialParameters.DensityParameterName, MergedDensity);
		DynamicCloudMaterial->SetScalarParameterValue(CloudMaterialParameters.WeatherBlendParameterName, static_cast<float>(CurrentWeatherState.WeatherType));
		DynamicCloudMaterial->SetScalarParameterValue(CloudMaterialParameters.WindSpeedParameterName, WindSettings.CurrentSpeed);
		DynamicCloudMaterial->SetVectorParameterValue(CloudMaterialParameters.WindDirectionParameterName,
			FLinearColor(WindSettings.CurrentDirection.X, WindSettings.CurrentDirection.Y, 0.0f, 0.0f));

		// Multi-layer params (engine material can read them if it exposes them)
		DynamicCloudMaterial->SetVectorParameterValue(TEXT("LayerOffset"), FLinearColor(MergedOffset.X, MergedOffset.Y, 0.0f, 0.0f));
		for (int32 Index = 0; Index < CloudLayers.Num() && Index < 4; ++Index)
		{
			const FZonefallCloudLayer& Layer = CloudLayers[Index];
			const FString CoverageName = FString::Printf(TEXT("Layer%d_Coverage"), Index);
			const FString DensityName = FString::Printf(TEXT("Layer%d_Density"), Index);
			const FString ScaleName = FString::Printf(TEXT("Layer%d_NoiseScale"), Index);
			const FString OffsetName = FString::Printf(TEXT("Layer%d_Offset"), Index);
			DynamicCloudMaterial->SetScalarParameterValue(FName(*CoverageName), Layer.Coverage);
			DynamicCloudMaterial->SetScalarParameterValue(FName(*DensityName), Layer.Density);
			DynamicCloudMaterial->SetScalarParameterValue(FName(*ScaleName), Layer.NoiseScale);
			DynamicCloudMaterial->SetVectorParameterValue(FName(*OffsetName), FLinearColor(Layer.AccumulatedOffset.X, Layer.AccumulatedOffset.Y, 0.0f, 0.0f));
		}
	}
}

void AZonefallWeatherController::ApplyPrecipitationEffects()
{
	const float RainIntensity = FMath::Clamp(CurrentWeatherState.RainIntensity, 0.0f, 1.0f);
	const float SnowIntensity = FMath::Clamp(CurrentWeatherState.SnowIntensity, 0.0f, 1.0f);

	FVector AnchorLocation = GetActorLocation();
	if (bPrecipitationFollowsCamera)
	{
		const FVector Observer = GetObserverLocation();
		const FVector WindOffset = GetCurrentWind3D() * 0.01f * PrecipitationCameraLeadDistance / FMath::Max(WindSettings.CurrentSpeed, 1.0f);
		AnchorLocation = Observer + WindOffset + FVector(0, 0, PrecipitationFollowHeight);
	}

	RainEffectComponent->SetWorldLocation(AnchorLocation);
	SnowEffectComponent->SetWorldLocation(AnchorLocation);

	const float WindSpeedForFX = WindSettings.CurrentSpeed;

	RainEffectComponent->SetFloatParameter(TEXT("RainIntensity"), RainIntensity);
	RainEffectComponent->SetFloatParameter(TEXT("WindSpeed"), WindSpeedForFX);
	RainEffectComponent->SetVariableFloat(TEXT("User.RainIntensity"), RainIntensity);
	RainEffectComponent->SetVariableFloat(TEXT("User.WindSpeed"), WindSpeedForFX);
	RainEffectComponent->SetVariableVec3(TEXT("User.WindDirection"), GetCurrentWind3D().GetSafeNormal());
	if (DynamicRainMaterial)
	{
		DynamicRainMaterial->SetScalarParameterValue(TEXT("Intensity"), RainIntensity);
		DynamicRainMaterial->SetScalarParameterValue(TEXT("WindSpeed"), WindSpeedForFX);
		DynamicRainMaterial->SetScalarParameterValue(TEXT("Temperature"), CurrentWeatherState.TemperatureCelsius);
		RainEffectComponent->SetVariableMaterial(TEXT("User.RainMaterial"), DynamicRainMaterial);
	}
	RainIntensity > 0.01f ? RainEffectComponent->Activate() : RainEffectComponent->Deactivate();

	SnowEffectComponent->SetFloatParameter(TEXT("SnowIntensity"), SnowIntensity);
	SnowEffectComponent->SetFloatParameter(TEXT("WindSpeed"), WindSpeedForFX);
	SnowEffectComponent->SetVariableFloat(TEXT("User.SnowIntensity"), SnowIntensity);
	SnowEffectComponent->SetVariableFloat(TEXT("User.WindSpeed"), WindSpeedForFX);
	SnowEffectComponent->SetVariableVec3(TEXT("User.WindDirection"), GetCurrentWind3D().GetSafeNormal());
	if (DynamicSnowMaterial)
	{
		DynamicSnowMaterial->SetScalarParameterValue(TEXT("Intensity"), SnowIntensity);
		DynamicSnowMaterial->SetScalarParameterValue(TEXT("WindSpeed"), WindSpeedForFX);
		DynamicSnowMaterial->SetScalarParameterValue(TEXT("Temperature"), CurrentWeatherState.TemperatureCelsius);
		SnowEffectComponent->SetVariableMaterial(TEXT("User.SnowMaterial"), DynamicSnowMaterial);
	}
	SnowIntensity > 0.01f ? SnowEffectComponent->Activate() : SnowEffectComponent->Deactivate();

	OnPrecipitationChanged.Broadcast(RainIntensity, SnowIntensity);
}

FZonefallWeatherState AZonefallWeatherController::MakeClearSkyState() const
{
	FZonefallWeatherState ClearState;
	ClearState.WeatherType = EZonefallWeatherType::Clear;
	ClearState.SunIntensity = 13.0f;       // brighter, fuller daylight
	ClearState.MoonIntensity = 0.05f;
	ClearState.SkyLightIntensity = 1.6f;
	ClearState.SunColor = FLinearColor(1.0f, 0.95f, 0.84f, 1.0f);
	ClearState.MoonColor = FLinearColor(0.45f, 0.55f, 1.0f, 1.0f);
	ClearState.SkyLightColor = FLinearColor(0.62f, 0.78f, 1.0f, 1.0f);
	ClearState.FogDensity = 0.001f;
	ClearState.FogHeightFalloff = 0.35f;
	ClearState.FogColor = FLinearColor(0.48f, 0.66f, 1.0f, 1.0f);
	ClearState.FogMaxOpacity = 0.35f;
	ClearState.CloudCoverage = 0.02f;
	ClearState.CloudDensity = 0.02f;
	ClearState.CloudLayerBottomAltitude = 4.0f;
	ClearState.CloudLayerHeight = 3.0f;
	ClearState.CloudTracingMaxDistance = 60.0f;
	ClearState.CloudShadowStrength = 0.02f;
	ClearState.WindDirection = FVector2D(1.0f, 0.0f);
	ClearState.WindSpeed = 4.0f;
	ClearState.RainIntensity = 0.0f;
	ClearState.SnowIntensity = 0.0f;
	ClearState.LightningIntensity = 0.0f;
	ClearState.TemperatureCelsius = 22.0f;
	ClearState.Humidity = 0.25f;
	return ClearState;
}

FZonefallWeatherState AZonefallWeatherController::ResolveStartupWeatherState() const
{
	if (InitialWeatherPreset)
	{
		return InitialWeatherPreset->State;
	}

	if (bUseBuiltInStartupWeather)
	{
		return GetBuiltInWeatherState(StartupWeatherType);
	}

	if (bForceClearSkyWhenNoPreset && DefaultWeatherState.WeatherType == EZonefallWeatherType::Clear)
	{
		return MakeClearSkyState();
	}

	return DefaultWeatherState;
}

FZonefallWeatherState AZonefallWeatherController::MakeWeatherState(EZonefallWeatherType WeatherType, float TemperatureCelsius, float Humidity) const
{
	FZonefallWeatherState State = MakeClearSkyState();
	State.WeatherType = WeatherType;
	State.TemperatureCelsius = TemperatureCelsius;
	State.Humidity = Humidity;

	switch (WeatherType)
	{
	case EZonefallWeatherType::Cloudy:
		State.SunIntensity = 7.0f;
		State.SkyLightIntensity = 1.1f;
		State.FogDensity = 0.003f;
		State.FogMaxOpacity = 0.45f;
		State.CloudCoverage = 0.45f;
		State.CloudDensity = 0.38f;
		State.CloudShadowStrength = 0.25f;
		State.WindSpeed = 8.0f;
		break;
	case EZonefallWeatherType::Overcast:
		State.SunIntensity = 3.2f;
		State.SkyLightIntensity = 0.85f;
		State.SunColor = FLinearColor(0.82f, 0.86f, 0.9f, 1.0f);
		State.SkyLightColor = FLinearColor(0.48f, 0.55f, 0.68f, 1.0f);
		State.FogDensity = 0.008f;
		State.FogColor = FLinearColor(0.45f, 0.5f, 0.6f, 1.0f);
		State.FogMaxOpacity = 0.75f;
		State.CloudCoverage = 0.88f;
		State.CloudDensity = 0.78f;
		State.CloudShadowStrength = 0.62f;
		State.WindSpeed = 12.0f;
		break;
	case EZonefallWeatherType::Foggy:
		State.SunIntensity = 4.0f;
		State.SkyLightIntensity = 0.8f;
		State.FogDensity = 0.045f;
		State.FogHeightFalloff = 0.12f;
		State.FogColor = FLinearColor(0.62f, 0.68f, 0.74f, 1.0f);
		State.FogMaxOpacity = 0.95f;
		State.CloudCoverage = 0.55f;
		State.CloudDensity = 0.45f;
		State.CloudShadowStrength = 0.35f;
		State.WindSpeed = 3.0f;
		break;
	case EZonefallWeatherType::Rain:
		State.SunIntensity = 2.2f;
		State.SkyLightIntensity = 0.65f;
		State.SunColor = FLinearColor(0.68f, 0.72f, 0.8f, 1.0f);
		State.SkyLightColor = FLinearColor(0.34f, 0.4f, 0.52f, 1.0f);
		State.FogDensity = 0.018f;
		State.FogColor = FLinearColor(0.33f, 0.39f, 0.48f, 1.0f);
		State.FogMaxOpacity = 0.88f;
		State.CloudCoverage = 0.94f;
		State.CloudDensity = 0.9f;
		State.CloudShadowStrength = 0.78f;
		State.WindSpeed = 16.0f;
		State.RainIntensity = 0.72f;
		break;
	case EZonefallWeatherType::Snow:
		State.SunIntensity = 2.8f;
		State.SkyLightIntensity = 1.05f;
		State.SunColor = FLinearColor(0.78f, 0.86f, 1.0f, 1.0f);
		State.SkyLightColor = FLinearColor(0.72f, 0.82f, 1.0f, 1.0f);
		State.FogDensity = 0.018f;
		State.FogColor = FLinearColor(0.74f, 0.82f, 0.92f, 1.0f);
		State.FogMaxOpacity = 0.82f;
		State.CloudCoverage = 0.86f;
		State.CloudDensity = 0.72f;
		State.CloudShadowStrength = 0.45f;
		State.WindSpeed = 9.0f;
		State.SnowIntensity = 0.75f;
		break;
	case EZonefallWeatherType::Storm:
		State.SunIntensity = 1.0f;
		State.SkyLightIntensity = 0.42f;
		State.SunColor = FLinearColor(0.45f, 0.5f, 0.62f, 1.0f);
		State.SkyLightColor = FLinearColor(0.2f, 0.24f, 0.34f, 1.0f);
		State.FogDensity = 0.03f;
		State.FogColor = FLinearColor(0.18f, 0.22f, 0.3f, 1.0f);
		State.FogMaxOpacity = 0.96f;
		State.CloudCoverage = 1.0f;
		State.CloudDensity = 1.0f;
		State.CloudShadowStrength = 0.95f;
		State.WindSpeed = 28.0f;
		State.RainIntensity = 1.0f;
		State.LightningIntensity = 0.85f;
		break;
	case EZonefallWeatherType::Clear:
	case EZonefallWeatherType::Custom:
	default:
		break;
	}

	State.CloudCoverage = FMath::Clamp(State.CloudCoverage + (Humidity - 0.5f) * 0.18f, 0.0f, 1.0f);
	State.FogDensity = FMath::Clamp(State.FogDensity + FMath::Max(0.0f, Humidity - 0.7f) * 0.015f, 0.0f, 0.08f);
	return State;
}

EZonefallWeatherType AZonefallWeatherController::PickRealisticWeatherType(float TemperatureCelsius, float Humidity)
{
	if (TemperatureCelsius <= 1.0f && Humidity > 0.55f)
	{
		return EZonefallWeatherType::Snow;
	}

	if (Humidity > 0.82f && WeatherRandomStream.FRand() < AutoWeatherSettings.StormChance)
	{
		return EZonefallWeatherType::Storm;
	}

	if (Humidity > 0.78f)
	{
		return EZonefallWeatherType::Rain;
	}

	if (Humidity > 0.68f && WeatherRandomStream.FRand() < AutoWeatherSettings.FogChance)
	{
		return EZonefallWeatherType::Foggy;
	}

	if (Humidity > 0.62f)
	{
		return EZonefallWeatherType::Overcast;
	}

	if (Humidity > 0.38f)
	{
		return EZonefallWeatherType::Cloudy;
	}

	return EZonefallWeatherType::Clear;
}

void AZonefallWeatherController::ResetAutoWeatherTimer()
{
	AutoWeatherElapsedSeconds = 0.0f;
	const float MinDuration = FMath::Max(5.0f, AutoWeatherSettings.MinWeatherDurationSeconds);
	const float MaxDuration = FMath::Max(MinDuration, AutoWeatherSettings.MaxWeatherDurationSeconds);
	NextAutoWeatherChangeSeconds = WeatherRandomStream.FRandRange(MinDuration, MaxDuration);
	NextLightningSeconds = WeatherRandomStream.FRandRange(MinLightningIntervalSeconds, MaxLightningIntervalSeconds);
}

void AZonefallWeatherController::SetupEffectAssets()
{
	if (RainNiagaraSystem)
	{
		RainEffectComponent->SetAsset(RainNiagaraSystem);
	}
	if (SnowNiagaraSystem)
	{
		SnowEffectComponent->SetAsset(SnowNiagaraSystem);
	}
	if (RainMaterial)
	{
		DynamicRainMaterial = UMaterialInstanceDynamic::Create(RainMaterial, this);
		RainEffectComponent->SetVariableMaterial(TEXT("User.RainMaterial"), DynamicRainMaterial);
	}
	if (SnowMaterial)
	{
		DynamicSnowMaterial = UMaterialInstanceDynamic::Create(SnowMaterial, this);
		SnowEffectComponent->SetVariableMaterial(TEXT("User.SnowMaterial"), DynamicSnowMaterial);
	}
	if (ThunderSound)
	{
		ThunderAudioComponent->SetSound(ThunderSound);
	}
}

void AZonefallWeatherController::EnsureDynamicCloudMaterial(UMaterialInterface* Material)
{
	if (!Material)
	{
		return;
	}

	DynamicCloudMaterial = UMaterialInstanceDynamic::Create(Material, this);
	VolumetricClouds->SetMaterial(DynamicCloudMaterial);
}

FZonefallWeatherState AZonefallWeatherController::BlendWeatherStates(const FZonefallWeatherState& A, const FZonefallWeatherState& B, float Alpha) const
{
	FZonefallWeatherState Result = B;
	Result.SunIntensity = FMath::Lerp(A.SunIntensity, B.SunIntensity, Alpha);
	Result.MoonIntensity = FMath::Lerp(A.MoonIntensity, B.MoonIntensity, Alpha);
	Result.SkyLightIntensity = FMath::Lerp(A.SkyLightIntensity, B.SkyLightIntensity, Alpha);
	Result.SunColor = FLinearColor::LerpUsingHSV(A.SunColor, B.SunColor, Alpha);
	Result.MoonColor = FLinearColor::LerpUsingHSV(A.MoonColor, B.MoonColor, Alpha);
	Result.SkyLightColor = FLinearColor::LerpUsingHSV(A.SkyLightColor, B.SkyLightColor, Alpha);
	Result.FogDensity = FMath::Lerp(A.FogDensity, B.FogDensity, Alpha);
	Result.FogHeightFalloff = FMath::Lerp(A.FogHeightFalloff, B.FogHeightFalloff, Alpha);
	Result.FogColor = FLinearColor::LerpUsingHSV(A.FogColor, B.FogColor, Alpha);
	Result.FogMaxOpacity = FMath::Lerp(A.FogMaxOpacity, B.FogMaxOpacity, Alpha);
	Result.CloudCoverage = FMath::Lerp(A.CloudCoverage, B.CloudCoverage, Alpha);
	Result.CloudDensity = FMath::Lerp(A.CloudDensity, B.CloudDensity, Alpha);
	Result.CloudLayerBottomAltitude = FMath::Lerp(A.CloudLayerBottomAltitude, B.CloudLayerBottomAltitude, Alpha);
	Result.CloudLayerHeight = FMath::Lerp(A.CloudLayerHeight, B.CloudLayerHeight, Alpha);
	Result.CloudTracingMaxDistance = FMath::Lerp(A.CloudTracingMaxDistance, B.CloudTracingMaxDistance, Alpha);
	Result.CloudShadowStrength = FMath::Lerp(A.CloudShadowStrength, B.CloudShadowStrength, Alpha);
	Result.WindDirection = FMath::Lerp(A.WindDirection, B.WindDirection, Alpha).GetSafeNormal();
	Result.WindSpeed = FMath::Lerp(A.WindSpeed, B.WindSpeed, Alpha);
	Result.RainIntensity = FMath::Lerp(A.RainIntensity, B.RainIntensity, Alpha);
	Result.SnowIntensity = FMath::Lerp(A.SnowIntensity, B.SnowIntensity, Alpha);
	Result.LightningIntensity = FMath::Lerp(A.LightningIntensity, B.LightningIntensity, Alpha);
	Result.TemperatureCelsius = FMath::Lerp(A.TemperatureCelsius, B.TemperatureCelsius, Alpha);
	Result.Humidity = FMath::Lerp(A.Humidity, B.Humidity, Alpha);
	return Result;
}

float AZonefallWeatherController::GetSunElevationAlpha() const
{
	const float Angle = GetNormalizedTimeOfDay() * UE_TWO_PI;
	return FMath::Clamp(FMath::Sin(Angle - HALF_PI) * 0.5f + 0.5f, 0.0f, 1.0f);
}

FVector AZonefallWeatherController::GetObserverLocation() const
{
	if (const UWorld* World = GetWorld())
	{
		if (APlayerController* PC = World->GetFirstPlayerController())
		{
			FVector CamLoc;
			FRotator CamRot;
			PC->GetPlayerViewPoint(CamLoc, CamRot);
			return CamLoc;
		}
	}
	return GetActorLocation();
}

void AZonefallWeatherController::RegisterWithSubsystem()
{
	if (bRegisteredWithSubsystem)
	{
		return;
	}
	if (UWorld* World = GetWorld())
	{
		if (UZonefallWeatherSubsystem* Sub = World->GetSubsystem<UZonefallWeatherSubsystem>())
		{
			Sub->RegisterController(this);
			bRegisteredWithSubsystem = true;
		}
	}
}

void AZonefallWeatherController::UnregisterFromSubsystem()
{
	if (!bRegisteredWithSubsystem)
	{
		return;
	}
	if (UWorld* World = GetWorld())
	{
		if (UZonefallWeatherSubsystem* Sub = World->GetSubsystem<UZonefallWeatherSubsystem>())
		{
			Sub->UnregisterController(this);
		}
	}
	bRegisteredWithSubsystem = false;
}

void AZonefallWeatherController::SetupDefaultCloudLayersIfEmpty()
{
	if (!bAutoPopulateDefaultCloudLayers || CloudLayers.Num() > 0)
	{
		return;
	}

	FZonefallCloudLayer High;
	High.Kind = EZonefallCloudLayerKind::Cirrus;
	High.Altitude = 10.0f;
	High.Height = 1.2f;
	High.Coverage = 0.25f;
	High.Density = 0.18f;
	High.NoiseScale = 0.25f;
	High.WindSpeedMultiplier = 2.4f;
	High.EvolutionSpeed = 0.04f;
	CloudLayers.Add(High);

	FZonefallCloudLayer Mid;
	Mid.Kind = EZonefallCloudLayerKind::Cumulus;
	Mid.Altitude = 4.0f;
	Mid.Height = 2.5f;
	Mid.Coverage = 0.4f;
	Mid.Density = 0.55f;
	Mid.NoiseScale = 0.55f;
	Mid.WindSpeedMultiplier = 1.0f;
	Mid.EvolutionSpeed = 0.12f;
	CloudLayers.Add(Mid);

	FZonefallCloudLayer Low;
	Low.Kind = EZonefallCloudLayerKind::Stratus;
	Low.Altitude = 1.6f;
	Low.Height = 1.0f;
	Low.Coverage = 0.18f;
	Low.Density = 0.45f;
	Low.NoiseScale = 0.85f;
	Low.WindSpeedMultiplier = 0.6f;
	Low.EvolutionSpeed = 0.22f;
	CloudLayers.Add(Low);
}

void AZonefallWeatherController::EnsureMPCInstance()
{
	if (!MaterialParameters.MaterialParameterCollection.IsValid())
	{
		MaterialParameters.MaterialParameterCollection.LoadSynchronous();
	}

	UMaterialParameterCollection* MPC = MaterialParameters.MaterialParameterCollection.Get();
	if (!MPC)
	{
		ResolvedMPCInstance = nullptr;
		return;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	ResolvedMPCInstance = World->GetParameterCollectionInstance(MPC);
}

void AZonefallWeatherController::AdvanceWind(float DeltaSeconds)
{
	WindNoiseTime += DeltaSeconds * FMath::Max(WindSettings.GustFrequency, 0.01f);

	const float NoiseA = FMath::Sin(WindNoiseTime * 1.3f + 0.7f);
	const float NoiseB = FMath::Sin(WindNoiseTime * 2.7f + 2.1f) * 0.6f;
	const float NoiseC = FMath::Sin(WindNoiseTime * 0.31f + 4.3f) * 0.4f;
	const float CombinedGust = FMath::Clamp((NoiseA + NoiseB + NoiseC) * 0.5f + 0.5f, 0.0f, 1.0f);
	WindSettings.CurrentGust = CombinedGust * WindSettings.GustAmplitude;

	const float BaseSpeedFromWeather = CurrentWeatherState.WindSpeed;
	WindSettings.CurrentSpeed = FMath::Max(BaseSpeedFromWeather, WindSettings.BaseSpeed) + WindSettings.CurrentGust;

	const FVector2D BaseDir = WindSettings.BaseDirection.GetSafeNormal();
	const FVector2D WeatherDir = CurrentWeatherState.WindDirection.GetSafeNormal();
	const FVector2D BlendedDir = FMath::Lerp(BaseDir, WeatherDir, 0.65f).GetSafeNormal();

	const float DriftAngle = FMath::Sin(WindNoiseTime * WindSettings.DirectionDriftSpeed) * 0.35f;
	const float CosA = FMath::Cos(DriftAngle);
	const float SinA = FMath::Sin(DriftAngle);
	WindSettings.CurrentDirection = FVector2D(
		BlendedDir.X * CosA - BlendedDir.Y * SinA,
		BlendedDir.X * SinA + BlendedDir.Y * CosA);

	if (WindSettings.CurrentGust > WindSettings.GustAmplitude * 0.75f &&
		FMath::Abs(WindSettings.CurrentGust - LastGustEmitStrength) > WindSettings.GustAmplitude * 0.25f)
	{
		LastGustEmitStrength = WindSettings.CurrentGust;
		OnWindGust.Broadcast(WindSettings.CurrentGust, WindSettings.CurrentDirection);
	}
}

void AZonefallWeatherController::AdvanceCloudLayers(float DeltaSeconds)
{
	for (FZonefallCloudLayer& Layer : CloudLayers)
	{
		const FVector2D LayerWind = WindSettings.CurrentDirection * (WindSettings.CurrentSpeed * Layer.WindSpeedMultiplier);
		Layer.AccumulatedOffset += LayerWind * DeltaSeconds * 0.01f;

		const float Evolution = Layer.EvolutionSpeed * DeltaSeconds;
		Layer.Coverage = FMath::Clamp(Layer.Coverage + FMath::Sin(WindNoiseTime * 0.13f + static_cast<float>(Layer.Kind)) * Evolution * 0.18f, 0.0f, 1.0f);
	}
}

void AZonefallWeatherController::AdvanceWetness(float DeltaSeconds)
{
	if (!WetnessSettings.bAutoDriveFromPrecipitation)
	{
		return;
	}

	const float TargetWet = FMath::Max(WetnessSettings.TargetWetness, CurrentWeatherState.RainIntensity);
	const float WetSpeed = TargetWet > WetnessSettings.CurrentWetness ? WetnessSettings.WetnessBuildUpSpeed : WetnessSettings.WetnessDryingSpeed;
	WetnessSettings.CurrentWetness = FMath::FInterpConstantTo(WetnessSettings.CurrentWetness, TargetWet, DeltaSeconds, WetSpeed);

	const float TargetSnow = CurrentWeatherState.SnowIntensity;
	const float SnowSpeed = TargetSnow > WetnessSettings.CurrentSnowAmount ? WetnessSettings.SnowBuildUpSpeed : WetnessSettings.SnowMeltSpeed;
	WetnessSettings.CurrentSnowAmount = FMath::FInterpConstantTo(WetnessSettings.CurrentSnowAmount, TargetSnow, DeltaSeconds, SnowSpeed);

	OnWetnessChanged.Broadcast(WetnessSettings.CurrentWetness, WetnessSettings.CurrentSnowAmount);
}

void AZonefallWeatherController::AdvanceForecast(float DeltaSeconds)
{
	if (Forecast.Num() == 0 || ForecastIndex >= Forecast.Num())
	{
		bForecastActive = false;
		return;
	}

	bForecastActive = true;

	if (ForecastHoldRemaining > 0.0f)
	{
		ForecastHoldRemaining -= DeltaSeconds;
		if (ForecastHoldRemaining <= 0.0f)
		{
			++ForecastIndex;
			ForecastNextEntryTimer = 0.0f;
			if (ForecastIndex < Forecast.Num())
			{
				ForecastNextEntryTimer = Forecast[ForecastIndex].StartInSeconds;
			}
		}
		return;
	}

	ForecastNextEntryTimer -= DeltaSeconds;
	if (ForecastNextEntryTimer <= 0.0f && ForecastIndex < Forecast.Num())
	{
		const FZonefallForecastEntry& Entry = Forecast[ForecastIndex];
		ApplyBuiltInWeather(Entry.WeatherType, Entry.BlendTimeSeconds);
		ForecastHoldRemaining = FMath::Max(Entry.HoldForSeconds, Entry.BlendTimeSeconds + 1.0f);
		OnForecastAdvanced.Broadcast(Entry);
	}
}

void AZonefallWeatherController::ApplyMPC()
{
	if (!ResolvedMPCInstance)
	{
		return;
	}

	ResolvedMPCInstance->SetScalarParameterValue(MaterialParameters.WetnessParameter, WetnessSettings.CurrentWetness);
	ResolvedMPCInstance->SetScalarParameterValue(MaterialParameters.SnowAmountParameter, WetnessSettings.CurrentSnowAmount);
	ResolvedMPCInstance->SetScalarParameterValue(MaterialParameters.WindSpeedParameter, WindSettings.CurrentSpeed);
	ResolvedMPCInstance->SetScalarParameterValue(MaterialParameters.RainIntensityParameter, CurrentWeatherState.RainIntensity);
	ResolvedMPCInstance->SetScalarParameterValue(MaterialParameters.TimeOfDayParameter, GetNormalizedTimeOfDay());
	ResolvedMPCInstance->SetScalarParameterValue(MaterialParameters.CloudCoverageParameter, CurrentWeatherState.CloudCoverage);
	ResolvedMPCInstance->SetScalarParameterValue(MaterialParameters.TemperatureParameter, CurrentWeatherState.TemperatureCelsius);
	ResolvedMPCInstance->SetVectorParameterValue(MaterialParameters.WindDirectionParameter,
		FLinearColor(WindSettings.CurrentDirection.X, WindSettings.CurrentDirection.Y, 0.0f, 0.0f));
}

void AZonefallWeatherController::QueueForecastEntry(FZonefallForecastEntry Entry)
{
	Forecast.Add(Entry);
}

void AZonefallWeatherController::ClearForecast()
{
	Forecast.Reset();
	ForecastIndex = 0;
	ForecastHoldRemaining = 0.0f;
	bForecastActive = false;
}

FVector AZonefallWeatherController::GetCurrentWind3D() const
{
	return FVector(WindSettings.CurrentDirection.X, WindSettings.CurrentDirection.Y, 0.0f) * WindSettings.CurrentSpeed;
}

float AZonefallWeatherController::GetCurrentWindSpeed() const
{
	return WindSettings.CurrentSpeed;
}

float AZonefallWeatherController::GetCurrentWetness() const
{
	return WetnessSettings.CurrentWetness;
}

float AZonefallWeatherController::GetCurrentSnowAmount() const
{
	return WetnessSettings.CurrentSnowAmount;
}

AZonefallLightningBolt* AZonefallWeatherController::SpawnLightningBolt(float Strength)
{
	if (!bSpawnVisibleLightningBolts || !LightningBoltClass)
	{
		return nullptr;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return nullptr;
	}

	const FVector Observer = GetObserverLocation();
	const float Yaw = WeatherRandomStream.FRandRange(0.0f, UE_TWO_PI);
	const float DistanceXY = WeatherRandomStream.FRandRange(LightningGroundOffsetXY * 0.5f, LightningGroundOffsetXY * 1.5f);
	const FVector Origin = Observer + FVector(FMath::Cos(Yaw) * DistanceXY, FMath::Sin(Yaw) * DistanceXY, LightningSpawnAltitude);
	const FVector Target = Observer + FVector(FMath::Cos(Yaw) * DistanceXY * 1.05f, FMath::Sin(Yaw) * DistanceXY * 1.05f, 0.0f);

	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	Params.Owner = this;

	AZonefallLightningBolt* Bolt = World->SpawnActor<AZonefallLightningBolt>(LightningBoltClass, Origin, FRotator::ZeroRotator, Params);
	if (Bolt)
	{
		Bolt->TriggerBolt(Origin, Target, Strength, Observer);
	}
	return Bolt;
}

void AZonefallWeatherController::SetAuroraIntensity(float NewIntensity)
{
	SkyExtras.AuroraIntensity = FMath::Clamp(NewIntensity, 0.0f, 1.0f);
}
