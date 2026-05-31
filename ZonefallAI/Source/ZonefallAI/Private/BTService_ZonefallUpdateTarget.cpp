#include "BTService_ZonefallUpdateTarget.h"

#include "AIController.h"
#include "BehaviorTree/BehaviorTreeComponent.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "GameFramework/Pawn.h"
#include "Perception/AIPerceptionComponent.h"
#include "Perception/AISense_Sight.h"
#include "ZonefallAICharacterComponent.h"
#include "ZonefallAIPerceptionMemoryComponent.h"

UBTService_ZonefallUpdateTarget::UBTService_ZonefallUpdateTarget()
{
	NodeName = TEXT("Zonefall Update Target");
	bNotifyBecomeRelevant = true;
	bNotifyTick = true;
	Interval = 0.25f;
	RandomDeviation = 0.1f;
}

void UBTService_ZonefallUpdateTarget::OnBecomeRelevant(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	Super::OnBecomeRelevant(OwnerComp, NodeMemory);
	UpdateTarget(OwnerComp);
}

void UBTService_ZonefallUpdateTarget::TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
	Super::TickNode(OwnerComp, NodeMemory, DeltaSeconds);
	UpdateTarget(OwnerComp);
}

void UBTService_ZonefallUpdateTarget::UpdateTarget(UBehaviorTreeComponent& OwnerComp) const
{
	AAIController* AIController = OwnerComp.GetAIOwner();
	UBlackboardComponent* Blackboard = OwnerComp.GetBlackboardComponent();
	if (!AIController || !Blackboard || !AIController->GetPawn())
	{
		return;
	}

	UAIPerceptionComponent* Perception = AIController->GetPerceptionComponent();
	if (!Perception)
	{
		return;
	}

	TArray<AActor*> PerceivedActors;
	if (bOnlyUseSightPerception)
	{
		Perception->GetCurrentlyPerceivedActors(UAISense_Sight::StaticClass(), PerceivedActors);
	}
	else
	{
		Perception->GetCurrentlyPerceivedActors(nullptr, PerceivedActors);
	}

	AActor* BestTarget = nullptr;
	float BestScore = TNumericLimits<float>::Lowest();
	const FVector PawnLocation = AIController->GetPawn()->GetActorLocation();
	const UZonefallAICharacterComponent* SelfIdentity = UZonefallAICharacterComponent::FindAICharacterComponent(AIController->GetPawn());
	UZonefallAIPerceptionMemoryComponent* MemoryComponent = AIController->FindComponentByClass<UZonefallAIPerceptionMemoryComponent>();
	if (MemoryComponent)
	{
		MemoryComponent->WriteToBlackboard(Blackboard, BlackboardKeys);
	}

	for (AActor* Candidate : PerceivedActors)
	{
		if (!IsValid(Candidate) || Candidate == AIController->GetPawn())
		{
			continue;
		}

		const float Distance = FVector::Dist(PawnLocation, Candidate->GetActorLocation());
		if (MaxTargetDistance > 0.0f && Distance > MaxTargetDistance)
		{
			continue;
		}

		const bool bHasLineOfSight = AIController->LineOfSightTo(Candidate);
		if (bRequireLineOfSight && !bHasLineOfSight)
		{
			continue;
		}

		float IdentityScore = 0.0f;
		if (bUseIdentityTargetRules && SelfIdentity)
		{
			const UZonefallAICharacterComponent* CandidateIdentity = UZonefallAICharacterComponent::FindAICharacterComponent(Candidate);
			if (SelfIdentity->bIgnorePlayerAsTarget && SelfIdentity->IsMainHeroActor(Candidate))
			{
				continue;
			}

			if (bIgnoreHarmlessTargets && CandidateIdentity && CandidateIdentity->IsHarmless() && !SelfIdentity->bCanTargetHarmlessActors)
			{
				continue;
			}

			if (!SelfIdentity->CanTargetActor(Candidate))
			{
				continue;
			}

			IdentityScore = SelfIdentity->GetThreatScoreAgainst(Candidate);
			if (SelfIdentity->ShouldForceChaseActor(Candidate))
			{
				IdentityScore += 10.0f;
			}
		}

		const float DistanceAlpha = MaxTargetDistance > 0.0f ? 1.0f - FMath::Clamp(Distance / MaxTargetDistance, 0.0f, 1.0f) : 1.0f;
		const float Score = (DistanceAlpha * DistanceWeight) + (bHasLineOfSight ? LineOfSightWeight : 0.0f) + IdentityScore;
		if (Score > BestScore)
		{
			BestScore = Score;
			BestTarget = Candidate;
		}
	}

	if (BestTarget)
	{
		if (MemoryComponent)
		{
			MemoryComponent->RecordSightStimulus(BestTarget, BestTarget->GetActorLocation(), 1.0f);
			MemoryComponent->WriteToBlackboard(Blackboard, BlackboardKeys);
			if (!MemoryComponent->IsDetectionComplete())
			{
				return;
			}
		}
		UZonefallAIBlackboardLibrary::SetZonefallTarget(Blackboard, BlackboardKeys, BestTarget, AIController->LineOfSightTo(BestTarget));
		Blackboard->SetValueAsFloat(BlackboardKeys.TargetThreatScore, BestScore);
		if (SelfIdentity && SelfIdentity->ShouldForceChaseActor(BestTarget))
		{
			Blackboard->SetValueAsFloat(BlackboardKeys.DesiredSpeed, SelfIdentity->MainHeroChaseSpeed);
		}
		return;
	}

	const float LastSeenTime = Blackboard->GetValueAsFloat(BlackboardKeys.LastSeenTime);
	const float CurrentTime = AIController->GetWorld() ? AIController->GetWorld()->GetTimeSeconds() : 0.0f;
	const bool bCanInvestigateLastKnown = (LastSeenTime > 0.0f && CurrentTime - LastSeenTime <= LostTargetGraceTime) || (MemoryComponent && MemoryComponent->bShouldInvestigate);

	UZonefallAIBlackboardLibrary::ClearZonefallTarget(Blackboard, BlackboardKeys, bCanInvestigateLastKnown);
	if (MemoryComponent)
	{
		MemoryComponent->WriteToBlackboard(Blackboard, BlackboardKeys);
	}
}
