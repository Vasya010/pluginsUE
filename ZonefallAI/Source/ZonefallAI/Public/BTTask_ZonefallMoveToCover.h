#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/Tasks/BTTask_BlackboardBase.h"
#include "ZonefallAIBlackboard.h"
#include "BTTask_ZonefallMoveToCover.generated.h"

UCLASS(DisplayName = "Zonefall Find Cover")
class ZONEFALLAI_API UBTTask_ZonefallMoveToCover : public UBTTask_BlackboardBase
{
	GENERATED_BODY()

public:
	UBTTask_ZonefallMoveToCover();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Blackboard")
	FZonefallAIBlackboardKeys BlackboardKeys;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cover")
	bool bForceRefresh = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cover")
	bool bAlsoWriteMoveLocation = true;

protected:
	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
};
