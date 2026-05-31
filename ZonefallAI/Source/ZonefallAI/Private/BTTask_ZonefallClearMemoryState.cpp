#include "BTTask_ZonefallClearMemoryState.h"

#include "AIController.h"
#include "BehaviorTree/BehaviorTreeComponent.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "ZonefallAIPerceptionMemoryComponent.h"

UBTTask_ZonefallClearMemoryState::UBTTask_ZonefallClearMemoryState()
{
	NodeName = TEXT("Zonefall Clear Memory State");
}

EBTNodeResult::Type UBTTask_ZonefallClearMemoryState::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	UBlackboardComponent* Blackboard = OwnerComp.GetBlackboardComponent();
	if (!Blackboard)
	{
		return EBTNodeResult::Failed;
	}

	if (bClearComponentMemory)
	{
		if (AAIController* AIController = OwnerComp.GetAIOwner())
		{
			if (UZonefallAIPerceptionMemoryComponent* MemoryComponent = AIController->FindComponentByClass<UZonefallAIPerceptionMemoryComponent>())
			{
				MemoryComponent->ClearMemory();
			}
		}
	}

	Blackboard->ClearValue(BlackboardKeys.TargetActor);
	Blackboard->ClearValue(BlackboardKeys.FocusActor);
	Blackboard->ClearValue(BlackboardKeys.LastHeardLocation);
	Blackboard->ClearValue(BlackboardKeys.InvestigationLocation);
	Blackboard->SetValueAsBool(BlackboardKeys.HasTarget, false);
	Blackboard->SetValueAsBool(BlackboardKeys.HasLineOfSight, false);
	Blackboard->SetValueAsBool(BlackboardKeys.ShouldInvestigate, false);
	Blackboard->SetValueAsBool(BlackboardKeys.IsInvestigating, false);
	Blackboard->SetValueAsFloat(BlackboardKeys.MemoryConfidence, 0.0f);
	Blackboard->SetValueAsFloat(BlackboardKeys.Suspicion, 0.0f);
	Blackboard->SetValueAsFloat(BlackboardKeys.TimeSinceLastStimulus, 0.0f);
	Blackboard->SetValueAsEnum(BlackboardKeys.StimulusType, static_cast<uint8>(EZonefallAIStimulusType::None));
	UZonefallAIBlackboardLibrary::SetZonefallThreatState(Blackboard, BlackboardKeys, EZonefallAIThreatState::Passive);
	return EBTNodeResult::Succeeded;
}
