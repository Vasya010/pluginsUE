#include "BTTask_ZonefallSmartMoveTo.h"

#include "AIController.h"
#include "BehaviorTree/BehaviorTreeComponent.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "BehaviorTree/Blackboard/BlackboardKeyType_Float.h"
#include "BehaviorTree/Blackboard/BlackboardKeyType_Object.h"
#include "BehaviorTree/Blackboard/BlackboardKeyType_Vector.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Pawn.h"
#include "Navigation/PathFollowingComponent.h"

UBTTask_ZonefallSmartMoveTo::UBTTask_ZonefallSmartMoveTo()
{
	NodeName = TEXT("Zonefall Smart Move To");
	bNotifyTick = true;
	INIT_TASK_NODE_NOTIFY_FLAGS();

	BlackboardKey.AddObjectFilter(this, GET_MEMBER_NAME_CHECKED(UBTTask_ZonefallSmartMoveTo, BlackboardKey), AActor::StaticClass());
	BlackboardKey.AddVectorFilter(this, GET_MEMBER_NAME_CHECKED(UBTTask_ZonefallSmartMoveTo, BlackboardKey));
	AcceptanceRadiusKey.AddFloatFilter(this, GET_MEMBER_NAME_CHECKED(UBTTask_ZonefallSmartMoveTo, AcceptanceRadiusKey));
	DesiredSpeedKey.AddFloatFilter(this, GET_MEMBER_NAME_CHECKED(UBTTask_ZonefallSmartMoveTo, DesiredSpeedKey));
}

void UBTTask_ZonefallSmartMoveTo::SetCodedBlackboardKey(const FBlackboardKeySelector& InBlackboardKey)
{
	BlackboardKey = InBlackboardKey;
}

uint16 UBTTask_ZonefallSmartMoveTo::GetInstanceMemorySize() const
{
	return sizeof(FZonefallSmartMoveMemory);
}

EBTNodeResult::Type UBTTask_ZonefallSmartMoveTo::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	FZonefallSmartMoveMemory& MoveMemory = *reinterpret_cast<FZonefallSmartMoveMemory*>(NodeMemory);
	MoveMemory = FZonefallSmartMoveMemory();

	return RequestMove(OwnerComp, MoveMemory);
}

void UBTTask_ZonefallSmartMoveTo::TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
	FZonefallSmartMoveMemory& MoveMemory = *reinterpret_cast<FZonefallSmartMoveMemory*>(NodeMemory);
	AAIController* AIController = OwnerComp.GetAIOwner();
	APawn* Pawn = AIController ? AIController->GetPawn() : nullptr;
	if (!AIController || !Pawn || !MoveMemory.bHadValidGoal)
	{
		RestoreDesiredSpeed(OwnerComp, MoveMemory);
		FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
		return;
	}

	FVector GoalLocation = MoveMemory.LastGoalLocation;
	AActor* GoalActor = nullptr;
	if (!ResolveGoal(OwnerComp, GoalLocation, GoalActor))
	{
		RestoreDesiredSpeed(OwnerComp, MoveMemory);
		FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
		return;
	}

	UBlackboardComponent* Blackboard = OwnerComp.GetBlackboardComponent();
	const float ResolvedAcceptanceRadius = GetResolvedAcceptanceRadius(Blackboard);
	if (IsAtGoal(*Pawn, GoalLocation, ResolvedAcceptanceRadius))
	{
		AIController->StopMovement();
		RestoreDesiredSpeed(OwnerComp, MoveMemory);
		FinishLatentTask(OwnerComp, EBTNodeResult::Succeeded);
		return;
	}

	if (bTrackMovingGoalActor && GoalActor && FVector::DistSquared(GoalLocation, MoveMemory.LastGoalLocation) > FMath::Square(RepathDistance))
	{
		RequestMove(OwnerComp, MoveMemory);
		return;
	}

	const UPathFollowingComponent* PathFollowingComponent = AIController->GetPathFollowingComponent();
	if (!PathFollowingComponent)
	{
		RestoreDesiredSpeed(OwnerComp, MoveMemory);
		FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
		return;
	}

	const EPathFollowingStatus::Type MoveStatus = PathFollowingComponent->GetStatus();
	if (MoveMemory.bMoveStarted && MoveStatus == EPathFollowingStatus::Idle)
	{
		const EBTNodeResult::Type Result = IsAtGoal(*Pawn, GoalLocation, ResolvedAcceptanceRadius) ? EBTNodeResult::Succeeded : EBTNodeResult::Failed;
		RestoreDesiredSpeed(OwnerComp, MoveMemory);
		FinishLatentTask(OwnerComp, Result);
	}
}

EBTNodeResult::Type UBTTask_ZonefallSmartMoveTo::AbortTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	if (bStopMovementOnAbort)
	{
		if (AAIController* AIController = OwnerComp.GetAIOwner())
		{
			AIController->StopMovement();
		}
	}

	FZonefallSmartMoveMemory& MoveMemory = *reinterpret_cast<FZonefallSmartMoveMemory*>(NodeMemory);
	RestoreDesiredSpeed(OwnerComp, MoveMemory);

	return EBTNodeResult::Aborted;
}

bool UBTTask_ZonefallSmartMoveTo::ResolveGoal(UBehaviorTreeComponent& OwnerComp, FVector& OutGoalLocation, AActor*& OutGoalActor) const
{
	UBlackboardComponent* Blackboard = OwnerComp.GetBlackboardComponent();
	if (!Blackboard)
	{
		return false;
	}

	if (AActor* ActorGoal = Cast<AActor>(Blackboard->GetValueAsObject(BlackboardKey.SelectedKeyName)))
	{
		OutGoalActor = ActorGoal;
		OutGoalLocation = ActorGoal->GetActorLocation();
		return true;
	}

	if (BlackboardKey.SelectedKeyType == UBlackboardKeyType_Vector::StaticClass())
	{
		OutGoalActor = nullptr;
		OutGoalLocation = Blackboard->GetValueAsVector(BlackboardKey.SelectedKeyName);
		return !OutGoalLocation.IsNearlyZero();
	}

	return false;
}

float UBTTask_ZonefallSmartMoveTo::GetResolvedAcceptanceRadius(const UBlackboardComponent* Blackboard) const
{
	if (Blackboard && AcceptanceRadiusKey.SelectedKeyType == UBlackboardKeyType_Float::StaticClass())
	{
		const float BlackboardAcceptanceRadius = Blackboard->GetValueAsFloat(AcceptanceRadiusKey.SelectedKeyName);
		if (BlackboardAcceptanceRadius > 0.0f)
		{
			return BlackboardAcceptanceRadius;
		}
	}

	return AcceptanceRadius;
}

bool UBTTask_ZonefallSmartMoveTo::IsAtGoal(const APawn& Pawn, const FVector& GoalLocation, float ResolvedAcceptanceRadius) const
{
	return FVector::DistSquared2D(Pawn.GetActorLocation(), GoalLocation) <= FMath::Square(ResolvedAcceptanceRadius);
}

float UBTTask_ZonefallSmartMoveTo::GetResolvedDesiredSpeed(const UBlackboardComponent* Blackboard) const
{
	if (bAllowBlackboardDesiredSpeedOverride && Blackboard && DesiredSpeedKey.SelectedKeyType == UBlackboardKeyType_Float::StaticClass())
	{
		const float BlackboardSpeed = Blackboard->GetValueAsFloat(DesiredSpeedKey.SelectedKeyName);
		if (BlackboardSpeed > 0.0f)
		{
			return BlackboardSpeed;
		}
	}

	return DesiredSpeed;
}

void UBTTask_ZonefallSmartMoveTo::ApplyDesiredSpeed(APawn& Pawn, FZonefallSmartMoveMemory& MoveMemory, float ResolvedDesiredSpeed) const
{
	if (!bApplyDesiredSpeedToCharacterMovement || ResolvedDesiredSpeed <= 0.0f)
	{
		return;
	}

	ACharacter* Character = Cast<ACharacter>(&Pawn);
	UCharacterMovementComponent* MovementComponent = Character ? Character->GetCharacterMovement() : nullptr;
	if (!MovementComponent)
	{
		return;
	}

	if (!MoveMemory.bAppliedMovementSpeed)
	{
		MoveMemory.PreviousMaxWalkSpeed = MovementComponent->MaxWalkSpeed;
		MoveMemory.bAppliedMovementSpeed = true;
	}

	MovementComponent->MaxWalkSpeed = ResolvedDesiredSpeed;
}

void UBTTask_ZonefallSmartMoveTo::RestoreDesiredSpeed(UBehaviorTreeComponent& OwnerComp, FZonefallSmartMoveMemory& MoveMemory) const
{
	if (!MoveMemory.bAppliedMovementSpeed)
	{
		return;
	}

	AAIController* AIController = OwnerComp.GetAIOwner();
	ACharacter* Character = AIController ? Cast<ACharacter>(AIController->GetPawn()) : nullptr;
	UCharacterMovementComponent* MovementComponent = Character ? Character->GetCharacterMovement() : nullptr;
	if (MovementComponent)
	{
		MovementComponent->MaxWalkSpeed = MoveMemory.PreviousMaxWalkSpeed;
	}

	MoveMemory.bAppliedMovementSpeed = false;
}

EBTNodeResult::Type UBTTask_ZonefallSmartMoveTo::RequestMove(UBehaviorTreeComponent& OwnerComp, FZonefallSmartMoveMemory& MoveMemory) const
{
	AAIController* AIController = OwnerComp.GetAIOwner();
	UBlackboardComponent* Blackboard = OwnerComp.GetBlackboardComponent();
	APawn* Pawn = AIController ? AIController->GetPawn() : nullptr;
	if (!AIController || !Blackboard || !Pawn)
	{
		return EBTNodeResult::Failed;
	}

	FVector GoalLocation = FVector::ZeroVector;
	AActor* GoalActor = nullptr;
	if (!ResolveGoal(OwnerComp, GoalLocation, GoalActor))
	{
		return EBTNodeResult::Failed;
	}

	MoveMemory.LastGoalLocation = GoalLocation;
	MoveMemory.bHadValidGoal = true;
	const float ResolvedAcceptanceRadius = GetResolvedAcceptanceRadius(Blackboard);
	const float ResolvedDesiredSpeed = GetResolvedDesiredSpeed(Blackboard);

	if (ResolvedDesiredSpeed > 0.0f && DesiredSpeedKey.SelectedKeyType == UBlackboardKeyType_Float::StaticClass())
	{
		Blackboard->SetValueAsFloat(DesiredSpeedKey.SelectedKeyName, ResolvedDesiredSpeed);
	}
	ApplyDesiredSpeed(*Pawn, MoveMemory, ResolvedDesiredSpeed);

	if (IsAtGoal(*Pawn, GoalLocation, ResolvedAcceptanceRadius))
	{
		RestoreDesiredSpeed(OwnerComp, MoveMemory);
		return EBTNodeResult::Succeeded;
	}

	FAIMoveRequest MoveRequest;
	MoveRequest.SetAcceptanceRadius(ResolvedAcceptanceRadius);
	MoveRequest.SetAllowPartialPath(bAllowPartialPath);
	MoveRequest.SetProjectGoalLocation(bProjectGoalToNavigation);
	MoveRequest.SetUsePathfinding(bUsePathfinding);
	MoveRequest.SetReachTestIncludesAgentRadius(bStopOnOverlap);
	MoveRequest.SetReachTestIncludesGoalRadius(bStopOnOverlap);

	if (GoalActor)
	{
		MoveRequest.SetGoalActor(GoalActor);
	}
	else
	{
		MoveRequest.SetGoalLocation(GoalLocation);
	}

	const FPathFollowingRequestResult Result = AIController->MoveTo(MoveRequest);
	switch (Result.Code)
	{
	case EPathFollowingRequestResult::AlreadyAtGoal:
		RestoreDesiredSpeed(OwnerComp, MoveMemory);
		return EBTNodeResult::Succeeded;
	case EPathFollowingRequestResult::RequestSuccessful:
		MoveMemory.bMoveStarted = true;
		return EBTNodeResult::InProgress;
	default:
		RestoreDesiredSpeed(OwnerComp, MoveMemory);
		return EBTNodeResult::Failed;
	}
}
