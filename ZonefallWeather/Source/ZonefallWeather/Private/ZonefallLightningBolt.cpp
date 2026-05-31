#include "ZonefallLightningBolt.h"

#include "Components/PointLightComponent.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInterface.h"
#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"
#include "Sound/SoundBase.h"

AZonefallLightningBolt::AZonefallLightningBolt()
{
	PrimaryActorTick.bCanEverTick = true;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);

	FlashLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("FlashLight"));
	FlashLight->SetupAttachment(SceneRoot);
	FlashLight->SetMobility(EComponentMobility::Movable);
	FlashLight->SetIntensity(0.0f);
	FlashLight->SetAttenuationRadius(40000.0f);
	FlashLight->SetLightColor(FLinearColor(0.65f, 0.78f, 1.0f, 1.0f));
	FlashLight->bUseInverseSquaredFalloff = false;

	BoltEffectComponent = CreateDefaultSubobject<UNiagaraComponent>(TEXT("BoltEffect"));
	BoltEffectComponent->SetupAttachment(SceneRoot);
	BoltEffectComponent->SetAutoActivate(false);

	BoltMeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BoltMesh"));
	BoltMeshComponent->SetupAttachment(SceneRoot);
	BoltMeshComponent->SetMobility(EComponentMobility::Movable);
	BoltMeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	BoltMeshComponent->SetVisibility(false);
}

void AZonefallLightningBolt::BeginPlay()
{
	Super::BeginPlay();
}

void AZonefallLightningBolt::TriggerBolt(FVector StrikeOrigin, FVector StrikeTarget, float Strength, FVector ObserverLocation)
{
	bTriggered = true;
	ElapsedSinceTrigger = 0.0f;
	CurrentStrength = FMath::Clamp(Strength, 0.05f, 2.0f);
	bThunderPlayed = false;

	const float StrikeDistance = FVector::Dist(ObserverLocation, StrikeTarget);
	ThunderDelay = ThunderSpeedCmPerSecond > 0.0f ? StrikeDistance / ThunderSpeedCmPerSecond : 0.0f;

	SetActorLocation(StrikeOrigin);
	FlashLight->SetWorldLocation(StrikeOrigin);
	FlashLight->SetIntensity(FlashIntensity * CurrentStrength);

	ConfigureBoltMesh(StrikeOrigin, StrikeTarget);

	if (BoltNiagaraSystem)
	{
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(this, BoltNiagaraSystem, StrikeOrigin, (StrikeTarget - StrikeOrigin).Rotation());
	}
}

void AZonefallLightningBolt::ConfigureBoltMesh(FVector Origin, FVector Target)
{
	if (!FallbackBoltMesh)
	{
		BoltMeshComponent->SetVisibility(false);
		return;
	}

	BoltMeshComponent->SetStaticMesh(FallbackBoltMesh);
	if (FallbackBoltMaterial)
	{
		BoltMeshComponent->SetMaterial(0, FallbackBoltMaterial);
	}

	const FVector ToTarget = Target - Origin;
	const float Length = ToTarget.Size();
	const FRotator Rot = ToTarget.Rotation();
	BoltMeshComponent->SetWorldLocation(Origin);
	BoltMeshComponent->SetWorldRotation(Rot);
	const float Thickness = FMath::Lerp(60.0f, 220.0f, CurrentStrength);
	BoltMeshComponent->SetWorldScale3D(FVector(Length / 100.0f, Thickness / 100.0f, Thickness / 100.0f));
	BoltMeshComponent->SetVisibility(true);
}

void AZonefallLightningBolt::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (!bTriggered)
	{
		return;
	}

	ElapsedSinceTrigger += DeltaSeconds;

	const float FlashAlpha = FMath::Clamp(1.0f - (ElapsedSinceTrigger / FMath::Max(FlashDuration, 0.01f)), 0.0f, 1.0f);
	FlashLight->SetIntensity(FlashIntensity * CurrentStrength * FlashAlpha);
	if (ElapsedSinceTrigger > FlashDuration)
	{
		BoltMeshComponent->SetVisibility(false);
	}

	if (!bThunderPlayed && ElapsedSinceTrigger >= ThunderDelay)
	{
		bThunderPlayed = true;
		if (ThunderSound)
		{
			UGameplayStatics::PlaySoundAtLocation(this, ThunderSound, GetActorLocation(), ThunderVolumeMultiplier * CurrentStrength);
		}
	}

	if (bAutoDestroyAfterThunder && bThunderPlayed && ElapsedSinceTrigger > FlashDuration + 0.5f)
	{
		Destroy();
	}
}
