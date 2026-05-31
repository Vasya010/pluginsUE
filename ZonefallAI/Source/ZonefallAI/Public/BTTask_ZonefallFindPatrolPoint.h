#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "BehaviorTree/BehaviorTreeTypes.h"
#include "BTTask_ZonefallFindPatrolPoint.generated.h"

UCLASS(DisplayName = "Zonefall Find Patrol Point")
class ZONEFALLAI_API UBTTask_ZonefallFindPatrolPoint : public UBTTaskNode
{
	GENERATED_BODY()

public:
	UBTTask_ZonefallFindPatrolPoint();

	UPROPERTY(EditAnywhere, Category = "Blackboard")
	FBlackboardKeySelector MoveLocationKey;

	UPROPERTY(EditAnywhere, Category = "Blackboard")
	FBlackboardKeySelector AnchorLocationKey;

	UPROPERTY(EditAnywhere, Category = "Blackboard")
	FBlackboardKeySelector RadiusKey;

	UPROPERTY(EditAnywhere, Category = "Blackboard")
	FBlackboardKeySelector AcceptanceRadiusKey;

	UPROPERTY(EditAnywhere, Category = "Blackboard")
	FBlackboardKeySelector WaitTimeKey;

	UPROPERTY(EditAnywhere, Category = "Patrol", meta = (ClampMin = "0.0"))
	float FallbackRadius = 1600.0f;

	UPROPERTY(EditAnywhere, Category = "Patrol", meta = (ClampMin = "0.0"))
	float MinDistanceFromPawn = 250.0f;

	UPROPERTY(EditAnywhere, Category = "Patrol", meta = (ClampMin = "1", ClampMax = "32"))
	int32 MaxAttempts = 8;

	UPROPERTY(EditAnywhere, Category = "Patrol")
	bool bUsePawnLocationWhenAnchorMissing = true;

protected:
	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
};
