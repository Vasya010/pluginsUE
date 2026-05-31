// Fill out your copyright notice in the Description page of Project Settings.


#include "CombatHitReactionsDatabase.h"

#if WITH_EDITOR

#include "Animation/AnimData/AnimDataModel.h"
#include "Animation/Skeleton.h"
#include "Animation/AnimSequenceDecompressionContext.h"
#include "Animation/AnimCompressionTypes.h"
#include "Animation/AnimTypes.h"

void UCombatHitReactionsDatabase::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
    RefHitBonePositions.Empty();
    RootBonePositions.Empty();

    if (HitReactionsAssets.Num() == 0) return;

    for (int i = 0; i < HitReactionsAssets.Num(); i++)
    {
        UAnimSequenceBase* CurrentAnim = HitReactionsAssets[i];
        if (!CurrentAnim) 
        {
            RefHitBonePositions.Add(FTransform::Identity);
            RootBonePositions.Add(FTransform::Identity);
            continue;
        }
        
        UAnimMontage* AsMontage = Cast<UAnimMontage>(CurrentAnim);
        if (AsMontage)
        {
            CurrentAnim = UForAnimsSimpleMacros::GetAnimSequenceFromMontage(AsMontage, 0);
        }

        if (!CurrentAnim)
        {
            RefHitBonePositions.Add(FTransform::Identity);
            RootBonePositions.Add(FTransform::Identity);
            continue;
        }

        if (ManualHitPositionReferences.Num() > 0)
        {
            if (const FTransform* FindedPos = ManualHitPositionReferences.Find(i))
            {
                RefHitBonePositions.Add(*FindedPos);
            }
            else
            {
                const FTransform BonePos01 = UForAnimsSimpleMacros::GetBonePositionAtTimeFromSeq(CurrentAnim, 0.0, ReferenceHitPositionBoneName);
                RefHitBonePositions.Add(BonePos01);
            }
        }
        else
        {
            const FTransform BonePos01 = UForAnimsSimpleMacros::GetBonePositionAtTimeFromSeq(CurrentAnim, 0.0, ReferenceHitPositionBoneName);
            RefHitBonePositions.Add(BonePos01);
        }

        const FTransform BonePos02 = UForAnimsSimpleMacros::GetBonePositionAtTimeFromSeq(CurrentAnim, CurrentAnim->GetPlayLength(), RootBoneName);
        RootBonePositions.Add(BonePos02);

        
    }
}

//FTransform UCombatHitReactionsDatabase::GetBonePositionAtTimeFromSeq(const UAnimSequenceBase* AnimationSequenceBase, double Time, FName BoneName)
//{
//    if (AnimationSequenceBase && AnimationSequenceBase->GetSkeleton())
//    {
//        FMemMark Mark(FMemStack::Get());
//
//        // asset to use for retarget proportions (can be either USkeletalMesh or USkeleton)
//        UObject* AssetToUse;
//        int32 NumRequiredBones;
//        if (OptionalSkeletalMesh)
//        {
//            AssetToUse = CastChecked<UObject>(OptionalSkeletalMesh);
//            NumRequiredBones = OptionalSkeletalMesh->GetRefSkeleton().GetNum();
//        }
//        else
//        {
//            AssetToUse = CastChecked<UObject>(AnimationSequenceBase->GetSkeleton());
//            NumRequiredBones = AnimationSequenceBase->GetSkeleton()->GetReferenceSkeleton().GetNum();
//        }
//
//        const USkeleton* Skeleton = AnimationSequenceBase->GetSkeleton();
//        const FReferenceSkeleton& RefSkeleton = Skeleton->GetReferenceSkeleton();
//
//        // Find the index of the bone
//        int32 BoneNameToIndex = RefSkeleton.FindBoneIndex(BoneName);
//        if (BoneNameToIndex == INDEX_NONE)
//        {
//            return FTransform::Identity;
//        }
//
//
//        TArray<FBoneIndexType> RequiredBoneIndexArray;
//        RequiredBoneIndexArray.AddUninitialized(NumRequiredBones);
//        for (int32 BoneIndex = 0; BoneIndex < RequiredBoneIndexArray.Num(); ++BoneIndex)
//        {
//            RequiredBoneIndexArray[BoneIndex] = static_cast<FBoneIndexType>(BoneIndex);
//        }
//
//        FBoneContainer RequiredBones;
//        RequiredBones.InitializeTo(RequiredBoneIndexArray, UE::Anim::FCurveFilterSettings(bEvaluateCurves ? UE::Anim::ECurveFilterMode::None : UE::Anim::ECurveFilterMode::DisallowAll), *AssetToUse);
//
//        RequiredBones.SetUseRAWData(EvaluationType == 1);
//        RequiredBones.SetUseSourceData(EvaluationType == 0);
//
//        RequiredBones.SetDisableRetargeting(!bShouldRetarget);
//
//        FCompactPose CompactPose;
//        FBlendedCurve Curve;
//        UE::Anim::FStackAttributeContainer Attributes;
//
//        FAnimationPoseData PoseData(CompactPose, Curve, Attributes);
//        FAnimExtractContext Context(0.0, bExtractRootMotion);
//
//        FCompactPose BasePose;
//        BasePose.SetBoneContainer(&RequiredBones);
//
//        CompactPose.SetBoneContainer(&RequiredBones);
//        Curve.InitFrom(RequiredBones);
//
//        const double EvalInterval = Time;
//        bool bValidTime = true;
//
//        Context.CurrentTime = EvalInterval;
//        Curve.InitFrom(RequiredBones);
//
//        FTransform BoneTransform;
//
//        if (bValidTime)
//        {
//            if (AnimationSequenceBase->IsValidAdditive())
//            {
//                CompactPose.ResetToAdditiveIdentity();
//                AnimationSequenceBase->GetAnimationPose(PoseData, Context);
//                BoneTransform = PoseData.GetPose()[FCompactPoseBoneIndex(BoneNameToIndex)];
//                //GEngine->AddOnScreenDebugMessage(-1, 0, FColor::Cyan, BoneTransform.ToString());
//            }
//            else
//            {
//                CompactPose.ResetToRefPose();
//                AnimationSequenceBase->GetAnimationPose(PoseData, Context);
//                BoneTransform = PoseData.GetPose()[FCompactPoseBoneIndex(BoneNameToIndex)];
//                //GEngine->AddOnScreenDebugMessage(-1, 0, FColor::Red, BoneTransform.ToString());
//            }
//
//            if (BoneNameToIndex != 0)
//            {
//                int32 ParentIndex = RefSkeleton.GetParentIndex(BoneNameToIndex);
//                while (ParentIndex != INDEX_NONE)
//                {
//                    FTransform ParentTransform = PoseData.GetPose()[FCompactPoseBoneIndex(ParentIndex)];;
//                    BoneTransform = BoneTransform * ParentTransform;
//                    ParentIndex = RefSkeleton.GetParentIndex(ParentIndex);
//                }
//            }
//            return BoneTransform;
//        }
//
//    }
//    return FTransform::Identity;
//}
//
//
//UAnimSequence* UCombatHitReactionsDatabase::GetAnimSequenceFromMontage(const UAnimMontage* TargetAnimMontage, int SequenceIndex)
//{
//    UAnimSequence* AnimSequence = nullptr;
//    const UAnimMontage* AnimMontage = TargetAnimMontage;
//    if (TargetAnimMontage && AnimMontage->SlotAnimTracks.Num() > 0 && AnimMontage->SlotAnimTracks[0].AnimTrack.AnimSegments.Num() > SequenceIndex)
//    {
//        AnimSequence = Cast<UAnimSequence>(AnimMontage->SlotAnimTracks[0].AnimTrack.AnimSegments[SequenceIndex].GetAnimReference());
//    }
//    return AnimSequence;
//}

#endif


const TArray<FTransform> UCombatHitReactionsDatabase::GetHitBonePositionPerAnim()
{
    return RefHitBonePositions;
}

const TArray<FTransform> UCombatHitReactionsDatabase::GetRootBonePositionPerAnim()
{
    return RootBonePositions;
}