#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "ZonefallAIBlackboard.h"
#include "BTTask_ZonefallSeedBlackboard.generated.h"

UCLASS(DisplayName = "Zonefall Seed Blackboard")
class ZONEFALLAI_API UBTTask_ZonefallSeedBlackboard : public UBTTaskNode
{
	GENERATED_BODY()

public:
	UBTTask_ZonefallSeedBlackboard();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Blackboard")
	FZonefallAIBlackboardKeys BlackboardKeys;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Defaults", meta = (ClampMin = "0.0"))
	float PatrolRadius = 1600.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Defaults", meta = (ClampMin = "0.0"))
	float SearchRadius = 2400.0f;

protected:
	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
};
