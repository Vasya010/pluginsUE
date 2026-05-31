

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Animation/AnimSequenceBase.h"
#include "CombatCoreBPLibrary.h"
#include "CombatAnimSetup.generated.h"


//#if WITH_EDITOR
//DECLARE_MULTICAST_DELEGATE_OneParam(FOnCombatSetupChanged, FName);
//FOnCombatSetupChanged OnChanged;
//#endif


/*
A DataAsset intended for animation configurations related to the CombatSystem. The primary use of this class type relates to the AGLS project. 
CombatAnimSet assets are used when analyzing the animation database. They play a key role in determining the weights for individual animations 
in the database.
*/
UCLASS(BlueprintType)
class COMBATCORE_API UCombatAnimSetup : public UDataAsset
{
	GENERATED_BODY()

public:
// Required Skeletal Mesh For Preview
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "CombatAnimSetup|Main")
    TObjectPtr<USkeletalMesh> SkeletalMesh = nullptr;

// Necessary animation assets for previewing and creating animations
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "CombatAnimSetup|Main")
    TObjectPtr<UAnimSequenceBase> Animation = nullptr;


// The PlayRate of the animation. This is used in the preview and is taken into account when trying to trigger the animation.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "CombatAnimSetup|SequenceProperties", meta = (ClampMin = "0.1", UIMin = "0.1"))
    float PlayRate = 1.0f;

/*Specifies the minimum and maximum range at which the animation can start playing. 
Note: In the CombatAnimSet preview, this value is ignored.*/
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "CombatAnimSetup|SequenceProperties", meta = (ClampMin = "0.0", UIMin = "0.0", ClampMax = "5.0", UIMax = "5.0"))
    FVector2D StartPositionRange = FVector2D(0.0, 0.0);

/*VictimReferencePositionBone - One of the most important values required when determining the final animation weight during 
database searching.By specifying the correct bone, the searching function gains access to information about the optimal 
position of the other character. Based on this, the weight related to XY and Z distance is calculated. In the case of AGLS, 
the bone that typically stores this information is attach.*/
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "CombatAnimSetup|SearchingProperties")
    FName VictimReferencePositionBone = TEXT("none");

/*CustomVictimReferenceTransform - An alternative solution to VictimReferencePositionBone. It can be used when the animation 
does not contain a valid reference to the other characters position authored directly in the AnimationSequence.*/
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "CombatAnimSetup|SearchingProperties")
    FTransform CustomVictimReferenceTransform = FTransform::Identity;

/*AttackingLimbBoneName - Specifies the primary bone associated with attacking. This is usually a limb-related bone such as 
hand_L/R or foot_L/R. The name must be valid so that the weight evaluation algorithm can determine which limb should be used 
to perform the attack.The AttackMaxPeakTime value is also determined relative to this bone.*/
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "CombatAnimSetup|SearchingProperties")
    FName AttackingLimbBoneName = TEXT("ik_hand_gun");

/*AttackMaxPeakTime - The time at which, during the attack animation, the maximum extension of the limb specified by 
AttackingLimbBoneName is most pronounced.*/
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "CombatAnimSetup|SearchingProperties", meta = (ClampMin = "0.0", UIMin = "0.0", ClampMax = "10.0", UIMax = "10.0"))
    float AttackMaxPeakTime = 0.5;

/*SearchingConstWeightBias - A constant value that reduces the final animation weight. It affects animation priority, 
which may result in the animation being selected more frequently or, conversely, less often.*/
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "CombatAnimSetup|SearchingProperties", meta = (ClampMin = "-2.0", UIMin = "-2.0", ClampMax = "2.0", UIMax = "2.0"))
    float SearchingConstWeightBias = 0.0;


/*bUseTrajectorySampling – Enabling this option triggers additional calculations related to determining the animation 
weight during database searching.*/
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "CombatAnimSetup|TrajectorySampling")
    bool bUseTrajectorySampling = true;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "CombatAnimSetup|TrajectorySampling", meta = (EditCondition = "bUseTrajectorySampling"))
    FAttackSequenceSamplingData AttackingTrajectorySamplingConfig;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "CombatAnimSetup|TrajectorySampling", meta = (EditCondition = "bUseTrajectorySampling"))
    TArray<FAttackSequenceSamplingData> AdditiveTrajectorySampling;

    // IF CustomWarpingTimeRangeMax <= 0 then AUTO value is used. 
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "CombatAnimSetup|Custom Warping", meta = (ClampMin = "-1", UIMin = "-1", ClampMax = "4.0", UIMax = "4.0"))
    float CustomWarpingTimeRangeMax = -1.0;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "CombatAnimSetup|Custom Warping")
    CustomWarpingInterpFunction InterpFunction = CustomWarpingInterpFunction::Linear;


    UFUNCTION(BlueprintPure, Category = "CombatAnimSetup")
    TMap<FName, FCombatAnimTrajectorySavedData> GetAssetCalculatedData() const;


#if WITH_EDITORONLY_DATA
    UPROPERTY(EditAnywhere, Category = "Preview")
    bool bPreviewRootMotion = false;

#endif
	
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Read Only")
    TMap<FName, FCombatAnimTrajectorySavedData> PreDefinedData;


#if WITH_EDITOR
    // Delegat sygnalizuj¹cy zmianê w³aœciwoœci w edytorze (Details Panel / asset editor)
    DECLARE_MULTICAST_DELEGATE_TwoParams(FOnCombatAnimSetupChanged, UCombatAnimSetup* /*Asset*/, FName /*PropertyName*/);
    FOnCombatAnimSetupChanged& OnChanged() { return OnChangedDelegate; }

    virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

private:
#if WITH_EDITOR
    FOnCombatAnimSetupChanged OnChangedDelegate;
#endif

};
