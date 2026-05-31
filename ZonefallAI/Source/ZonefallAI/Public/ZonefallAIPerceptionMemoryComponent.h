#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ZonefallAIBlackboard.h"
#include "ZonefallAIPerceptionMemoryComponent.generated.h"

class AActor;
class UBlackboardComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FZonefallAIMemoryActorEvent, AActor*, Actor);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FZonefallAIMemoryLocationEvent, FVector, Location, EZonefallAIStimulusType, StimulusType);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FZonefallAIDetectionProgressEvent, float, DetectionProgress);

UCLASS(ClassGroup = (ZonefallAI), Blueprintable, meta = (BlueprintSpawnableComponent, DisplayName = "Zonefall AI Perception Memory"))
class ZONEFALLAI_API UZonefallAIPerceptionMemoryComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UZonefallAIPerceptionMemoryComponent();

	UPROPERTY(BlueprintAssignable, Category = "Zonefall AI|Perception Memory")
	FZonefallAIMemoryActorEvent OnTargetSpotted;

	UPROPERTY(BlueprintAssignable, Category = "Zonefall AI|Perception Memory")
	FZonefallAIMemoryLocationEvent OnSuspiciousStimulus;

	UPROPERTY(BlueprintAssignable, Category = "Zonefall AI|Perception Memory")
	FZonefallAIMemoryActorEvent OnTargetLost;

	UPROPERTY(BlueprintAssignable, Category = "Zonefall AI|Perception Memory")
	FZonefallAIMemoryLocationEvent OnInvestigationStarted;

	UPROPERTY(BlueprintAssignable, Category = "Zonefall AI|Perception Memory")
	FZonefallAIDetectionProgressEvent OnDetectionProgressChanged;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Memory|Tuning", meta = (ClampMin = "0.0"))
	float SightConfidenceGain = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Memory|Tuning", meta = (ClampMin = "0.0"))
	float HearingSuspicionGain = 0.45f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Memory|Tuning", meta = (ClampMin = "0.0"))
	float LostSightSuspicionGain = 0.25f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Memory|Tuning", meta = (ClampMin = "0.0"))
	float MemoryDecayPerSecond = 0.07f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Memory|Tuning", meta = (ClampMin = "0.0"))
	float SuspicionDecayPerSecond = 0.05f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Memory|Tuning", meta = (ClampMin = "0.0"))
	float InvestigationThreshold = 0.25f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Memory|Tuning", meta = (ClampMin = "0.0"))
	float ForgetTargetAfterSeconds = 12.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Detection|Tuning", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float DetectionThreshold = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Detection|Tuning", meta = (ClampMin = "0.0"))
	float SightDetectionGain = 0.22f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Detection|Tuning", meta = (ClampMin = "0.0"))
	float DetectionDecayPerSecond = 0.2f;

	UPROPERTY(BlueprintReadOnly, Category = "Memory")
	TObjectPtr<AActor> KnownTargetActor;

	UPROPERTY(BlueprintReadOnly, Category = "Memory")
	FVector LastSeenLocation = FVector::ZeroVector;

	UPROPERTY(BlueprintReadOnly, Category = "Memory")
	FVector LastHeardLocation = FVector::ZeroVector;

	UPROPERTY(BlueprintReadOnly, Category = "Memory")
	FVector InvestigationLocation = FVector::ZeroVector;

	UPROPERTY(BlueprintReadOnly, Category = "Memory")
	float LastSeenTime = -1.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Memory")
	float LastHeardTime = -1.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Memory")
	float LastStimulusTime = -1.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Memory")
	float MemoryConfidence = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Memory")
	float Suspicion = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Memory")
	float Alertness = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Detection")
	float DetectionProgress = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Memory")
	bool bHasLineOfSight = false;

	UPROPERTY(BlueprintReadOnly, Category = "Detection")
	bool bDetectionComplete = false;

	UPROPERTY(BlueprintReadOnly, Category = "Memory")
	bool bShouldInvestigate = false;

	UPROPERTY(BlueprintReadOnly, Category = "Memory")
	EZonefallAIStimulusType LastStimulusType = EZonefallAIStimulusType::None;

	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	UFUNCTION(BlueprintCallable, Category = "Zonefall AI|Perception Memory")
	void RecordSightStimulus(AActor* TargetActor, FVector SeenLocation, float StimulusStrength = 1.0f);

	UFUNCTION(BlueprintCallable, Category = "Zonefall AI|Perception Memory")
	void RecordHearingStimulus(AActor* SourceActor, FVector HeardLocation, float StimulusStrength = 1.0f);

	UFUNCTION(BlueprintCallable, Category = "Zonefall AI|Perception Memory")
	void RecordLostSight(AActor* TargetActor, FVector LastKnownLocation);

	UFUNCTION(BlueprintCallable, Category = "Zonefall AI|Perception Memory")
	void ClearMemory();

	UFUNCTION(BlueprintCallable, Category = "Zonefall AI|Perception Memory")
	void ClearDetection();

	UFUNCTION(BlueprintCallable, Category = "Zonefall AI|Perception Memory")
	void WriteToBlackboard(UBlackboardComponent* Blackboard, const FZonefallAIBlackboardKeys& Keys) const;

	UFUNCTION(BlueprintPure, Category = "Zonefall AI|Perception Memory")
	float GetTimeSinceLastStimulus() const;

	UFUNCTION(BlueprintPure, Category = "Zonefall AI|Perception Memory")
	bool HasActionableMemory() const;

	UFUNCTION(BlueprintPure, Category = "Zonefall AI|Perception Memory")
	bool IsDetectionComplete() const;

private:
	float GetNow() const;
	void RefreshInvestigationState();
};
