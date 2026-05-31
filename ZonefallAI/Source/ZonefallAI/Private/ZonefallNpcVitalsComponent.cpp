#include "ZonefallNpcVitalsComponent.h"

#include "ZonefallAICharacterComponent.h"
#include "ZonefallNpcStatusWidget.h"

#include "Blueprint/UserWidget.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/WidgetComponent.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Net/UnrealNetwork.h"
#include "TimerManager.h"

UZonefallNpcVitalsComponent::UZonefallNpcVitalsComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.TickInterval = 0.05f; // 20 Hz is plenty for detection/UI
	SetIsReplicatedByDefault(true);
}

void UZonefallNpcVitalsComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(UZonefallNpcVitalsComponent, Health);
	DOREPLIFETIME(UZonefallNpcVitalsComponent, bIsDead);
}

void UZonefallNpcVitalsComponent::BeginPlay()
{
	Super::BeginPlay();

	AActor* Owner = GetOwner();
	if (!Owner)
	{
		return;
	}

	AIChar = UZonefallAICharacterComponent::FindAICharacterComponent(Owner);

	if (Owner->HasAuthority())
	{
		Health = MaxHealth;
		Owner->OnTakeAnyDamage.AddDynamic(this, &UZonefallNpcVitalsComponent::HandleAnyDamage);
	}

	// Create the floating status widget. Deferred one tick so the local player / viewport
	// exists (Screen-space widget components need that to render) — this is the usual reason
	// a head widget "doesn't draw" when created too early in BeginPlay.
	if (UWorld* World = GetWorld())
	{
		FTimerHandle Handle;
		World->GetTimerManager().SetTimerForNextTick(this, &UZonefallNpcVitalsComponent::SetupStatusWidget);
	}
	else
	{
		SetupStatusWidget();
	}
}

void UZonefallNpcVitalsComponent::ApplyWidgetSettingsFromAICharacter(const UZonefallAICharacterComponent* Identity)
{
	if (!Identity)
	{
		return;
	}

	// Let the AI Character drive widget look/placement when it provides values.
	if (Identity->StatusWidgetClass)
	{
		StatusWidgetClass = Identity->StatusWidgetClass;
	}
	WidgetHeightOffset = Identity->WidgetHeightOffset;
	WidgetDrawSize = Identity->WidgetDrawSize;
	WidgetVisibleDistance = Identity->WidgetVisibleDistance;
}

void UZonefallNpcVitalsComponent::SetupStatusWidget()
{
	AActor* Owner = GetOwner();
	if (!Owner || StatusWidgetComp)
	{
		return; // already created
	}

	// Respect the AI Character's toggle (it may want vitals/health/awareness without a floater).
	if (!AIChar)
	{
		AIChar = UZonefallAICharacterComponent::FindAICharacterComponent(Owner);
	}
	ApplyWidgetSettingsFromAICharacter(AIChar);
	if (AIChar && !AIChar->bEnableFloatingStatusWidget)
	{
		return;
	}

	const TSubclassOf<UUserWidget> WidgetClass = StatusWidgetClass
		? StatusWidgetClass
		: TSubclassOf<UUserWidget>(UZonefallNpcStatusWidget::StaticClass());

	StatusWidgetComp = NewObject<UWidgetComponent>(Owner, TEXT("ZonefallNpcStatusWidget"));
	if (!StatusWidgetComp)
	{
		return;
	}

	// Attach to the mesh if there is one (so it tracks the head), else the root.
	USceneComponent* AttachTo = Owner->GetRootComponent();
	if (const ACharacter* OwnerChar = Cast<ACharacter>(Owner))
	{
		if (OwnerChar->GetMesh())
		{
			AttachTo = OwnerChar->GetMesh();
		}
	}

	StatusWidgetComp->SetupAttachment(AttachTo);
	// Configure BEFORE registering so the widget is built with the right class/space.
	StatusWidgetComp->SetWidgetSpace(EWidgetSpace::Screen);
	StatusWidgetComp->SetDrawSize(WidgetDrawSize);
	StatusWidgetComp->SetWidgetClass(WidgetClass);
	StatusWidgetComp->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	StatusWidgetComp->SetDrawAtDesiredSize(false);
	StatusWidgetComp->SetRelativeLocation(FVector(0.0f, 0.0f, WidgetHeightOffset));
	StatusWidgetComp->RegisterComponent();

	// Force the underlying UUserWidget to be created now, then bind it to us.
	StatusWidgetComp->InitWidget();
	if (UZonefallNpcStatusWidget* W = Cast<UZonefallNpcStatusWidget>(StatusWidgetComp->GetUserWidgetObject()))
	{
		W->BindToVitals(this);
	}

	StatusWidgetComp->SetVisibility(false);
}

bool UZonefallNpcVitalsComponent::IsHostileNpc() const
{
	return AIChar ? AIChar->IsHostile() : false;
}

APawn* UZonefallNpcVitalsComponent::ResolveHeroPawn() const
{
	if (const UWorld* World = GetWorld())
	{
		if (const APlayerController* PC = World->GetFirstPlayerController())
		{
			return PC->GetPawn();
		}
	}
	return nullptr;
}

bool UZonefallNpcVitalsComponent::HasLineOfSightTo(const AActor* Target) const
{
	const AActor* Owner = GetOwner();
	UWorld* World = GetWorld();
	if (!Owner || !Target || !World)
	{
		return false;
	}

	const FVector Eyes = Owner->GetActorLocation() + FVector(0, 0, 60.0f);
	const FVector TargetLoc = Target->GetActorLocation();

	FHitResult Hit;
	FCollisionQueryParams Params(SCENE_QUERY_STAT(ZonefallNpcLOS), /*bTraceComplex*/ false);
	Params.AddIgnoredActor(Owner);
	Params.AddIgnoredActor(Target);

	// Visible if nothing solid blocks the ray to the target.
	return !World->LineTraceSingleByChannel(Hit, Eyes, TargetLoc, ECC_Visibility, Params);
}

void UZonefallNpcVitalsComponent::UpdateAwareness(float DeltaTime)
{
	if (bIsDead || (bOnlyHostilesDetect && !IsHostileNpc()))
	{
		Awareness = FMath::Max(0.0f, Awareness - AwarenessDecayRate * DeltaTime);
		return;
	}

	const AActor* Owner = GetOwner();
	APawn* Hero = ResolveHeroPawn();
	if (!Owner || !Hero)
	{
		Awareness = FMath::Max(0.0f, Awareness - AwarenessDecayRate * DeltaTime);
		return;
	}

	const FVector ToHero = Hero->GetActorLocation() - Owner->GetActorLocation();
	const float Dist = ToHero.Size();

	bool bCanSee = false;
	if (Dist <= VisionRange && Dist > 1.0f)
	{
		const FVector Forward = Owner->GetActorForwardVector();
		const float CosAngle = FVector::DotProduct(Forward, ToHero.GetSafeNormal());
		const float CosHalf = FMath::Cos(FMath::DegreesToRadians(VisionHalfAngleDegrees));
		if (CosAngle >= CosHalf && HasLineOfSightTo(Hero))
		{
			bCanSee = true;
		}
	}

	if (bCanSee)
	{
		// Closer = spotted faster.
		const float DistanceFactor = FMath::Lerp(1.6f, 0.6f, FMath::Clamp(Dist / FMath::Max(1.0f, VisionRange), 0.0f, 1.0f));
		Awareness = FMath::Min(1.0f, Awareness + AwarenessGainRate * DistanceFactor * DeltaTime);

		// Remember where the hero is while we can see them.
		LastKnownHeroLocation = Hero->GetActorLocation();
		bHasLastKnownHeroLocation = true;

		if (Awareness >= 1.0f && !bAlerted)
		{
			bAlerted = true;
			OnAlerted.Broadcast(const_cast<AActor*>(static_cast<const AActor*>(Hero)));
		}
	}
	else
	{
		Awareness = FMath::Max(0.0f, Awareness - AwarenessDecayRate * DeltaTime);
		if (Awareness <= 0.0f)
		{
			bAlerted = false; // lost track (but LastKnownHeroLocation is retained as memory)
		}
	}
}

void UZonefallNpcVitalsComponent::UpdateWidget()
{
	// Lazily create the widget if the deferred setup hasn't run yet (robustness).
	if (!StatusWidgetComp)
	{
		SetupStatusWidget();
		if (!StatusWidgetComp)
		{
			return;
		}
	}

	bool bShow = !bIsDead;

	// Hostiles: always show (health + detection). Civilians: only flash the detection bar
	// while something is spotting, otherwise stay clean (no floating health bar).
	const bool bHostile = IsHostileNpc();
	if (!bHostile && Awareness <= 0.02f)
	{
		bShow = false;
	}

	// Distance cull from the local camera.
	if (bShow && WidgetVisibleDistance > 0.0f)
	{
		if (const UWorld* World = GetWorld())
		{
			if (const APlayerController* PC = World->GetFirstPlayerController())
			{
				FVector CamLoc; FRotator CamRot;
				PC->GetPlayerViewPoint(CamLoc, CamRot);
				if (FVector::Dist(CamLoc, GetOwner()->GetActorLocation()) > WidgetVisibleDistance)
				{
					bShow = false;
				}
			}
		}
	}

	StatusWidgetComp->SetVisibility(bShow);
}

void UZonefallNpcVitalsComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	UpdateAwareness(DeltaTime);
	UpdateWidget();
}

void UZonefallNpcVitalsComponent::HandleAnyDamage(AActor* /*DamagedActor*/, float Damage, const UDamageType* /*DamageType*/, AController* /*InstigatedBy*/, AActor* /*DamageCauser*/)
{
	if (bIsDead || Damage <= 0.0f || !GetOwner() || !GetOwner()->HasAuthority())
	{
		return;
	}

	Health = FMath::Clamp(Health - Damage, 0.0f, MaxHealth);
	OnHealthChanged.Broadcast(Health, MaxHealth);

	// Any hit makes a hostile instantly suspicious.
	if (IsHostileNpc())
	{
		Awareness = FMath::Max(Awareness, 0.6f);
	}

	if (Health <= 0.0f)
	{
		Die();
	}
}

void UZonefallNpcVitalsComponent::Heal(float Amount)
{
	if (bIsDead || Amount <= 0.0f || !GetOwner() || !GetOwner()->HasAuthority())
	{
		return;
	}
	Health = FMath::Clamp(Health + Amount, 0.0f, MaxHealth);
	OnHealthChanged.Broadcast(Health, MaxHealth);
}

void UZonefallNpcVitalsComponent::OnRep_Health()
{
	OnHealthChanged.Broadcast(Health, MaxHealth);
}

void UZonefallNpcVitalsComponent::Die()
{
	if (bIsDead)
	{
		return;
	}
	bIsDead = true;
	OnDied.Broadcast();

	if (ACharacter* OwnerChar = Cast<ACharacter>(GetOwner()))
	{
		if (USkeletalMeshComponent* MeshComp = OwnerChar->GetMesh())
		{
			MeshComp->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
			MeshComp->SetCollisionProfileName(TEXT("Ragdoll"));
			MeshComp->SetSimulatePhysics(true);
		}
		if (UCapsuleComponent* Capsule = OwnerChar->GetCapsuleComponent())
		{
			Capsule->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		}
		if (UCharacterMovementComponent* Move = OwnerChar->GetCharacterMovement())
		{
			Move->DisableMovement();
			Move->StopMovementImmediately();
		}
	}

	if (StatusWidgetComp)
	{
		StatusWidgetComp->SetVisibility(false);
	}

	if (bDestroyAfterDeath && GetOwner())
	{
		FTimerHandle Handle;
		GetWorld()->GetTimerManager().SetTimer(Handle, FTimerDelegate::CreateWeakLambda(GetOwner(), [Owner = GetOwner()]()
		{
			if (Owner) { Owner->Destroy(); }
		}), FMath::Max(0.1f, DestroyDelaySeconds), false);
	}
}
