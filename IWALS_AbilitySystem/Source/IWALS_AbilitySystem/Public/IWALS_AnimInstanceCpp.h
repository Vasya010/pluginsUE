// Jakub W

#pragma once

#include "CoreMinimal.h"
#include "IWALS_EnumsAndStruct.h"
#include "ALS_StructuresAndEnumsCpp.h"
#include "CharacterFocusingComponent.h"
#include "Animation/AnimInstance.h"
#include "GameplayTagContainer.h"
#include "IWALS_AnimInstanceCpp.generated.h"

/**
 * 
 */
UCLASS()
class IWALS_ABILITYSYSTEM_API UIWALS_AnimInstanceCpp : public UAnimInstance
{
	GENERATED_BODY()

public:

	UPROPERTY(BlueprintReadWrite, Category = "Character Information", meta = (AllowPrivateAccess = "True"))
	AGLS_WalkingType CurrentWalkingType = AGLS_WalkingType::Default;

	UPROPERTY(BlueprintReadWrite, Category = "Character Information", meta = (AllowPrivateAccess = "True"))
	AGLS_RunningType CurrentRunningType = AGLS_RunningType::Jog;


	/* Zdefiniuj podstawowe zmienne, które bêd¹ potrzebne dla systemu Overlay States. ¿eby nie odwo³ywaæ siê do klasy ALS_AnimBP przy pomocy Property Access utworzony zosta³ w³aœnie
	Anim Instance. W nim zdefiniowane s¹ potrzebne zmienne, wiêc dziêki temu odwo³ywaæ siê bêdziemy w³aœnie do tej klasy a nie ca³ego ALS_AnimBP */

	UPROPERTY(BlueprintReadWrite, Category = "IWALS Anim Graph", meta = (AllowPrivateAccess = "True"))
		float SecondaryMotionMaskC = 0.0;

	UPROPERTY(BlueprintReadWrite, Category = "IWALS Anim Graph", meta = (AllowPrivateAccess = "True"))
		float BlendOverlayWithCoverModeC = 0.0;

	UPROPERTY(BlueprintReadWrite, Category = "IWALS Anim Graph", meta = (AllowPrivateAccess = "True"))
		float LandPredictionC = 0.0;

	UPROPERTY(BlueprintReadWrite, Category = "IWALS Anim Graph", meta = (AllowPrivateAccess = "True"))
		bool IsHeldArrowC = false;

	UPROPERTY(BlueprintReadWrite, Category = "IWALS Anim Graph", meta = (AllowPrivateAccess = "True"))
		bool IsHaveArrowsC = false;

	UPROPERTY(BlueprintReadWrite, Category = "IWALS Anim Graph", meta = (AllowPrivateAccess = "True"))
		bool AddRecoilImpulseC = false;

	UPROPERTY(BlueprintReadWrite, Category = "IWALS Anim Graph", meta = (AllowPrivateAccess = "True"))
		int OverlayOverrideStateC = 0;

	UPROPERTY(BlueprintReadWrite, Category = "IWALS Anim Graph", meta = (AllowPrivateAccess = "True"))
		int CombatStateIndexC = 0;

	UPROPERTY(BlueprintReadWrite, Category = "IWALS Anim Graph", meta = (AllowPrivateAccess = "True"))
		FVector2D CoverCrouchWithDirectionC = FVector2D(0, 0);

	UPROPERTY(BlueprintReadWrite, Category = "IWALS Anim Graph", meta = (AllowPrivateAccess = "True"))
		FVector RelativeAccelerationAmoutC = FVector(0, 0, 0);

	//Foots IK Variables

	//Ragdoll

	UPROPERTY(BlueprintReadWrite, Category = "Anim Graph - Ragdoll", meta = (AllowPrivateAccess = "True"))
		float FlailRateC = 0.0;

	//Aiming

	UPROPERTY(BlueprintReadWrite, Category = "Anim Graph - Aiming Values", meta = (AllowPrivateAccess = "True"))
		FRotator SpineRotationC = FRotator(0, 0, 0);

	UPROPERTY(BlueprintReadWrite, Category = "Anim Graph - Aiming Values", meta = (AllowPrivateAccess = "True"))
		FRotator SmoothedAimingRotationC = FRotator(0, 0, 0);

	UPROPERTY(BlueprintReadWrite, Category = "Anim Graph - Aiming Values", meta = (AllowPrivateAccess = "True"))
		FVector2D AimingAngleC = FVector2D(0, 0);

	UPROPERTY(BlueprintReadWrite, Category = "Anim Graph - Aiming Values", meta = (AllowPrivateAccess = "True"))
		FVector2D SmoothedAimingAngleC = FVector2D(0, 0);

	UPROPERTY(BlueprintReadWrite, Category = "Anim Graph - Aiming Values", meta = (AllowPrivateAccess = "True"))
		float AimSweepTimeC = 0.5;

	UPROPERTY(BlueprintReadWrite, Category = "Anim Graph - Aiming Values", meta = (AllowPrivateAccess = "True"))
		float ForwardYawTimeC = 0.0;

	UPROPERTY(BlueprintReadWrite, Category = "Anim Graph - Aiming Values", meta = (AllowPrivateAccess = "True"))
		FVector AnimPrepertiesCustomC = FVector(0, 0, 0);

	UPROPERTY(BlueprintReadWrite, Category = "Anim Graph - Aiming Values", meta = (AllowPrivateAccess = "True"))
		FTransform NeckTransformFromSnapshot = FTransform::Identity;

	//Anim Graph - Grounded

	UPROPERTY(BlueprintReadWrite, Category = "Anim Graph - Grounded", meta = (AllowPrivateAccess = "True"))
		HipsDirectionC TrackedHipsDirectionC = HipsDirectionC::F;

	UPROPERTY(BlueprintReadWrite, Category = "Anim Graph - Grounded", meta = (AllowPrivateAccess = "True"))
		bool ShouldMoveC = false;

	//UPROPERTY(BlueprintReadWrite, Category = "Anim Graph - Grounded", meta = (AllowPrivateAccess = "True"))
		//bool PivotC = false;

	//UPROPERTY(BlueprintReadWrite, Category = "Anim Graph - Grounded", meta = (AllowPrivateAccess = "True"))
		//bool PivotPlayingC = false;

	//UPROPERTY(BlueprintReadWrite, Category = "Anim Graph - Grounded", meta = (AllowPrivateAccess = "True"))
		//bool PlayStopMovementTransitionC = false;

	//UPROPERTY(BlueprintReadWrite, Category = "Anim Graph - Grounded", meta = (AllowPrivateAccess = "True"))
		//bool FinishStopTransitionC = false;

	UPROPERTY(BlueprintReadWrite, Category = "Anim Graph - Grounded", meta = (AllowPrivateAccess = "True"))
		bool Rotate_LC = false;

	UPROPERTY(BlueprintReadWrite, Category = "Anim Graph - Grounded", meta = (AllowPrivateAccess = "True"))
		bool Rotate_RC = false;

	UPROPERTY(BlueprintReadWrite, Category = "Anim Graph - Grounded", meta = (AllowPrivateAccess = "True"))
		float RotateRateC = 0.0;

	//UPROPERTY(BlueprintReadWrite, Category = "Anim Graph - Grounded", meta = (AllowPrivateAccess = "True"))
		//float RotationScaleC = 0.0;

	//UPROPERTY(BlueprintReadWrite, Category = "Anim Graph - Grounded", meta = (AllowPrivateAccess = "True"))
		//float DiagonalScaleAmoutC = 0.0;

	UPROPERTY(BlueprintReadWrite, Category = "Anim Graph - Grounded", meta = (AllowPrivateAccess = "True"))
		FVelocityBlendC VelocityBlend = {};

	UPROPERTY(BlueprintReadWrite, Category = "Anim Graph - Grounded", meta = (AllowPrivateAccess = "True"))
		FLeanAmoutC LeanAmountC = {};

	//UPROPERTY(BlueprintReadWrite, Category = "Anim Graph - Grounded", meta = (AllowPrivateAccess = "True"))
		//float FYawC = 0.0;

	//UPROPERTY(BlueprintReadWrite, Category = "Anim Graph - Grounded", meta = (AllowPrivateAccess = "True"))
		//float BYawC = 0.0;

	//UPROPERTY(BlueprintReadWrite, Category = "Anim Graph - Grounded", meta = (AllowPrivateAccess = "True"))
		//float LYawC = 0.0;

	//UPROPERTY(BlueprintReadWrite, Category = "Anim Graph - Grounded", meta = (AllowPrivateAccess = "True"))
		//float RYawC = 0.0;

	UPROPERTY(BlueprintReadWrite, Category = "Anim Graph - Grounded", meta = (AllowPrivateAccess = "True"))
		float RootYawChangeSpeed = 0.0;

	//Anim Graph - In Air

	UPROPERTY(BlueprintReadWrite, Category = "Anim Graph - In Air", meta = (AllowPrivateAccess = "True"))
		bool JumpedC = false;

	UPROPERTY(BlueprintReadWrite, Category = "Anim Graph - In Air", meta = (AllowPrivateAccess = "True"))
		float JumpPlayRateC = 1.0;

	UPROPERTY(BlueprintReadWrite, Category = "Anim Graph - In Air", meta = (AllowPrivateAccess = "True"))
		float FallSpeedC = 0.0;


	//-------------------------------------------     For Motion Matching    ------------------------------------------------------

	UPROPERTY(BlueprintReadWrite, Category = "Motion Matching", meta = (AllowPrivateAccess = "True"))
		bool OffsetRootBoneEnabledC = true;

	UPROPERTY(BlueprintReadWrite, Category = "Essential Values", meta = (AllowPrivateAccess = "True"))
		bool IsMovingC = false;

	UPROPERTY(BlueprintReadWrite, Category = "Essential Values", meta = (AllowPrivateAccess = "True"))
		bool PickUpLootItemC = false;

	UPROPERTY(BlueprintReadWrite, Category = "Essential Values", meta = (AllowPrivateAccess = "True"))
		bool HasMovementInputC = false;

	UPROPERTY(BlueprintReadWrite, Category = "Essential Values", meta = (AllowPrivateAccess = "True"))
		bool CapsuleCollidingC = false;

	UPROPERTY(BlueprintReadWrite, Category = "Essential Values", meta = (AllowPrivateAccess = "True"))
		bool JustLandedC = false;

	UPROPERTY(BlueprintReadWrite, Category = "Essential Values", meta = (AllowPrivateAccess = "True"))
		bool OnStairsC = false;

	UPROPERTY(BlueprintReadWrite, Category = "Motion Matching", meta = (AllowPrivateAccess = "True"))
		bool InterruptOnDatabaseC = false;

	UPROPERTY(BlueprintReadWrite, Category = "Essential Values", meta = (AllowPrivateAccess = "True"))
		bool IsTurnInPlaceAimingC = false;

	UPROPERTY(BlueprintReadWrite, Category = "Essential Values", meta = (AllowPrivateAccess = "True"))
		float SpeedC = 0.0;

	UPROPERTY(BlueprintReadWrite, Category = "Essential Values", meta = (AllowPrivateAccess = "True"))
		FVector VelocityC = FVector(0, 0, 0);

	UPROPERTY(BlueprintReadWrite, Category = "Essential Values", meta = (AllowPrivateAccess = "True"))
		FVector FutureVelocityC = FVector(0, 0, 0);

	UPROPERTY(BlueprintReadWrite, Category = "Essential Values", meta = (AllowPrivateAccess = "True"))
		FVector VelocityLastFrameC = FVector(0, 0, 0);

	UPROPERTY(BlueprintReadWrite, Category = "Essential Values", meta = (AllowPrivateAccess = "True"))
		FVector LastNonZeroVelocityC = FVector(0, 0, 0);

	UPROPERTY(BlueprintReadWrite, Category = "Essential Values", meta = (AllowPrivateAccess = "True"))
		FVector AccelerationC = FVector(0, 0, 0);

	UPROPERTY(BlueprintReadWrite, Category = "Essential Values", meta = (AllowPrivateAccess = "True"))
		FVector LandVelocityC = FVector(0, 0, 0);

	UPROPERTY(BlueprintReadWrite, Category = "Essential Values", meta = (AllowPrivateAccess = "True"))
		FRotator AimingRotationC = FRotator(0, 0, 0);

	UPROPERTY(BlueprintReadWrite, Category = "Motion Matching", meta = (AllowPrivateAccess = "True"))
		FTransform CharacterTransformC = FTransform::Identity;

	UPROPERTY(BlueprintReadWrite, Category = "Motion Matching", meta = (AllowPrivateAccess = "True"))
		FTransform RootTransformC = FTransform::Identity;

	UPROPERTY(BlueprintReadWrite, Category = "Motion Matching", meta = (AllowPrivateAccess = "True"))
		FTransform InteractionTransformC = FTransform::Identity;

	UPROPERTY(BlueprintReadWrite, Category = "Motion Matching", meta = (AllowPrivateAccess = "True"))
		FRotator FutureMovementAngleC = FRotator(0, 0, 0);

	UPROPERTY(BlueprintReadWrite, Category = "Motion Matching", meta = (AllowPrivateAccess = "True"))
		TArray<FName> CurrentDatabaseTags;

	// Added FOR AGLS v1.8
	UPROPERTY(BlueprintReadWrite, Category = "Motion Matching", meta = (AllowPrivateAccess = "True"))
		bool UseCustomTrajectoryFacing = false;

	UPROPERTY(BlueprintReadWrite, Category = "Motion Matching", meta = (AllowPrivateAccess = "True"))
		FRotator CustomFacingDesiredRotation = FRotator(0, 0, 0);

	UPROPERTY(BlueprintReadWrite, Category = "Motion Matching", meta = (AllowPrivateAccess = "True"))
		UCharacterFocusingComponent* FocusingComponent = nullptr;

	UPROPERTY(BlueprintReadWrite, Category = "Motion Matching", meta = (AllowPrivateAccess = "True"))
		FGameplayTagContainer OwnerTagContainer;

};
