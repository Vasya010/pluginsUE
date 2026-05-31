#include "BTTask_ZonefallFlankTarget.h"

#include "AIController.h"
#include "BehaviorTree/BehaviorTreeComponent.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "GameFramework/Pawn.h"
#include "ZonefallAITacticalCoverComponent.h"

UBTTask_ZonefallFlankTarget::UBTTask_ZonefallFlankTarget()
{
	NodeName = TEXT("Zonefall Flank Target");
}

EBTNodeResult::Type UBTTask_ZonefallFlankTarget::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	AAIController* Controller = OwnerComp.GetAIOwner();
	UBlackboardComponent* Blackboard = OwnerComp.GetBlackboardComponent();
	if (!Controller || !Blackboard)
	{
		return EBTNodeResult::Failed;
	}

	AActor* TargetActor = Cast<AActor>(Blackboard->GetValueAsObject(BlackboardKeys.TargetActor));
	if (!TargetActor)
	{
		return EBTNodeResult::Failed;
	}

	APawn* Pawn = Controller->GetPawn();
	if (!Pawn)
	{
		return EBTNodeResult::Failed;
	}

	EZonefallAIFlankSide Side = PreferredSide;
	if (bAutoChoseSide)
	{
		const FVector Forward = TargetActor->GetActorForwardVector();
		const FVector Offset = (Pawn->GetActorLocation() - TargetActor->GetActorLocation()).GetSafeNormal();
		const float DotForward = FVector::DotProduct(Forward, Offset);
		const FVector Right = TargetActor->GetActorRightVector();
		const float DotRight = FVector::DotProduct(Right, Offset);

		if (DotForward < -0.5f)
		{
			Side = EZonefallAIFlankSide::Rear;
		}
		else if (DotRight >= 0.0f)
		{
			Side = EZonefallAIFlankSide::Right;
		}
		else
		{
			Side = EZonefallAIFlankSide::Left;
		}
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

	FVector FlankLocation;
	if (!CoverComponent->RequestFlankPosition(TargetActor, Side, FlankLocation))
	{
		return EBTNodeResult::Failed;
	}

	Blackboard->SetValueAsVector(BlackboardKeys.MoveLocation, FlankLocation);
	UZonefallAIBlackboardLibrary::SetZonefallFlank(Blackboard, BlackboardKeys, FlankLocation, Side);
	UZonefallAIBlackboardLibrary::SetZonefallTacticalIntent(Blackboard, BlackboardKeys, EZonefallAITacticalIntent::Flank);
	return EBTNodeResult::Succeeded;
}
