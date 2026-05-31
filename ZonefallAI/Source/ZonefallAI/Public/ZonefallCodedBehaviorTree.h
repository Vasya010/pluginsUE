#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "ZonefallAIBlackboard.h"
#include "ZonefallCodedBehaviorTree.generated.h"

class UBehaviorTree;
class UBlackboardData;

USTRUCT(BlueprintType)
struct ZONEFALLAI_API FZonefallCodedBehaviorTreeSettings
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Blackboard")
	FZonefallAIBlackboardKeys BlackboardKeys;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat", meta = (ClampMin = "0.0"))
	float CombatAcceptanceRadius = 260.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat", meta = (ClampMin = "0.0"))
	float CombatMoveSpeed = 420.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Investigation", meta = (ClampMin = "0.0"))
	float InvestigationAcceptanceRadius = 140.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Investigation", meta = (ClampMin = "0.0"))
	float InvestigationMoveSpeed = 260.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Patrol", meta = (ClampMin = "0.0"))
	float PatrolAcceptanceRadius = 120.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Patrol", meta = (ClampMin = "0.0"))
	float PatrolMoveSpeed = 165.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Patrol", meta = (ClampMin = "0.0"))
	float PatrolWaitTime = 1.25f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Patrol", meta = (ClampMin = "0.0"))
	float PatrolWaitRandomDeviation = 0.75f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Targeting", meta = (ClampMin = "0.0"))
	float MaxTargetDistance = 4500.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Targeting", meta = (ClampMin = "0.0"))
	float LostTargetGraceTime = 4.0f;
};

UCLASS()
class ZONEFALLAI_API UZonefallCodedBehaviorTreeLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Zonefall AI|Coded Behavior Tree")
	static UBlackboardData* CreateAdventureBlackboard(UObject* Outer, const FZonefallAIBlackboardKeys& Keys);

	UFUNCTION(BlueprintCallable, Category = "Zonefall AI|Coded Behavior Tree")
	static UBehaviorTree* CreateAdventureBehaviorTree(UObject* Outer, UBlackboardData* BlackboardAsset, const FZonefallCodedBehaviorTreeSettings& Settings);
};
