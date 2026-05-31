#include "ZonefallAITacticalCoverComponent.h"

#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "ZonefallAIController.h"

UZonefallAITacticalCoverComponent::UZonefallAITacticalCoverComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

APawn* UZonefallAITacticalCoverComponent::GetAgentPawn() const
{
	APawn* Owner = Cast<APawn>(GetOwner());
	if (Owner)
	{
		return Owner;
	}

	if (AAIController* Controller = Cast<AAIController>(GetOwner()))
	{
		return Controller->GetPawn();
	}

	return nullptr;
}

bool UZonefallAITacticalCoverComponent::RequestCoverFromTarget(AActor* TargetActor, bool bForceRefresh)
{
	APawn* Agent = GetAgentPawn();
	if (!Agent || !TargetActor)
	{
		return false;
	}

	const UWorld* World = GetWorld();
	const float Now = World ? World->GetTimeSeconds() : 0.0f;

	if (!bForceRefresh && (Now - LastQueryTime) < MinTimeBetweenQueries && bHasCover)
	{
		return true;
	}

	FZonefallTacticalCandidate BestCandidate;
	if (!UZonefallAITacticalQueryLibrary::FindBestCoverPoint(this, Agent, TargetActor, CoverQueryParams, BestCandidate))
	{
		return false;
	}

	LastQueryTime = Now;

	const float DistanceToOldCover = bHasCover ? FVector::Dist(CurrentCoverLocation, BestCandidate.Location) : TNumericLimits<float>::Max();
	if (bHasCover && DistanceToOldCover < MinCoverDistanceToReposition && BestCandidate.Score <= CurrentCoverScore + 0.5f)
	{
		return true;
	}

	CurrentCoverLocation = BestCandidate.Location;
	CurrentCoverFacing = BestCandidate.FacingDirection;
	CurrentCoverScore = BestCandidate.Score;
	bHasCover = true;

	if (bAutoUpdateBlackboard)
	{
		if (AAIController* Controller = Cast<AAIController>(GetOwner()))
		{
			if (UBlackboardComponent* Blackboard = Controller->GetBlackboardComponent())
			{
				const AZonefallAIController* ZonefallController = Cast<AZonefallAIController>(Controller);
				const FZonefallAIBlackboardKeys& Keys = ZonefallController ? ZonefallController->BlackboardKeys : FZonefallAIBlackboardKeys();
				UZonefallAIBlackboardLibrary::SetZonefallCover(Blackboard, Keys, CurrentCoverLocation, CurrentCoverFacing, CurrentCoverScore);
			}
		}
	}

	OnCoverChanged.Broadcast(CurrentCoverLocation, CurrentCoverScore);
	return true;
}

bool UZonefallAITacticalCoverComponent::RequestFlankPosition(AActor* TargetActor, EZonefallAIFlankSide PreferredSide, FVector& OutFlankLocation)
{
	APawn* Agent = GetAgentPawn();
	if (!Agent || !TargetActor)
	{
		return false;
	}

	FZonefallTacticalCandidate BestCandidate;
	if (!UZonefallAITacticalQueryLibrary::FindBestFlankPoint(this, Agent, TargetActor, PreferredSide, CoverQueryParams, BestCandidate))
	{
		return false;
	}

	OutFlankLocation = BestCandidate.Location;
	return true;
}

bool UZonefallAITacticalCoverComponent::RequestRetreatPoint(AActor* ThreatActor, FVector& OutRetreatLocation)
{
	APawn* Agent = GetAgentPawn();
	if (!Agent || !ThreatActor)
	{
		return false;
	}

	FZonefallTacticalCandidate BestCandidate;
	if (!UZonefallAITacticalQueryLibrary::FindBestRetreatPoint(this, Agent, ThreatActor, CoverQueryParams, BestCandidate))
	{
		return false;
	}

	OutRetreatLocation = BestCandidate.Location;
	return true;
}

bool UZonefallAITacticalCoverComponent::IsCurrentCoverCompromisedBy(const AActor* TargetActor) const
{
	if (!bHasCover || !TargetActor)
	{
		return false;
	}

	const FVector CandidateEye = CurrentCoverLocation + FVector(0, 0, CoverQueryParams.EyeHeight);
	const FVector TargetEye = TargetActor->GetActorLocation() + FVector(0, 0, CoverQueryParams.EyeHeight);

	UWorld* World = GetWorld();
	if (!World)
	{
		return false;
	}

	FCollisionQueryParams Params(SCENE_QUERY_STAT(ZonefallCoverCompromised), false);
	Params.AddIgnoredActor(GetOwner());

	FHitResult Hit;
	const bool bBlocked = World->LineTraceSingleByChannel(Hit, CandidateEye, TargetEye, ECC_Visibility, Params) && Hit.bBlockingHit;
	return !bBlocked;
}

void UZonefallAITacticalCoverComponent::ClearCover()
{
	bHasCover = false;
	CurrentCoverLocation = FVector::ZeroVector;
	CurrentCoverFacing = FVector::ZeroVector;
	CurrentCoverScore = 0.0f;

	if (bAutoUpdateBlackboard)
	{
		if (AAIController* Controller = Cast<AAIController>(GetOwner()))
		{
			if (UBlackboardComponent* Blackboard = Controller->GetBlackboardComponent())
			{
				const AZonefallAIController* ZonefallController = Cast<AZonefallAIController>(Controller);
				const FZonefallAIBlackboardKeys& Keys = ZonefallController ? ZonefallController->BlackboardKeys : FZonefallAIBlackboardKeys();
				UZonefallAIBlackboardLibrary::ClearZonefallCover(Blackboard, Keys);
			}
		}
	}
}
