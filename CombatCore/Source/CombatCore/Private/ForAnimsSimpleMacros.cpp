// Fill out your copyright notice in the Description page of Project Settings.


#include "ForAnimsSimpleMacros.h"

#include "Animation/AnimData/AnimDataModel.h"
#include "Animation/Skeleton.h"
#include "Animation/AnimSequenceDecompressionContext.h"
#include "Animation/AnimCompressionTypes.h"
#include "Animation/AnimTypes.h"


FTransform UForAnimsSimpleMacros::GetBonePositionAtTimeFromSeq(const UAnimSequenceBase* AnimationSequenceBase, double Time, FName BoneName)
{

    FAnimMacros_EvaluationConfig DefaultEvaluationConfig;

    if (AnimationSequenceBase && AnimationSequenceBase->GetSkeleton())
    {
        FMemMark Mark(FMemStack::Get());

        // asset to use for retarget proportions (can be either USkeletalMesh or USkeleton)
        UObject* AssetToUse;
        int32 NumRequiredBones;
        if (DefaultEvaluationConfig.OptionalSkeletalMesh)
        {
            AssetToUse = CastChecked<UObject>(DefaultEvaluationConfig.OptionalSkeletalMesh);
            NumRequiredBones = DefaultEvaluationConfig.OptionalSkeletalMesh->GetRefSkeleton().GetNum();
        }
        else
        {
            AssetToUse = CastChecked<UObject>(AnimationSequenceBase->GetSkeleton());
            NumRequiredBones = AnimationSequenceBase->GetSkeleton()->GetReferenceSkeleton().GetNum();
        }

        const USkeleton* Skeleton = AnimationSequenceBase->GetSkeleton();
        const FReferenceSkeleton& RefSkeleton = Skeleton->GetReferenceSkeleton();

        // Find the index of the bone
        int32 BoneNameToIndex = RefSkeleton.FindBoneIndex(BoneName);
        if (BoneNameToIndex == INDEX_NONE)
        {
            return FTransform::Identity;
        }


        TArray<FBoneIndexType> RequiredBoneIndexArray;
        RequiredBoneIndexArray.AddUninitialized(NumRequiredBones);
        for (int32 BoneIndex = 0; BoneIndex < RequiredBoneIndexArray.Num(); ++BoneIndex)
        {
            RequiredBoneIndexArray[BoneIndex] = static_cast<FBoneIndexType>(BoneIndex);
        }

        FBoneContainer RequiredBones;
        RequiredBones.InitializeTo(RequiredBoneIndexArray, UE::Anim::FCurveFilterSettings(DefaultEvaluationConfig.bEvaluateCurves ? UE::Anim::ECurveFilterMode::None : UE::Anim::ECurveFilterMode::DisallowAll), *AssetToUse);

        RequiredBones.SetUseRAWData(DefaultEvaluationConfig.EvaluationType == EAnimMacros_EvalType::Raw);
        RequiredBones.SetUseSourceData(DefaultEvaluationConfig.EvaluationType == EAnimMacros_EvalType::Source);

        RequiredBones.SetDisableRetargeting(!DefaultEvaluationConfig.bShouldRetarget);

        FCompactPose CompactPose;
        FBlendedCurve Curve;
        UE::Anim::FStackAttributeContainer Attributes;

        FAnimationPoseData PoseData(CompactPose, Curve, Attributes);
        FAnimExtractContext Context(0.0, DefaultEvaluationConfig.bExtractRootMotion);

        FCompactPose BasePose;
        BasePose.SetBoneContainer(&RequiredBones);

        CompactPose.SetBoneContainer(&RequiredBones);
        Curve.InitFrom(RequiredBones);

        const double EvalInterval = Time;
        bool bValidTime = true;

        Context.CurrentTime = EvalInterval;
        Curve.InitFrom(RequiredBones);

        FTransform BoneTransform;

        if (bValidTime)
        {
            if (AnimationSequenceBase->IsValidAdditive())
            {
                CompactPose.ResetToAdditiveIdentity();
                AnimationSequenceBase->GetAnimationPose(PoseData, Context);
                BoneTransform = PoseData.GetPose()[FCompactPoseBoneIndex(BoneNameToIndex)];
            }
            else
            {
                CompactPose.ResetToRefPose();
                AnimationSequenceBase->GetAnimationPose(PoseData, Context);
                BoneTransform = PoseData.GetPose()[FCompactPoseBoneIndex(BoneNameToIndex)];
            }

            if (BoneNameToIndex != 0)
            {
                int32 ParentIndex = RefSkeleton.GetParentIndex(BoneNameToIndex);
                while (ParentIndex != INDEX_NONE)
                {
                    FTransform ParentTransform = PoseData.GetPose()[FCompactPoseBoneIndex(ParentIndex)];;
                    BoneTransform = BoneTransform * ParentTransform;
                    ParentIndex = RefSkeleton.GetParentIndex(ParentIndex);
                }
            }
            return BoneTransform;
        }

    }
    return FTransform::Identity;
}

FTransform UForAnimsSimpleMacros::GetBonePositionAtTimeFromSeqAdvanced(const UAnimSequenceBase* AnimationSequenceBase, double Time, FName BoneName, FAnimMacros_EvaluationConfig EvaluationOptions)
{
    const FAnimMacros_EvaluationConfig DefaultEvaluationConfig = EvaluationOptions;

    if (AnimationSequenceBase && AnimationSequenceBase->GetSkeleton())
    {
        FMemMark Mark(FMemStack::Get());

        // asset to use for retarget proportions (can be either USkeletalMesh or USkeleton)
        UObject* AssetToUse;
        int32 NumRequiredBones;
        if (DefaultEvaluationConfig.OptionalSkeletalMesh)
        {
            AssetToUse = CastChecked<UObject>(DefaultEvaluationConfig.OptionalSkeletalMesh);
            NumRequiredBones = DefaultEvaluationConfig.OptionalSkeletalMesh->GetRefSkeleton().GetNum();
        }
        else
        {
            AssetToUse = CastChecked<UObject>(AnimationSequenceBase->GetSkeleton());
            NumRequiredBones = AnimationSequenceBase->GetSkeleton()->GetReferenceSkeleton().GetNum();
        }

        const USkeleton* Skeleton = AnimationSequenceBase->GetSkeleton();
        const FReferenceSkeleton& RefSkeleton = Skeleton->GetReferenceSkeleton();

        // Find the index of the bone
        int32 BoneNameToIndex = RefSkeleton.FindBoneIndex(BoneName);
        if (BoneNameToIndex == INDEX_NONE)
        {
            return FTransform::Identity;
        }


        TArray<FBoneIndexType> RequiredBoneIndexArray;
        RequiredBoneIndexArray.AddUninitialized(NumRequiredBones);
        for (int32 BoneIndex = 0; BoneIndex < RequiredBoneIndexArray.Num(); ++BoneIndex)
        {
            RequiredBoneIndexArray[BoneIndex] = static_cast<FBoneIndexType>(BoneIndex);
        }

        FBoneContainer RequiredBones;
        RequiredBones.InitializeTo(RequiredBoneIndexArray, UE::Anim::FCurveFilterSettings(DefaultEvaluationConfig.bEvaluateCurves ? UE::Anim::ECurveFilterMode::None : UE::Anim::ECurveFilterMode::DisallowAll), *AssetToUse);

        RequiredBones.SetUseRAWData(DefaultEvaluationConfig.EvaluationType == EAnimMacros_EvalType::Raw);
        RequiredBones.SetUseSourceData(DefaultEvaluationConfig.EvaluationType == EAnimMacros_EvalType::Source);

        RequiredBones.SetDisableRetargeting(!DefaultEvaluationConfig.bShouldRetarget);

        FCompactPose CompactPose;
        FBlendedCurve Curve;
        UE::Anim::FStackAttributeContainer Attributes;

        FAnimationPoseData PoseData(CompactPose, Curve, Attributes);
        FAnimExtractContext Context(0.0, DefaultEvaluationConfig.bExtractRootMotion);

        FCompactPose BasePose;
        BasePose.SetBoneContainer(&RequiredBones);

        CompactPose.SetBoneContainer(&RequiredBones);
        Curve.InitFrom(RequiredBones);

        const double EvalInterval = Time;
        bool bValidTime = true;

        Context.CurrentTime = EvalInterval;
        Curve.InitFrom(RequiredBones);

        FTransform BoneTransform;

        if (bValidTime)
        {
            if (AnimationSequenceBase->IsValidAdditive() && DefaultEvaluationConfig.bCanResetToAdditiveIdentity)
            {
                CompactPose.ResetToAdditiveIdentity();
                AnimationSequenceBase->GetAnimationPose(PoseData, Context);
                BoneTransform = PoseData.GetPose()[FCompactPoseBoneIndex(BoneNameToIndex)];
            }
            else
            {
                CompactPose.ResetToRefPose();
                AnimationSequenceBase->GetAnimationPose(PoseData, Context);
                BoneTransform = PoseData.GetPose()[FCompactPoseBoneIndex(BoneNameToIndex)];
            }

            if (BoneNameToIndex != 0)
            {
                int32 ParentIndex = RefSkeleton.GetParentIndex(BoneNameToIndex);
                while (ParentIndex != INDEX_NONE)
                {
                    FTransform ParentTransform = PoseData.GetPose()[FCompactPoseBoneIndex(ParentIndex)];;
                    BoneTransform = BoneTransform * ParentTransform;
                    ParentIndex = RefSkeleton.GetParentIndex(ParentIndex);
                }
            }
            return BoneTransform;
        }

    }
    return FTransform::Identity;
}


UAnimSequence* UForAnimsSimpleMacros::GetAnimSequenceFromMontage(const UAnimMontage* TargetAnimMontage, int SequenceIndex)
{
    UAnimSequence* AnimSequence = nullptr;
    const UAnimMontage* AnimMontage = TargetAnimMontage;
    if (TargetAnimMontage && AnimMontage->SlotAnimTracks.Num() > 0 && AnimMontage->SlotAnimTracks[0].AnimTrack.AnimSegments.Num() > SequenceIndex)
    {
        AnimSequence = Cast<UAnimSequence>(AnimMontage->SlotAnimTracks[0].AnimTrack.AnimSegments[SequenceIndex].GetAnimReference());
    }
    return AnimSequence;
}