#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTService.h"
#include "ZonefallAIBlackboard.h"
#include "BTService_ZonefallReportToSquad.generated.h"

UCLASS(DisplayName = "Zonefall Report To Squad")
class ZONEFALLAI_API UBTService_ZonefallReportToSquad : public UBTService
{
	GENERATED_BODY()

public:
	UBTService_ZonefallReportToSquad();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Blackboard")
	FZonefallAIBlackboardKeys BlackboardKeys;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Squad")
	bool bAutoRegister = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Squad")
	FName FallbackSquadName = NAME_None;

protected:
	virtual void OnBecomeRelevant(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
	virtual void TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;

private:
	void Report(UBehaviorTreeComponent& OwnerComp) const;
};
