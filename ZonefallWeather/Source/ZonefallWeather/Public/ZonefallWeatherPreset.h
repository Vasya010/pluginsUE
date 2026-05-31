#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "ZonefallWeatherTypes.h"
#include "ZonefallWeatherPreset.generated.h"

class UMaterialInterface;

UCLASS(BlueprintType)
class ZONEFALLWEATHER_API UZonefallWeatherPreset : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weather")
	FName PresetName = TEXT("WeatherPreset");

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weather")
	FZonefallWeatherState State;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Clouds")
	TObjectPtr<UMaterialInterface> CloudMaterial;
};
