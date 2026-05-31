// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "ForAnimsSimpleMacros.h"
#include "DyingSequencesDatabase.generated.h"

USTRUCT(BlueprintType)
struct FAnimExtractionResultType
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FName BoneName01 = TEXT("Root");

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<FTransform> ExtractedBonePositions01;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FName BoneName02 = TEXT("none");

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<FTransform> ExtractedBonePositions02;

};



UCLASS(BlueprintType)
class COMBATCORE_API UDyingSequencesDatabase : public UDataAsset
{
	GENERATED_BODY()

public:

	//Animations assets
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dying Sequences|Assets")
	TArray<UAnimSequenceBase*> DyingSequences;

	/*
	Animations assets as Soft References(system using this option only when  DyingSequences.Num() == 0).To correctly 
	extracting bones position assets need to by in memory. When any of asset is not loaded in memory DataAsset was 
	trying to load asset using LoadSynchronous(). Only AnimationsSequences accetable
	*/
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dying Sequences|Assets")
	TArray<TSoftObjectPtr<UAnimSequence>> DyingSequencesAsSoftRef;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dying Sequences|Searcher Config")
	bool bNormalizeData = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dying Sequences|Searcher Config", meta = (ClampMin = "-4.0", UIMin = "-4.0", ClampMax = "4.0", UIMax = "4.0"))
	float DatabaseBaseConstSearchBias = 0.0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dying Sequences|Searcher Config", meta = (ClampMin = "-2.0", UIMin = "-2.0", ClampMax = "2.0", UIMax = "2.0"))
	float RandomizationSearchingBias = 0.0;
	
	//This is optional. When 'NONE' this weight calculation is ignored
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dying Sequences|Searcher Config")
	FName ReferenceHitPositionBoneName = TEXT("none");

	//This is optional. When ReferenceHitPositionBoneName = 'NONE' this weight calculation is ignored
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dying Sequences|Searcher Config", meta = (ClampMin = "0.0", UIMin = "0.0", ClampMax = "5.0", UIMax = "5.0"))
	float HitReferenceWeightScale = 1.0;

	/*
	Hit Reference Angle Lerp bettwen distance to hit. 
	By Default DistanceToHit = MapRange(DistMin, DistMax, 0.0, 1.0, CurrentDistance) * HitReferenceWeightScale

	But when HitReferenceAngleLerp > 0.0 equation is something like this:
	CurrentWeight += Lerp(MapRange(DistMin, DistMax, 0.0, 1.0, CurrentDistance) * HitReferenceWeightScale, Angle[Normalized], HitReferenceAngleLerp)

	When ReferenceHitPositionBoneName = 'NONE' this weight calculation is ignored
	*/
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dying Sequences|Searcher Config", meta = (ClampMin = "0.0", UIMin = "0.0", ClampMax = "1.0", UIMax = "1.0"))
	float HitReferenceAngleLerp = 0.0;

	//Required RootBone name to property extract motion from animations
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dying Sequences|Searcher Config")
	FName RootBoneName = TEXT("Root");

	/*
	ENG:
	A parameter specifying how many iterations should be performed during RootMotion extraction for each animation. This data will 
	later be used by the Searcher function to check whether the current animation will significantly conflict with its surroundings.

	PL:
	Parametr okredlajacy ile iteracji powinno zostac wykonanych podczas extracji RootMotion dla kazdej z animacji. Z tych danych 
	bedzie pozniej kozystac funkcja Szukajaca sprawdzajaca czy obecnia animacja nie bedzie istotnie kolidowac z otoczeniem.
	*/
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dying Sequences|Searcher Config")
	int RootMotionExtractPositionNumber = 3;

	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dying Sequences|Searcher Config")
	bool bCheckColliding = true;


	//Root Motion Extract Time Range NORMALIZED
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dying Sequences|Searcher Config|Collide", meta = (ClampMin = "0.0", UIMin = "0.0", ClampMax = "1.0", UIMax = "1.0", EditCondition = "bCheckColliding"))
	FVector2D RootMotionExtractTimeRange = FVector2D(0.0, 1.0);

	/*
	if (bHitValid && CurrentDatabase->bCheckColliding)
	{
		if (InCharacter->GetCharacterMovement()->IsWalkable(CapsuleResult))
		{
			CurrentWeight += (CurrentDatabase->AnimCollideAddValueToWeight * 0.5);
		}
		else if(Cast<ACharacter*>(CapsuleResult.GetActor()))
		{
			CurrentWeight += CurrentDatabase->AnimCollideAddValueToWeight * CurrentDatabase->WhenHitCharacterCollideValueScale;
		}
		else
		{
			CurrentWeight += CurrentDatabase->AnimCollideAddValueToWeight;
		}
	}
	*/
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dying Sequences|Searcher Config|Collide", meta = (ClampMin = "0.0", UIMin = "0.0", ClampMax = "10.0", UIMax = "10.0", EditCondition = "bCheckColliding"))
	float AnimCollideAddValueToWeight = 1.0;

	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dying Sequences|Searcher Config|Collide", meta = (ClampMin = "0.0", UIMin = "0.0", ClampMax = "2.0", UIMax = "2.0", EditCondition = "bCheckColliding"))
	float WhenHitCharacterCollideValueScale = 1.0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dying Sequences|Searcher Config|Collide", meta = (ClampMin = "2.0", UIMin = "2.0", ClampMax = "40.0", UIMax = "40.0", EditCondition = "bCheckColliding"))
	float CapsuleCollisionRadius = 20.0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dying Sequences|Searcher Config|Collide", meta = (ClampMin = "0.1", UIMin = "0.1", ClampMax = "2.0", UIMax = "2.0", EditCondition = "bCheckColliding"))
	float CapsuleCollisionHalfHeightScale = 1.0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dying Sequences|Searcher Config|Collide", meta = (EditCondition = "bCheckColliding"))
	TArray<TEnumAsByte<EObjectTypeQuery>> TraceObjectTypes;
	

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dying Sequences|Anims Properties", meta = (ClampMin = "0.5", UIMin = "0.5", ClampMax = "3.0", UIMax = "3.0"))
	float DefaultAnimsPlayRate = 1.0;


	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dying Sequences|Debugging")
	bool bDrawDebugInfo = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dying Sequences|Debugging", meta = (EditCondition = "bDrawDebugInfo"));
	FLinearColor ShapesColorA = FLinearColor::Red;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dying Sequences|Debugging", meta = (EditCondition = "bDrawDebugInfo"));
	FLinearColor ShapesColorB = FLinearColor(0.5, 0, 0, 1);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dying Sequences|Debugging", meta = (EditCondition = "bDrawDebugInfo"));
	FLinearColor ShapesColorC = FLinearColor(0.2, 0.5, 0, 1);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dying Sequences|Debugging", meta = (EditCondition = "bDrawDebugInfo"));
	float DebugDrawTime = 2.0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dying Sequences|Debugging", meta = (EditCondition = "bDrawDebugInfo"));
	int DrawShapesDepth = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dying Sequences|Debugging", meta = (EditCondition = "bDrawDebugInfo"));
	bool bDrawCollisionShapes = true;


	//READ ONLY - Saved data from animations
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Dying Sequences|Anims Read Values")
	TMap<int,FAnimExtractionResultType> ExtractedAniamtionsData;



	UFUNCTION(BlueprintPure, Category = "Dying Sequences|Anims Read Values", meta = (Keywords = "Dying,Death,Sequence,Bone"))
	const TArray<FTransform> GetHitBonePositionFromDyingAnim(int AnimIndex);

	UFUNCTION(BlueprintPure, Category = "Dying Sequences|Anims Read Values", meta = (Keywords = "Dying,Death,Sequence,Bone"))
	const TArray<FTransform> GetRootBonePositionFromDyingAnim(int AnimIndex);



	UFUNCTION(BlueprintCallable, Category = "Human AI|Health And Damage|Searchers", meta = (Keywords = "Dying,Death,Sequence,Bone,Search", DisplayName = "Run Dying Sequence Searcher From Databases", AdvancedDisplay = 7))
	static bool RunDyingSequenceSearcher
	(
		UAnimSequenceBase*& ReturnAnimation, 
		float& ReturnPlayRate, 
		float& ReturnStartTime,
		ACharacter* InCharacter,
		const TArray<UDyingSequencesDatabase*>& InDataAssets,
		FTransform InHitTransform,
		FRotator CustomCharacterRot,
		int SearchProperty01, 
		int SearchProperty02, 
		bool bCanDrawDebugs = true
	);


	UFUNCTION(BlueprintCallable, Category = "Human AI|Health And Damage|Searchers", meta = (Keywords = "Dying,Death,Sequence,Bone,Search", DisplayName = "Run Dying Sequence Searcher From Databases SoftRef", AdvancedDisplay = 7))
	static bool RunDyingSequenceSearcherSoftRef
	(
		TSoftObjectPtr<UAnimSequence>& ReturnAnimationRef,
		float& ReturnPlayRate,
		float& ReturnStartTime,
		ACharacter* InCharacter,
		const TArray<UDyingSequencesDatabase*>& InDataAssets,
		FTransform InHitTransform,
		FRotator CustomCharacterRot,
		int SearchProperty01,
		int SearchProperty02,
		bool bCanDrawDebugs = true
	);


#if WITH_EDITOR

	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

};
