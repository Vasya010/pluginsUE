#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "ZonefallWeatherTypes.h"
#include "ZonefallWeatherSubsystem.generated.h"

class AZonefallWeatherController;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FZonefallPressureSampleEvent, float, Temperature, float, Humidity);

UCLASS()
class ZONEFALLWEATHER_API UZonefallWeatherSubsystem : public UTickableWorldSubsystem
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintAssignable, Category = "Zonefall Weather|Subsystem")
	FZonefallPressureSampleEvent OnPressureSampled;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zonefall Weather|Pressure")
	int32 InitialPressureSystemCount = 3;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zonefall Weather|Pressure")
	float SpawnAreaHalfExtent = 60000.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zonefall Weather|Pressure", meta = (ClampMin = "30.0"))
	float MinPressureLifeSeconds = 180.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zonefall Weather|Pressure", meta = (ClampMin = "60.0"))
	float MaxPressureLifeSeconds = 600.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zonefall Weather|Pressure")
	int32 PressureRandomSeed = 4242;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zonefall Weather|Pressure")
	bool bAutoSpawnPressureSystems = true;

	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;
	virtual TStatId GetStatId() const override;
	virtual void Tick(float DeltaTime) override;
	virtual bool DoesSupportWorldType(const EWorldType::Type WorldType) const override;

	UFUNCTION(BlueprintCallable, Category = "Zonefall Weather|Subsystem")
	void RegisterController(AZonefallWeatherController* Controller);

	UFUNCTION(BlueprintCallable, Category = "Zonefall Weather|Subsystem")
	void UnregisterController(AZonefallWeatherController* Controller);

	UFUNCTION(BlueprintPure, Category = "Zonefall Weather|Subsystem")
	AZonefallWeatherController* GetActiveController() const;

	UFUNCTION(BlueprintCallable, Category = "Zonefall Weather|Pressure")
	void AddPressureSystem(const FZonefallPressureSystem& System);

	UFUNCTION(BlueprintCallable, Category = "Zonefall Weather|Pressure")
	void ClearPressureSystems();

	UFUNCTION(BlueprintPure, Category = "Zonefall Weather|Pressure")
	void SamplePressureAt(FVector2D WorldLocationXY, float& OutTemperatureBias, float& OutHumidityBias, float& OutStormBoost) const;

	UFUNCTION(BlueprintPure, Category = "Zonefall Weather|Pressure")
	int32 GetActivePressureSystemCount() const;

	UFUNCTION(BlueprintPure, Category = "Zonefall Weather|Pressure")
	TArray<FZonefallPressureSystem> GetPressureSystems() const;

private:
	UPROPERTY(Transient)
	TArray<FZonefallPressureSystem> PressureSystems;

	UPROPERTY(Transient)
	TWeakObjectPtr<AZonefallWeatherController> ActiveController;

	FRandomStream PressureRandom;

	void SpawnInitialPressureSystems();
	FZonefallPressureSystem MakeRandomPressureSystem();
};
