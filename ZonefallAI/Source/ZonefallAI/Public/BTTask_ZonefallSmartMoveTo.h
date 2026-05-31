#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BehaviorTreeTypes.h"
#include "BehaviorTree/Tasks/BTTask_BlackboardBase.h"
#include "BTTask_ZonefallSmartMoveTo.generated.h"

struct FZonefallSmartMoveMemory
{
	FVector LastGoalLocation = FVector::ZeroVector;
	float PreviousMaxWalkSpeed = 0.0f;
	uint8 bMoveStarted : 1;
	uint8 bHadValidGoal : 1;
	uint8 bAppliedMovementSpeed : 1;

	FZonefallSmartMoveMemory()
		: bMoveStarted(false)
		, bHadValidGoal(false)
		, bAppliedMovementSpeed(false)
	{
	}
};

UCLASS(DisplayName = "Zonefall Smart Move To")
class ZONEFALLAI_API UBTTask_ZonefallSmartMoveTo : public UBTTask_BlackboardBase
{
	GENERATED_BODY()

public:
	UBTTask_ZonefallSmartMoveTo();

	void SetCodedBlackboardKey(const FBlackboardKeySelector& InBlackboardKey);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Move", meta = (ClampMin = "0.0"))
	float AcceptanceRadius = 85.0f;

	UPROPERTY(EditAnywhere, Category = "Move")
	FBlackboardKeySelector AcceptanceRadiusKey;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Move")
	bool bUsePathfinding = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Move")
	bool bAllowPartialPath = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Move")
	bool bProjectGoalToNavigation = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Move")
	bool bStopOnOverlap = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Move")
	bool bTrackMovingGoalActor = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Move", meta = (ClampMin = "0.0"))
	float RepathDistance = 160.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Move")
	bool bStopMovementOnAbort = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tuning", meta = (ClampMin = "0.0"))
	float DesiredSpeed = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tuning")
	bool bApplyDesiredSpeedToCharacterMovement = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tuning")
	bool bAllowBlackboardDesiredSpeedOverride = false;

	UPROPERTY(EditAnywhere, Category = "Tuning")
	FBlackboardKeySelector DesiredSpeedKey;

protected:
	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
	virtual void TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;
	virtual EBTNodeResult::Type AbortTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
	virtual uint16 GetInstanceMemorySize() const override;

private:
	bool ResolveGoal(UBehaviorTreeComponent& OwnerComp, FVector& OutGoalLocation, AActor*& OutGoalActor) const;
	float GetResolvedAcceptanceRadius(const UBlackboardComponent* Blackboard) const;
	float GetResolvedDesiredSpeed(const UBlackboardComponent* Blackboard) const;
	bool IsAtGoal(const APawn& Pawn, const FVector& GoalLocation, float ResolvedAcceptanceRadius) const;
	void ApplyDesiredSpeed(APawn& Pawn, FZonefallSmartMoveMemory& MoveMemory, float ResolvedDesiredSpeed) const;
	void RestoreDesiredSpeed(UBehaviorTreeComponent& OwnerComp, FZonefallSmartMoveMemory& MoveMemory) const;
	EBTNodeResult::Type RequestMove(UBehaviorTreeComponent& OwnerComp, FZonefallSmartMoveMemory& MoveMemory) const;
};
