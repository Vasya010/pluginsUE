#include "BTDecorator_ZonefallIsSuppressed.h"

#include "BehaviorTree/BehaviorTreeComponent.h"
#include "BehaviorTree/BlackboardComponent.h"

UBTDecorator_ZonefallIsSuppressed::UBTDecorator_ZonefallIsSuppressed()
{
	NodeName = TEXT("Zonefall Is Suppressed");
}

bool UBTDecorator_ZonefallIsSuppressed::CalculateRawConditionValue(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) const
{
	const UBlackboardComponent* Blackboard = OwnerComp.GetBlackboardComponent();
	if (!Blackboard)
	{
		return false;
	}
	const float Level = Blackboard->GetValueAsFloat(BlackboardKeys.SuppressionLevel);
	return Level >= MinimumLevel;
}

FString UBTDecorator_ZonefallIsSuppressed::GetStaticDescription() const
{
	return FString::Printf(TEXT("Suppression >= %.2f"), MinimumLevel);
}
