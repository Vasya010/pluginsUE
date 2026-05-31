#include "ZonefallAISuppressionComponent.h"

#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Engine/World.h"
#include "ZonefallAIBlackboard.h"
#include "ZonefallAIController.h"

UZonefallAISuppressionComponent::UZonefallAISuppressionComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.TickInterval = 0.1f;
}

void UZonefallAISuppressionComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (CurrentLevel <= 0.0f && !bIsSuppressed)
	{
		return;
	}

	const float Previous = CurrentLevel;
	CurrentLevel = FMath::Max(0.0f, CurrentLevel - SuppressionDecayPerSecond * DeltaTime);

	const bool bNowSuppressed = CurrentLevel >= SuppressedThreshold;
	if (bNowSuppressed != bIsSuppressed || !FMath::IsNearlyEqual(Previous, CurrentLevel, 0.02f))
	{
		bIsSuppressed = bNowSuppressed;
		PushToBlackboard();
		OnSuppressionChanged.Broadcast(CurrentLevel, LastSuppressionSource.Get());
	}
}

void UZonefallAISuppressionComponent::RegisterIncomingDamage(AActor* Source, float DamageAmount)
{
	ApplyStimulus(DamageSuppressionGain * FMath::Max(0.1f, DamageAmount / 10.0f), Source);
}

void UZonefallAISuppressionComponent::RegisterNearMiss(AActor* Source)
{
	ApplyStimulus(NearMissSuppressionGain, Source);
}

void UZonefallAISuppressionComponent::RegisterSuppressiveFire(AActor* Source)
{
	ApplyStimulus(SuppressiveFireGain, Source);
}

void UZonefallAISuppressionComponent::ResetSuppression()
{
	CurrentLevel = 0.0f;
	bIsSuppressed = false;
	LastSuppressionSource = nullptr;
	PushToBlackboard();
	OnSuppressionChanged.Broadcast(0.0f, nullptr);
}

void UZonefallAISuppressionComponent::ApplyStimulus(float Gain, AActor* Source)
{
	CurrentLevel = FMath::Clamp(CurrentLevel + Gain, 0.0f, 1.0f);
	LastSuppressionSource = Source;
	LastSuppressionTime = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0f;

	const bool bWasSuppressed = bIsSuppressed;
	bIsSuppressed = CurrentLevel >= SuppressedThreshold;

	PushToBlackboard();
	if (bWasSuppressed != bIsSuppressed || Gain > 0.0f)
	{
		OnSuppressionChanged.Broadcast(CurrentLevel, Source);
	}
}

void UZonefallAISuppressionComponent::PushToBlackboard()
{
	if (!bAutoUpdateBlackboard)
	{
		return;
	}

	AAIController* Controller = Cast<AAIController>(GetOwner());
	if (!Controller)
	{
		if (APawn* OwnerPawn = Cast<APawn>(GetOwner()))
		{
			Controller = Cast<AAIController>(OwnerPawn->GetController());
		}
	}

	if (!Controller)
	{
		return;
	}

	UBlackboardComponent* Blackboard = Controller->GetBlackboardComponent();
	if (!Blackboard)
	{
		return;
	}

	const AZonefallAIController* ZonefallController = Cast<AZonefallAIController>(Controller);
	const FZonefallAIBlackboardKeys& Keys = ZonefallController ? ZonefallController->BlackboardKeys : FZonefallAIBlackboardKeys();
	UZonefallAIBlackboardLibrary::SetZonefallSuppression(Blackboard, Keys, CurrentLevel, LastSuppressionSource.Get());
}
