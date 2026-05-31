#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTDecorator.h"
#include "ZonefallAIBlackboard.h"
#include "BTDecorator_ZonefallIsSquadRole.generated.h"

UCLASS(DisplayName = "Zonefall Is Squad Role")
class ZONEFALLAI_API UBTDecorator_ZonefallIsSquadRole : public UBTDecorator
{
	GENERATED_BODY()

public:
	UBTDecorator_ZonefallIsSquadRole();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Blackboard")
	FZonefallAIBlackboardKeys BlackboardKeys;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Squad")
	EZonefallAISquadRole RequiredRole = EZonefallAISquadRole::Attacker;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Squad")
	bool bAllowIfNoRoleAssigned = false;

protected:
	virtual bool CalculateRawConditionValue(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) const override;
	virtual FString GetStaticDescription() const override;
};
