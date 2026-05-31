#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ZonefallNpcVitalsComponent.generated.h"

class AActor;
class APawn;
class UWidgetComponent;
class UUserWidget;
class UZonefallAICharacterComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FZonefallNpcHealthChanged, float, NewHealth, float, MaxHealth);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FZonefallNpcDied);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FZonefallNpcAlerted, AActor*, Threat);

/**
 * Drop-in NPC vitals + perception for Zonefall AI (works with UZonefallAICharacterComponent):
 *  - Health / damage / death (ragdoll), replicated.
 *  - A detection "awareness" meter (0..1) that fills while the NPC has line-of-sight to the
 *    hero (inside its vision cone), decays otherwise; at full it raises an alert and remembers
 *    the hero's last-seen position.
 *  - Spawns and drives a floating status widget above the head (health for hostiles/bandits,
 *    the detection meter while spotting). Civilians show no health bar.
 *
 * Just add it to any NPC character — no Blueprint wiring needed.
 */
UCLASS(ClassGroup = (ZonefallAI), Blueprintable, meta = (BlueprintSpawnableComponent, DisplayName = "Zonefall NPC Vitals"))
class ZONEFALLAI_API UZonefallNpcVitalsComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UZonefallNpcVitalsComponent();

	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	// --- Health ---
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Vitals|Health", meta = (ClampMin = "1.0"))
	float MaxHealth = 100.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Vitals|Health", meta = (ClampMin = "0.0"))
	float RagdollOnDeathImpulse = 12000.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Vitals|Health")
	bool bDestroyAfterDeath = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Vitals|Health", meta = (ClampMin = "0.0", EditCondition = "bDestroyAfterDeath"))
	float DestroyDelaySeconds = 8.0f;

	// --- Detection (awareness) ---
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Vitals|Detection", meta = (ClampMin = "0.0"))
	float VisionRange = 2600.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Vitals|Detection", meta = (ClampMin = "1.0", ClampMax = "180.0"))
	float VisionHalfAngleDegrees = 65.0f;

	// Awareness gained per second at point-blank LOS (scales with distance).
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Vitals|Detection", meta = (ClampMin = "0.0"))
	float AwarenessGainRate = 0.85f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Vitals|Detection", meta = (ClampMin = "0.0"))
	float AwarenessDecayRate = 0.35f;

	// Only hostiles track/alert on the hero (civilians can still flee elsewhere).
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Vitals|Detection")
	bool bOnlyHostilesDetect = true;

	// --- Floating status widget ---
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Vitals|Widget")
	TSubclassOf<UUserWidget> StatusWidgetClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Vitals|Widget")
	float WidgetHeightOffset = 110.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Vitals|Widget")
	FVector2D WidgetDrawSize = FVector2D(150.0f, 30.0f);

	// Hide the widget past this distance from the local camera (0 = never hide).
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Vitals|Widget", meta = (ClampMin = "0.0"))
	float WidgetVisibleDistance = 4500.0f;

	// --- Events ---
	UPROPERTY(BlueprintAssignable, Category = "Vitals")
	FZonefallNpcHealthChanged OnHealthChanged;

	UPROPERTY(BlueprintAssignable, Category = "Vitals")
	FZonefallNpcDied OnDied;

	UPROPERTY(BlueprintAssignable, Category = "Vitals")
	FZonefallNpcAlerted OnAlerted;

	// --- Queries ---
	UFUNCTION(BlueprintPure, Category = "Vitals")
	float GetHealth() const { return Health; }

	UFUNCTION(BlueprintPure, Category = "Vitals")
	float GetHealthFraction() const { return MaxHealth > 0.0f ? FMath::Clamp(Health / MaxHealth, 0.0f, 1.0f) : 0.0f; }

	UFUNCTION(BlueprintPure, Category = "Vitals")
	bool IsDead() const { return bIsDead; }

	UFUNCTION(BlueprintPure, Category = "Vitals")
	float GetAwareness() const { return Awareness; }

	UFUNCTION(BlueprintPure, Category = "Vitals")
	bool IsAlerted() const { return bAlerted; }

	UFUNCTION(BlueprintPure, Category = "Vitals")
	bool IsHostileNpc() const;

	UFUNCTION(BlueprintPure, Category = "Vitals")
	FVector GetLastKnownHeroLocation() const { return LastKnownHeroLocation; }

	UFUNCTION(BlueprintPure, Category = "Vitals")
	bool HasLastKnownHeroLocation() const { return bHasLastKnownHeroLocation; }

	UFUNCTION(BlueprintCallable, Category = "Vitals")
	void Heal(float Amount);

	/** Creates the WidgetComponent + binds ZonefallNpcStatusWidget (safe to call multiple times). */
	UFUNCTION(BlueprintCallable, Category = "Vitals|Widget")
	void SetupStatusWidget();

	/** Pull widget settings from Zonefall AI Character when present. */
	void ApplyWidgetSettingsFromAICharacter(const UZonefallAICharacterComponent* Identity);

private:
	UFUNCTION()
	void HandleAnyDamage(AActor* DamagedActor, float Damage, const class UDamageType* DamageType, class AController* InstigatedBy, AActor* DamageCauser);

	UFUNCTION()
	void OnRep_Health();

	void Die();
	void UpdateAwareness(float DeltaTime);
	void UpdateWidget();
	APawn* ResolveHeroPawn() const;
	bool HasLineOfSightTo(const AActor* Target) const;

	UPROPERTY(ReplicatedUsing = OnRep_Health)
	float Health = 100.0f;

	UPROPERTY(Replicated)
	bool bIsDead = false;

	UPROPERTY(Transient)
	float Awareness = 0.0f;

	UPROPERTY(Transient)
	bool bAlerted = false;

	UPROPERTY(Transient)
	bool bHasLastKnownHeroLocation = false;

	UPROPERTY(Transient)
	FVector LastKnownHeroLocation = FVector::ZeroVector;

	UPROPERTY(Transient)
	TObjectPtr<UWidgetComponent> StatusWidgetComp;

	UPROPERTY(Transient)
	TObjectPtr<UZonefallAICharacterComponent> AIChar;
};
