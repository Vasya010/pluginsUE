#include "ZonefallAIPerceptionMemoryComponent.h"

#include "BehaviorTree/BlackboardComponent.h"
#include "GameFramework/Actor.h"

UZonefallAIPerceptionMemoryComponent::UZonefallAIPerceptionMemoryComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
}

void UZonefallAIPerceptionMemoryComponent::BeginPlay()
{
	Super::BeginPlay();
}

void UZonefallAIPerceptionMemoryComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (LastStimulusTime < 0.0f)
	{
		return;
	}

	const float TimeSinceStimulus = GetTimeSinceLastStimulus();
	MemoryConfidence = FMath::Max(0.0f, MemoryConfidence - MemoryDecayPerSecond * DeltaTime);
	Suspicion = FMath::Max(0.0f, Suspicion - SuspicionDecayPerSecond * DeltaTime);
	if (!bHasLineOfSight && !bDetectionComplete)
	{
		DetectionProgress = FMath::Max(0.0f, DetectionProgress - DetectionDecayPerSecond * DeltaTime);
		OnDetectionProgressChanged.Broadcast(DetectionProgress);
	}
	Alertness = FMath::Clamp(FMath::Max(MemoryConfidence, Suspicion), 0.0f, 1.0f);

	if (TimeSinceStimulus >= ForgetTargetAfterSeconds && !bHasLineOfSight)
	{
		ClearMemory();
		return;
	}

	RefreshInvestigationState();
}

void UZonefallAIPerceptionMemoryComponent::RecordSightStimulus(AActor* TargetActor, FVector SeenLocation, float StimulusStrength)
{
	KnownTargetActor = TargetActor;
	LastSeenLocation = SeenLocation;
	InvestigationLocation = SeenLocation;
	LastSeenTime = GetNow();
	LastStimulusTime = LastSeenTime;
	LastStimulusType = EZonefallAIStimulusType::Sight;
	bHasLineOfSight = true;
	DetectionProgress = FMath::Clamp(DetectionProgress + SightDetectionGain * FMath::Max(StimulusStrength, 0.25f), 0.0f, DetectionThreshold);
	bDetectionComplete = DetectionProgress >= DetectionThreshold;
	MemoryConfidence = FMath::Clamp(MemoryConfidence + SightConfidenceGain * FMath::Max(StimulusStrength, 0.25f), 0.0f, 1.0f);
	Suspicion = FMath::Max(Suspicion, 0.65f);
	Alertness = 1.0f;
	bShouldInvestigate = false;
	OnDetectionProgressChanged.Broadcast(DetectionProgress);

	if (bDetectionComplete)
	{
		OnTargetSpotted.Broadcast(TargetActor);
	}
}

void UZonefallAIPerceptionMemoryComponent::RecordHearingStimulus(AActor* SourceActor, FVector HeardLocation, float StimulusStrength)
{
	LastHeardLocation = HeardLocation;
	InvestigationLocation = HeardLocation;
	LastHeardTime = GetNow();
	LastStimulusTime = LastHeardTime;
	LastStimulusType = EZonefallAIStimulusType::Hearing;
	bHasLineOfSight = false;
	bDetectionComplete = false;
	Suspicion = FMath::Clamp(Suspicion + HearingSuspicionGain * FMath::Max(StimulusStrength, 0.25f), 0.0f, 1.0f);
	MemoryConfidence = FMath::Max(MemoryConfidence, Suspicion * 0.5f);
	Alertness = FMath::Clamp(FMath::Max(Alertness, Suspicion), 0.0f, 1.0f);
	RefreshInvestigationState();

	OnSuspiciousStimulus.Broadcast(HeardLocation, EZonefallAIStimulusType::Hearing);
	if (bShouldInvestigate)
	{
		OnInvestigationStarted.Broadcast(InvestigationLocation, LastStimulusType);
	}
}

void UZonefallAIPerceptionMemoryComponent::RecordLostSight(AActor* TargetActor, FVector LastKnownLocation)
{
	if (TargetActor && KnownTargetActor != TargetActor)
	{
		KnownTargetActor = TargetActor;
	}

	LastSeenLocation = LastKnownLocation;
	InvestigationLocation = LastKnownLocation;
	LastStimulusTime = GetNow();
	LastStimulusType = EZonefallAIStimulusType::Sight;
	bHasLineOfSight = false;
	bDetectionComplete = false;
	Suspicion = FMath::Clamp(Suspicion + LostSightSuspicionGain, 0.0f, 1.0f);
	MemoryConfidence = FMath::Max(MemoryConfidence, 0.45f);
	RefreshInvestigationState();

	OnTargetLost.Broadcast(TargetActor);
	if (bShouldInvestigate)
	{
		OnInvestigationStarted.Broadcast(InvestigationLocation, LastStimulusType);
	}
}

void UZonefallAIPerceptionMemoryComponent::ClearMemory()
{
	KnownTargetActor = nullptr;
	LastSeenLocation = FVector::ZeroVector;
	LastHeardLocation = FVector::ZeroVector;
	InvestigationLocation = FVector::ZeroVector;
	LastSeenTime = -1.0f;
	LastHeardTime = -1.0f;
	LastStimulusTime = -1.0f;
	MemoryConfidence = 0.0f;
	Suspicion = 0.0f;
	Alertness = 0.0f;
	DetectionProgress = 0.0f;
	bHasLineOfSight = false;
	bDetectionComplete = false;
	bShouldInvestigate = false;
	LastStimulusType = EZonefallAIStimulusType::None;
}

void UZonefallAIPerceptionMemoryComponent::ClearDetection()
{
	DetectionProgress = 0.0f;
	bDetectionComplete = false;
	OnDetectionProgressChanged.Broadcast(DetectionProgress);
}

void UZonefallAIPerceptionMemoryComponent::WriteToBlackboard(UBlackboardComponent* Blackboard, const FZonefallAIBlackboardKeys& Keys) const
{
	if (!Blackboard)
	{
		return;
	}

	Blackboard->SetValueAsVector(Keys.LastKnownTargetLocation, LastSeenLocation);
	Blackboard->SetValueAsVector(Keys.LastHeardLocation, LastHeardLocation);
	Blackboard->SetValueAsVector(Keys.InvestigationLocation, InvestigationLocation);
	Blackboard->SetValueAsFloat(Keys.LastSeenTime, LastSeenTime);
	Blackboard->SetValueAsFloat(Keys.MemoryConfidence, MemoryConfidence);
	Blackboard->SetValueAsFloat(Keys.Suspicion, Suspicion);
	Blackboard->SetValueAsFloat(Keys.Alertness, Alertness);
	Blackboard->SetValueAsFloat(Keys.TimeSinceLastStimulus, GetTimeSinceLastStimulus());
	Blackboard->SetValueAsEnum(Keys.StimulusType, static_cast<uint8>(LastStimulusType));
	Blackboard->SetValueAsBool(Keys.ShouldInvestigate, bShouldInvestigate);
	Blackboard->SetValueAsBool(Keys.HasLineOfSight, bHasLineOfSight);

	if (KnownTargetActor)
	{
		Blackboard->SetValueAsObject(Keys.TargetActor, KnownTargetActor);
	}
}

float UZonefallAIPerceptionMemoryComponent::GetTimeSinceLastStimulus() const
{
	return LastStimulusTime >= 0.0f ? FMath::Max(0.0f, GetNow() - LastStimulusTime) : 0.0f;
}

bool UZonefallAIPerceptionMemoryComponent::HasActionableMemory() const
{
	return bShouldInvestigate || bHasLineOfSight || MemoryConfidence >= InvestigationThreshold || Suspicion >= InvestigationThreshold;
}

bool UZonefallAIPerceptionMemoryComponent::IsDetectionComplete() const
{
	return bDetectionComplete || DetectionProgress >= DetectionThreshold;
}

float UZonefallAIPerceptionMemoryComponent::GetNow() const
{
	return GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0f;
}

void UZonefallAIPerceptionMemoryComponent::RefreshInvestigationState()
{
	bShouldInvestigate = !bHasLineOfSight && (Suspicion >= InvestigationThreshold || MemoryConfidence >= InvestigationThreshold) && !InvestigationLocation.IsNearlyZero();
}
