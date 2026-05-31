#include "ZonefallAITacticalQuery.h"

#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "NavigationSystem.h"
#include "Kismet/KismetSystemLibrary.h"

namespace
{
	UWorld* ResolveWorld(UObject* WorldContextObject)
	{
		return WorldContextObject ? WorldContextObject->GetWorld() : nullptr;
	}

	bool ProjectToNav(UWorld* World, const FVector& InLocation, float Extent, FVector& OutProjected)
	{
		if (!World)
		{
			return false;
		}
		UNavigationSystemV1* NavSys = UNavigationSystemV1::GetNavigationSystem(World);
		if (!NavSys)
		{
			OutProjected = InLocation;
			return false;
		}
		FNavLocation NavLocation;
		const bool bSuccess = NavSys->ProjectPointToNavigation(InLocation, NavLocation, FVector(Extent, Extent, Extent));
		if (bSuccess)
		{
			OutProjected = NavLocation.Location;
		}
		else
		{
			OutProjected = InLocation;
		}
		return bSuccess;
	}

	bool TraceBlocksLineOfSight(UWorld* World, const FVector& Start, const FVector& End, AActor* IgnoreActor)
	{
		if (!World)
		{
			return false;
		}
		FCollisionQueryParams Params(SCENE_QUERY_STAT(ZonefallTacticalLOS), false);
		if (IgnoreActor)
		{
			Params.AddIgnoredActor(IgnoreActor);
		}
		FHitResult Hit;
		const bool bHit = World->LineTraceSingleByChannel(Hit, Start, End, ECC_Visibility, Params);
		return bHit && Hit.bBlockingHit;
	}

	float ScoreCoverCandidate(const FZonefallTacticalCandidate& Candidate, const FZonefallTacticalQueryParams& Params, float FlankDot)
	{
		float Score = 0.0f;
		Score += (Candidate.bHasCoverFromTarget ? Params.CoverWeight : -Params.CoverWeight * 0.5f);
		Score += (Candidate.bHasLineOfSightToTarget ? Params.LineOfSightWeight : 0.0f);
		Score -= Candidate.DistanceFromAgent * 0.001f * Params.DistanceFromAgentPenalty;

		const float TargetDelta = FMath::Abs(Candidate.DistanceFromTarget - Params.IdealDistanceFromTarget);
		Score -= (TargetDelta * 0.001f) * Params.DistanceFromTargetWeight;

		if (Candidate.DistanceFromTarget < Params.MinDistanceFromTarget)
		{
			Score -= Params.TooCloseToTargetPenalty;
		}

		if (Params.FlankWeight != 0.0f)
		{
			Score += FlankDot * Params.FlankWeight;
		}
		return Score;
	}
}

bool UZonefallAITacticalQueryLibrary::GatherTacticalCandidates(
	UObject* WorldContextObject,
	APawn* Agent,
	AActor* TargetActor,
	const FZonefallTacticalQueryParams& Params,
	TArray<FZonefallTacticalCandidate>& OutCandidates)
{
	OutCandidates.Reset();

	UWorld* World = ResolveWorld(WorldContextObject);
	if (!World || !Agent || !TargetActor)
	{
		return false;
	}

	const FVector AgentLocation = Agent->GetActorLocation();
	const FVector TargetLocation = TargetActor->GetActorLocation();
	const FVector TargetEye = TargetLocation + FVector(0, 0, Params.EyeHeight);
	const FVector ToTarget = (TargetLocation - AgentLocation).GetSafeNormal2D();
	FVector RightAxis = FVector::CrossProduct(FVector::UpVector, ToTarget).GetSafeNormal();
	if (RightAxis.IsNearlyZero())
	{
		RightAxis = FVector::RightVector;
	}

	const int32 Samples = FMath::Max(4, Params.SampleCount);
	OutCandidates.Reserve(Samples);

	for (int32 Index = 0; Index < Samples; ++Index)
	{
		const float Angle = (2.0f * PI * Index) / static_cast<float>(Samples);
		const float Radius = FMath::Lerp(Params.MinSampleRadius, Params.SampleRadius, FMath::Frac(0.37f * Index + 0.13f));
		const FVector Offset(FMath::Cos(Angle) * Radius, FMath::Sin(Angle) * Radius, 0.0f);
		FVector RawLocation = AgentLocation + Offset;

		FVector ProjectedLocation = RawLocation;
		if (Params.bMustProjectToNav)
		{
			if (!ProjectToNav(World, RawLocation, Params.NavProjectionExtent, ProjectedLocation))
			{
				continue;
			}
		}

		const FVector CandidateEye = ProjectedLocation + FVector(0, 0, Params.EyeHeight);
		const bool bBlocked = TraceBlocksLineOfSight(World, CandidateEye, TargetEye, Agent);

		FZonefallTacticalCandidate Candidate;
		Candidate.Location = ProjectedLocation;
		Candidate.FacingDirection = (TargetLocation - ProjectedLocation).GetSafeNormal();
		Candidate.bHasCoverFromTarget = bBlocked;
		Candidate.bHasLineOfSightToTarget = !bBlocked;
		Candidate.DistanceFromAgent = FVector::Dist(ProjectedLocation, AgentLocation);
		Candidate.DistanceFromTarget = FVector::Dist(ProjectedLocation, TargetLocation);
		Candidate.Score = ScoreCoverCandidate(Candidate, Params, 0.0f);

		OutCandidates.Add(Candidate);
	}

	return OutCandidates.Num() > 0;
}

bool UZonefallAITacticalQueryLibrary::FindBestCoverPoint(
	UObject* WorldContextObject,
	APawn* Agent,
	AActor* TargetActor,
	const FZonefallTacticalQueryParams& Params,
	FZonefallTacticalCandidate& OutBestCandidate)
{
	TArray<FZonefallTacticalCandidate> Candidates;
	if (!GatherTacticalCandidates(WorldContextObject, Agent, TargetActor, Params, Candidates))
	{
		return false;
	}

	float BestScore = TNumericLimits<float>::Lowest();
	bool bFound = false;
	for (const FZonefallTacticalCandidate& Candidate : Candidates)
	{
		if (Candidate.Score > BestScore)
		{
			BestScore = Candidate.Score;
			OutBestCandidate = Candidate;
			bFound = true;
		}
	}
	return bFound;
}

bool UZonefallAITacticalQueryLibrary::FindBestFlankPoint(
	UObject* WorldContextObject,
	APawn* Agent,
	AActor* TargetActor,
	EZonefallAIFlankSide PreferredSide,
	const FZonefallTacticalQueryParams& Params,
	FZonefallTacticalCandidate& OutBestCandidate)
{
	UWorld* World = ResolveWorld(WorldContextObject);
	if (!World || !Agent || !TargetActor)
	{
		return false;
	}

	const FVector AgentLocation = Agent->GetActorLocation();
	const FVector TargetLocation = TargetActor->GetActorLocation();
	const FVector TargetForward = TargetActor->GetActorForwardVector().GetSafeNormal2D();
	const FVector TargetRight = TargetActor->GetActorRightVector().GetSafeNormal2D();

	FVector PreferredOffset = FVector::ZeroVector;
	switch (PreferredSide)
	{
	case EZonefallAIFlankSide::Left:
		PreferredOffset = -TargetRight * Params.IdealDistanceFromTarget - TargetForward * (Params.IdealDistanceFromTarget * 0.5f);
		break;
	case EZonefallAIFlankSide::Right:
		PreferredOffset = TargetRight * Params.IdealDistanceFromTarget - TargetForward * (Params.IdealDistanceFromTarget * 0.5f);
		break;
	case EZonefallAIFlankSide::Rear:
		PreferredOffset = -TargetForward * Params.IdealDistanceFromTarget * 1.2f;
		break;
	default:
		PreferredOffset = (AgentLocation - TargetLocation).GetSafeNormal2D() * Params.IdealDistanceFromTarget;
		break;
	}

	FZonefallTacticalQueryParams FlankParams = Params;
	FlankParams.FlankWeight = 4.0f;

	TArray<FZonefallTacticalCandidate> Candidates;
	if (!GatherTacticalCandidates(WorldContextObject, Agent, TargetActor, FlankParams, Candidates))
	{
		return false;
	}

	const FVector PreferredAnchor = TargetLocation + PreferredOffset;
	const FVector PreferredDir = (PreferredAnchor - TargetLocation).GetSafeNormal2D();

	float BestScore = TNumericLimits<float>::Lowest();
	bool bFound = false;

	for (FZonefallTacticalCandidate& Candidate : Candidates)
	{
		const FVector CandidateDir = (Candidate.Location - TargetLocation).GetSafeNormal2D();
		const float Dot = FVector::DotProduct(CandidateDir, PreferredDir);
		Candidate.Score = ScoreCoverCandidate(Candidate, FlankParams, Dot);

		if (Candidate.Score > BestScore)
		{
			BestScore = Candidate.Score;
			OutBestCandidate = Candidate;
			bFound = true;
		}
	}
	return bFound;
}

bool UZonefallAITacticalQueryLibrary::FindBestRetreatPoint(
	UObject* WorldContextObject,
	APawn* Agent,
	AActor* ThreatActor,
	const FZonefallTacticalQueryParams& Params,
	FZonefallTacticalCandidate& OutBestCandidate)
{
	if (!Agent || !ThreatActor)
	{
		return false;
	}

	FZonefallTacticalQueryParams RetreatParams = Params;
	RetreatParams.IdealDistanceFromTarget = FMath::Max(Params.IdealDistanceFromTarget, Params.SampleRadius * 0.9f);
	RetreatParams.DistanceFromTargetWeight = 3.0f;
	RetreatParams.TooCloseToTargetPenalty = 8.0f;

	TArray<FZonefallTacticalCandidate> Candidates;
	if (!GatherTacticalCandidates(WorldContextObject, Agent, ThreatActor, RetreatParams, Candidates))
	{
		return false;
	}

	const FVector AwayDir = (Agent->GetActorLocation() - ThreatActor->GetActorLocation()).GetSafeNormal2D();

	float BestScore = TNumericLimits<float>::Lowest();
	bool bFound = false;

	for (FZonefallTacticalCandidate& Candidate : Candidates)
	{
		const FVector CandidateDir = (Candidate.Location - ThreatActor->GetActorLocation()).GetSafeNormal2D();
		const float Dot = FVector::DotProduct(CandidateDir, AwayDir);
		Candidate.Score = ScoreCoverCandidate(Candidate, RetreatParams, Dot * 2.0f);
		if (Candidate.Score > BestScore)
		{
			BestScore = Candidate.Score;
			OutBestCandidate = Candidate;
			bFound = true;
		}
	}
	return bFound;
}

bool UZonefallAITacticalQueryLibrary::HasLineOfSightBetween(
	UObject* WorldContextObject,
	FVector ObserverEyeLocation,
	AActor* TargetActor,
	AActor* ObserverActorToIgnore)
{
	UWorld* World = ResolveWorld(WorldContextObject);
	if (!World || !TargetActor)
	{
		return false;
	}

	const FVector TargetLocation = TargetActor->GetActorLocation() + FVector(0, 0, 60.0f);
	return !TraceBlocksLineOfSight(World, ObserverEyeLocation, TargetLocation, ObserverActorToIgnore);
}
