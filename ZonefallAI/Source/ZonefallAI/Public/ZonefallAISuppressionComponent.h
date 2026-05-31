#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ZonefallAISuppressionComponent.generated.h"

class AActor;
class UBlackboardComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FZonefallSuppressionChangedEvent, float, NewLevel, AActor*, Source);

UCLASS(ClassGroup = (ZonefallAI), Blueprintable, meta = (BlueprintSpawnableComponent, DisplayName = "Zonefall AI Suppression"))
class ZONEFALLAI_API UZonefallAISuppressionComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UZonefallAISuppressionComponent();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Suppression|Tuning", meta = (ClampMin = "0.0"))
	float DamageSuppressionGain = 0.6f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Suppression|Tuning", meta = (ClampMin = "0.0"))
	float NearMissSuppressionGain = 0.18f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Suppression|Tuning", meta = (ClampMin = "0.0"))
	float SuppressiveFireGain = 0.1f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Suppression|Tuning", meta = (ClampMin = "0.0"))
	float SuppressionDecayPerSecond = 0.35f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Suppression|Tuning", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float SuppressedThreshold = 0.5f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Suppression|Tuning")
	bool bAutoUpdateBlackboard = true;

	UPROPERTY(BlueprintReadOnly, Category = "Suppression|State")
	float CurrentLevel = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Suppression|State")
	bool bIsSuppressed = false;

	UPROPERTY(BlueprintReadOnly, Category = "Suppression|State")
	TWeakObjectPtr<AActor> LastSuppressionSource;

	UPROPERTY(BlueprintReadOnly, Category = "Suppression|State")
	float LastSuppressionTime = -100.0f;

	UPROPERTY(BlueprintAssignable, Category = "Suppression|Events")
	FZonefallSuppressionChangedEvent OnSuppressionChanged;

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	UFUNCTION(BlueprintCallable, Category = "Zonefall AI|Suppression")
	void RegisterIncomingDamage(AActor* Source, float DamageAmount = 1.0f);

	UFUNCTION(BlueprintCallable, Category = "Zonefall AI|Suppression")
	void RegisterNearMiss(AActor* Source);

	UFUNCTION(BlueprintCallable, Category = "Zonefall AI|Suppression")
	void RegisterSuppressiveFire(AActor* Source);

	UFUNCTION(BlueprintCallable, Category = "Zonefall AI|Suppression")
	void ResetSuppression();

private:
	void ApplyStimulus(float Gain, AActor* Source);
	void PushToBlackboard();
};
