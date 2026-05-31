#include "BTTask_ZonefallFindPatrolPoint.h"

#include "AIController.h"
#include "BehaviorTree/BehaviorTreeComponent.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "BehaviorTree/Blackboard/BlackboardKeyType_Float.h"
#include "BehaviorTree/Blackboard/BlackboardKeyType_Vector.h"
#include "GameFramework/Pawn.h"
#include "NavigationSystem.h"
#include "ZonefallAIPatrolComponent.h"

UBTTask_ZonefallFindPatrolPoint::UBTTask_ZonefallFindPatrolPoint()
{
	NodeName = TEXT("Zonefall Find Patrol Point");
	MoveLocationKey.AddVectorFilter(this, GET_MEMBER_NAME_CHECKED(UBTTask_ZonefallFindPatrolPoint, MoveLocationKey));
	AnchorLocationKey.AddVectorFilter(this, GET_MEMBER_NAME_CHECKED(UBTTask_ZonefallFindPatrolPoint, AnchorLocationKey));
	RadiusKey.AddFloatFilter(this, GET_MEMBER_NAME_CHECKED(UBTTask_ZonefallFindPatrolPoint, RadiusKey));
	AcceptanceRadiusKey.AddFloatFilter(this, GET_MEMBER_NAME_CHECKED(UBTTask_ZonefallFindPatrolPoint, AcceptanceRadiusKey));
	WaitTimeKey.AddFloatFilter(this, GET_MEMBER_NAME_CHECKED(UBTTask_ZonefallFindPatrolPoint, WaitTimeKey));
}

EBTNodeResult::Type UBTTask_ZonefallFindPatrolPoint::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	AAIController* AIController = OwnerComp.GetAIOwner();
	UBlackboardComponent* Blackboard = OwnerComp.GetBlackboardComponent();
	APawn* Pawn = AIController ? AIController->GetPawn() : nullptr;
	if (!AIController || !Blackboard || !Pawn)
	{
		return EBTNodeResult::Failed;
	}

	if (UZonefallAIPatrolComponent* PatrolComponent = Pawn->FindComponentByClass<UZonefallAIPatrolComponent>())
	{
		FVector PatrolLocation = FVector::ZeroVector;
		float PointAcceptanceRadius = 0.0f;
		float PointWaitTime = 0.0f;
		if (PatrolComponent->GetNextPatrolLocation(PatrolLocation, PointAcceptanceRadius, PointWaitTime))
		{
			Blackboard->SetValueAsVector(MoveLocationKey.SelectedKeyName, PatrolLocation);
			Blackboard->SetValueAsVector(AnchorLocationKey.SelectedKeyName, PatrolComponent->GetPatrolAnchor());
			Blackboard->SetValueAsFloat(RadiusKey.SelectedKeyName, PatrolComponent->PatrolRadius);
			Blackboard->SetValueAsFloat(AcceptanceRadiusKey.SelectedKeyName, PointAcceptanceRadius);
			Blackboard->SetValueAsFloat(WaitTimeKey.SelectedKeyName, PointWaitTime);
			return EBTNodeResult::Succeeded;
		}
	}

	const FVector PawnLocation = Pawn->GetActorLocation();
	FVector AnchorLocation = Blackboard->GetValueAsVector(AnchorLocationKey.SelectedKeyName);
	if (AnchorLocation.IsNearlyZero() && bUsePawnLocationWhenAnchorMissing)
	{
		AnchorLocation = PawnLocation;
	}

	float Radius = FallbackRadius;
	if (RadiusKey.SelectedKeyType == UBlackboardKeyType_Float::StaticClass())
	{
		const float BlackboardRadius = Blackboard->GetValueAsFloat(RadiusKey.SelectedKeyName);
		Radius = BlackboardRadius > 0.0f ? BlackboardRadius : FallbackRadius;
	}

	UNavigationSystemV1* NavigationSystem = UNavigationSystemV1::GetCurrent(Pawn);
	if (!NavigationSystem || Radius <= 0.0f)
	{
		return EBTNodeResult::Failed;
	}

	FNavLocation BestLocation;
	bool bFoundLocation = false;
	float BestDistance = 0.0f;

	for (int32 Attempt = 0; Attempt < MaxAttempts; ++Attempt)
	{
		FNavLocation CandidateLocation;
		if (!NavigationSystem->GetRandomReachablePointInRadius(AnchorLocation, Radius, CandidateLocation))
		{
			continue;
		}

		const float DistanceFromPawn = FVector::Dist(PawnLocation, CandidateLocation.Location);
		if (DistanceFromPawn >= MinDistanceFromPawn)
		{
			BestLocation = CandidateLocation;
			bFoundLocation = true;
			break;
		}

		if (!bFoundLocation || DistanceFromPawn > BestDistance)
		{
			BestLocation = CandidateLocation;
			BestDistance = DistanceFromPawn;
			bFoundLocation = true;
		}
	}

	if (!bFoundLocation)
	{
		return EBTNodeResult::Failed;
	}

	Blackboard->SetValueAsVector(MoveLocationKey.SelectedKeyName, BestLocation.Location);
	return EBTNodeResult::Succeeded;
}
