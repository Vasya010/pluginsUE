#include "BTDecorator_ZonefallShouldInvestigate.h"

#include "AIController.h"
#include "BehaviorTree/BehaviorTreeComponent.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "ZonefallAIPerceptionMemoryComponent.h"

UBTDecorator_ZonefallShouldInvestigate::UBTDecorator_ZonefallShouldInvestigate()
{
	NodeName = TEXT("Zonefall Should Investigate");
}

bool UBTDecorator_ZonefallShouldInvestigate::CalculateRawConditionValue(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) const
{
	const UBlackboardComponent* Blackboard = OwnerComp.GetBlackboardComponent();
	if (!Blackboard)
	{
		return false;
	}

	AAIController* AIController = OwnerComp.GetAIOwner();
	const UZonefallAIPerceptionMemoryComponent* MemoryComponent = AIController ? AIController->FindComponentByClass<UZonefallAIPerceptionMemoryComponent>() : nullptr;
	if (MemoryComponent && MemoryComponent->HasActionableMemory() && !MemoryComponent->bHasLineOfSight)
	{
		return true;
	}

	const bool bShouldInvestigate = Blackboard->GetValueAsBool(BlackboardKeys.ShouldInvestigate);
	const float Suspicion = Blackboard->GetValueAsFloat(BlackboardKeys.Suspicion);
	const float Confidence = Blackboard->GetValueAsFloat(BlackboardKeys.MemoryConfidence);
	const FVector InvestigationLocation = Blackboard->GetValueAsVector(BlackboardKeys.InvestigationLocation);
	return bShouldInvestigate && !InvestigationLocation.IsNearlyZero() && (Suspicion >= MinimumSuspicion || Confidence >= MinimumConfidence);
}
