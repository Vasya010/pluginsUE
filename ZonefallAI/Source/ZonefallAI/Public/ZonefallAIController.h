#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "Perception/AIPerceptionTypes.h"
#include "ZonefallAIBlackboard.h"
#include "ZonefallCodedBehaviorTree.h"
#include "ZonefallAIController.generated.h"

class UAIPerceptionComponent;
class UAISenseConfig_Hearing;
class UAISenseConfig_Sight;
class UBehaviorTree;
class UBlackboardData;
class UTextRenderComponent;
class UZonefallAIBarkComponent;
class UZonefallAIPerceptionMemoryComponent;
class UZonefallAISuppressionComponent;
class UZonefallAITacticalCoverComponent;
class UZonefallAISquadSubsystem;

UCLASS(Blueprintable)
class ZONEFALLAI_API AZonefallAIController : public AAIController
{
	GENERATED_BODY()

public:
	AZonefallAIController(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Zonefall AI|Perception")
	TObjectPtr<UAIPerceptionComponent> ZonefallPerceptionComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Zonefall AI|Perception")
	TObjectPtr<UAISenseConfig_Sight> SightConfig;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Zonefall AI|Perception")
	TObjectPtr<UAISenseConfig_Hearing> HearingConfig;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Zonefall AI|Perception")
	TObjectPtr<UZonefallAIPerceptionMemoryComponent> PerceptionMemoryComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Zonefall AI|Tactical")
	TObjectPtr<UZonefallAITacticalCoverComponent> CoverComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Zonefall AI|Tactical")
	TObjectPtr<UZonefallAISuppressionComponent> SuppressionComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Zonefall AI|Tactical")
	TObjectPtr<UZonefallAIBarkComponent> BarkComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Zonefall AI|Indicator")
	TObjectPtr<UTextRenderComponent> AlertIndicatorComponent;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zonefall AI|Indicator")
	bool bUseBanditAlertIndicator = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zonefall AI|Indicator", meta = (ClampMin = "0.0"))
	float AlertIndicatorHeight = 220.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Zonefall AI|Behavior")
	TObjectPtr<UBehaviorTree> DefaultBehaviorTree;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Zonefall AI|Behavior")
	TObjectPtr<UBlackboardData> DefaultBlackboardAsset;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zonefall AI|Coded Behavior")
	bool bUseCodedBehaviorTreeWhenAssetMissing = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zonefall AI|Coded Behavior", meta = (EditCondition = "bUseCodedBehaviorTreeWhenAssetMissing"))
	FZonefallCodedBehaviorTreeSettings CodedBehaviorSettings;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zonefall AI|Blackboard")
	FZonefallAIBlackboardKeys BlackboardKeys;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zonefall AI|Blackboard", meta = (ClampMin = "0.0"))
	float DefaultPatrolRadius = 1600.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zonefall AI|Blackboard", meta = (ClampMin = "0.0"))
	float DefaultSearchRadius = 2400.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zonefall AI|Squad")
	bool bAutoJoinSquad = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zonefall AI|Squad")
	FName SquadOverrideName = NAME_None;

	UFUNCTION(BlueprintCallable, Category = "Zonefall AI|Squad")
	int32 RegisterWithSquad(FName SquadName);

	UFUNCTION(BlueprintCallable, Category = "Zonefall AI|Squad")
	void UnregisterFromSquad();

	UFUNCTION(BlueprintCallable, Category = "Zonefall AI|Behavior")
	bool StartZonefallBehavior();

	UFUNCTION(BlueprintCallable, Category = "Zonefall AI|Behavior")
	void StopZonefallBehavior(const FString& Reason);

	UFUNCTION(BlueprintNativeEvent, Category = "Zonefall AI|Perception")
	void OnZonefallTargetSpotted(AActor* TargetActor);

	UFUNCTION(BlueprintNativeEvent, Category = "Zonefall AI|Perception")
	void OnZonefallSuspiciousSoundHeard(FVector SoundLocation, AActor* SourceActor);

	UFUNCTION(BlueprintNativeEvent, Category = "Zonefall AI|Perception")
	void OnZonefallTargetLost(AActor* LostActor, FVector LastKnownLocation);

	UFUNCTION(BlueprintNativeEvent, Category = "Zonefall AI|Perception")
	void OnZonefallInvestigationStarted(FVector InvestigationLocation, EZonefallAIStimulusType StimulusType);

protected:
	UPROPERTY(Transient)
	TObjectPtr<UBehaviorTree> RuntimeCodedBehaviorTree;

	UPROPERTY(Transient)
	TObjectPtr<UBlackboardData> RuntimeCodedBlackboardAsset;

	virtual void OnPossess(APawn* InPawn) override;
	virtual void OnUnPossess() override;
	virtual void Tick(float DeltaSeconds) override;

	UFUNCTION()
	void HandleTargetPerceptionUpdated(AActor* Actor, FAIStimulus Stimulus);

	bool CanAcceptPerceivedTarget(AActor* Actor) const;
	void PushMemoryToBlackboard();
	void AttachAlertIndicator(APawn* InPawn);
	void UpdateAlertIndicator();
	bool ShouldShowAlertIndicator() const;
	void SeedBlackboard(APawn* InPawn);
};
