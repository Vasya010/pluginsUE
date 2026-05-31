

#include "CombatAnimSetupViewportClient.h"


#include "Components/SkeletalMeshComponent.h"
#include "Animation/AnimSingleNodeInstance.h"
#include "Animation/AnimSequence.h"
#include "Animation/AnimTypes.h" // FAnimExtractContext
#include "Misc/EngineVersionComparison.h"
#include "Animation/AnimInstance.h" 
#include "Animation/AnimData/AnimDataModel.h"
#include "Animation/Skeleton.h"
#include "Animation/AnimSequenceDecompressionContext.h"
#include "Animation/AnimCompressionTypes.h"
#include "SceneView.h"
#include "PrimitiveDrawInterface.h"

#define ANIMASSETSAFE GetAnimationAssetSafe()

FCombatAnimSetupViewportClient::FCombatAnimSetupViewportClient(
    const TSharedRef<FAdvancedPreviewScene>& InPreviewScene,
    const TSharedRef<SEditorViewport>& InViewportWidget)
    : FEditorViewportClient(nullptr, &InPreviewScene.Get(), InViewportWidget)
    , PreviewScene(InPreviewScene)
{
    SetViewportType(LVT_Perspective);
    EngineShowFlags.SetSelectionOutline(false);
    EngineShowFlags.SetGrid(false);
    bSetListenerPosition = false;

    EnsurePreviewComponent();
}

FCombatAnimSetupViewportClient::~FCombatAnimSetupViewportClient()
{
    UnbindFromAsset();
}

void FCombatAnimSetupViewportClient::EnsurePreviewComponent()
{
    if (PreviewSkeletalComp)
    {
        return;
    }

    PreviewSkeletalComp = NewObject<USkeletalMeshComponent>(GetTransientPackage(), NAME_None, RF_Transient);
    PreviewSkeletalComp->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    PreviewSkeletalComp->SetAnimationMode(EAnimationMode::AnimationSingleNode);
    PreviewSkeletalComp->SetVisibility(true);

    PreviewScene->AddComponent(PreviewSkeletalComp, FTransform::Identity);
}


void FCombatAnimSetupViewportClient::ApplyAsset(UCombatAnimSetup* Asset)
{
    EnsurePreviewComponent();

    UnbindFromAsset();

    CurrentAsset = Asset;
    UpdateMeshAndAnimation(Asset);

    BindToAsset();

    Invalidate();
}


float FCombatAnimSetupViewportClient::GetSequenceLength() const
{
    if (!CurrentAsset) return 0.f;

    if (UAnimSequenceBase* Anim = CurrentAsset->Animation.Get())
    {
        return Anim->GetPlayLength();
    }
    return 0.f;
}

float FCombatAnimSetupViewportClient::GetPreviewTime() const
{
    if (!PreviewSkeletalComp) return 0.f;
    if (UAnimSingleNodeInstance* SingleNode = PreviewSkeletalComp->GetSingleNodeInstance())
    {
        return SingleNode->GetCurrentTime();
    }
    return 0.f;
}

void FCombatAnimSetupViewportClient::ResetPreviewTransform()
{
    if (!PreviewSkeletalComp) return;

    PreviewSkeletalComp->SetWorldTransform(FTransform::Identity);

    // jeżeli masz stan do RM (np. LastConsumedRootMotion), wyczyść go tutaj:
    // bHasLastConsumedRM = false;
    // LastConsumedRootMotion = FTransform::Identity;
}

void FCombatAnimSetupViewportClient::SetPlaying(bool bInPlaying)
{
    bIsPlaying = bInPlaying;
    if (PreviewSkeletalComp)
    {
        if (UAnimSingleNodeInstance* SingleNode = PreviewSkeletalComp->GetSingleNodeInstance())
        {
            const float Rate = CurrentAsset ? CurrentAsset->PlayRate : 1.0f;
            SingleNode->SetPlayRate(Rate);
            SingleNode->SetPlaying(ANIMASSETSAFE != nullptr);

            // ✅ ważne: włącz ekstrakcję root motion w instancji
            SingleNode->SetRootMotionMode(ERootMotionMode::RootMotionFromEverything);

        }
    }
}

void FCombatAnimSetupViewportClient::SetCurrentTime(float InTime, bool bFireNotifies)
{
    const float Len = GetSequenceLength();
    if (Len <= 0.f) { CurrentTime = 0.f; return; }

    CurrentTime = FMath::Clamp(InTime, 0.f, Len);

    if (PreviewSkeletalComp)
    {
        if (UAnimSingleNodeInstance* SingleNode = PreviewSkeletalComp->GetSingleNodeInstance())
        {
            SingleNode->SetPosition(CurrentTime, bFireNotifies);

            // ✅ mocniejsze odświeżenie dla edytora
            PreviewSkeletalComp->RefreshBoneTransforms();
            PreviewSkeletalComp->RefreshFollowerComponents();   // zamiast deprecated
            PreviewSkeletalComp->UpdateComponentToWorld();
            PreviewSkeletalComp->MarkRenderTransformDirty();
            PreviewSkeletalComp->MarkRenderDynamicDataDirty();

            // --- ROOT MOTION (preview) ---
#if WITH_EDITORONLY_DATA
            const bool bPreviewRM = CurrentAsset->bPreviewRootMotion;
#else
            const bool bPreviewRM = false;
#endif

            if (bPreviewRM)
            {
                FAnimPoseEvaluationConfig RootEvaluationMode;
                FTransform RootBone = GetBonePositionAtTimeFromSeq(ANIMASSETSAFE, GetPreviewTime(), TEXT("Root"), RootEvaluationMode);
                PreviewSkeletalComp->SetWorldTransform(RootBone);
            }
            else
            {
                PreviewSkeletalComp->SetWorldTransform(FTransform::Identity);
            }
        }
    }

    // ✅ wymuś repaint viewportu
    Invalidate();
}

//HELPERS
static void DrawCube(
    FPrimitiveDrawInterface* PDI,
    const FVector& Origin,
    float HalfSize,
    const FLinearColor& Color,
    uint8 DepthPriority = SDPG_World,
    float Thickness = 1.0f
)
{
    if (!PDI) return;

    const FVector V[8] =
    {
        Origin + FVector(-HalfSize, -HalfSize, -HalfSize),
        Origin + FVector(HalfSize, -HalfSize, -HalfSize),
        Origin + FVector(HalfSize,  HalfSize, -HalfSize),
        Origin + FVector(-HalfSize,  HalfSize, -HalfSize),

        Origin + FVector(-HalfSize, -HalfSize,  HalfSize),
        Origin + FVector(HalfSize, -HalfSize,  HalfSize),
        Origin + FVector(HalfSize,  HalfSize,  HalfSize),
        Origin + FVector(-HalfSize,  HalfSize,  HalfSize),
    };

    auto L = [&](int A, int B)
        {
            PDI->DrawLine(V[A], V[B], Color, DepthPriority, Thickness);
        };

    // dolna
    L(0, 1); L(1, 2); L(2, 3); L(3, 0);
    // górna
    L(4, 5); L(5, 6); L(6, 7); L(7, 4);
    // pionowe
    L(0, 4); L(1, 5); L(2, 6); L(3, 7);
}

static void DrawDiamond(
    FPrimitiveDrawInterface* PDI,
    const FVector& Origin,
    float Size,
    const FLinearColor& Color,
    uint8 DepthPriority = SDPG_World,
    float Thickness = 1.0f,
    float DepthBias = 0.0
)
{
    if (!PDI) return;

    const FVector Top = Origin + FVector(0, 0, Size);
    const FVector Bottom = Origin + FVector(0, 0, -Size);

    const FVector P[4] =
    {
        Origin + FVector(Size, 0, 0),
        Origin + FVector(0, Size, 0),
        Origin + FVector(-Size, 0, 0),
        Origin + FVector(0,-Size, 0),
    };

    for (int i = 0; i < 4; ++i)
    {
        const int Next = (i + 1) % 4;

        // obwód
        PDI->DrawLine(P[i], P[Next], Color, DepthPriority, Thickness, DepthBias);

        // góra
        PDI->DrawLine(Top, P[i], Color, DepthPriority, Thickness, DepthBias);

        // dół
        PDI->DrawLine(Bottom, P[i], Color, DepthPriority, Thickness, DepthBias);
    }
}

static void DrawCircle(
    FPrimitiveDrawInterface* PDI,
    const FVector& Origin,
    const FVector& Normal,
    float Radius,
    int32 Segments,
    const FLinearColor& Color,
    uint8 DepthPriority = SDPG_World,
    float Thickness = 1.0f
)
{
    if (!PDI || Segments < 3) return;

    FVector X, Y;
    Normal.FindBestAxisVectors(X, Y);

    const float AngleStep = 2.f * PI / Segments;

    FVector Prev = Origin + X * Radius;

    for (int32 i = 1; i <= Segments; ++i)
    {
        const float Angle = AngleStep * i;
        const FVector Next =
            Origin +
            (X * FMath::Cos(Angle) + Y * FMath::Sin(Angle)) * Radius;

        PDI->DrawLine(Prev, Next, Color, DepthPriority, Thickness);
        Prev = Next;
    }
}

static void DrawCustomDashedLine(
    FPrimitiveDrawInterface* PDI,
    const FVector& Start,
    const FVector& End,
    float DashLength,
    float GapLength,
    const FLinearColor& Color,
    uint8 DepthPriority = SDPG_World,
    float Thickness = 1.0f
)
{
    if (!PDI) return;

    const FVector Delta = End - Start;
    const float TotalLength = Delta.Size();

    if (TotalLength <= KINDA_SMALL_NUMBER)
    {
        return;
    }

    const FVector Direction = Delta / TotalLength;
    const float SegmentLength = DashLength + GapLength;

    float Distance = 0.0f;

    while (Distance < TotalLength)
    {
        const float DashStartDist = Distance;
        const float DashEndDist = FMath::Min(Distance + DashLength, TotalLength);

        const FVector DashStart = Start + Direction * DashStartDist;
        const FVector DashEnd = Start + Direction * DashEndDist;

        PDI->DrawTranslucentLine(
            DashStart,
            DashEnd,
            Color,
            DepthPriority,
            Thickness
        );

        Distance += SegmentLength;
    }
}

static void DrawCustomSphere(
    FPrimitiveDrawInterface* PDI,
    const FVector& Origin,
    float Radius = 10.0,
    int32 CircleSegments = 8,
    const FLinearColor& Color = FColor::Red,
    uint8 DepthPriority = SDPG_World,
    float Thickness = 1.0f
)
{
    if (!PDI || Radius <= KINDA_SMALL_NUMBER || CircleSegments < 3)
    {
        return;
    }

    auto DrawCircleOnAxes = [&](const FVector& AxisX, const FVector& AxisY)
        {
            const float Step = 2.f * PI / (float)CircleSegments;

            FVector Prev = Origin + AxisX * Radius; // angle 0
            for (int32 i = 1; i <= CircleSegments; ++i)
            {
                const float A = Step * i;
                const FVector Next = Origin + (AxisX * FMath::Cos(A) + AxisY * FMath::Sin(A)) * Radius;

                PDI->DrawTranslucentLine(Prev, Next, Color, DepthPriority, Thickness);
                Prev = Next;
            }
        };

    // XY plane
    DrawCircleOnAxes(FVector::ForwardVector, FVector::RightVector);

    // XZ plane
    DrawCircleOnAxes(FVector::ForwardVector, FVector::UpVector);

    // YZ plane
    DrawCircleOnAxes(FVector::RightVector, FVector::UpVector);
}

static FColor LerpColorLinear(const FColor& A, const FColor& B, float Alpha)
{
    Alpha = FMath::Clamp(Alpha, 0.f, 1.f);

    const FLinearColor LA = FLinearColor(A);
    const FLinearColor LB = FLinearColor(B);

    const FLinearColor L = FMath::Lerp(LA, LB, Alpha);

    return L.ToFColor(true); // true = sRGB
}

// MAIN
void FCombatAnimSetupViewportClient::UpdateMeshAndAnimation(UCombatAnimSetup* Asset)
{
    if (!PreviewSkeletalComp) return;

    USkeletalMesh* NewMesh = Asset ? Asset->SkeletalMesh.Get() : nullptr;

    const bool bMeshChanged = (NewMesh != LastMesh);
    if (bMeshChanged)
    {
        PreviewSkeletalComp->SetSkeletalMesh(NewMesh);
        LastMesh = NewMesh;
    }

    // Anim/PlayRate możesz aktualizować zawsze:
    UAnimSequenceBase* Anim = Asset ? ANIMASSETSAFE.Get() : nullptr;
    PreviewSkeletalComp->SetAnimationMode(EAnimationMode::AnimationSingleNode);
    PreviewSkeletalComp->SetAnimation(Anim);

    if (UAnimSingleNodeInstance* SingleNode = PreviewSkeletalComp->GetSingleNodeInstance())
    {
        const float Rate = Asset ? Asset->PlayRate : 1.0f;
        SingleNode->SetPlayRate(Rate);
        SingleNode->SetPlaying(Anim != nullptr);
    }

    // ✅ Frame tylko gdy zmienił się mesh (albo pierwszy raz, gdy mesh istnieje)
    if (NewMesh && (bMeshChanged || !bHasFramedOnce))
    {
        const FBoxSphereBounds Bounds = PreviewSkeletalComp->Bounds;
        const float Radius = Bounds.SphereRadius;

        SetViewLocation(Bounds.Origin + FVector(-Radius * 2.5f, 0.f, Radius * 0.8f));
        SetLookAtLocation(Bounds.Origin);

        bHasFramedOnce = true;
    }
}

void FCombatAnimSetupViewportClient::Tick(float DeltaSeconds)
{
    FEditorViewportClient::Tick(DeltaSeconds);

    if (!PreviewSkeletalComp || !CurrentAsset)
    {
        return;
    }

    UAnimSequenceBase* AnimBase = ANIMASSETSAFE.Get();
    if (!AnimBase)
    {
        return;
    }

    const float SequenceLength = AnimBase->GetPlayLength();
    if (SequenceLength <= 0.f)
    {
        return;
    }

    float NewTime = CurrentTime;
    bool bAdvancedTime = false;

    // --- TIME UPDATE (play) ---
    if (bIsPlaying)
    {
        const float Rate = CurrentAsset->PlayRate;
        NewTime = CurrentTime + DeltaSeconds * Rate;
        bAdvancedTime = true;

        if (bLoop)
        {
            while (NewTime > SequenceLength) NewTime -= SequenceLength;
        }
        else
        {
            NewTime = FMath::Min(NewTime, SequenceLength);
        }

        SetCurrentTime(NewTime, false); // ustawia pozycję animacji
    }

    // --- ROOT MOTION (preview) ---
#if WITH_EDITORONLY_DATA
    const bool bPreviewRM = CurrentAsset->bPreviewRootMotion;
#else
    const bool bPreviewRM = false;
#endif

    if (bPreviewRM && bAdvancedTime)
    {
        // Upewnij się, że SingleNode ma root motion mode ustawiony.
        if (UAnimSingleNodeInstance* SingleNode = PreviewSkeletalComp->GetSingleNodeInstance())
        {
            SingleNode->SetRootMotionMode(ERootMotionMode::RootMotionFromEverything);
        }

        // ✅ To jest klucz: tick animacji na komponencie, żeby engine policzył ExtractedRootMotion.
        // TickAnimation jest "lżejsze" i bardziej kontrolowane niż TickComponent.
        PreviewSkeletalComp->TickAnimation(DeltaSeconds, false);

        // Odśwież kości / dane po ticku animacji
        PreviewSkeletalComp->RefreshBoneTransforms();
        PreviewSkeletalComp->UpdateComponentToWorld();

        
        // ✅ Skonsumuj wyekstrahowany root motion (jak w runtime)
        FRootMotionMovementParams RM = PreviewSkeletalComp->ConsumeRootMotion();

        if (RM.bHasRootMotion == true)
        {
            FAnimPoseEvaluationConfig RootEvaluationMode;
            FTransform RootBone = GetBonePositionAtTimeFromSeq(ANIMASSETSAFE, GetPreviewTime(), TEXT("Root"), RootEvaluationMode);
            PreviewSkeletalComp->SetWorldTransform(RootBone);
        }
    }

    CurrentTime = NewTime;
}

TObjectPtr<UAnimSequenceBase> FCombatAnimSetupViewportClient::GetAnimationAssetSafe()
{
    if (!CurrentAsset)
    {
        return nullptr;
    }

    UAnimSequenceBase* Anim = CurrentAsset->Animation.Get();
    if (!Anim)
    {
        return nullptr;
    }

    // Jeżeli to zwykła sekwencja – zwracamy od razu
    if (UAnimSequence* AnimSequence = Cast<UAnimSequence>(Anim))
    {
        return AnimSequence;
    }

    // Jeżeli to montage – spróbuj wyciągnąć pierwszą sekwencję
    if (UAnimMontage* Montage = Cast<UAnimMontage>(Anim))
    {
        ////UE_LOG(LogTemp, Warning, TEXT("Asset is montage"));
        //if (Montage->SlotAnimTracks.Num() > 0)
        //{
        //    const FSlotAnimationTrack& SlotTrack = Montage->SlotAnimTracks[0];

        //    if (SlotTrack.AnimTrack.AnimSegments.Num() > 0)
        //    {
        //        const FAnimSegment& Segment = SlotTrack.AnimTrack.AnimSegments[0];                
        //        if (Segment.GetAnimReference())
        //        {
        //            return Segment.GetAnimReference(); // UAnimSequenceBase*
        //        }
        //        else
        //        {
        //            UE_LOG(LogTemp, Warning, TEXT("Nie udalo sie wyodrebnic animacji a anim montage"));
        //            return CurrentAsset->Animation;
        //        }
        //    }
        //}

        UAnimSequence* AsSequence = GetAnimSequenceFromMontage(Montage, 0);
        if (AsSequence)
        {
            return AsSequence;
        }
        else
        {
            return Anim;
        }

    }

    return nullptr;
}



FTransform FCombatAnimSetupViewportClient::GetBonePositionAtTimeFromSeq(const UAnimSequenceBase* AnimationSequenceBase, double Time, FName BoneName, FAnimPoseEvaluationConfig EvaluationOptions)
{
    if (AnimationSequenceBase && AnimationSequenceBase->GetSkeleton())
    {
        //UAnimMontage* Montage = Cast<UAnimMontage>(AnimationSequenceBase);
        //if (Montage) { return FTransform::Identity; }

        FMemMark Mark(FMemStack::Get());

        // asset to use for retarget proportions (can be either USkeletalMesh or USkeleton)
        UObject* AssetToUse;
        int32 NumRequiredBones;
        if (EvaluationOptions.OptionalSkeletalMesh)
        {
            AssetToUse = CastChecked<UObject>(EvaluationOptions.OptionalSkeletalMesh);
            NumRequiredBones = EvaluationOptions.OptionalSkeletalMesh->GetRefSkeleton().GetNum();
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
        RequiredBones.InitializeTo(RequiredBoneIndexArray, UE::Anim::FCurveFilterSettings(EvaluationOptions.bEvaluateCurves ? UE::Anim::ECurveFilterMode::None : UE::Anim::ECurveFilterMode::DisallowAll), *AssetToUse);

        RequiredBones.SetUseRAWData(EvaluationOptions.EvaluationType == EAnimBaseEvalType::Raw);
        RequiredBones.SetUseSourceData(EvaluationOptions.EvaluationType == EAnimBaseEvalType::Source);

        RequiredBones.SetDisableRetargeting(!EvaluationOptions.bShouldRetarget);

        FCompactPose CompactPose;
        FBlendedCurve Curve;
        UE::Anim::FStackAttributeContainer Attributes;

        FAnimationPoseData PoseData(CompactPose, Curve, Attributes);
        FAnimExtractContext Context(0.0, EvaluationOptions.bExtractRootMotion);

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
                //GEngine->AddOnScreenDebugMessage(-1, 0, FColor::Cyan, BoneTransform.ToString());
            }
            else
            {
                CompactPose.ResetToRefPose();
                AnimationSequenceBase->GetAnimationPose(PoseData, Context);
                BoneTransform = PoseData.GetPose()[FCompactPoseBoneIndex(BoneNameToIndex)];
                //GEngine->AddOnScreenDebugMessage(-1, 0, FColor::Red, BoneTransform.ToString());
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

UAnimSequence* FCombatAnimSetupViewportClient::GetAnimSequenceFromMontage(const UAnimMontage* TargetAnimMontage, int SequenceIndex)
{
    UAnimSequence* AnimSequence = nullptr;

    const UAnimMontage* AnimMontage = TargetAnimMontage;
    if (TargetAnimMontage && AnimMontage->SlotAnimTracks.Num() > 0 && AnimMontage->SlotAnimTracks[0].AnimTrack.AnimSegments.Num() > SequenceIndex)
    {
        AnimSequence = Cast<UAnimSequence>(AnimMontage->SlotAnimTracks[0].AnimTrack.AnimSegments[SequenceIndex].GetAnimReference());
    }
    return AnimSequence;

}

int FCombatAnimSetupViewportClient::GetAnimFramesNum()
{
    UAnimSequence* Seq = Cast<UAnimSequence>(ANIMASSETSAFE);
    if (Seq)
    {
        return Seq->GetNumberOfSampledKeys();
    }
    return -1;
}

UAnimSequence* FCombatAnimSetupViewportClient::GetAsAnimSequence()
{

    UAnimSequenceBase* Anim = CurrentAsset->Animation.Get();
    if (!Anim)
    {
        //UE_LOG(LogTemp, Warning, TEXT("Asset NOT VALID"));
        return nullptr;
    }

    if (UAnimSequence* AnimSequence = Cast<UAnimSequence>(Anim))
    {
        //UE_LOG(LogTemp, Warning, TEXT("Asset is SEQ"));
        return AnimSequence;
    }

    if (UAnimMontage* Montage = Cast<UAnimMontage>(Anim))
    {
        //UE_LOG(LogTemp, Warning, TEXT("Asset is montage"));
        return GetAnimSequenceFromMontage(Montage, 0);
    }

    return nullptr;
}


//▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒
//▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒
void FCombatAnimSetupViewportClient::Draw(const FSceneView* View, FPrimitiveDrawInterface* PDI)
{
    FEditorViewportClient::Draw(View, PDI);

    DrawVictimReferenceTriangle(PDI);
    if (bShowSampledTrajectories)
    {
        DrawMainTrajectory(PDI);
        DrawOtherTrajectiories(PDI);
    }

    if (CurrentAsset && PreviewSkeletalComp)
    {
        if (CurrentAsset->AttackingLimbBoneName != TEXT("none"))
        {
            FAnimPoseEvaluationConfig EvalCfg;
            const FTransform BoneXfLocal = GetBonePositionAtTimeFromSeq(ANIMASSETSAFE, CurrentAsset->AttackMaxPeakTime, CurrentAsset->AttackingLimbBoneName, EvalCfg);

            DrawDiamond(PDI, BoneXfLocal.GetLocation(), 4.0, FColor::Orange, SDPG_Foreground, 0.4, 3);
            const FVector LineEnd = FVector(BoneXfLocal.GetLocation().X, BoneXfLocal.GetLocation().Y, 0.0);
            FColor OrangeAlpha = FColor::Orange; OrangeAlpha.A = 90;
            DrawCustomDashedLine(PDI, BoneXfLocal.GetLocation(), LineEnd, 2, 4, OrangeAlpha, SDPG_Foreground, 0.5);
        }

        if (bShowRootMotionPreview)
        {
            FVector PrevPosition = FVector(0, 0, 0);

            for (int i = 0; i < GetAnimFramesNum() - 1; i++)
            {
                float AsTime = GetAsAnimSequence()->GetTimeAtFrame(i);
                float PrevTime = GetAsAnimSequence()->GetTimeAtFrame(i - 1);

                FAnimPoseEvaluationConfig EvalCfg;
                const FVector BonePos = GetBonePositionAtTimeFromSeq(ANIMASSETSAFE, AsTime, TEXT("root"), EvalCfg).GetLocation();

                if (PrevPosition != BonePos)
                {
                    FColor DrawColor = FColor::Black;
                    if (AsTime <= CurrentAsset->CustomWarpingTimeRangeMax && CurrentAsset->CustomWarpingTimeRangeMax > 0)
                    {
                        DrawColor = FColor(0, 200, 30, 255);
                    }

                    PDI->DrawPoint(BonePos, DrawColor, 4.5, SDPG_Foreground);

                    if (AsTime != PrevTime)
                    {
                        FColor ColorBlack = DrawColor; ColorBlack.A = 40;
                        PDI->DrawTranslucentLine(PrevPosition, BonePos, ColorBlack, SDPG_Foreground, 0.3, -0.5);
                    }
                }
                PrevPosition = BonePos;
            }

        }

        if (bShowCollisionShapes)
        {
            DrawCollisionsToCheck(PDI, CurrentAsset->AttackingTrajectorySamplingConfig);

            if (CurrentAsset->AdditiveTrajectorySampling.Num() > 0)
            {
                for (int j = 0; j < CurrentAsset->AdditiveTrajectorySampling.Num(); j++)
                {
                    DrawCollisionsToCheck(PDI, CurrentAsset->AdditiveTrajectorySampling[j], true);
                }
            }
        }

        PreviewSkeletalComp->bDisplayBones = bShowBones;
        EngineShowFlags.SetBones(bShowBones);

    }

}


void FCombatAnimSetupViewportClient::DrawVictimReferenceTriangle(FPrimitiveDrawInterface* PDI)
{
    if (!PDI || !CurrentAsset)
    {
        return;
    }

    // Musimy mieć komponent podglądu, żeby znać "środek komponentu" i world transform preview.
    if (!PreviewSkeletalComp)
    {
        return;
    }

    // 1) Ustal punkt referencyjny (local/asset space) – kość lub custom transform
    FVector RefPointLocal = FVector::ZeroVector;
    bool bHasRef = false;

    const bool bUseCustom =
        !CurrentAsset->CustomVictimReferenceTransform.Equals(FTransform::Identity, 0.0001f);

    if (bUseCustom)
    {
        // Interpretacja: custom transform jest w przestrzeni komponentu (local).
        RefPointLocal = CurrentAsset->CustomVictimReferenceTransform.GetLocation();
        bHasRef = true;
    }
    else
    {
        const FName BoneName = CurrentAsset->VictimReferencePositionBone;
        if (!BoneName.IsNone() && BoneName != TEXT("none") && CurrentAsset->Animation)
        {
            // GetBonePositionAtTimeFromSeq zwraca transform kości w przestrzeni "modelowej"/local (jak w Twojej funkcji).
            // Używamy CurrentTime z klienta (timeline/transport).
            FAnimPoseEvaluationConfig EvalCfg;
            // jeśli wspierasz OptionalSkeletalMesh w tej funkcji, ustaw ją:
            EvalCfg.OptionalSkeletalMesh = CurrentAsset->SkeletalMesh;
            EvalCfg.bExtractRootMotion = false;
            EvalCfg.bEvaluateCurves = false;
            EvalCfg.bShouldRetarget = true;
            EvalCfg.EvaluationType = EAnimBaseEvalType::Raw;

            const FTransform BoneXfLocal =
                GetBonePositionAtTimeFromSeq(ANIMASSETSAFE, CurrentTime, BoneName, EvalCfg);

            RefPointLocal = BoneXfLocal.GetLocation();
            bHasRef = true;
        }
    }

    if (!bHasRef)
    {
        return;
    }

    const FTransform CompXf = PreviewSkeletalComp->GetComponentTransform();
    const FVector CompCenterWorld = CompXf.GetLocation();


    const FVector A = RefPointLocal;
    const FVector B = FVector(RefPointLocal.X, RefPointLocal.Y, CompCenterWorld.Z);
    const FVector C = CompCenterWorld;

    const float Thickness = 0.4f;
    const uint8 DepthPriority = SDPG_Foreground;

    // Kolor zostawiam bez “customów” – możesz podmienić na coś z assetu.
    const FLinearColor Color = FLinearColor::Blue;
    FLinearColor C2 = FLinearColor::Blue;
    C2.A = 0.2;

    PDI->DrawTranslucentLine(A, B, C2, DepthPriority, Thickness);
    PDI->DrawLine(A, C, Color, DepthPriority, Thickness, 1);
    PDI->DrawTranslucentLine(B, C, C2, DepthPriority, Thickness);
    PDI->DrawPoint(A, Color, 15.0, DepthPriority);

}

void FCombatAnimSetupViewportClient::DrawMainTrajectory(FPrimitiveDrawInterface* PDI)
{
    if (!PDI || !CurrentAsset)
    { return; }

    if (!PreviewSkeletalComp)
    {  return; }

    if (CurrentAsset->bUseTrajectorySampling == false || CurrentAsset->AttackingTrajectorySamplingConfig.BoneName == TEXT("none"))
    { return; }

    const float Thickness = 0.3f;
    const uint8 DepthPriority = SDPG_Foreground;


    float Scale = 30.0;
    int SamplesCount = FMath::RoundToInt(abs(CurrentAsset->AttackingTrajectorySamplingConfig.TimeRangeOffsetMin * Scale) + abs(CurrentAsset->AttackingTrajectorySamplingConfig.TimeRangeOffsetMax * Scale));
    if (SamplesCount == 0) return;
    float SamplingTime = 0.0;
    FTransform PrevPosition = FTransform::Identity;

    const float TimeOrigin = CurrentAsset->AttackMaxPeakTime + CurrentAsset->AttackingTrajectorySamplingConfig.OriginTimeOffset;

    for (int i = 0; i <= SamplesCount; i++)
    {
        SamplingTime = FMath::GetMappedRangeValueClamped
        (
            FVector2D(0, (float)SamplesCount), 
            FVector2D(TimeOrigin + CurrentAsset->AttackingTrajectorySamplingConfig.TimeRangeOffsetMin,
                TimeOrigin + CurrentAsset->AttackingTrajectorySamplingConfig.TimeRangeOffsetMax),
            (float)i
        );
        SamplingTime = FMath::Clamp<float>(SamplingTime, 0.0, GetSequenceLength());

        FAnimPoseEvaluationConfig EvalCfg;
        EvalCfg.EvaluationType = EAnimBaseEvalType::Raw;

        const FTransform BoneXfLocal =
            GetBonePositionAtTimeFromSeq(ANIMASSETSAFE, SamplingTime, CurrentAsset->AttackingTrajectorySamplingConfig.BoneName, EvalCfg);

        if (i > 0)
        {
            PDI->DrawLine(PrevPosition.GetLocation(), BoneXfLocal.GetLocation(), FColor::Yellow, DepthPriority, Thickness, 0.5);
        }
        PrevPosition = BoneXfLocal;
    }
}

void FCombatAnimSetupViewportClient::DrawOtherTrajectiories(FPrimitiveDrawInterface* PDI)
{
    if (!PDI || !CurrentAsset)
    { return; }

    if (!PreviewSkeletalComp)
    { return; }

    if (CurrentAsset->bUseTrajectorySampling == false || CurrentAsset->AdditiveTrajectorySampling.Num() == 0)
    { return; }

    const float Thickness = 0.25f;
    const uint8 DepthPriority = SDPG_Foreground;
    float Scale = 20.0;
    FAnimPoseEvaluationConfig EvalCfg;
    EvalCfg.EvaluationType = EAnimBaseEvalType::Raw;
    const FColor ColorA = FColor(80, 50, 180, 255);
    const FColor ColorB = FColor(80, 180, 50, 255);

    for (int i = 0; i < CurrentAsset->AdditiveTrajectorySampling.Num(); i++)
    {
        FAttackSequenceSamplingData TrajectoryData = CurrentAsset->AdditiveTrajectorySampling[i];

        int SamplesCount = FMath::RoundToInt(abs(TrajectoryData.TimeRangeOffsetMin * Scale) + abs(TrajectoryData.TimeRangeOffsetMax * Scale));
        if (SamplesCount == 0) return;
        float SamplingTime = 0.0;
        FTransform PrevPosition = FTransform::Identity;

        const float TimeOrigin = TrajectoryData.OriginTimeOffset;

        for (int ii = 0; ii <= SamplesCount; ii++)
        {
            SamplingTime = FMath::GetMappedRangeValueClamped
            (
                FVector2D(0, (float)SamplesCount),
                FVector2D(TimeOrigin + TrajectoryData.TimeRangeOffsetMin,
                    TimeOrigin + TrajectoryData.TimeRangeOffsetMax),
                (float)ii
            );
            SamplingTime = FMath::Clamp<float>(SamplingTime, 0.0, GetSequenceLength());

            const FTransform BoneXfLocal =
                GetBonePositionAtTimeFromSeq(ANIMASSETSAFE.Get(), SamplingTime, TrajectoryData.BoneName, EvalCfg);



            FColor ColorGradient = FColor::Red;
            const float GradientAlpha = FMath::GetMappedRangeValueClamped(FVector2D(0.0, (float)SamplesCount), FVector2D(0.0, 1.0), (float)ii);
            ColorGradient = LerpColorLinear(ColorA, ColorB, GradientAlpha);

            if (ii > 0)
            {
                PDI->DrawTranslucentLine(PrevPosition.GetLocation(), BoneXfLocal.GetLocation(), ColorGradient, DepthPriority, Thickness, 0.2);
            }
            PrevPosition = BoneXfLocal;
        }

        for (int ii = 0; ii < TrajectoryData.SamplesNumber; ii++)
        {
            SamplingTime = FMath::GetMappedRangeValueClamped
            (
                FVector2D(0, (float)TrajectoryData.SamplesNumber - 1),
                FVector2D(TimeOrigin + TrajectoryData.TimeRangeOffsetMin,
                    TimeOrigin + TrajectoryData.TimeRangeOffsetMax),
                (float)ii
            );
            if (TrajectoryData.SamplesNumber == 1)
            {
                //SamplingTime = FMath::Lerp<float>(TimeOrigin + TrajectoryData.TimeRangeOffsetMin, TimeOrigin + TrajectoryData.TimeRangeOffsetMax, 0.5);
                SamplingTime = TimeOrigin;
            }

            SamplingTime = FMath::Clamp<float>(SamplingTime, 0.0, GetSequenceLength());
            const FTransform BoneXfLocal =
                GetBonePositionAtTimeFromSeq(ANIMASSETSAFE.Get(), SamplingTime, TrajectoryData.BoneName, EvalCfg);
            DrawDiamond(PDI, BoneXfLocal.GetLocation(), 1.2, ColorA, DepthPriority, 0.5, 0.4);
        }

        SamplingTime = FMath::Clamp<float>(TimeOrigin, 0.0, GetSequenceLength());
        const FVector BoneDesired = GetBonePositionAtTimeFromSeq(ANIMASSETSAFE.Get(), SamplingTime, TrajectoryData.BoneName, EvalCfg).GetLocation();
        const FVector BoneCurrent = GetBonePositionAtTimeFromSeq(ANIMASSETSAFE.Get(), GetPreviewTime(), TrajectoryData.BoneName, EvalCfg).GetLocation();
        DrawCustomDashedLine(PDI, BoneCurrent, BoneDesired, 2, 3, FColor(80,50,180, 80), DepthPriority, 0.18);
    }


}

void FCombatAnimSetupViewportClient::DrawCollisionsToCheck(FPrimitiveDrawInterface* PDI, const FAttackSequenceSamplingData SSD, bool bAddtiveMode)
{
    if (CurrentAsset->bUseTrajectorySampling == false || CurrentAsset->AttackingTrajectorySamplingConfig.BoneName == TEXT("none"))
    { return; }

    float SamplingTime = 0.0;
    //FAttackSequenceSamplingData SSD = CurrentAsset->AttackingTrajectorySamplingConfig;
    FAnimPoseEvaluationConfig EvalCfg;

    int SegmentsNumber = 8;
    if (SSD.CollisionRadius > 20) { SegmentsNumber = 16; }
    else if (SSD.CollisionRadius > 15) { SegmentsNumber = 12; }

    if (SSD.bCheckCollision)
    {
        if (SSD.SamplesNumber == 1)
        {
            SamplingTime = FMath::Clamp<float>(CurrentAsset->AttackMaxPeakTime + SSD.OriginTimeOffset, 0.0, GetSequenceLength());
            const FVector BonePosition = GetBonePositionAtTimeFromSeq(ANIMASSETSAFE, SamplingTime, SSD.BoneName, EvalCfg).GetLocation();
            DrawCustomSphere(PDI, BonePosition, SSD.CollisionRadius, SegmentsNumber, FColor::Red, SDPG_MAX, 0.1);

        }
        else if(SSD.SamplesNumber > 1)
        {
            for (int i = 0; i < SSD.SamplesNumber; i++)
            {
                float TimeOrigin = SSD.OriginTimeOffset;
                if (bAddtiveMode == false) { TimeOrigin = TimeOrigin + CurrentAsset->AttackMaxPeakTime; }

                SamplingTime = FMath::GetMappedRangeValueClamped
                (  
                    FVector2D(0, (float)SSD.SamplesNumber -1),
                    FVector2D(TimeOrigin + SSD.TimeRangeOffsetMin,
                        TimeOrigin + SSD.TimeRangeOffsetMax),
                    (float)i
                );
                SamplingTime = FMath::Clamp<float>(SamplingTime, 0.0, GetSequenceLength());

                const FVector BonePosition = GetBonePositionAtTimeFromSeq(ANIMASSETSAFE, SamplingTime, SSD.BoneName, EvalCfg).GetLocation();
                DrawCustomSphere(PDI, BonePosition, SSD.CollisionRadius, SegmentsNumber, FColor::Red, SDPG_MAX, 0.1);
            }
        }
    }

}

void FCombatAnimSetupViewportClient::HandleAssetChanged(UCombatAnimSetup* Asset, FName PropertyName)
{
    if (!CurrentAsset) return;

    CurrentAsset->PreDefinedData.Empty();
    FAnimPoseEvaluationConfig EvalCfg;
    FCombatAnimTrajectorySavedData ATSD;

    const bool UseCustomT = CurrentAsset->CustomVictimReferenceTransform.Equals(FTransform::Identity, 0.0001f);
    //UE_LOG(LogTemp, Warning, TEXT("UPDATE 01"));

    if (CurrentAsset->VictimReferencePositionBone != TEXT("none") || UseCustomT == false)
    {
        FTransform BoneTransformA = GetBonePositionAtTimeFromSeq(ANIMASSETSAFE, 0.1, CurrentAsset->VictimReferencePositionBone, EvalCfg);
        if (UseCustomT == false) { BoneTransformA = CurrentAsset->CustomVictimReferenceTransform; }

        ATSD.BoneReference = CurrentAsset->VictimReferencePositionBone;
        ATSD.SampledBoneTransforms.Add(BoneTransformA);

        CurrentAsset->PreDefinedData.Add(TEXT("MainRef"), ATSD);
    }
    ATSD.SampledBoneTransforms.Empty();


    if (CurrentAsset->AttackingLimbBoneName != TEXT("none"))
    {
        FTransform BoneTransformA = GetBonePositionAtTimeFromSeq(ANIMASSETSAFE, CurrentAsset->AttackMaxPeakTime, CurrentAsset->AttackingLimbBoneName, EvalCfg);

        ATSD.BoneReference = CurrentAsset->AttackingLimbBoneName;
        ATSD.SampledBoneTransforms.Add(BoneTransformA);

        CurrentAsset->PreDefinedData.Add(TEXT("AttackLimb"), ATSD);
    }
    ATSD.SampledBoneTransforms.Empty();


    if (CurrentAsset->bUseTrajectorySampling)
    {
        FAttackSequenceSamplingData TrajectoryData = CurrentAsset->AttackingTrajectorySamplingConfig;
        FTransform BoneTransformA = FTransform::Identity;
        FTransform BoneTransformB = FTransform::Identity;
        const float TimeDelta = 0.05;

        ATSD.BoneReference = TrajectoryData.BoneName;

        if (TrajectoryData.SamplesNumber == 1)
        {
            BoneTransformA = GetBonePositionAtTimeFromSeq(ANIMASSETSAFE, CurrentAsset->AttackMaxPeakTime + TrajectoryData.OriginTimeOffset, TrajectoryData.BoneName, EvalCfg);
            BoneTransformB = GetBonePositionAtTimeFromSeq(ANIMASSETSAFE, CurrentAsset->AttackMaxPeakTime + TrajectoryData.OriginTimeOffset + TimeDelta, TrajectoryData.BoneName, EvalCfg);
            FVector DeltaDirection = (BoneTransformA.GetLocation() - BoneTransformB.GetLocation()); DeltaDirection.Normalize();

            BoneTransformA.SetRotation(DeltaDirection.ToOrientationQuat());

            ATSD.SampledBoneTransforms.Add(BoneTransformA);
        }
        else
        {
            for (int i = 0; i < TrajectoryData.SamplesNumber; i++)
            {
                const float TimeOrigin = CurrentAsset->AttackMaxPeakTime + TrajectoryData.OriginTimeOffset;
                float SamplingTime = FMath::GetMappedRangeValueClamped
                (
                    FVector2D(0, (float)TrajectoryData.SamplesNumber - 1),
                    FVector2D(TimeOrigin + TrajectoryData.TimeRangeOffsetMin,
                        TimeOrigin + TrajectoryData.TimeRangeOffsetMax),
                    (float)i
                );
                BoneTransformA = GetBonePositionAtTimeFromSeq(ANIMASSETSAFE, SamplingTime, TrajectoryData.BoneName, EvalCfg);
                BoneTransformA = GetBonePositionAtTimeFromSeq(ANIMASSETSAFE, SamplingTime + TimeDelta, TrajectoryData.BoneName, EvalCfg);
                FVector DeltaDirection = (BoneTransformA.GetLocation() - BoneTransformB.GetLocation()); DeltaDirection.Normalize();

                BoneTransformA.SetRotation(DeltaDirection.ToOrientationQuat());

                ATSD.SampledBoneTransforms.Add(BoneTransformA);
            }
        }
        CurrentAsset->PreDefinedData.Add(TEXT("MainTrajectory"), ATSD);
    }
    ATSD.SampledBoneTransforms.Empty();


    Asset->MarkPackageDirty();

}

void FCombatAnimSetupViewportClient::BindToAsset()
{
#if WITH_EDITOR
    if (UCombatAnimSetup* Asset = CurrentAsset.Get())
    {
        // AddRaw jest OK jeśli ViewportClient żyje dłużej niż asset editor tab.
        // Ważne: odpinamy w UnbindFromAsset i destruktorze.
        Asset->OnChanged().AddRaw(this, &FCombatAnimSetupViewportClient::HandleAssetChanged);
        bBoundToAsset = true;
    }
#endif
}

void FCombatAnimSetupViewportClient::UnbindFromAsset()
{
#if WITH_EDITOR
    if (bBoundToAsset)
    {
        if (UCombatAnimSetup* Asset = CurrentAsset.Get())
        {
            Asset->OnChanged().RemoveAll(this);
        }
        bBoundToAsset = false;
    }
#endif
}
