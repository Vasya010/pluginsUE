#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/Tasks/BTTask_BlackboardBase.h"
#include "ZonefallAIBlackboard.h"
#include "BTTask_ZonefallFlankTarget.generated.h"

UCLASS(DisplayName = "Zonefall Flank Target")
class ZONEFALLAI_API UBTTask_ZonefallFlankTarget : public UBTTask_BlackboardBase
{
	GENERATED_BODY()

public:
	UBTTask_ZonefallFlankTarget();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Blackboard")
	FZonefallAIBlackboardKeys BlackboardKeys;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flank")
	EZonefallAIFlankSide PreferredSide = EZonefallAIFlankSide::Left;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flank")
	bool bAutoChoseSide = true;

protected:
	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
};
