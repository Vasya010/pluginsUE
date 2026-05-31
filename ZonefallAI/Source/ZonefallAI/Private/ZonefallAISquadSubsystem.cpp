#include "ZonefallAISquadSubsystem.h"

#include "AIController.h"
#include "BehaviorTree/BehaviorTreeComponent.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "ZonefallAI.h"
#include "ZonefallAIBlackboard.h"
#include "ZonefallAICharacterComponent.h"
#include "ZonefallAIController.h"

namespace
{
	FName MakeSquadNameForFaction(const FGameplayTag& FactionTag)
	{
		if (FactionTag.IsValid())
		{
			return FactionTag.GetTagName();
		}
		return TEXT("Zonefall.AutoSquad.Default");
	}
}

void UZonefallAISquadSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	Squads.Reset();
	SquadNameToId.Reset();
	ControllerToSquad.Reset();
	NextSquadId = 1;
	AccumulatedTime = 0.0f;
}

void UZonefallAISquadSubsystem::Deinitialize()
{
	Squads.Reset();
	SquadNameToId.Reset();
	ControllerToSquad.Reset();
	Super::Deinitialize();
}

bool UZonefallAISquadSubsystem::DoesSupportWorldType(const EWorldType::Type WorldType) const
{
	return WorldType == EWorldType::Game || WorldType == EWorldType::PIE || WorldType == EWorldType::Editor;
}

TStatId UZonefallAISquadSubsystem::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(UZonefallAISquadSubsystem, STATGROUP_Tickables);
}

void UZonefallAISquadSubsystem::Tick(float DeltaTime)
{
	const float CurrentTime = NowSeconds();
	AccumulatedTime += DeltaTime;

	for (auto It = Squads.CreateIterator(); It; ++It)
	{
		FInternalSquad& Squad = It.Value();

		Squad.Record.Members.RemoveAll([](const FZonefallSquadMemberState& Member)
		{
			return !Member.Controller.IsValid() || !Member.bIsAlive;
		});

		if (Squad.Record.Members.Num() == 0)
		{
			SquadNameToId.Remove(Squad.Record.SquadName);
			It.RemoveCurrent();
			continue;
		}

		PropagateSharedTarget(Squad);

		if (CurrentTime - Squad.Record.LastRoleEvaluationTime >= Squad.Config.RoleEvaluationInterval)
		{
			EvaluateRoles(Squad, CurrentTime);
			Squad.Record.LastRoleEvaluationTime = CurrentTime;
		}
	}
}

int32 UZonefallAISquadSubsystem::RegisterAgent(AAIController* Controller, FName SquadName, FGameplayTag FactionTag)
{
	if (!Controller)
	{
		return INDEX_NONE;
	}

	if (SquadName.IsNone())
	{
		SquadName = MakeSquadNameForFaction(FactionTag);
	}

	FInternalSquad* Squad = FindOrCreateSquad(SquadName, FactionTag);
	if (!Squad)
	{
		return INDEX_NONE;
	}

	FZonefallSquadMemberState* Existing = FindMemberState(*Squad, Controller);
	if (!Existing)
	{
		FZonefallSquadMemberState NewMember;
		NewMember.Controller = Controller;
		NewMember.Role = EZonefallAISquadRole::None;
		NewMember.TimeRoleAssigned = NowSeconds();
		NewMember.bIsAlive = true;
		Squad->Record.Members.Add(NewMember);
	}

	ControllerToSquad.FindOrAdd(Controller) = Squad->Record.SquadId;

	if (!Squad->Record.Leader.IsValid() && Squad->Record.Members.Num() > 0)
	{
		Squad->Record.Leader = Squad->Record.Members[0].Controller;
	}

	WriteSquadStateToBlackboard(Controller, *Squad);
	UE_LOG(LogZonefallAI, Verbose, TEXT("Squad %s: registered %s (members=%d)"),
		*Squad->Record.SquadName.ToString(), *Controller->GetName(), Squad->Record.Members.Num());
	return Squad->Record.SquadId;
}

void UZonefallAISquadSubsystem::UnregisterAgent(AAIController* Controller)
{
	if (!Controller)
	{
		return;
	}

	const int32* SquadIdPtr = ControllerToSquad.Find(Controller);
	if (!SquadIdPtr)
	{
		return;
	}

	if (FInternalSquad* Squad = Squads.Find(*SquadIdPtr))
	{
		Squad->Record.Members.RemoveAll([Controller](const FZonefallSquadMemberState& Member)
		{
			return Member.Controller.Get() == Controller;
		});

		if (Squad->Record.Leader.Get() == Controller && Squad->Record.Members.Num() > 0)
		{
			Squad->Record.Leader = Squad->Record.Members[0].Controller;
		}
		else if (Squad->Record.Members.Num() == 0)
		{
			Squad->Record.Leader = nullptr;
		}
	}

	ControllerToSquad.Remove(Controller);
}

void UZonefallAISquadSubsystem::ReportTargetSpotted(AAIController* Reporter, AActor* TargetActor)
{
	if (!Reporter || !TargetActor)
	{
		return;
	}

	FInternalSquad* Squad = FindSquadByController(Reporter);
	if (!Squad)
	{
		return;
	}

	const bool bIsNewTarget = Squad->Record.SharedTarget.Get() != TargetActor;
	Squad->Record.SharedTarget = TargetActor;
	Squad->Record.SharedLastKnownLocation = TargetActor->GetActorLocation();
	Squad->Record.SharedLastSeenTime = NowSeconds();

	if (bIsNewTarget)
	{
		OnSquadTargetSpotted.Broadcast(Squad->Record.SquadId, TargetActor, Reporter);
		EvaluateRoles(*Squad, NowSeconds());
	}
}

void UZonefallAISquadSubsystem::ReportTargetLost(AAIController* Reporter, AActor* TargetActor, FVector LastKnownLocation)
{
	if (!Reporter)
	{
		return;
	}

	FInternalSquad* Squad = FindSquadByController(Reporter);
	if (!Squad)
	{
		return;
	}

	Squad->Record.SharedLastKnownLocation = LastKnownLocation;
	Squad->Record.SharedLastSeenTime = NowSeconds();

	if (Squad->Record.SharedTarget.Get() == TargetActor)
	{
		Squad->Record.SharedTarget = nullptr;
		OnSquadTargetLost.Broadcast(Squad->Record.SquadId, TargetActor, Reporter);
		EvaluateRoles(*Squad, NowSeconds());
	}
}

bool UZonefallAISquadSubsystem::TryEmitSquadCallout(AAIController* Speaker, FName BarkId, float MinSquadCooldown, float MinSpeakerCooldown)
{
	if (!Speaker || BarkId.IsNone())
	{
		return false;
	}

	FInternalSquad* Squad = FindSquadByController(Speaker);
	if (!Squad)
	{
		OnSquadCallout.Broadcast(INDEX_NONE, BarkId, Speaker);
		return true;
	}

	const float Now = NowSeconds();
	if (Now - Squad->Record.LastSquadBarkTime < MinSquadCooldown)
	{
		return false;
	}

	if (FZonefallSquadMemberState* Member = FindMemberState(*Squad, Speaker))
	{
		if (Now - Member->LastBarkTime < MinSpeakerCooldown)
		{
			return false;
		}
		Member->LastBarkTime = Now;
	}

	Squad->Record.LastSquadBarkTime = Now;
	OnSquadCallout.Broadcast(Squad->Record.SquadId, BarkId, Speaker);
	return true;
}

bool UZonefallAISquadSubsystem::TryAssignSquadRoles(int32 SquadId)
{
	if (FInternalSquad* Squad = Squads.Find(SquadId))
	{
		EvaluateRoles(*Squad, NowSeconds());
		return true;
	}
	return false;
}

EZonefallAISquadRole UZonefallAISquadSubsystem::GetAssignedRole(const AAIController* Controller) const
{
	if (const FInternalSquad* Squad = FindSquadByController(Controller))
	{
		for (const FZonefallSquadMemberState& Member : Squad->Record.Members)
		{
			if (Member.Controller.Get() == Controller)
			{
				return Member.Role;
			}
		}
	}
	return EZonefallAISquadRole::None;
}

int32 UZonefallAISquadSubsystem::GetSquadId(const AAIController* Controller) const
{
	if (!Controller)
	{
		return INDEX_NONE;
	}
	if (const int32* Found = ControllerToSquad.Find(Controller))
	{
		return *Found;
	}
	return INDEX_NONE;
}

int32 UZonefallAISquadSubsystem::GetSquadMemberCount(int32 SquadId) const
{
	if (const FInternalSquad* Squad = Squads.Find(SquadId))
	{
		return Squad->Record.Members.Num();
	}
	return 0;
}

FVector UZonefallAISquadSubsystem::GetSharedLastKnownLocation(int32 SquadId) const
{
	if (const FInternalSquad* Squad = Squads.Find(SquadId))
	{
		return Squad->Record.SharedLastKnownLocation;
	}
	return FVector::ZeroVector;
}

AActor* UZonefallAISquadSubsystem::GetSharedTarget(int32 SquadId) const
{
	if (const FInternalSquad* Squad = Squads.Find(SquadId))
	{
		return Squad->Record.SharedTarget.Get();
	}
	return nullptr;
}

bool UZonefallAISquadSubsystem::HasAttackToken(const AAIController* Controller) const
{
	const EZonefallAISquadRole Role = GetAssignedRole(Controller);
	return Role == EZonefallAISquadRole::Attacker || Role == EZonefallAISquadRole::Flanker;
}

void UZonefallAISquadSubsystem::NotifyMemberDeath(AAIController* Controller)
{
	if (!Controller)
	{
		return;
	}

	if (FInternalSquad* Squad = FindSquadByController(Controller))
	{
		if (FZonefallSquadMemberState* Member = FindMemberState(*Squad, Controller))
		{
			Member->bIsAlive = false;
		}
		EvaluateRoles(*Squad, NowSeconds());
	}
}

UZonefallAISquadSubsystem::FInternalSquad* UZonefallAISquadSubsystem::FindOrCreateSquad(FName SquadName, FGameplayTag FactionTag)
{
	if (const int32* ExistingId = SquadNameToId.Find(SquadName))
	{
		return Squads.Find(*ExistingId);
	}

	FInternalSquad NewSquad;
	NewSquad.Record.SquadId = NextSquadId++;
	NewSquad.Record.SquadName = SquadName;
	NewSquad.Record.FactionTag = FactionTag;
	NewSquad.Record.LastRoleEvaluationTime = -100.0f;
	NewSquad.Config = DefaultConfig;

	SquadNameToId.Add(SquadName, NewSquad.Record.SquadId);
	FInternalSquad& Inserted = Squads.Add(NewSquad.Record.SquadId, NewSquad);
	return &Inserted;
}

UZonefallAISquadSubsystem::FInternalSquad* UZonefallAISquadSubsystem::FindSquadByController(const AAIController* Controller)
{
	if (!Controller)
	{
		return nullptr;
	}
	if (const int32* Found = ControllerToSquad.Find(Controller))
	{
		return Squads.Find(*Found);
	}
	return nullptr;
}

const UZonefallAISquadSubsystem::FInternalSquad* UZonefallAISquadSubsystem::FindSquadByController(const AAIController* Controller) const
{
	if (!Controller)
	{
		return nullptr;
	}
	if (const int32* Found = ControllerToSquad.Find(Controller))
	{
		return Squads.Find(*Found);
	}
	return nullptr;
}

FZonefallSquadMemberState* UZonefallAISquadSubsystem::FindMemberState(FInternalSquad& Squad, AAIController* Controller)
{
	for (FZonefallSquadMemberState& Member : Squad.Record.Members)
	{
		if (Member.Controller.Get() == Controller)
		{
			return &Member;
		}
	}
	return nullptr;
}

void UZonefallAISquadSubsystem::EvaluateRoles(FInternalSquad& Squad, float CurrentTime)
{
	AActor* SharedTarget = Squad.Record.SharedTarget.Get();
	const bool bHasTarget = SharedTarget != nullptr;

	if (!bHasTarget)
	{
		for (FZonefallSquadMemberState& Member : Squad.Record.Members)
		{
			if (Member.Role != EZonefallAISquadRole::None)
			{
				AssignRoleToMember(Squad, Member, EZonefallAISquadRole::None, CurrentTime);
			}
		}
		return;
	}

	const FVector TargetLocation = SharedTarget->GetActorLocation();

	struct FCandidate
	{
		FZonefallSquadMemberState* Member = nullptr;
		float DistanceSq = 0.0f;
		bool bHasCover = false;
		bool bHasLOS = false;
	};

	TArray<FCandidate> Candidates;
	Candidates.Reserve(Squad.Record.Members.Num());

	for (FZonefallSquadMemberState& Member : Squad.Record.Members)
	{
		AAIController* Controller = Member.Controller.Get();
		if (!Controller || !Controller->GetPawn())
		{
			continue;
		}

		FCandidate Candidate;
		Candidate.Member = &Member;
		Candidate.DistanceSq = FVector::DistSquared(Controller->GetPawn()->GetActorLocation(), TargetLocation);
		if (UBlackboardComponent* BB = Controller->GetBlackboardComponent())
		{
			const AZonefallAIController* ZonefallController = Cast<AZonefallAIController>(Controller);
			const FZonefallAIBlackboardKeys& Keys = ZonefallController ? ZonefallController->BlackboardKeys : FZonefallAIBlackboardKeys();
			Candidate.bHasLOS = BB->GetValueAsBool(Keys.HasLineOfSight);
			Candidate.bHasCover = BB->GetValueAsBool(Keys.HasCover);
		}
		Candidates.Add(Candidate);
	}

	Candidates.Sort([](const FCandidate& A, const FCandidate& B)
	{
		return A.DistanceSq < B.DistanceSq;
	});

	int32 Attackers = 0;
	int32 Flankers = 0;
	int32 Suppressors = 0;

	for (FCandidate& Candidate : Candidates)
	{
		if (!Candidate.Member)
		{
			continue;
		}

		FZonefallSquadMemberState& Member = *Candidate.Member;
		const float TimeInRole = CurrentTime - Member.TimeRoleAssigned;
		const bool bRoleLocked = (Member.Role != EZonefallAISquadRole::None) && (TimeInRole < Squad.Config.MinTimeInRoleBeforeReassign);

		EZonefallAISquadRole DesiredRole = EZonefallAISquadRole::Support;
		const float Distance = FMath::Sqrt(Candidate.DistanceSq);

		if (Distance <= Squad.Config.CloseRangeForRoles && Attackers < Squad.Config.MaxConcurrentAttackers)
		{
			DesiredRole = EZonefallAISquadRole::Attacker;
			++Attackers;
		}
		else if (Flankers < Squad.Config.MaxConcurrentFlankers && Distance > Squad.Config.CloseRangeForRoles * 0.5f)
		{
			DesiredRole = EZonefallAISquadRole::Flanker;
			++Flankers;
		}
		else if (Suppressors < Squad.Config.MaxConcurrentSuppressors && (Candidate.bHasCover || Distance > Squad.Config.MediumRangeForRoles * 0.5f))
		{
			DesiredRole = EZonefallAISquadRole::Suppressor;
			++Suppressors;
		}
		else
		{
			DesiredRole = EZonefallAISquadRole::Support;
		}

		if (bRoleLocked && Member.Role != EZonefallAISquadRole::None)
		{
			DesiredRole = Member.Role;
			if (DesiredRole == EZonefallAISquadRole::Attacker) ++Attackers;
			else if (DesiredRole == EZonefallAISquadRole::Flanker) ++Flankers;
			else if (DesiredRole == EZonefallAISquadRole::Suppressor) ++Suppressors;
		}

		if (DesiredRole != Member.Role)
		{
			AssignRoleToMember(Squad, Member, DesiredRole, CurrentTime);
		}
	}
}

void UZonefallAISquadSubsystem::AssignRoleToMember(FInternalSquad& Squad, FZonefallSquadMemberState& Member, EZonefallAISquadRole NewRole, float CurrentTime)
{
	const EZonefallAISquadRole OldRole = Member.Role;
	Member.Role = NewRole;
	Member.TimeRoleAssigned = CurrentTime;

	AAIController* Controller = Member.Controller.Get();
	if (Controller)
	{
		WriteRoleToBlackboard(Controller, NewRole);
		WriteSquadStateToBlackboard(Controller, Squad);
	}

	OnSquadRoleChanged.Broadcast(Squad.Record.SquadId, Controller, OldRole, NewRole);
}

void UZonefallAISquadSubsystem::WriteRoleToBlackboard(AAIController* Controller, EZonefallAISquadRole Role) const
{
	if (!Controller)
	{
		return;
	}

	UBlackboardComponent* Blackboard = Controller->GetBlackboardComponent();
	if (!Blackboard)
	{
		return;
	}

	if (const AZonefallAIController* ZonefallController = Cast<AZonefallAIController>(Controller))
	{
		UZonefallAIBlackboardLibrary::SetZonefallSquadRole(Blackboard, ZonefallController->BlackboardKeys, Role);
	}
	else
	{
		FZonefallAIBlackboardKeys DefaultKeys;
		UZonefallAIBlackboardLibrary::SetZonefallSquadRole(Blackboard, DefaultKeys, Role);
	}
}

void UZonefallAISquadSubsystem::WriteSquadStateToBlackboard(AAIController* Controller, const FInternalSquad& Squad) const
{
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

	Blackboard->SetValueAsInt(Keys.SquadMemberCount, Squad.Record.Members.Num());
	Blackboard->SetValueAsObject(Keys.SquadLeader, Squad.Record.Leader.Get());

	const float Cohesion = Squad.Record.Members.Num() > 1 ? 1.0f : 0.0f;
	Blackboard->SetValueAsFloat(Keys.SquadCohesion, Cohesion);
}

void UZonefallAISquadSubsystem::PropagateSharedTarget(FInternalSquad& Squad)
{
	AActor* SharedTarget = Squad.Record.SharedTarget.Get();
	if (!SharedTarget)
	{
		return;
	}

	Squad.Record.SharedLastKnownLocation = SharedTarget->GetActorLocation();

	for (FZonefallSquadMemberState& Member : Squad.Record.Members)
	{
		AAIController* Controller = Member.Controller.Get();
		if (!Controller)
		{
			continue;
		}

		UBlackboardComponent* Blackboard = Controller->GetBlackboardComponent();
		if (!Blackboard)
		{
			continue;
		}

		const AZonefallAIController* ZonefallController = Cast<AZonefallAIController>(Controller);
		const FZonefallAIBlackboardKeys& Keys = ZonefallController ? ZonefallController->BlackboardKeys : FZonefallAIBlackboardKeys();

		if (!Blackboard->GetValueAsObject(Keys.TargetActor))
		{
			Blackboard->SetValueAsVector(Keys.LastKnownTargetLocation, Squad.Record.SharedLastKnownLocation);
		}
	}
}

float UZonefallAISquadSubsystem::NowSeconds() const
{
	const UWorld* World = GetWorld();
	return World ? World->GetTimeSeconds() : 0.0f;
}
