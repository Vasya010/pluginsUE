#include "BTTask_ZonefallBark.h"

#include "AIController.h"
#include "BehaviorTree/BehaviorTreeComponent.h"
#include "GameFramework/Pawn.h"
#include "ZonefallAIBarkComponent.h"

UBTTask_ZonefallBark::UBTTask_ZonefallBark()
{
	NodeName = TEXT("Zonefall Bark");
}

EBTNodeResult::Type UBTTask_ZonefallBark::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	AAIController* Controller = OwnerComp.GetAIOwner();
	if (!Controller || !Controller->GetPawn())
	{
		return bSucceedEvenIfNoBarkAvailable ? EBTNodeResult::Succeeded : EBTNodeResult::Failed;
	}

	UZonefallAIBarkComponent* Bark = Controller->GetPawn()->FindComponentByClass<UZonefallAIBarkComponent>();
	if (!Bark)
	{
		Bark = Controller->FindComponentByClass<UZonefallAIBarkComponent>();
	}

	if (!Bark)
	{
		return bSucceedEvenIfNoBarkAvailable ? EBTNodeResult::Succeeded : EBTNodeResult::Failed;
	}

	const bool bBarkPlayed = !ExplicitBarkId.IsNone() ? Bark->TryBarkId(ExplicitBarkId) : Bark->TryBarkCategory(Category);

	if (!bBarkPlayed && !bSucceedEvenIfNoBarkAvailable)
	{
		return EBTNodeResult::Failed;
	}

	return EBTNodeResult::Succeeded;
}
