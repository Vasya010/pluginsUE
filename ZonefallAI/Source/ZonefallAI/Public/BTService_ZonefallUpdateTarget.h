#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTService.h"
#include "ZonefallAIBlackboard.h"
#include "BTService_ZonefallUpdateTarget.generated.h"

UCLASS(DisplayName = "Zonefall Update Target")
class ZONEFALLAI_API UBTService_ZonefallUpdateTarget : public UBTService
{
	GENERATED_BODY()

public:
	UBTService_ZonefallUpdateTarget();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Blackboard")
	FZonefallAIBlackboardKeys BlackboardKeys;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Targeting", meta = (ClampMin = "0.0"))
	float MaxTargetDistance = 4500.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Targeting", meta = (ClampMin = "0.0"))
	float LostTargetGraceTime = 4.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Targeting")
	bool bOnlyUseSightPerception = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Targeting")
	bool bRequireLineOfSight = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Targeting")
	bool bUseIdentityTargetRules = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Targeting")
	bool bIgnoreHarmlessTargets = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scoring", meta = (ClampMin = "0.0"))
	float DistanceWeight = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scoring", meta = (ClampMin = "0.0"))
	float LineOfSightWeight = 0.75f;

protected:
	virtual void OnBecomeRelevant(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
	virtual void TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;

private:
	void UpdateTarget(UBehaviorTreeComponent& OwnerComp) const;
};
