#include "BTDecorator_ZonefallHasLineOfSight.h"

#include "AIController.h"
#include "BehaviorTree/BehaviorTreeComponent.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "BehaviorTree/Blackboard/BlackboardKeyType_Object.h"
#include "BehaviorTree/Blackboard/BlackboardKeyType_Vector.h"
#include "GameFramework/Pawn.h"

UBTDecorator_ZonefallHasLineOfSight::UBTDecorator_ZonefallHasLineOfSight()
{
	NodeName = TEXT("Zonefall Has Line Of Sight");
	BlackboardKey.AddObjectFilter(this, GET_MEMBER_NAME_CHECKED(UBTDecorator_ZonefallHasLineOfSight, BlackboardKey), AActor::StaticClass());
	BlackboardKey.AddVectorFilter(this, GET_MEMBER_NAME_CHECKED(UBTDecorator_ZonefallHasLineOfSight, BlackboardKey));
}

void UBTDecorator_ZonefallHasLineOfSight::SetCodedBlackboardKey(const FBlackboardKeySelector& InBlackboardKey)
{
	BlackboardKey = InBlackboardKey;
}

bool UBTDecorator_ZonefallHasLineOfSight::CalculateRawConditionValue(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) const
{
	AAIController* AIController = OwnerComp.GetAIOwner();
	UBlackboardComponent* Blackboard = OwnerComp.GetBlackboardComponent();
	APawn* Pawn = AIController ? AIController->GetPawn() : nullptr;
	if (!AIController || !Blackboard || !Pawn)
	{
		return false;
	}

	if (AActor* TargetActor = Cast<AActor>(Blackboard->GetValueAsObject(BlackboardKey.SelectedKeyName)))
	{
		const float Distance = FVector::Dist(Pawn->GetActorLocation(), TargetActor->GetActorLocation());
		return (MaxDistance <= 0.0f || Distance <= MaxDistance) && AIController->LineOfSightTo(TargetActor);
	}

	if (bAcceptVectorKey && BlackboardKey.SelectedKeyType == UBlackboardKeyType_Vector::StaticClass())
	{
		const FVector TargetLocation = Blackboard->GetValueAsVector(BlackboardKey.SelectedKeyName);
		const float Distance = FVector::Dist(Pawn->GetActorLocation(), TargetLocation);
		return MaxDistance <= 0.0f || Distance <= MaxDistance;
	}

	return false;
}
