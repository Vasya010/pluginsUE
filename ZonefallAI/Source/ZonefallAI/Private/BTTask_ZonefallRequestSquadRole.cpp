#include "BTTask_ZonefallRequestSquadRole.h"

#include "AIController.h"
#include "BehaviorTree/BehaviorTreeComponent.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "ZonefallAICharacterComponent.h"
#include "ZonefallAISquadSubsystem.h"

UBTTask_ZonefallRequestSquadRole::UBTTask_ZonefallRequestSquadRole()
{
	NodeName = TEXT("Zonefall Request Squad Role");
}

EBTNodeResult::Type UBTTask_ZonefallRequestSquadRole::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	AAIController* Controller = OwnerComp.GetAIOwner();
	UBlackboardComponent* Blackboard = OwnerComp.GetBlackboardComponent();
	if (!Controller || !Blackboard)
	{
		return EBTNodeResult::Failed;
	}

	UWorld* World = Controller->GetWorld();
	if (!World)
	{
		return EBTNodeResult::Failed;
	}

	UZonefallAISquadSubsystem* SquadSubsystem = World->GetSubsystem<UZonefallAISquadSubsystem>();
	if (!SquadSubsystem)
	{
		return EBTNodeResult::Failed;
	}

	int32 SquadId = SquadSubsystem->GetSquadId(Controller);
	if (SquadId == INDEX_NONE && bRegisterWithSquadIfMissing)
	{
		FGameplayTag FactionTag;
		if (const APawn* Pawn = Controller->GetPawn())
		{
			if (const UZonefallAICharacterComponent* Identity = UZonefallAICharacterComponent::FindAICharacterComponent(Pawn))
			{
				FactionTag = Identity->FactionTag;
			}
		}
		SquadId = SquadSubsystem->RegisterAgent(Controller, FallbackSquadName, FactionTag);
	}

	if (SquadId == INDEX_NONE)
	{
		return EBTNodeResult::Failed;
	}

	if (AActor* TargetActor = Cast<AActor>(Blackboard->GetValueAsObject(BlackboardKeys.TargetActor)))
	{
		SquadSubsystem->ReportTargetSpotted(Controller, TargetActor);
	}

	SquadSubsystem->TryAssignSquadRoles(SquadId);

	const EZonefallAISquadRole Role = SquadSubsystem->GetAssignedRole(Controller);
	UZonefallAIBlackboardLibrary::SetZonefallSquadRole(Blackboard, BlackboardKeys, Role);

	return EBTNodeResult::Succeeded;
}
