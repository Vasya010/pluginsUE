#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "BehaviorTree/BehaviorTreeTypes.h"
#include "ZonefallAIBlackboard.h"
#include "BTTask_ZonefallPickInvestigationPoint.generated.h"

UCLASS(DisplayName = "Zonefall Pick Investigation Point")
class ZONEFALLAI_API UBTTask_ZonefallPickInvestigationPoint : public UBTTaskNode
{
	GENERATED_BODY()

public:
	UBTTask_ZonefallPickInvestigationPoint();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Blackboard")
	FZonefallAIBlackboardKeys BlackboardKeys;

	UPROPERTY(EditAnywhere, Category = "Blackboard")
	FBlackboardKeySelector MoveLocationKey;

	UPROPERTY(EditAnywhere, Category = "Investigation", meta = (ClampMin = "0.0"))
	float ScatterRadius = 250.0f;

	UPROPERTY(EditAnywhere, Category = "Investigation")
	bool bProjectToNavigation = true;

protected:
	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
};
