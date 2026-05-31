#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTDecorator.h"
#include "ZonefallAIBlackboard.h"
#include "BTDecorator_ZonefallShouldInvestigate.generated.h"

UCLASS(DisplayName = "Zonefall Should Investigate")
class ZONEFALLAI_API UBTDecorator_ZonefallShouldInvestigate : public UBTDecorator
{
	GENERATED_BODY()

public:
	UBTDecorator_ZonefallShouldInvestigate();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Blackboard")
	FZonefallAIBlackboardKeys BlackboardKeys;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Investigation", meta = (ClampMin = "0.0"))
	float MinimumSuspicion = 0.2f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Investigation", meta = (ClampMin = "0.0"))
	float MinimumConfidence = 0.2f;

protected:
	virtual bool CalculateRawConditionValue(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) const override;
};
