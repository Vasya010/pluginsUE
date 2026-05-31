#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "ZonefallAIBlackboard.h"
#include "BTTask_ZonefallClearMemoryState.generated.h"

UCLASS(DisplayName = "Zonefall Clear Memory State")
class ZONEFALLAI_API UBTTask_ZonefallClearMemoryState : public UBTTaskNode
{
	GENERATED_BODY()

public:
	UBTTask_ZonefallClearMemoryState();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Blackboard")
	FZonefallAIBlackboardKeys BlackboardKeys;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Memory")
	bool bClearComponentMemory = true;

protected:
	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
};
