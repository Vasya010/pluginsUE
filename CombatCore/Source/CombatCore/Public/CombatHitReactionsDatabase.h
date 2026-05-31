// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"

#if WITH_EDITOR
#include "ForAnimsSimpleMacros.h"
#endif

#include "CombatHitReactionsDatabase.generated.h"

/*
A important database related to the AGLS project. It's used by systems like MeleeCombat. Its purpose is to store 
HitReaction animations. This data is used to determine the best possible HitReaction animation based on damage received.
*/
UCLASS(BlueprintType)
class COMBATCORE_API UCombatHitReactionsDatabase : public UDataAsset
{
	GENERATED_BODY()
	
public:

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Hit Reactions|Assets")
	TArray<UAnimSequenceBase*> HitReactionsAssets;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Hit Reactions|Searcher Config")
	bool bNormalizeData = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Hit Reactions|Searcher Config", meta = (ClampMin = "-4.0", UIMin = "-4.0", ClampMax = "4.0", UIMax = "4.0"))
	float DatabaseBaseConstSearchBias = 0.0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Hit Reactions|Searcher Config", meta = (ClampMin = "-2.0", UIMin = "-2.0", ClampMax = "2.0", UIMax = "2.0"))
	float RandomizationSearchingBias = 0.0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Hit Reactions|Searcher Config", meta = (ClampMin = "0.0", UIMin = "0.0", ClampMax = "1.0", UIMax = "1.0"))
	float DistanceToAngleAlpha = 0.5;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Hit Reactions|Searcher Config", meta = (ClampMin = "0.0", UIMin = "0.0", ClampMax = "2.0", UIMax = "2.0"))
	float EndAnimPositionWeightScale = 0.5;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Hit Reactions|Searcher Config")
	FName ReferenceHitPositionBoneName = TEXT("Attach");

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Hit Reactions|Searcher Config")
	FName RootBoneName = TEXT("Root");

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Hit Reactions|Anims Properties", meta = (ClampMin = "0.5", UIMin = "0.5", ClampMax = "3.0", UIMax = "3.0"))
	float DefaultAnimsPlayRate = 1.0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Hit Reactions|Anims Properties")
	TMap<int, float> OverridePlayRatePerAnim;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Hit Reactions|Anims Properties", meta = (ClampMin = "0.0", UIMin = "0.0", ClampMax = "3.0", UIMax = "3.0"))
	float MaxPossibleAnimStartPosition = 0.0;

	//int = index number from HitReactionsAssets. FTransform = Referene Position.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Hit Reactions|Anims Properties")
	TMap<int, FTransform> ManualHitPositionReferences;


	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Hit Reactions|Anims Read Values")
	TArray<FTransform> RefHitBonePositions;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Hit Reactions|Anims Read Values")
	TArray<FTransform> RootBonePositions;


	UFUNCTION(BlueprintPure, Category = "Hit Reactions|Anims Read Values")
	const TArray<FTransform> GetHitBonePositionPerAnim();

	UFUNCTION(BlueprintPure, Category = "Hit Reactions|Anims Read Values")
	const TArray<FTransform> GetRootBonePositionPerAnim();


#if WITH_EDITOR

	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;

#endif

};
