#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "ZonefallAIBarkComponent.h"
#include "BTTask_ZonefallBark.generated.h"

UCLASS(DisplayName = "Zonefall Bark")
class ZONEFALLAI_API UBTTask_ZonefallBark : public UBTTaskNode
{
	GENERATED_BODY()

public:
	UBTTask_ZonefallBark();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bark")
	EZonefallAIBarkCategory Category = EZonefallAIBarkCategory::Contact;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bark")
	FName ExplicitBarkId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bark")
	bool bSucceedEvenIfNoBarkAvailable = true;

protected:
	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
};
