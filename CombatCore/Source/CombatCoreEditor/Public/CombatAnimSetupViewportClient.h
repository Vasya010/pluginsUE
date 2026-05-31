
#pragma once

#include "CoreMinimal.h"
#include "EditorViewportClient.h"
#include "AdvancedPreviewScene.h"
#include "CombatAnimSetup.h"

enum class EAnimBaseEvalType : uint8
{
    // Evaluates the original Animation Source data 
    Source,
    // Evaluates the original Animation Source data with additive animation layers
    Raw,
    // Evaluates the compressed Animation data - matching runtime (cooked)
    Compressed
};

struct FAnimPoseEvaluationConfig
{
    // Type of evaluation which should be used
    EAnimBaseEvalType EvaluationType = EAnimBaseEvalType::Raw;

    // Whether or not to retarget animation during evaluation
    bool bShouldRetarget = true;

    bool bExtractRootMotion = false;

    // Whether or not to force root motion being incorporated into retrieved pose
    bool bIncorporateRootMotionIntoPose = true;

    // Optional skeletal mesh with proportions to use when evaluating a pose
    TObjectPtr<USkeletalMesh> OptionalSkeletalMesh = nullptr;

    // Whether or additive animations should be applied to their base-pose 
    bool bRetrieveAdditiveAsFullPose = true;

    // Whether or not to evaluate Animation Curves
    bool bEvaluateCurves = true;
};


class UCombatAnimSetup;
class USkeletalMeshComponent;

class FCombatAnimSetupViewportClient : public FEditorViewportClient
{
public:
    FCombatAnimSetupViewportClient(
        const TSharedRef<FAdvancedPreviewScene>& InPreviewScene,
        const TSharedRef<class SEditorViewport>& InViewportWidget);

    virtual ~FCombatAnimSetupViewportClient();

    void ApplyAsset(UCombatAnimSetup* Asset);

    virtual void Tick(float DeltaSeconds) override;

    TObjectPtr<UAnimSequenceBase> GetAnimationAssetSafe();

private:
    void EnsurePreviewComponent();
    void UpdateMeshAndAnimation(UCombatAnimSetup* Asset);

private:
    TSharedRef<FAdvancedPreviewScene> PreviewScene;

    TObjectPtr<USkeletalMeshComponent> PreviewSkeletalComp = nullptr;
    TObjectPtr<UCombatAnimSetup> CurrentAsset = nullptr;

    TObjectPtr<USkeletalMesh> LastMesh = nullptr;
    bool bHasFramedOnce = false;

public:
    float GetSequenceLength() const;
    float GetCurrentTime() const { return CurrentTime; }
    void SetCurrentTime(float InTime, bool bFireNotifies = false);
    void SetPlaying(bool bInPlaying);
    bool IsPlaying() const { return bIsPlaying; }
    float GetPreviewTime() const;
    void ResetPreviewTransform();
    void SetLooping(bool bInLoop) { bLoop = bInLoop; }
    bool IsLooping() const { return bLoop; }

    bool bShowRootMotionPreview = false;
    bool bShowSampledTrajectories = true;
    bool bShowBones = false;
    bool bShowCollisionShapes = false;

private:
    float CurrentTime = 0.0f;
    bool bIsPlaying = true;
    bool bLoop = true;

    float PreviousTimeForRootMotion = 0.0f;
    FTransform GetBonePositionAtTimeFromSeq(const UAnimSequenceBase* AnimationSequenceBase, double Time, FName BoneName, FAnimPoseEvaluationConfig EvaluationOptions);
    UAnimSequence* GetAnimSequenceFromMontage(const UAnimMontage* TargetAnimMontage, int SequenceIndex);

    int GetAnimFramesNum();
    UAnimSequence* GetAsAnimSequence();
    virtual void Draw(const FSceneView* View, FPrimitiveDrawInterface* PDI) override;
    void DrawVictimReferenceTriangle(FPrimitiveDrawInterface* PDI);
    void DrawMainTrajectory(FPrimitiveDrawInterface* PDI);
    void DrawOtherTrajectiories(FPrimitiveDrawInterface* PDI);
    void DrawCollisionsToCheck(FPrimitiveDrawInterface* PDI, const FAttackSequenceSamplingData SSD, bool bAddtiveMode = false);


    // reakcja na zmiany assetu (bez tick)
    void HandleAssetChanged(UCombatAnimSetup* Asset, FName PropertyName);
    void BindToAsset();
    void UnbindFromAsset();
    bool bBoundToAsset = false;


};