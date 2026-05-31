#include "BTService_ZonefallReportToSquad.h"

#include "AIController.h"
#include "BehaviorTree/BehaviorTreeComponent.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "ZonefallAICharacterComponent.h"
#include "ZonefallAISquadSubsystem.h"

UBTService_ZonefallReportToSquad::UBTService_ZonefallReportToSquad()
{
	NodeName = TEXT("Zonefall Report To Squad");
	bNotifyBecomeRelevant = true;
	bNotifyTick = true;
	Interval = 0.6f;
	RandomDeviation = 0.15f;
}

void UBTService_ZonefallReportToSquad::OnBecomeRelevant(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	Super::OnBecomeRelevant(OwnerComp, NodeMemory);

	AAIController* Controller = OwnerComp.GetAIOwner();
	UWorld* World = Controller ? Controller->GetWorld() : nullptr;
	if (!Controller || !World)
	{
		return;
	}

	UZonefallAISquadSubsystem* SquadSubsystem = World->GetSubsystem<UZonefallAISquadSubsystem>();
	if (!SquadSubsystem)
	{
		return;
	}

	if (bAutoRegister && SquadSubsystem->GetSquadId(Controller) == INDEX_NONE)
	{
		FGameplayTag FactionTag;
		if (const APawn* Pawn = Controller->GetPawn())
		{
			if (const UZonefallAICharacterComponent* Identity = UZonefallAICharacterComponent::FindAICharacterComponent(Pawn))
			{
				FactionTag = Identity->FactionTag;
			}
		}
		SquadSubsystem->RegisterAgent(Controller, FallbackSquadName, FactionTag);
	}

	Report(OwnerComp);
}

void UBTService_ZonefallReportToSquad::TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
	Super::TickNode(OwnerComp, NodeMemory, DeltaSeconds);
	Report(OwnerComp);
}

void UBTService_ZonefallReportToSquad::Report(UBehaviorTreeComponent& OwnerComp) const
{
	AAIController* Controller = OwnerComp.GetAIOwner();
	UBlackboardComponent* Blackboard = OwnerComp.GetBlackboardComponent();
	UWorld* World = Controller ? Controller->GetWorld() : nullptr;
	if (!Controller || !Blackboard || !World)
	{
		return;
	}

	UZonefallAISquadSubsystem* SquadSubsystem = World->GetSubsystem<UZonefallAISquadSubsystem>();
	if (!SquadSubsystem)
	{
		return;
	}

	const int32 SquadId = SquadSubsystem->GetSquadId(Controller);
	if (SquadId == INDEX_NONE)
	{
		return;
	}

	AActor* TargetActor = Cast<AActor>(Blackboard->GetValueAsObject(BlackboardKeys.TargetActor));
	if (TargetActor)
	{
		SquadSubsystem->ReportTargetSpotted(Controller, TargetActor);
	}
	else
	{
		const FVector LKP = Blackboard->GetValueAsVector(BlackboardKeys.LastKnownTargetLocation);
		if (!LKP.IsNearlyZero())
		{
			SquadSubsystem->ReportTargetLost(Controller, nullptr, LKP);
		}
	}

	const EZonefallAISquadRole Role = SquadSubsystem->GetAssignedRole(Controller);
	UZonefallAIBlackboardLibrary::SetZonefallSquadRole(Blackboard, BlackboardKeys, Role);
}
