#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTDecorator.h"
#include "ZonefallAIBlackboard.h"
#include "BTDecorator_ZonefallIsSuppressed.generated.h"

UCLASS(DisplayName = "Zonefall Is Suppressed")
class ZONEFALLAI_API UBTDecorator_ZonefallIsSuppressed : public UBTDecorator
{
	GENERATED_BODY()

public:
	UBTDecorator_ZonefallIsSuppressed();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Blackboard")
	FZonefallAIBlackboardKeys BlackboardKeys;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Suppression", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float MinimumLevel = 0.5f;

protected:
	virtual bool CalculateRawConditionValue(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) const override;
	virtual FString GetStaticDescription() const override;
};
