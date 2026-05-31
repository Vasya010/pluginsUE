#include "BTDecorator_ZonefallIsSquadRole.h"

#include "BehaviorTree/BehaviorTreeComponent.h"
#include "BehaviorTree/BlackboardComponent.h"

UBTDecorator_ZonefallIsSquadRole::UBTDecorator_ZonefallIsSquadRole()
{
	NodeName = TEXT("Zonefall Is Squad Role");
}

bool UBTDecorator_ZonefallIsSquadRole::CalculateRawConditionValue(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) const
{
	const UBlackboardComponent* Blackboard = OwnerComp.GetBlackboardComponent();
	if (!Blackboard)
	{
		return false;
	}

	const EZonefallAISquadRole CurrentRole = UZonefallAIBlackboardLibrary::GetZonefallSquadRole(Blackboard, BlackboardKeys);
	if (CurrentRole == EZonefallAISquadRole::None)
	{
		return bAllowIfNoRoleAssigned;
	}
	return CurrentRole == RequiredRole;
}

FString UBTDecorator_ZonefallIsSquadRole::GetStaticDescription() const
{
	return FString::Printf(TEXT("Require Squad Role: %s"), *UEnum::GetValueAsString(RequiredRole));
}
