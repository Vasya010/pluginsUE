#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/Tasks/BTTask_BlackboardBase.h"
#include "ZonefallAIBlackboard.h"
#include "BTTask_ZonefallRequestSquadRole.generated.h"

UCLASS(DisplayName = "Zonefall Request Squad Role")
class ZONEFALLAI_API UBTTask_ZonefallRequestSquadRole : public UBTTask_BlackboardBase
{
	GENERATED_BODY()

public:
	UBTTask_ZonefallRequestSquadRole();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Blackboard")
	FZonefallAIBlackboardKeys BlackboardKeys;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Squad")
	bool bRegisterWithSquadIfMissing = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Squad")
	FName FallbackSquadName = NAME_None;

protected:
	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
};
