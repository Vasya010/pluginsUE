#pragma once

#include "CoreMinimal.h"
#include "ZonefallWeatherTypes.generated.h"

class UMaterialParameterCollection;

UENUM(BlueprintType)
enum class EZonefallWeatherType : uint8
{
	Clear UMETA(DisplayName = "Clear"),
	Cloudy UMETA(DisplayName = "Cloudy"),
	Overcast UMETA(DisplayName = "Overcast"),
	Foggy UMETA(DisplayName = "Foggy"),
	Rain UMETA(DisplayName = "Rain"),
	Snow UMETA(DisplayName = "Snow"),
	Storm UMETA(DisplayName = "Storm"),
	Custom UMETA(DisplayName = "Custom")
};

UENUM(BlueprintType)
enum class EZonefallWeatherSeason : uint8
{
	Spring UMETA(DisplayName = "Spring"),
	Summer UMETA(DisplayName = "Summer"),
	Autumn UMETA(DisplayName = "Autumn"),
	Winter UMETA(DisplayName = "Winter")
};

USTRUCT(BlueprintType)
struct ZONEFALLWEATHER_API FZonefallCloudMaterialParameters
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Clouds")
	FName CoverageParameterName = TEXT("Coverage");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Clouds")
	FName DensityParameterName = TEXT("Density");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Clouds")
	FName WeatherBlendParameterName = TEXT("WeatherBlend");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Clouds")
	FName WindSpeedParameterName = TEXT("WindSpeed");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Clouds")
	FName WindDirectionParameterName = TEXT("WindDirection");
};

USTRUCT(BlueprintType)
struct ZONEFALLWEATHER_API FZonefallWeatherState
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weather")
	EZonefallWeatherType WeatherType = EZonefallWeatherType::Clear;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lighting", meta = (ClampMin = "0.0"))
	float SunIntensity = 10.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lighting", meta = (ClampMin = "0.0"))
	float MoonIntensity = 0.08f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lighting", meta = (ClampMin = "0.0"))
	float SkyLightIntensity = 1.25f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lighting")
	FLinearColor SunColor = FLinearColor(1.0f, 0.92f, 0.82f, 1.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lighting")
	FLinearColor MoonColor = FLinearColor(0.45f, 0.55f, 1.0f, 1.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lighting")
	FLinearColor SkyLightColor = FLinearColor(0.62f, 0.78f, 1.0f, 1.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fog", meta = (ClampMin = "0.0"))
	float FogDensity = 0.001f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fog", meta = (ClampMin = "0.001"))
	float FogHeightFalloff = 0.25f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fog")
	FLinearColor FogColor = FLinearColor(0.48f, 0.66f, 1.0f, 1.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fog", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float FogMaxOpacity = 0.35f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Clouds", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float CloudCoverage = 0.02f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Clouds", meta = (ClampMin = "0.0"))
	float CloudDensity = 0.02f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Clouds", meta = (ClampMin = "0.0"))
	float CloudLayerBottomAltitude = 2.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Clouds", meta = (ClampMin = "0.1"))
	float CloudLayerHeight = 6.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Clouds", meta = (ClampMin = "0.1"))
	float CloudTracingMaxDistance = 80.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Clouds", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float CloudShadowStrength = 0.02f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wind")
	FVector2D WindDirection = FVector2D(1.0f, 0.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wind", meta = (ClampMin = "0.0"))
	float WindSpeed = 8.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Precipitation", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float RainIntensity = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Precipitation", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float SnowIntensity = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Precipitation", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float LightningIntensity = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Simulation", meta = (ClampMin = "-60.0", ClampMax = "60.0"))
	float TemperatureCelsius = 18.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Simulation", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float Humidity = 0.35f;
};

USTRUCT(BlueprintType)
struct ZONEFALLWEATHER_API FZonefallAutoWeatherSettings
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Auto Weather")
	bool bEnableAutoWeather = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Auto Weather")
	EZonefallWeatherSeason Season = EZonefallWeatherSeason::Summer;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Auto Weather", meta = (ClampMin = "-60.0", ClampMax = "60.0"))
	float BaseTemperatureCelsius = 18.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Auto Weather", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float BaseHumidity = 0.45f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Auto Weather", meta = (ClampMin = "5.0"))
	float MinWeatherDurationSeconds = 90.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Auto Weather", meta = (ClampMin = "5.0"))
	float MaxWeatherDurationSeconds = 240.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Auto Weather", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float StormChance = 0.12f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Auto Weather", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float FogChance = 0.18f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Auto Weather", meta = (ClampMin = "0"))
	int32 RandomSeed = 1337;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Auto Weather", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float NightHumidityBias = 0.12f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Auto Weather", meta = (ClampMin = "-10.0", ClampMax = "10.0"))
	float NightTemperatureBiasCelsius = -6.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Auto Weather", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float DawnFogChance = 0.45f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Auto Weather")
	bool bUsePressureSimulation = true;
};

UENUM(BlueprintType)
enum class EZonefallCloudLayerKind : uint8
{
	Cirrus UMETA(DisplayName = "Cirrus (High, Wispy)"),
	Cumulus UMETA(DisplayName = "Cumulus (Mid, Puffy)"),
	Stratus UMETA(DisplayName = "Stratus (Low, Flat)")
};

USTRUCT(BlueprintType)
struct ZONEFALLWEATHER_API FZonefallCloudLayer
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cloud Layer")
	EZonefallCloudLayerKind Kind = EZonefallCloudLayerKind::Cumulus;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cloud Layer", meta = (ClampMin = "0.0"))
	float Altitude = 4.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cloud Layer", meta = (ClampMin = "0.0"))
	float Height = 2.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cloud Layer", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float Coverage = 0.35f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cloud Layer", meta = (ClampMin = "0.0"))
	float Density = 0.4f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cloud Layer", meta = (ClampMin = "0.001"))
	float NoiseScale = 0.6f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cloud Layer")
	float WindSpeedMultiplier = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cloud Layer", meta = (ClampMin = "0.0"))
	float EvolutionSpeed = 0.1f;

	UPROPERTY(BlueprintReadOnly, Category = "Cloud Layer|Runtime")
	FVector2D AccumulatedOffset = FVector2D::ZeroVector;
};

USTRUCT(BlueprintType)
struct ZONEFALLWEATHER_API FZonefallWindState
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wind")
	FVector2D BaseDirection = FVector2D(1.0f, 0.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wind", meta = (ClampMin = "0.0"))
	float BaseSpeed = 6.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wind", meta = (ClampMin = "0.0"))
	float GustAmplitude = 4.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wind", meta = (ClampMin = "0.0"))
	float GustFrequency = 0.18f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wind", meta = (ClampMin = "0.0"))
	float DirectionDriftSpeed = 0.05f;

	UPROPERTY(BlueprintReadOnly, Category = "Wind|Runtime")
	float CurrentGust = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Wind|Runtime")
	float CurrentSpeed = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Wind|Runtime")
	FVector2D CurrentDirection = FVector2D(1.0f, 0.0f);
};

USTRUCT(BlueprintType)
struct ZONEFALLWEATHER_API FZonefallWetnessSettings
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wetness")
	bool bAutoDriveFromPrecipitation = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wetness", meta = (ClampMin = "0.01"))
	float WetnessBuildUpSpeed = 0.18f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wetness", meta = (ClampMin = "0.005"))
	float WetnessDryingSpeed = 0.05f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wetness", meta = (ClampMin = "0.01"))
	float SnowBuildUpSpeed = 0.025f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wetness", meta = (ClampMin = "0.005"))
	float SnowMeltSpeed = 0.012f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wetness", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float TargetWetness = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Wetness|Runtime")
	float CurrentWetness = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Wetness|Runtime")
	float CurrentSnowAmount = 0.0f;
};

USTRUCT(BlueprintType)
struct ZONEFALLWEATHER_API FZonefallSkyExtras
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sky Extras", meta = (ClampMin = "0.0"))
	float StarBaseIntensity = 1.2f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sky Extras", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float MilkyWayIntensity = 0.55f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sky Extras", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float AuroraIntensity = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sky Extras")
	FLinearColor AuroraColor = FLinearColor(0.18f, 0.95f, 0.55f, 1.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sky Extras", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float DawnDuskWarmth = 0.65f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sky Extras")
	FLinearColor DawnSunColor = FLinearColor(1.0f, 0.62f, 0.38f, 1.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sky Extras")
	FLinearColor DuskSunColor = FLinearColor(1.0f, 0.46f, 0.22f, 1.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sky Extras")
	bool bAuroraOnlyInCold = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sky Extras", meta = (ClampMin = "-30.0", ClampMax = "30.0"))
	float AuroraColdTemperatureCelsius = -2.0f;
};

USTRUCT(BlueprintType)
struct ZONEFALLWEATHER_API FZonefallPressureSystem
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pressure")
	FVector2D Center = FVector2D::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pressure")
	FVector2D Velocity = FVector2D(120.0f, 50.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pressure", meta = (ClampMin = "1000.0"))
	float Radius = 12000.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pressure", meta = (ClampMin = "-1.0", ClampMax = "1.0"))
	float Strength = -0.6f;

	UPROPERTY(BlueprintReadOnly, Category = "Pressure|Runtime")
	float LifeRemaining = 360.0f;
};

USTRUCT(BlueprintType)
struct ZONEFALLWEATHER_API FZonefallForecastEntry
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Forecast")
	EZonefallWeatherType WeatherType = EZonefallWeatherType::Cloudy;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Forecast", meta = (ClampMin = "0.0"))
	float StartInSeconds = 60.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Forecast", meta = (ClampMin = "0.0"))
	float BlendTimeSeconds = 30.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Forecast", meta = (ClampMin = "0.0"))
	float HoldForSeconds = 180.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Forecast")
	FName Label = NAME_None;
};

USTRUCT(BlueprintType)
struct ZONEFALLWEATHER_API FZonefallMPCParameters
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MPC")
	TSoftObjectPtr<UMaterialParameterCollection> MaterialParameterCollection;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MPC")
	FName WetnessParameter = TEXT("Wetness");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MPC")
	FName SnowAmountParameter = TEXT("SnowAmount");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MPC")
	FName WindSpeedParameter = TEXT("WindSpeed");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MPC")
	FName WindDirectionParameter = TEXT("WindDirection");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MPC")
	FName RainIntensityParameter = TEXT("RainIntensity");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MPC")
	FName TimeOfDayParameter = TEXT("TimeOfDay");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MPC")
	FName CloudCoverageParameter = TEXT("CloudCoverage");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MPC")
	FName TemperatureParameter = TEXT("Temperature");
};
