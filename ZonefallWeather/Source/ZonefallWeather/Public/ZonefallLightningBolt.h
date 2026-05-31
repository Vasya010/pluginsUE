#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ZonefallLightningBolt.generated.h"

class UNiagaraComponent;
class UNiagaraSystem;
class UPointLightComponent;
class USoundBase;
class USceneComponent;
class UStaticMeshComponent;
class UStaticMesh;
class UMaterialInterface;

UCLASS(Blueprintable)
class ZONEFALLWEATHER_API AZonefallLightningBolt : public AActor
{
	GENERATED_BODY()

public:
	AZonefallLightningBolt();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Zonefall Lightning")
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Zonefall Lightning")
	TObjectPtr<UPointLightComponent> FlashLight;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Zonefall Lightning")
	TObjectPtr<UNiagaraComponent> BoltEffectComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Zonefall Lightning")
	TObjectPtr<UStaticMeshComponent> BoltMeshComponent;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zonefall Lightning")
	TObjectPtr<UNiagaraSystem> BoltNiagaraSystem;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zonefall Lightning")
	TObjectPtr<UStaticMesh> FallbackBoltMesh;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zonefall Lightning")
	TObjectPtr<UMaterialInterface> FallbackBoltMaterial;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zonefall Lightning")
	TObjectPtr<USoundBase> ThunderSound;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zonefall Lightning", meta = (ClampMin = "0.05"))
	float FlashDuration = 0.22f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zonefall Lightning", meta = (ClampMin = "0.0"))
	float FlashIntensity = 80000.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zonefall Lightning", meta = (ClampMin = "0.0"))
	float ThunderSpeedCmPerSecond = 34300.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zonefall Lightning", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float ThunderVolumeMultiplier = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zonefall Lightning")
	bool bAutoDestroyAfterThunder = true;

	UFUNCTION(BlueprintCallable, Category = "Zonefall Lightning")
	void TriggerBolt(FVector StrikeOrigin, FVector StrikeTarget, float Strength, FVector ObserverLocation);

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

private:
	UPROPERTY(Transient)
	float ElapsedSinceTrigger = 0.0f;

	UPROPERTY(Transient)
	float ThunderDelay = 0.0f;

	UPROPERTY(Transient)
	float CurrentStrength = 1.0f;

	UPROPERTY(Transient)
	bool bThunderPlayed = false;

	UPROPERTY(Transient)
	bool bTriggered = false;

	void ConfigureBoltMesh(FVector Origin, FVector Target);
};
