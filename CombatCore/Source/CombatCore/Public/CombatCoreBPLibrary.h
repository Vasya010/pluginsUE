// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Kismet/BlueprintFunctionLibrary.h"
#include "CombatCoreBPLibrary.generated.h"

USTRUCT(BlueprintType)
struct FAttackSequenceSamplingData
{
	GENERATED_BODY()

/** Bone name for sampling */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sampling")
	FName BoneName = NAME_None;

/*TimeRangeOffsetMin - Specifies the offset relative to the sampling center for the beginning of the trajectory.
When, for CombatAnimSet, this refers to AttackingTrajectorySamplingConfig, then
OriginTime = AttackMaxPeakTime + OriginTimeOffset.
The final value is calculated as OriginTime - abs(TimeRangeOffsetMin).*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sampling",
		meta = (ClampMin = "-5.0", ClampMax = "0.0", UIMin = "-5.0", UIMax = "0.0"))
	float TimeRangeOffsetMin = -0.1f;

/*TimeRangeOffsetMax - Specifies the offset relative to the sampling center for the end of the trajectory.
When, for CombatAnimSet, this refers to AttackingTrajectorySamplingConfig, then
OriginTime = AttackMaxPeakTime + OriginTimeOffset.
The final value is calculated as OriginTime + abs(TimeRangeOffsetMax).*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sampling",
		meta = (ClampMin = "0.0", ClampMax = "5.0", UIMin = "0.0", UIMax = "5.0"))
	float TimeRangeOffsetMax = 0.1f;

/*OriginTimeOffset - Specifies an offset on the timeline for the sampling center.*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sampling",
		meta = (ClampMin = "-8.0", ClampMax = "8.0", UIMin = "-8.0", UIMax = "8.0"))
	float OriginTimeOffset = 0.0f;

/*SamplesNumber - This value is taken into account when analyzing and searching for the best matching animation based on the input data.
Additionally, it affects the number of collision checks performed if the bCheckCollision option is enabled.*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sampling",
		meta = (ClampMin = "1", ClampMax = "30", UIMin = "1", UIMax = "30"))
	int32 SamplesNumber = 3;

/*bCheckCollision - Affects the final weight during animation analysis.
If, during weight calculation, a collision occurs, the assets weight will be reduced by the CollisionHitBias value.
The number of collision checks performed for a single asset depends on the SamplesNumber value.*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sampling")
	bool bCheckCollision = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sampling",
		meta = (ClampMin = "2.0", ClampMax = "25.0", UIMin = "2.0", UIMax = "25.0"))
	float CollisionRadius = 10.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sampling",
		meta = (ClampMin = "-10.0", ClampMax = "1.0", UIMin = "-10.0", UIMax = "1.0"))
	float CollisionHitBias = 0.0f;

	int32 DebugColorID = 0;
};

USTRUCT(BlueprintType)
struct FCombatAnimTrajectorySavedData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pre Defined Data")
	FName BoneReference = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pre Defined Data")
	TArray<FTransform> SampledBoneTransforms;
};

UENUM(BlueprintType)
enum class CustomWarpingInterpFunction : uint8
{
	Linear,
	EaseIn,
	EaseOut,
	EaseInOut,
	CircularIn,
	CircularOut,
	CircularInOut,
	ExpoOut,
	Custom
};


UCLASS()
class UCombatCoreBPLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_UCLASS_BODY()

	//UFUNCTION(BlueprintCallable, meta = (DisplayName = "Execute Sample function", Keywords = "CombatCore sample test testing"), Category = "CombatCoreTesting")
	static float CombatCoreSampleFunction(float Param);

};
