#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/Tasks/BTTask_BlackboardBase.h"
#include "BTTask_ZonefallSetFocus.generated.h"

UCLASS(DisplayName = "Zonefall Set Focus")
class ZONEFALLAI_API UBTTask_ZonefallSetFocus : public UBTTask_BlackboardBase
{
	GENERATED_BODY()

public:
	UBTTask_ZonefallSetFocus();

	void SetCodedBlackboardKey(const FBlackboardKeySelector& InBlackboardKey);

	UPROPERTY(EditAnywhere, Category = "Focus")
	bool bClearFocus = false;

protected:
	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
};
