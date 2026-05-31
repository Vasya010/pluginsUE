#include "BTTask_ZonefallSuppressFire.h"

#include "AIController.h"
#include "BehaviorTree/BehaviorTreeComponent.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "ZonefallAIBarkComponent.h"
#include "ZonefallAISquadSubsystem.h"
#include "ZonefallAISuppressionComponent.h"

UBTTask_ZonefallSuppressFire::UBTTask_ZonefallSuppressFire()
{
	NodeName = TEXT("Zonefall Suppress Fire");
}

EBTNodeResult::Type UBTTask_ZonefallSuppressFire::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	AAIController* Controller = OwnerComp.GetAIOwner();
	UBlackboardComponent* Blackboard = OwnerComp.GetBlackboardComponent();
	if (!Controller || !Blackboard)
	{
		return EBTNodeResult::Failed;
	}

	AActor* TargetActor = Cast<AActor>(Blackboard->GetValueAsObject(BlackboardKeys.TargetActor));
	FVector SuppressLocation = FVector::ZeroVector;

	if (TargetActor)
	{
		SuppressLocation = TargetActor->GetActorLocation();
	}
	else if (bUseLastKnownLocationIfNoTarget)
	{
		SuppressLocation = Blackboard->GetValueAsVector(BlackboardKeys.LastKnownTargetLocation);
		if (SuppressLocation.IsNearlyZero())
		{
			return EBTNodeResult::Failed;
		}
	}
	else
	{
		return EBTNodeResult::Failed;
	}

	UZonefallAIBlackboardLibrary::SetZonefallTacticalIntent(Blackboard, BlackboardKeys, EZonefallAITacticalIntent::Suppress);

	if (bSetFocus)
	{
		if (TargetActor)
		{
			Controller->SetFocus(TargetActor, EAIFocusPriority::Gameplay);
		}
		else
		{
			Controller->SetFocalPoint(SuppressLocation, EAIFocusPriority::Gameplay);
		}
	}

	if (bBroadcastSuppressionToSquad)
	{
		if (UWorld* World = Controller->GetWorld())
		{
			if (UZonefallAISquadSubsystem* SquadSubsystem = World->GetSubsystem<UZonefallAISquadSubsystem>())
			{
				SquadSubsystem->TryEmitSquadCallout(Controller, FName(TEXT("Bark.Suppress")), 1.5f, 3.0f);
			}
		}
	}

	if (TargetActor)
	{
		if (UZonefallAISuppressionComponent* TargetSuppression = TargetActor->FindComponentByClass<UZonefallAISuppressionComponent>())
		{
			TargetSuppression->RegisterSuppressiveFire(Controller->GetPawn());
		}
	}

	if (APawn* Pawn = Controller->GetPawn())
	{
		if (UZonefallAIBarkComponent* Bark = Pawn->FindComponentByClass<UZonefallAIBarkComponent>())
		{
			Bark->TryBarkCategory(EZonefallAIBarkCategory::Suppressing);
		}
	}

	return EBTNodeResult::Succeeded;
}
