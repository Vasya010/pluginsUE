// Fill out your copyright notice in the Description page of Project Settings.


#include "MeleeCombatCoreComponent.h"
//Include Kismet
#include "Kismet/KismetSystemLibrary.h"
#include "Kismet/KismetMathLibrary.h"
#include "HelpfulFunctionsBPLibrary.h"
//Include Curves
#include "Curves/CurveFloat.h"
#include "Curves/CurveVector.h"

#include "Engine/AssetManager.h"
#include "TimerManager.h"

#define KSL UKismetSystemLibrary
#define KML UKismetMathLibrary
#define HFL UHelpfulFunctionsBPLibrary
#define WarpTYPE CMaleeCombat_CustomWarpingActorsSet
#define MotionSolverTYPE OpponentMotionSolverType
#define AccumulatedSOLVER CMaleeCombat_WarpingPositionSolver::AccumulatedMotion
#define InitPositionSOLVER CMaleeCombat_WarpingPositionSolver::SubtractInitPosition
#define ZEROVEC FVector::ZeroVector

// Sets default values for this component's properties
UMeleeCombatCoreComponent::UMeleeCombatCoreComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;

}


void UMeleeCombatCoreComponent::CallMontagelAction(CE_MontageDelegateActions Type, FName NotifyName)
{
	OnMontageLatendAction.Broadcast(Type, NotifyName);
}

void UMeleeCombatCoreComponent::CallAttackInputDelegate(bool InputValue)
{
	MeleeCombatInputTrigger_Attack.Broadcast(InputValue);
}

void UMeleeCombatCoreComponent::CallDodgeInputDelegate(bool InputValue)
{
	MeleeCombatInputTrigger_Dodge.Broadcast(InputValue);
}

// Called when the game starts
void UMeleeCombatCoreComponent::BeginPlay()
{
	Super::BeginPlay();

}


//▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒
//▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒ 𝐓𝐈𝐂𝐊 𝐄𝐕𝐄𝐍𝐓
//▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒
void UMeleeCombatCoreComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (LinkedAnimLayerTimeElapsed > -0.5)
	{
		LinkedAnimLayerTimeElapsed = FMath::Clamp<float>(LinkedAnimLayerTimeElapsed - DeltaTime, 0.0, DurationToWaitUntillRemoveAnim);

		if (LinkedAnimLayerTimeElapsed == 0.0)
		{
			UnlinkAnimLayer(false);
			LinkedAnimLayerTimeElapsed = -1;
		}
	}

	// ...
}


UMeleeCombatCoreComponent* UMeleeCombatCoreComponent::GryGetMeleeCombatComponent(UObject* WorldContextObject, ACharacter* Target)
{
	if (Target)
	{
		UMeleeCombatCoreComponent* TargetComp = Cast<UMeleeCombatCoreComponent>(Target->GetComponentByClass(UMeleeCombatCoreComponent::StaticClass()));
		return TargetComp;
	}
	else
	{
		ACharacter* TargetChar = Cast<ACharacter>(WorldContextObject);
		if (TargetChar)
		{
			UMeleeCombatCoreComponent* TargetComp = Cast<UMeleeCombatCoreComponent>(TargetChar->GetComponentByClass(UMeleeCombatCoreComponent::StaticClass()));
			return TargetComp;
		}
		return nullptr;
	}
}


FVector UMeleeCombatCoreComponent::GetCurrentMovementInput_Implementation()
{
	return FVector();
}


void UMeleeCombatCoreComponent::GetCharacterMainStates_Implementation(int FromEnemy, CALS_MovementState& MovementState, CALS_MovementAction& MovementAction, CALS_RotationMode& RotationMode,
	CALS_Gait& ActualGait, CALS_Stance& ActualStance, CALS_OverlayState& OverlayState)
{
	ACharacter* FromCharacter = RefChar;
	if (FromEnemy == 1) { FromCharacter = GetTargetedCharacter(); }
	else if (FromEnemy == 2) { FromCharacter = GetChoosedEnemyChar(); }

	if (FromCharacter)
	{
		if (FromCharacter->GetClass()->ImplementsInterface(UAGLS_AI_CharacterInterface::StaticClass()))
		{
			IAGLS_AI_CharacterInterface::Execute_BPI_AI_Get_CurrentStates
			(
				FromCharacter,
				IgnoreOut<TEnumAsByte<EMovementMode>>(),
				MovementState,
				IgnoreOut<CALS_MovementState>(),
				MovementAction,
				RotationMode,
				ActualGait,
				ActualStance,
				OverlayState,
				IgnoreOut<CALS_GroundedMoveMode>()
			);
			return;
		}
	}
	return;
}


bool UMeleeCombatCoreComponent::ShouldMarkTargetedEnemyForSF_Implementation(ACharacter* InCharacter)
{
	return false;
}


bool UMeleeCombatCoreComponent::TrySetCurrentCombatActionState_Implementation(CMaleeCombat_CurrentAction NewState)
{
	if (NewState != CurrentPerformedAction)
	{
		PrevPerformedAction = CurrentPerformedAction;
	}
	CurrentPerformedAction = NewState;
	return true;
}


//░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░
//░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░ 𝐑𝐔𝐍 𝐒𝐄𝐀𝐑𝐂𝐇𝐈𝐍𝐆
//░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░
bool UMeleeCombatCoreComponent::RunSearchingAnimSetFromDatabases_Implementation(UCombatAnimSetup*& ReturnAnimSetup, const TArray<UCombatAnimsDatabase*>& DatabasesToSearch, UCombatAnimSetup* CurrentActivatedSet,
	bool bFastSearch, float SearchPropertyA, float SearchPropertyB, int DebugType, float DebugDuration)
{
	return false;
}


bool UMeleeCombatCoreComponent::RunSearchingAnimSetEnvContext_Implementation(UCombatAnimSetup*& ReturnAnimSetup, bool bFastSearch, int DebugType, float DebugDuration)
{
	return false;
}


bool UMeleeCombatCoreComponent::RunSearchingBestHitReactionAnim_Implementation(UAnimSequenceBase*& ReturnAnimSeq, float& ReturnPlayRate, float& ReturnStartTime, ACharacter* InCharacter,
	const TArray<UCombatHitReactionsDatabase*>& DatabasesToSearch, FHitResult HitReactionResult, bool bFastSearch, FRotator CustomInCharRot, float SearchPropertyB, int DebugType, float DebugDuration)
{
	if (!InCharacter || DatabasesToSearch.Num() == 0) return false;

	TArray<float> DistancesArray;
	TArray<float> DistancesArray2;
	TArray<float> AnglesArray;
	TArray<float> FinalWeigths;
	TArray<UCombatHitReactionsDatabase*> DatabasesArray;
	TArray<UAnimSequenceBase*> ValidAnimsArray;

	UCombatHitReactionsDatabase* CurrentDatabase = nullptr;
	UAnimSequenceBase* CurrentAnim = nullptr;

#if WITH_EDITOR
	TArray<FTransform> SavedHitTransforms;
#endif // WITH_EDITOR


	float HitDistanceMin = 9999;
	float HitDistanceMax = -9999;
	float RootDistanceMin = 9999;
	float RootDistanceMax = -9999;

	//For Finding Final Min Weight
	float MinFinalWeight = 999;
	int MinFinalIndex = -1;

	FRotator InCharacterRot = InCharacter->GetActorRotation();
	if (!CustomInCharRot.IsNearlyZero(0.01))
	{
		InCharacterRot = CustomInCharRot;
	}

	const FRotator InCharacterRootBoneRot = KML::MakeRotFromX(KML::GetRightVector(InCharacterRot) * -1.0);
	const FTransform ToCharacterSpace = FTransform(InCharacterRootBoneRot, HFL::GetPlayerCapsuleStartLocation(InCharacter, InCharacter), FVector(1, 1, 1));

	for (int i = 0; i < DatabasesToSearch.Num(); i++)
	{
		if (DatabasesToSearch[i]->HitReactionsAssets.Num() == 0) continue;
		if (DatabasesToSearch[i]->HitReactionsAssets.Num() != DatabasesToSearch[i]->GetHitBonePositionPerAnim().Num()) return false; //Error

		CurrentDatabase = DatabasesToSearch[i];

		for (int ii = 0; ii < CurrentDatabase->HitReactionsAssets.Num(); ii++)
		{
			CurrentAnim = CurrentDatabase->HitReactionsAssets[ii];
			if (!CurrentAnim) continue;

			const FTransform HitPositionRefWS = KML::ComposeTransforms(CurrentDatabase->GetHitBonePositionPerAnim()[ii], ToCharacterSpace);

			const float CurrentDist = FVector::Distance(HitPositionRefWS.GetLocation(), HitReactionResult.ImpactPoint);
			float CurrentAngle = FVector::DotProduct(KML::GetUpVector(HitPositionRefWS.Rotator()), HitReactionResult.Normal * -1.0);
			CurrentAngle = KML::MapRangeClamped(CurrentAngle , -0.2, 1.0, 1.0, 0.0);

			DistancesArray.Add(CurrentDist);
			AnglesArray.Add(CurrentAngle);

			DatabasesArray.Add(CurrentDatabase);
			ValidAnimsArray.Add(CurrentAnim);

			//Hit Distance
			if (CurrentDist > HitDistanceMax) //Hold Max Distance
			{
				HitDistanceMax = CurrentDist;
			}
			if (CurrentDist < HitDistanceMin) //Hold Min Distance
			{
				HitDistanceMin = CurrentDist;
			}

			//Root Distance
			if (CurrentDatabase->EndAnimPositionWeightScale > 0)
			{
				const FTransform EndAnimTransform = KML::ComposeTransforms(CurrentDatabase->GetRootBonePositionPerAnim()[ii], ToCharacterSpace);
				const float CurrentRootDistance = FVector::Dist2D(EndAnimTransform.GetLocation(), HFL::GetPlayerCapsuleStartLocation(RefChar, RefChar));

				DistancesArray2.Add(CurrentRootDistance);

				if (CurrentRootDistance > RootDistanceMax) //Hold Max Distance
				{
					RootDistanceMax = CurrentRootDistance;
				}
				if (CurrentRootDistance < RootDistanceMin) //Hold Min Distance
				{
					RootDistanceMin = CurrentRootDistance;
				}
			}

#if WITH_EDITOR
			SavedHitTransforms.Add(HitPositionRefWS);
#endif

		}
	}

	if (DistancesArray.Num() == 0) return false;

	for(int i2 = 0; i2 < DistancesArray.Num(); i2++)
	{
		const float CurrentDistanceNormalized = KML::MapRangeClamped(DistancesArray[i2], HitDistanceMin, HitDistanceMax, 0.0, 1.0); //Normalize HitDistance
		DistancesArray[i2] = CurrentDistanceNormalized;
		float NormalizedRootDistance = 0.0;
		if (DistancesArray2.IsValidIndex(i2))
		{
			NormalizedRootDistance = KML::MapRangeClamped(DistancesArray2[i2], RootDistanceMin, RootDistanceMax, 1.0, 0.0);
		}

		CurrentDatabase = DatabasesArray[i2];

		const float HitDistToAngle = FMath::Lerp<float>(CurrentDistanceNormalized, AnglesArray[i2], CurrentDatabase->DistanceToAngleAlpha);
		const float RandomBias = FMath::FRandRange(0.0, CurrentDatabase->RandomizationSearchingBias);

		const float CurrentFinalWeight = HitDistToAngle + CurrentDatabase->DatabaseBaseConstSearchBias + (NormalizedRootDistance * CurrentDatabase->EndAnimPositionWeightScale) + RandomBias;
		FinalWeigths.Add(CurrentFinalWeight);

		if (CurrentFinalWeight < MinFinalWeight)
		{
			MinFinalWeight = CurrentFinalWeight;
			MinFinalIndex = i2;
		}

#if WITH_EDITOR //DEBUG Section
		if (!SavedHitTransforms.IsValidIndex(i2)) continue;
		if (DebugType == 0) continue;

		const float DebugLifeTime = DebugDuration;
		const int DebugDepth = -1;
		const float ForColorLerp = KML::MapRangeClamped(CurrentFinalWeight, 0.0, 1.5, 1.0, 0.0);
		FColor PointColor = FMath::Lerp(
			FLinearColor(FColor(0, 255, 0, 255)),
			FLinearColor(FColor(255, 10, 20, 255)),
			ForColorLerp
		).ToFColor(true);

		DrawDebugPoint(RefChar->GetWorld(), SavedHitTransforms[i2].GetLocation(), 16, PointColor, false, DebugLifeTime, DebugDepth);
		DrawDebugLine(RefChar->GetWorld(), SavedHitTransforms[i2].GetLocation(), HitReactionResult.ImpactPoint, FColor(PointColor.R, PointColor.G, PointColor.B, 100), false, DebugLifeTime, DebugDepth, 0.4);

		DrawDebugString(RefChar->GetWorld(), SavedHitTransforms[i2].GetLocation() + FVector(0, 0, 4), FString::SanitizeFloat(CurrentFinalWeight), nullptr, PointColor, DebugLifeTime, true, 0.95);
		FColor DirectionLineColor = FColor::Blue;
		DrawDebugLine(RefChar->GetWorld(), SavedHitTransforms[i2].GetLocation(), SavedHitTransforms[i2].GetLocation() + (KML::GetUpVector(SavedHitTransforms[i2].Rotator()) * 7), 
			DirectionLineColor, false, DebugLifeTime, DebugDepth, 0.25);

		DrawDebugPoint(RefChar->GetWorld(), HitReactionResult.ImpactPoint, 14, FColor::Red, false, DebugLifeTime, DebugDepth);
#endif
	}

	if (MinFinalIndex >= 0) //Return Searcher Result
	{
		ReturnAnimSeq = ValidAnimsArray[MinFinalIndex];
		ReturnPlayRate = 1.0;
		ReturnStartTime = 0.0;
		return true;
	}

	ReturnPlayRate = 1.0;
	ReturnStartTime = 0.0;
	return false;
}

bool UMeleeCombatCoreComponent::TryLinkCombatAnimLayer_Implementation(bool bForEnemy)
{

	UAnimInstance* AnimInstance = RefChar->GetMesh()->GetAnimInstance();
	if (!AnimInstance)
		return false;

	if (MeleeCombatAnimLayer.IsNull())
		return false;

	// Jeśli klasa już jest załadowana -> link od razu
	if (UClass* LayerClass = MeleeCombatAnimLayer.Get())
	{
		LinkedAnimLayerTimeElapsed = DurationToWaitUntillRemoveAnim;
		// Opcjonalnie: jeśli już podlinkowane, nie rób nic
		if (AnimInstance->GetLinkedAnimLayerInstanceByClass(LayerClass))
			return true;

		AnimInstance->LinkAnimClassLayers(LayerClass);
		return true;
	}

	PendingAnimInstance = AnimInstance;

	if (bCombatLayerLoadInProgress) return false;

	bCombatLayerLoadInProgress = true;

	const FSoftObjectPath Path = MeleeCombatAnimLayer.ToSoftObjectPath();
	if (!Path.IsValid())
	{
		bCombatLayerLoadInProgress = false;
		PendingAnimInstance = nullptr;
		return false;
	}

	LinkedAnimLayerTimeElapsed = DurationToWaitUntillRemoveAnim;

	FStreamableManager& Streamable = UAssetManager::GetStreamableManager();

	CombatLayerLoadHandle = Streamable.RequestAsyncLoad(
		Path,
		FStreamableDelegate::CreateUObject(this, &UMeleeCombatCoreComponent::OnCombatAnimLayerClassLoaded),
		FStreamableManager::AsyncLoadHighPriority
	);

	return false; // jeszcze nie podlinkowano (zrobi to callback)
}


void UMeleeCombatCoreComponent::OnCombatAnimLayerClassLoaded()
{
	bCombatLayerLoadInProgress = false;
	// Uchwyt już niepotrzebny
	CombatLayerLoadHandle.Reset();
	UAnimInstance* AnimInstance = PendingAnimInstance;
	PendingAnimInstance = nullptr;

	if (!AnimInstance) return;

	// Component/Owner mógł zostać zniszczony — RefChar może być null
	if (!MeleeCombatAnimLayer.IsValid()) return;

	UClass* LayerClass = MeleeCombatAnimLayer.Get();
	if (!LayerClass || !LayerClass->IsChildOf(UAnimInstance::StaticClass())) { return; }
	if (AnimInstance->GetLinkedAnimLayerInstanceByClass(LayerClass)) { return; }
	AnimInstance->LinkAnimClassLayers(LayerClass);
}


void UMeleeCombatCoreComponent::UnlinkAnimLayer(bool ForEnemy)
{
	if (ForEnemy == false)
	{
		if (UClass* LayerClass = MeleeCombatAnimLayer.Get())
		{
			if (RefChar->GetMesh()->GetAnimInstance()->GetLinkedAnimLayerInstanceByClass(LayerClass))
			{
				RefChar->GetMesh()->GetAnimInstance()->UnlinkAnimClassLayers(LayerClass);
			}
		}
	}
	return;
}



//▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓
//▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓ 𝐑𝐔𝐍 𝐌𝐄𝐋𝐄𝐄 𝐀𝐓𝐓𝐀𝐂𝐊
//▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓
bool UMeleeCombatCoreComponent::StartMeleeAttackAction_Implementation()
{
	return false;
}


void UMeleeCombatCoreComponent::MoveActorsCapsulesTo_Implementation(float Duration, float StopTime, bool bEase)
{
}


bool UMeleeCombatCoreComponent::CheckEarlyBlendOutActionMontage_Implementation(UAnimSequenceBase* InAnimation, float InCurrentAnimTime, float InCurrentAnimTimeRatio, float InNotifyStateTime, bool InNotifyBlendOut)
{
	return false;
}


void UMeleeCombatCoreComponent::ActualAttackInputExecute_Implementation()
{
}




//░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░
bool UMeleeCombatCoreComponent::StartCustomWarpingPositions(float Duration, bool FInterpMode, float InterpSpeedLoc, float InterpSpeedRot, float GetPositionMaxTime, UCurveFloat* CustomInterpCurve)
{
	FTimerManager& TM = GetWorld()->GetTimerManager();

	if (TM.IsTimerActive(UpdatingWarpingTimer) == true) return false;

	InitWarpingPositionOwner = RefChar->GetActorTransform();
	if (GetChoosedEnemyChar()) 
	{ 
		InitWarpingPositionEnemy = GetChoosedEnemyChar()->GetActorTransform();
		OnWarpingStartEnemyPosition.Transform = GetChoosedEnemyChar()->GetActorTransform();
		OnWarpingStartEnemyPosition.Component = DesiredPositionEnemy.Component;
		if (bTryResolveToWorldSpace == true)
		{
			OnWarpingStartEnemyPosition = HFL::ConvertWorldToLocalFastMatrix(OnWarpingStartEnemyPosition);
		}
	}

	CustomWarpingCurve = CustomInterpCurve;

	UpdatingWarpingParams = FVector(InterpSpeedLoc, InterpSpeedRot, GetPositionMaxTime);

	OpponentMotionFollowScale = 1.0;

	UpdatingWarpingDuration = Duration;
	GetWorld()->GetTimerManager().SetTimer(
		UpdatingWarpingTimer,
		this,
		&UMeleeCombatCoreComponent::OnWapingUpdatingFinished,
		Duration,
		false
	);

	//Reset Accumulated positions;
	AccumulatedRootOffset = ZEROVEC;
	AccumulatedEnemyOffset = ZEROVEC;
	EnemyPositionDelta = ZEROVEC;

	return true;
}


//▁▂▃▄▅▆▇▆▅▄▃▂▁▂▃▄▅▆▇▆▅▄▃▂▁▂▃▄▅▆▇▆▅▄▃▂▁▂▃▄▅▆▇▆▅▄▃▂▁▂▃▄▅▆▇▆▅▄▃▂▁▂▃▄▅▆▇▆▅▄▃▂▁▂▃▄▅▆▇▆▅▄▃▂▁▂▃▄▅▆▇▆▅▄▃▂▁▂▃▄▅▆▇▆▅▄▃▂▁▂▃▄▅▆▇▆▅▄▃▂▁▂▃▄▅▆▇▆▅▄▃▂▁▂▃▄▅▆▇▆▅▄▃▂▂▃▄▅▆▇▆▅▄▃▂▂▃▄▅▆▇▆▅▄▃▂▂▃▄▅▆▇▆▅▄▃▂▂▃▄▅▆▇▆▅▄▃▂▂▃▄▅▆▇▆▅▄▃▂
//███████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████
//▝▝▝▝▝▝▝▝▝▝▝▝▝▝▝▝▝▝▝▝▝▝▝▝▝▝▝▝▝▝▝▝▝▝▝▝▝▝▝▝▝▝▝▝▝▝▝▝▝▝▝▝▝▝▝▝▝▝▝▝▝▝▝▝▝▝▝▝▝▝▝▝▝▝▝▝▝▝▝▝▝▝▝▝▝▝▝▝▝▝▝▝▝▝▝▝▝▝▝▝▝▝▝▝▝▝▝▝▝▝▝▝▝▝▝▝▝▝▝▝▝▝▝▝▝▝▝▝▝▝▝▝▝▝▝▝▝▝▝▝▝▝▝▝▝▝▝▝▝▝▝▝▝▝▝▝▝▝▝▝▝▝▝▝▝▝▝▝▝▝▝▝▝▝▝▝▝▝▝▝▝▝▝▝▝▝▝▝▝▝▝▝▝▝▝▝▝▝▝
void UMeleeCombatCoreComponent::UpdateCustomWarpingCharacters_Implementation(float dt, bool UseFInterpModeUpdating, bool ResolveToWorldSpace, float RootDeltaApplyScale)
{
	FTimerManager& TM = GetWorld()->GetTimerManager();
	if (TM.IsTimerActive(UpdatingWarpingTimer) == false) return;

	const float NormalizedTimeLinear = FMath::GetMappedRangeValueClamped(FVector2D(0.0, UpdatingWarpingDuration), FVector2D(0.0, 1.0), TM.GetTimerElapsed(UpdatingWarpingTimer));
	float NormalizedTime = NormalizedTimeLinear;

	FHitResult SweepResultOwner;
	const float DebugDuration = 8;

	if (CustomWarpingCurve)
	{
		NormalizedTime = CustomWarpingCurve->GetFloatValue(NormalizedTimeLinear);
	}
	else if (ChoosedAnimSetup)
	{
		NormalizedTime = CustomFloatInterp(0.0, 1.0, NormalizedTimeLinear, ChoosedAnimSetup->InterpFunction);
	}

	FTransform DesiredOwnerTransform = DesiredPositionOwner.Transform;
	FTransform DesiredEnemyTransform = DesiredPositionEnemy.Transform;
	if (ResolveToWorldSpace)
	{
		DesiredOwnerTransform = HFL::ConvertLocalToWorldFastMatrix(DesiredPositionOwner).Transform;
		DesiredEnemyTransform = HFL::ConvertLocalToWorldFastMatrix(DesiredPositionEnemy).Transform;
	}

	const FVector RootDeltaAcc = KML::SelectVector(AccumulatedRootOffset, FVector(0, 0, 0), 
		SolvingPositionType == CMaleeCombat_WarpingMotionType::AllAboveAndOriginOffset ||
		SolvingPositionType == CMaleeCombat_WarpingMotionType::TransformsAndRootMotion
	);

	if (UseFInterpModeUpdating) //░░  ░░  ░░  ░░  ░░  ░░  ░░  ░░  ░░  ░░  ░░  ░░  ░░  ░░  ░░  ░░  ░░  ░░  ░░  ░░  ░░  ░░  ░░  ░░  ░░  ░░  ░░  ░░  ░░  ░░													▎INTERP MODE ▎
	{
		if (WarpingTypeOwner != WarpTYPE::NoPositionUpdate)
		{
			const FVector NewLocationOwner = KML::VInterpTo(RefChar->GetActorLocation() - RootDeltaAcc, DesiredOwnerTransform.GetLocation(), dt, UpdatingWarpingParams.X);
			const FRotator NewRotationOwner = KML::RInterpTo(RefChar->GetActorRotation(), DesiredOwnerTransform.Rotator(), dt, UpdatingWarpingParams.Y);

			RefChar->SetActorLocation(FVector(NewLocationOwner.X, NewLocationOwner.Y, RefChar->GetActorLocation().Z) + RootDeltaAcc, true, &SweepResultOwner);
			if (WarpingTypeOwner != WarpTYPE::LocationOnly) { RefChar->SetActorRotation(NewRotationOwner); }
		}

		
		if (GetChoosedEnemyChar() && WarpingTypeOpponent != WarpTYPE::NoPositionUpdate)
		{
			const FVector NewLocationEnemy = KML::VInterpTo(GetChoosedEnemyChar()->GetActorLocation(), DesiredEnemyTransform.GetLocation(), dt, UpdatingWarpingParams.X);
			GetChoosedEnemyChar()->SetActorLocation(FVector(NewLocationEnemy.X, NewLocationEnemy.Y, GetChoosedEnemyChar()->GetActorLocation().Z), true);

			if (WarpingTypeOpponent != WarpTYPE::LocationOnly)
			{
				const FRotator NewRotationEnemy = KML::RInterpTo(GetChoosedEnemyChar()->GetActorRotation(), DesiredEnemyTransform.Rotator(), dt, UpdatingWarpingParams.Y);
				GetChoosedEnemyChar()->SetActorRotation(NewRotationEnemy);
			}
		}

	}
	else //░░  ░░  ░░  ░░  ░░  ░░  ░░  ░░  ░░  ░░  ░░  ░░  ░░  ░░  ░░  ░░  ░░  ░░  ░░  ░░  ░░  ░░  ░░  ░░  ░░  ░░  ░░  ░░  ░░  ░░  ░░  ░░  ░░  ░░  ░░  ░░  ░░  ░░  ░░										▎LERP MODE ▎
	{
		if (WarpingTypeOwner != WarpTYPE::NoPositionUpdate)
		{
			const FVector NewLocationOwner = KML::VLerp(InitWarpingPositionOwner.GetLocation(), DesiredOwnerTransform.GetLocation(), NormalizedTime);
			const FRotator NewRotationOwner = KML::RLerp(InitWarpingPositionOwner.Rotator(), DesiredOwnerTransform.Rotator(), NormalizedTime, true);

			const FVector OriginOffset = KML::SelectVector(AccumulatedEnemyOffset, FVector(0, 0, 0), SolvingPositionType == CMaleeCombat_WarpingMotionType::AllAboveAndOriginOffset );

			RefChar->SetActorLocation
			(
				ApplySafeMoveToLocation(
					RefChar, 
					FVector(NewLocationOwner.X, NewLocationOwner.Y, RefChar->GetActorLocation().Z) + 
					RootDeltaAcc + 
					OriginOffset, 
					1.0, 
					dt
				), 
				true, 
				&SweepResultOwner
			);

			if (WarpingTypeOwner != WarpTYPE::LocationOnly) 
			{ 
				RefChar->SetActorRotation(NewRotationOwner); 
				//GEngine->AddOnScreenDebugMessage(0, 0.5, FColor::Purple, NewRotationOwner.ToCompactString());
			}

			if (bDrawWarpingDebug) //▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄ ●
			{																																														//▐ D
				const float PositionZ = HFL::GetPlayerCapsuleStartLocation(RefChar, RefChar).Z;																										//▐ E
				DrawDebugPoint(GetWorld(), FVector(NewLocationOwner.X, NewLocationOwner.Y, PositionZ + 5), 6, FColor::Red, false, DebugDuration, 0);												//▐ B
				DrawDebugPoint(GetWorld(), FVector(NewLocationOwner.X, NewLocationOwner.Y, PositionZ + 7) + AccumulatedRootOffset, 8, FColor::Green, false, DebugDuration, 0);						//▐ U
				DrawDebugPoint(GetWorld(), FVector(NewLocationOwner.X, NewLocationOwner.Y, PositionZ + 9) + AccumulatedRootOffset + OriginOffset, 10, FColor::Cyan, false, DebugDuration, 0);		//▐ G
			}  //▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀ ●
		}

		if (GetChoosedEnemyChar() && WarpingTypeOpponent != WarpTYPE::NoPositionUpdate)
		{
			const FVector NewLocationEnemy = KML::VLerp(InitWarpingPositionEnemy.GetLocation(), DesiredEnemyTransform.GetLocation(), NormalizedTime);
			GetChoosedEnemyChar()->SetActorLocation(FVector(NewLocationEnemy.X, NewLocationEnemy.Y, GetChoosedEnemyChar()->GetActorLocation().Z), true);

			if (WarpingTypeOpponent != WarpTYPE::LocationOnly)
			{
				const FRotator NewRotationEnemy = KML::RLerp(InitWarpingPositionEnemy.Rotator(), DesiredEnemyTransform.Rotator(), NormalizedTime, true);
				GetChoosedEnemyChar()->SetActorRotation(NewRotationEnemy);
			}
		}

	}

	bool bCanAccumulate = true;
	if (SweepResultOwner.bBlockingHit == true)
	{
		bCanAccumulate = false;
	}

	//Calculated Aniamtion RootMotion Offset 
	if (RootDeltaApplyScale <= 0.0) return;

	if (!ChoosedAnimSetup) return;
	if (!ChoosedAnimSetup->Animation) return;

	float ElapsedTime = TM.GetTimerElapsed(UpdatingWarpingTimer);
	ElapsedTime = FMath::Clamp<float>(ElapsedTime, 0.0, ChoosedAnimSetup->Animation->GetPlayLength() - dt);

	UAnimSequenceBase* CurrentAnimSeq = nullptr;
	if (UAnimMontage* AsMontage = Cast<UAnimMontage>(ChoosedAnimSetup->Animation))
	{
		CurrentAnimSeq = HFL::GetAnimSequenceFromMontage(AsMontage, 0);
	}
	else
	{
		CurrentAnimSeq = ChoosedAnimSetup->Animation;
	}
	//▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁
	const FRotator ActorRot = RefChar->GetActorRotation();																										//┃ R
																																								//┃ O
	FAnimPoseEvaluationConfig EvaluationOptions;																												//┃ O
	const FVector PosA = HFL::GetBonePositionAtTimeFromSeq(CurrentAnimSeq, ElapsedTime, TEXT("root"), EvaluationOptions).GetLocation();							//┃ T
	const FVector PosB = HFL::GetBonePositionAtTimeFromSeq(CurrentAnimSeq, ElapsedTime + dt, TEXT("root"), EvaluationOptions).GetLocation();					//┃
	FVector Delta = PosB - PosA;																																//┃ D
	Delta = FVector(Delta.Y * 1.0, Delta.X * -1.0, 0.0);																										//┃ E
	Delta = KML::Quat_RotateVector(ActorRot.Quaternion(), Delta) * RootDeltaApplyScale;																			//┃ L
																																								//┃ T
	//RefChar->AddActorWorldOffset(Delta, true);																												//┃ A
	if(bCanAccumulate) AccumulatedRootOffset = AccumulatedRootOffset + Delta;																					//┃
	//▔▔▔▔▔▔▔▔▔▔▔▔▔▔▔▔▔▔▔▔▔▔▔▔▔▔▔▔▔▔▔▔▔▔▔▔▔▔▔▔▔▔▔▔▔▔▔▔▔▔▔▔▔▔▔▔▔▔▔▔▔▔▔▔▔▔▔▔▔▔▔▔▔▔▔▔▔▔▔▔▔▔▔▔▔▔▔▔▔▔▔▔▔▔▔▔▔▔▔▔▔▔▔▔▔▔▔▔▔▔▔▔▔▔▔▔▔▔▔▔▔▔▔▔▔▔▔▔▔▔▔▔▔▔▔▔▔▔▔▔▔▔▔▔▔▔▔▔▔▔▔▔▔▔▔▔

	//Calculate Enemy Offset
	if (!GetChoosedEnemyChar()) return; //════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════╗
	if (CustomWarpingCenterAlpha > 0.99 && UseFInterpModeUpdating == false && SolvingPositionType == CMaleeCombat_WarpingMotionType::AllAboveAndOriginOffset)	//║ O
	{																																							//║ P
		FVector SolvedInitPosition = FVector::ZeroVector;																										//║ P
		if (ResolveToWorldSpace)																																//║ O
		{																																						//║ N
			SolvedInitPosition = HFL::ConvertLocalToWorldFastMatrix(OnWarpingStartEnemyPosition).Transform.GetLocation();										//║ E 
		}																																						//║ N 
		else																																					//║ T
		{																																						//║
			SolvedInitPosition = OnWarpingStartEnemyPosition.Transform.GetLocation();																			//║ M
		}																																						//║ O
		//════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════╣	T																																	
		if (OpponentMotionSolverType == AccumulatedSOLVER)																										//║ I
		{																																						//║ O
			EnemyPositionDelta = (GetChoosedEnemyChar()->GetActorLocation() - SolvedInitPosition) * OpponentMotionFollowScale;									//║ N
			if (EnemyPositionDelta.Length() > MaxDeltaBetweenPrevPosition) { EnemyPositionDelta = ZEROVEC; }													//║
			if (bCanAccumulate) AccumulatedEnemyOffset += EnemyPositionDelta;																					//║
			if (ResolveToWorldSpace)																															//║
			{																																					//║
				OnWarpingStartEnemyPosition.Transform = GetChoosedEnemyChar()->GetActorTransform();																//║
				OnWarpingStartEnemyPosition = HFL::ConvertWorldToLocalFastMatrix(OnWarpingStartEnemyPosition);													//║
			}																																					//║
			else																																				//║
			{																																					//║
				OnWarpingStartEnemyPosition.Transform = GetChoosedEnemyChar()->GetActorTransform();																//║
			}																																					//║
		}																																						//║
		else if(OpponentMotionSolverType == InitPositionSOLVER)	//════════════════════════════════════════════════════════════════════════════════════════════════╣
		{																																						//║
			const float Dist2D = FVector::Dist2D(GetChoosedEnemyChar()->GetActorLocation(), SolvedInitPosition);												//║
			if (Dist2D > 0.2 && Dist2D < 200 && OpponentMotionFollowScale > 0.5)																				//║
			{																																					//║
				AccumulatedEnemyOffset = GetChoosedEnemyChar()->GetActorLocation() - SolvedInitPosition;														//║
			}																																					//║
		} //══════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════╝

	}

}


float UMeleeCombatCoreComponent::CustomFloatInterp(float A, float B, float Alpha, CustomWarpingInterpFunction FunctionType)
{
	switch (FunctionType)
	{
	case CustomWarpingInterpFunction::Linear:
		return FMath::Lerp<float>(A, B, Alpha);
	case CustomWarpingInterpFunction::EaseIn:
		return FMath::InterpEaseIn(A, B, Alpha, 2);
	case CustomWarpingInterpFunction::EaseOut:
		return FMath::InterpEaseOut(A, B, Alpha, 2);
	case CustomWarpingInterpFunction::EaseInOut:
		return FMath::InterpEaseInOut(A, B, Alpha, 2);
	case CustomWarpingInterpFunction::CircularIn:
		return FMath::InterpCircularIn(A, B, Alpha);
	case CustomWarpingInterpFunction::CircularOut:
		return FMath::InterpCircularOut(A, B, Alpha);
	case CustomWarpingInterpFunction::CircularInOut:
		return FMath::InterpCircularInOut(A, B, Alpha);
	case CustomWarpingInterpFunction::ExpoOut:
		return FMath::InterpExpoOut(A, B, Alpha);
	default:
		return FMath::Lerp<float>(A, B, Alpha);
	}
}


bool UMeleeCombatCoreComponent::StopCustomWarpingUpdating()
{
	FTimerManager& TM = GetWorld()->GetTimerManager();
	if (TM.IsTimerActive(UpdatingWarpingTimer) == true)
	{
		TM.ClearTimer(UpdatingWarpingTimer);
		return true;
	}
	return false;
}


bool UMeleeCombatCoreComponent::GetCustomWarpingElapsedTime(float& ReturnTime)
{
	FTimerManager& TM = GetWorld()->GetTimerManager();
	if (TM.IsTimerActive(UpdatingWarpingTimer))
	{
		ReturnTime = TM.GetTimerElapsed(UpdatingWarpingTimer);
		return true;
	}
	ReturnTime = -1;
	return false;
}


void UMeleeCombatCoreComponent::SetCustomWarpingFollowOpponentMotion(float MotionFollowingScale)
{
	OpponentMotionFollowScale = MotionFollowingScale;
}


void UMeleeCombatCoreComponent::CalculateRquiredWarpingPositions(bool& ReturnSolved, FCALS_ComponentAndTransform& ReturnAttacker, FCALS_ComponentAndTransform& ReturnOpponent, 
	float FindingFloorTraceUp, float FindingFloorTraceDown, float FreeSpaceRadiusCheck, float FreeSpaceHeightCheck)
{
	if (!ChoosedAnimSetup) return;

	FTransform AttackOrigin = FTransform::Identity;

	//Then 1
	const FCombatAnimTrajectorySavedData* TrajectoryData = ChoosedAnimSetup->GetAssetCalculatedData().Find("MainRef");
	if (TrajectoryData->SampledBoneTransforms.Num() == 0) return;

	FTransform RefCharPositionFromAnim = TrajectoryData->SampledBoneTransforms[0];
	const float RefCharPositionFromAnimZ = RefCharPositionFromAnim.GetLocation().Z;

	float InvertRotation = 0.0;
	if (IsValid(GetChoosedEnemyChar()))
	{
		InvertRotation = 180;
	}
	RefCharPositionFromAnim.SetLocation(FVector(RefCharPositionFromAnim.GetLocation().Y, RefCharPositionFromAnim.GetLocation().X, 0.0)); //Invert Axes
	RefCharPositionFromAnim.SetRotation(FRotator(0, RefCharPositionFromAnim.Rotator().Yaw - InvertRotation, 0).Quaternion());

	//Then 2
	const ACharacter* Opponent = GetChoosedEnemyChar();
	const FVector PositionOwner = HFL::GetPlayerCapsuleStartLocation(RefChar, RefChar);
	const FVector PositionOpponent = HFL::GetPlayerCapsuleStartLocation(Opponent, Opponent);
	const FVector DefScale = FVector(1, 1, 1);

	if (GetChoosedEnemyChar())
	{

		const FRotator LookAtEnemy = FRotator(0, KML::FindLookAtRotation(PositionOwner, PositionOpponent).Yaw, 0);
		
		const FTransform RefPosition01WS = RefCharPositionFromAnim * FTransform(LookAtEnemy, PositionOwner, DefScale);
		const FTransform RefPosition02WS = KML::TLerp(RefCharPositionFromAnim, FTransform::Identity, CustomWarpingCenterAlpha) * FTransform(LookAtEnemy, PositionOpponent, DefScale);

		AttackOrigin = KML::TLerp(RefPosition01WS, RefPosition02WS, CustomWarpingCenterAlpha);

		TArray<AActor*> ToIgnore = { RefChar, GetChoosedEnemyChar() };
		FVector CircleCenter = FVector::ZeroVector; float CircleRadius = 0.0; FVector CircleNormal = FVector(1, 0, 0);
		HFL::FindNearestCollisionByCylinder
		(
			CircleCenter, 
			CircleRadius, 
			CircleNormal, 
			RefChar, 
			FTransform(AttackOrigin.Rotator(), AttackOrigin.GetLocation() + FVector(0, 0, FreeSpaceHeightCheck + 15)), 
			ToIgnore, 
			FreeSpaceRadiusCheck, 
			FreeSpaceHeightCheck, 
			12
		);

		AttackOrigin.SetLocation(FVector(CircleCenter.X, CircleCenter.Y, AttackOrigin.GetLocation().Z));
	}
	else
	{
		AttackOrigin = FTransform(RefChar->GetActorRotation(), PositionOwner, DefScale);
	}

	//Then 3
	FCALS_ComponentAndTransform AsReturnAttacker;
	FCALS_ComponentAndTransform AsReturnOpponent;

	AsReturnAttacker.Transform = RefCharPositionFromAnim.Inverse() * AttackOrigin;
	AsReturnAttacker.Component = SimpleFindFloorComponentByTrace(AsReturnAttacker.Transform.GetLocation(), FindingFloorTraceUp, FindingFloorTraceDown);

	AsReturnOpponent.Transform = AttackOrigin;
	AsReturnOpponent.Component = SimpleFindFloorComponentByTrace(AttackOrigin.GetLocation(), FindingFloorTraceUp, FindingFloorTraceDown);

	//Then 4
	const FRotator FinalLooking = FRotator(0, KML::FindLookAtRotation(AsReturnAttacker.Transform.GetLocation(), AsReturnOpponent.Transform.GetLocation()).Yaw, 0);
	AsReturnAttacker.Transform.SetRotation(FinalLooking.Quaternion());
	AsReturnOpponent.Transform.SetRotation(KML::ComposeRotators(FinalLooking, RefCharPositionFromAnim.Rotator()).Quaternion());

	if (bTryResolveToWorldSpace)
	{
		AsReturnAttacker = HFL::ConvertWorldToLocalFastMatrix(AsReturnAttacker);
		AsReturnOpponent = HFL::ConvertWorldToLocalFastMatrix(AsReturnOpponent);
	}
	ReturnAttacker = AsReturnAttacker;
	ReturnOpponent = AsReturnOpponent;
	ReturnSolved = true;
	return;
}


UPrimitiveComponent* UMeleeCombatCoreComponent::SimpleFindFloorComponentByTrace(FVector CheckLocation, float TraceUp, float TraceDown)
{
	FHitResult FloorHit;
	ETraceTypeQuery TraceType = ETraceTypeQuery::TraceTypeQuery1;
	TArray<AActor*> ToIgnore;
	ToIgnore.Add(RefChar); ToIgnore.Add(GetTargetedCharacter());

	const bool HitValid = KSL::SphereTraceSingle(RefChar, CheckLocation + FVector(0, 0, TraceUp), CheckLocation - FVector(0, 0, TraceDown), 8, TraceType, 
		false, ToIgnore, EDrawDebugTrace::None, FloorHit, true, FColor::Black, FColor::Blue, 2.0);

	if (HitValid)
	{
		return FloorHit.GetComponent();
	}
	else if(GetTargetedCharacter())
	{
		return GetTargetedCharacter()->GetMesh();
	}
	else
	{
		return nullptr;
	}
}

FVector UMeleeCombatCoreComponent::ApplySafeMoveToLocation(AActor* InActor, FVector DesiredPosition, float InterpScale, float DeltaT)
{
	const float InterpSpeed = KML::MapRangeClamped(FVector::Distance(InActor->GetActorLocation(), DesiredPosition), 50.0, 200, 20.0, 4.0) * InterpScale;
	return KML::VInterpTo(InActor->GetActorLocation(), DesiredPosition, DeltaT, InterpSpeed);
}

//░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░




void UMeleeCombatCoreComponent::TryApplyDamageForOpponent_Implementation(bool HitValid, FHitResult HitResult, float NotifyTime, float AnimTime)
{
}


void UMeleeCombatCoreComponent::GetFocusingOnOpponentValues_Implementation(bool& RequireFocusing, FRotator& FocusingRotation, float& InterpSpeedBias, FRotator OptionalInDesiredRot)
{
}

void UMeleeCombatCoreComponent::RunPerOpponentGameplayTask_Implementation(bool bRun)
{
}



//╳╳╳╳╳╳╳╳╳╳╳╳╳╳╳╳╳╳╳╳╳╳╳╳╳╳╳╳╳╳╳╳╳╳╳╳╳╳╳╳╳╳╳╳╳╳╳╳╳╳╳╳╳╳╳╳╳╳╳╳╳╳╳╳╳╳╳╳╳╳╳╳╳╳╳╳╳╳╳╳╳╳╳╳╳╳╳╳╳╳╳╳╳╳╳╳╳ ➊ ᴄᴏɴsᴛ
ACharacter* UMeleeCombatCoreComponent::GetTargetedCharacter() const
{
	return TargetedEnemyData.TargetedCharacter;
}

//╳╳╳╳╳╳╳╳╳╳╳╳╳╳╳╳╳╳╳╳╳╳╳╳╳╳╳╳╳╳╳╳╳╳╳╳╳╳╳╳╳╳╳╳╳╳╳╳╳╳╳╳╳╳╳╳╳╳╳╳╳╳╳╳╳╳╳╳╳╳╳╳╳╳╳╳╳╳╳╳╳╳╳╳╳╳╳╳╳╳╳╳╳╳╳╳╳ ➋ ᴄᴏɴsᴛ
ACharacter* UMeleeCombatCoreComponent::GetChoosedEnemyChar() const
{
	return CurrentlyActiveEnemy.TargetedCharacter;
}

//╳╳╳╳╳╳╳╳╳╳╳╳╳╳╳╳╳╳╳╳╳╳╳╳╳╳╳╳╳╳╳╳╳╳╳╳╳╳╳╳╳╳╳╳╳╳╳╳╳╳╳╳╳╳╳╳╳╳╳╳╳╳╳╳╳╳╳╳╳╳╳╳╳╳╳╳╳╳╳╳╳╳╳╳╳╳╳╳╳╳╳╳╳╳╳╳╳ ➌ ᴄᴏɴsᴛ
float UMeleeCombatCoreComponent::GetOwnerLinkedAnimClassElapsedTime() const
{
	return LinkedAnimLayerTimeElapsed;
}

bool UMeleeCombatCoreComponent::GetShouldFocusOnOpponent() const
{
	return FocusingOnOpponentStrength > 0.1;
}

void UMeleeCombatCoreComponent::SimpleTimerUpdater(bool& IsRunning, bool& IsStopped, UPARAM(ref) float& InTimerVar, float Delta)
{
	if (InTimerVar <= -1.0) 
	{
		IsRunning = false;
		IsStopped = false;
		return;
	}

	InTimerVar = InTimerVar - Delta;

	if (InTimerVar <= 0.0)
	{
		IsRunning = false;
		IsStopped = true;
		return;
	}
	else
	{
		IsRunning = true;
		IsStopped = false;
		return;
	}
}


void UMeleeCombatCoreComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (CombatLayerLoadHandle.IsValid())
	{
		CombatLayerLoadHandle->CancelHandle();
		CombatLayerLoadHandle.Reset();
	}
	PendingAnimInstance = nullptr;
	bCombatLayerLoadInProgress = false;

	Super::EndPlay(EndPlayReason);
}
