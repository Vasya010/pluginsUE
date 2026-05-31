#include "BTTask_ZonefallPickInvestigationPoint.h"

#include "AIController.h"
#include "BehaviorTree/BehaviorTreeComponent.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "BehaviorTree/Blackboard/BlackboardKeyType_Vector.h"
#include "NavigationSystem.h"
#include "ZonefallAIPerceptionMemoryComponent.h"

UBTTask_ZonefallPickInvestigationPoint::UBTTask_ZonefallPickInvestigationPoint()
{
	NodeName = TEXT("Zonefall Pick Investigation Point");
	MoveLocationKey.AddVectorFilter(this, GET_MEMBER_NAME_CHECKED(UBTTask_ZonefallPickInvestigationPoint, MoveLocationKey));
}

EBTNodeResult::Type UBTTask_ZonefallPickInvestigationPoint::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	AAIController* AIController = OwnerComp.GetAIOwner();
	UBlackboardComponent* Blackboard = OwnerComp.GetBlackboardComponent();
	if (!AIController || !Blackboard)
	{
		return EBTNodeResult::Failed;
	}

	FVector InvestigationLocation = Blackboard->GetValueAsVector(BlackboardKeys.InvestigationLocation);
	if (const UZonefallAIPerceptionMemoryComponent* MemoryComponent = AIController->FindComponentByClass<UZonefallAIPerceptionMemoryComponent>())
	{
		if (!MemoryComponent->InvestigationLocation.IsNearlyZero())
		{
			InvestigationLocation = MemoryComponent->InvestigationLocation;
		}
	}

	if (InvestigationLocation.IsNearlyZero())
	{
		InvestigationLocation = Blackboard->GetValueAsVector(BlackboardKeys.LastHeardLocation);
	}
	if (InvestigationLocation.IsNearlyZero())
	{
		InvestigationLocation = Blackboard->GetValueAsVector(BlackboardKeys.LastKnownTargetLocation);
	}
	if (InvestigationLocation.IsNearlyZero())
	{
		return EBTNodeResult::Failed;
	}

	if (ScatterRadius > 0.0f)
	{
		InvestigationLocation += FMath::VRand() * FMath::RandRange(0.0f, ScatterRadius);
	}

	if (bProjectToNavigation)
	{
		if (UNavigationSystemV1* NavigationSystem = UNavigationSystemV1::GetCurrent(AIController))
		{
			FNavLocation ProjectedLocation;
			if (NavigationSystem->ProjectPointToNavigation(InvestigationLocation, ProjectedLocation))
			{
				InvestigationLocation = ProjectedLocation.Location;
			}
		}
	}

	Blackboard->SetValueAsVector(BlackboardKeys.InvestigationLocation, InvestigationLocation);
	Blackboard->SetValueAsVector(MoveLocationKey.SelectedKeyName, InvestigationLocation);
	Blackboard->SetValueAsBool(BlackboardKeys.IsInvestigating, true);
	UZonefallAIBlackboardLibrary::SetZonefallThreatState(Blackboard, BlackboardKeys, EZonefallAIThreatState::Suspicious);
	return EBTNodeResult::Succeeded;
}
