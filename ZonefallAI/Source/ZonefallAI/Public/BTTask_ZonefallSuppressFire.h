#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/Tasks/BTTask_BlackboardBase.h"
#include "ZonefallAIBlackboard.h"
#include "BTTask_ZonefallSuppressFire.generated.h"

UCLASS(DisplayName = "Zonefall Suppress Fire")
class ZONEFALLAI_API UBTTask_ZonefallSuppressFire : public UBTTask_BlackboardBase
{
	GENERATED_BODY()

public:
	UBTTask_ZonefallSuppressFire();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Blackboard")
	FZonefallAIBlackboardKeys BlackboardKeys;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Suppress")
	bool bUseLastKnownLocationIfNoTarget = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Suppress")
	bool bSetFocus = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Suppress")
	bool bBroadcastSuppressionToSquad = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Suppress", meta = (ClampMin = "0.0"))
	float SuppressDuration = 2.5f;

protected:
	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
};
