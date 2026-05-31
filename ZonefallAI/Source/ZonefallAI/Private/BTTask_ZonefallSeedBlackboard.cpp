#include "BTTask_ZonefallSeedBlackboard.h"

#include "AIController.h"
#include "BehaviorTree/BehaviorTreeComponent.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "GameFramework/Pawn.h"

UBTTask_ZonefallSeedBlackboard::UBTTask_ZonefallSeedBlackboard()
{
	NodeName = TEXT("Zonefall Seed Blackboard");
}

EBTNodeResult::Type UBTTask_ZonefallSeedBlackboard::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	AAIController* AIController = OwnerComp.GetAIOwner();
	UBlackboardComponent* Blackboard = OwnerComp.GetBlackboardComponent();
	APawn* Pawn = AIController ? AIController->GetPawn() : nullptr;
	if (!Blackboard || !Pawn)
	{
		return EBTNodeResult::Failed;
	}

	if (!Blackboard->GetValueAsVector(BlackboardKeys.HomeLocation).IsNearlyZero())
	{
		return EBTNodeResult::Succeeded;
	}

	const FVector PawnLocation = Pawn->GetActorLocation();
	Blackboard->SetValueAsVector(BlackboardKeys.HomeLocation, PawnLocation);
	Blackboard->SetValueAsVector(BlackboardKeys.PatrolAnchor, PawnLocation);
	Blackboard->SetValueAsFloat(BlackboardKeys.PatrolRadius, PatrolRadius);
	Blackboard->SetValueAsFloat(BlackboardKeys.SearchRadius, SearchRadius);
	Blackboard->SetValueAsBool(BlackboardKeys.HasTarget, false);
	Blackboard->SetValueAsBool(BlackboardKeys.IsInvestigating, false);
	Blackboard->SetValueAsFloat(BlackboardKeys.Alertness, 0.0f);
	UZonefallAIBlackboardLibrary::SetZonefallThreatState(Blackboard, BlackboardKeys, EZonefallAIThreatState::Passive);

	return EBTNodeResult::Succeeded;
}
