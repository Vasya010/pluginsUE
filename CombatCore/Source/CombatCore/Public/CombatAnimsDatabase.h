

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "CombatAnimSetup.h"
#include "CombatAnimsDatabase.generated.h"

/**
 * 
 */
UCLASS(BlueprintType)
class COMBATCORE_API UCombatAnimsDatabase : public UDataAsset
{
	GENERATED_BODY()

public:

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "CombatDatabase")
	bool bNormalizeData = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "CombatDatabase", meta = (ClampMin = "-2.0", UIMin = "-2.0", ClampMax = "2.0", UIMax = "2.0"))
	float DatabaseBaseConstSearchBias = 0.0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "CombatDatabase", meta = (ClampMin = "-2.0", UIMin = "-2.0", ClampMax = "2.0", UIMax = "2.0"))
	float RandomizationSearchingBias = 0.0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "CombatDatabase", meta = (ClampMin = "-4.0", UIMin = "-4.0", ClampMax = "4.0", UIMax = "4.0"))
	float SameAnimationSearchingBias = 2.0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "CombatDatabase", meta = (ClampMin = "0", UIMin = "0", ClampMax = "5.0", UIMax = "5.0"))
	float ReferencePositionWeightScale = 1.0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "CombatDatabase", meta = (ClampMin = "0", UIMin = "0", ClampMax = "5.0", UIMax = "5.0"))
	float SampledTrajectoriesWeightScale = 1.0;


	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "CombatDatabase")
	TArray<UCombatAnimSetup*> CombatAnimAssets;


	FVector2D NormalizedVictimRefXY = FVector2D(0, 50);
	FVector2D NormalizedVictimRefZ = FVector2D(0, 50);
	FVector NormalizedAttackPeakXY = FVector(0, 0, 0);
	FVector NormalizedAttackPeakZ = FVector(0, 0, 0);

	UFUNCTION(BlueprintPure, Category = "CombatDatabase")
	virtual void GetNormalizedValues(FVector2D& VictimRefXY, FVector2D& VictimRefZ, FVector& AttackPeakXY, FVector& AttackPeakZ);


#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

};
