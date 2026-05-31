// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "ForAnimsSimpleMacros.generated.h"

UENUM(BlueprintType)
enum class EAnimMacros_EvalType : uint8
{
    // Evaluates the original Animation Source data 
    Source,
    // Evaluates the original Animation Source data with additive animation layers
    Raw,
    // Evaluates the compressed Animation data - matching runtime (cooked)
    Compressed
};


USTRUCT(BlueprintType)
struct FAnimMacros_EvaluationConfig
{
    GENERATED_BODY()

    // Type of evaluation which should be used
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Evaluation Config")
    EAnimMacros_EvalType EvaluationType = EAnimMacros_EvalType::Raw;

    // Whether or not to retarget animation during evaluation
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Evaluation Config")
    bool bShouldRetarget = true;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Evaluation Config")
    bool bExtractRootMotion = false;

    // Whether or not to force root motion being incorporated into retrieved pose
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Evaluation Config")
    bool bIncorporateRootMotionIntoPose = true;

    // Optional skeletal mesh with proportions to use when evaluating a pose
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Evaluation Config")
    TObjectPtr<USkeletalMesh> OptionalSkeletalMesh = nullptr;

    // Whether or additive animations should be applied to their base-pose
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Evaluation Config")
    bool bRetrieveAdditiveAsFullPose = true;

    // Whether or not to evaluate Animation Curves
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Evaluation Config")
    bool bEvaluateCurves = true;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Evaluation Config")
    bool bCanResetToAdditiveIdentity = true;
};


UCLASS()
class COMBATCORE_API UForAnimsSimpleMacros : public UObject
{
	GENERATED_BODY()

public:

	static FTransform GetBonePositionAtTimeFromSeq(const UAnimSequenceBase* AnimationSequenceBase, double Time, FName BoneName);
    static FTransform GetBonePositionAtTimeFromSeqAdvanced(const UAnimSequenceBase* AnimationSequenceBase, double Time, FName BoneName, FAnimMacros_EvaluationConfig EvaluationOptions);
	static UAnimSequence* GetAnimSequenceFromMontage(const UAnimMontage* TargetAnimMontage, int SequenceIndex);
	
};
