#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/Decorators/BTDecorator_BlackboardBase.h"
#include "BTDecorator_ZonefallHasLineOfSight.generated.h"

UCLASS(DisplayName = "Zonefall Has Line Of Sight")
class ZONEFALLAI_API UBTDecorator_ZonefallHasLineOfSight : public UBTDecorator_BlackboardBase
{
	GENERATED_BODY()

public:
	UBTDecorator_ZonefallHasLineOfSight();

	void SetCodedBlackboardKey(const FBlackboardKeySelector& InBlackboardKey);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Line Of Sight", meta = (ClampMin = "0.0"))
	float MaxDistance = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Line Of Sight")
	bool bAcceptVectorKey = true;

protected:
	virtual bool CalculateRawConditionValue(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) const override;
};
