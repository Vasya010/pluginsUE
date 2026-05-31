#include "BTTask_ZonefallMoveToCover.h"

#include "AIController.h"
#include "BehaviorTree/BehaviorTreeComponent.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "GameFramework/Pawn.h"
#include "ZonefallAITacticalCoverComponent.h"

UBTTask_ZonefallMoveToCover::UBTTask_ZonefallMoveToCover()
{
	NodeName = TEXT("Zonefall Find Cover");
}

EBTNodeResult::Type UBTTask_ZonefallMoveToCover::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	AAIController* Controller = OwnerComp.GetAIOwner();
	UBlackboardComponent* Blackboard = OwnerComp.GetBlackboardComponent();
	if (!Controller || !Blackboard)
	{
		return EBTNodeResult::Failed;
	}

	APawn* Pawn = Controller->GetPawn();
	if (!Pawn)
	{
		return EBTNodeResult::Failed;
	}

	UZonefallAITacticalCoverComponent* CoverComponent = Controller->FindComponentByClass<UZonefallAITacticalCoverComponent>();
	if (!CoverComponent)
	{
		CoverComponent = Pawn->FindComponentByClass<UZonefallAITacticalCoverComponent>();
	}
	if (!CoverComponent)
	{
		return EBTNodeResult::Failed;
	}

	AActor* TargetActor = Cast<AActor>(Blackboard->GetValueAsObject(BlackboardKeys.TargetActor));
	if (!TargetActor)
	{
		const FVector LastKnown = Blackboard->GetValueAsVector(BlackboardKeys.LastKnownTargetLocation);
		if (LastKnown.IsNearlyZero())
		{
			return EBTNodeResult::Failed;
		}
	}

	if (!CoverComponent->RequestCoverFromTarget(TargetActor, bForceRefresh))
	{
		return EBTNodeResult::Failed;
	}

	if (bAlsoWriteMoveLocation)
	{
		Blackboard->SetValueAsVector(BlackboardKeys.MoveLocation, CoverComponent->CurrentCoverLocation);
	}

	return EBTNodeResult::Succeeded;
}
