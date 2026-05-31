#include "ZonefallAIBlackboard.h"

#include "BehaviorTree/BlackboardComponent.h"
#include "GameFramework/Actor.h"
#include "ZonefallAICharacterComponent.h"

namespace
{
	FName TagToBlackboardName(const FGameplayTag& Tag)
	{
		return Tag.IsValid() ? Tag.GetTagName() : NAME_None;
	}
}

FZonefallAIBlackboardKeys UZonefallAIBlackboardLibrary::MakeDefaultZonefallBlackboardKeys()
{
	return FZonefallAIBlackboardKeys();
}

void UZonefallAIBlackboardLibrary::SetZonefallTarget(UBlackboardComponent* Blackboard, const FZonefallAIBlackboardKeys& Keys, AActor* TargetActor, bool bHasLineOfSight)
{
	if (!Blackboard)
	{
		return;
	}

	Blackboard->SetValueAsObject(Keys.TargetActor, TargetActor);
	Blackboard->SetValueAsObject(Keys.FocusActor, TargetActor);
	Blackboard->SetValueAsBool(Keys.HasTarget, TargetActor != nullptr);
	Blackboard->SetValueAsBool(Keys.HasLineOfSight, bHasLineOfSight);
	Blackboard->SetValueAsBool(Keys.IsInvestigating, TargetActor == nullptr);

	if (TargetActor)
	{
		if (const UZonefallAICharacterComponent* TargetIdentity = UZonefallAICharacterComponent::FindAICharacterComponent(TargetActor))
		{
			Blackboard->SetValueAsName(Keys.TargetArchetypeTag, TagToBlackboardName(TargetIdentity->ArchetypeTag));
			Blackboard->SetValueAsName(Keys.TargetRoleTag, TagToBlackboardName(TargetIdentity->RoleTag));
			Blackboard->SetValueAsName(Keys.TargetFactionTag, TagToBlackboardName(TargetIdentity->FactionTag));
			Blackboard->SetValueAsName(Keys.TargetDispositionTag, TagToBlackboardName(TargetIdentity->DispositionTag));
			Blackboard->SetValueAsName(Keys.TargetRelationshipTag, TagToBlackboardName(TargetIdentity->RelationshipTag));
		}

		Blackboard->SetValueAsVector(Keys.LastKnownTargetLocation, TargetActor->GetActorLocation());
		Blackboard->SetValueAsFloat(Keys.LastSeenTime, TargetActor->GetWorld() ? TargetActor->GetWorld()->GetTimeSeconds() : 0.0f);
		Blackboard->SetValueAsFloat(Keys.Alertness, bHasLineOfSight ? 1.0f : 0.65f);
		SetZonefallThreatState(Blackboard, Keys, bHasLineOfSight ? EZonefallAIThreatState::Combat : EZonefallAIThreatState::Alert);
	}
}

void UZonefallAIBlackboardLibrary::ClearZonefallTarget(UBlackboardComponent* Blackboard, const FZonefallAIBlackboardKeys& Keys, bool bKeepLastKnownLocation)
{
	if (!Blackboard)
	{
		return;
	}

	if (!bKeepLastKnownLocation)
	{
		Blackboard->ClearValue(Keys.LastKnownTargetLocation);
	}

	Blackboard->ClearValue(Keys.TargetActor);
	Blackboard->ClearValue(Keys.FocusActor);
	Blackboard->ClearValue(Keys.TargetArchetypeTag);
	Blackboard->ClearValue(Keys.TargetRoleTag);
	Blackboard->ClearValue(Keys.TargetFactionTag);
	Blackboard->ClearValue(Keys.TargetDispositionTag);
	Blackboard->ClearValue(Keys.TargetRelationshipTag);
	Blackboard->SetValueAsFloat(Keys.TargetThreatScore, 0.0f);
	Blackboard->SetValueAsBool(Keys.HasTarget, false);
	Blackboard->SetValueAsBool(Keys.HasLineOfSight, false);
	Blackboard->SetValueAsBool(Keys.IsInvestigating, bKeepLastKnownLocation);
	Blackboard->SetValueAsFloat(Keys.Alertness, bKeepLastKnownLocation ? 0.45f : 0.0f);
	SetZonefallThreatState(Blackboard, Keys, bKeepLastKnownLocation ? EZonefallAIThreatState::LostTarget : EZonefallAIThreatState::Passive);
}

void UZonefallAIBlackboardLibrary::SetZonefallThreatState(UBlackboardComponent* Blackboard, const FZonefallAIBlackboardKeys& Keys, EZonefallAIThreatState ThreatState)
{
	if (Blackboard)
	{
		Blackboard->SetValueAsEnum(Keys.ThreatState, static_cast<uint8>(ThreatState));
	}
}

void UZonefallAIBlackboardLibrary::SetZonefallSquadRole(UBlackboardComponent* Blackboard, const FZonefallAIBlackboardKeys& Keys, EZonefallAISquadRole Role)
{
	if (Blackboard)
	{
		Blackboard->SetValueAsEnum(Keys.SquadRole, static_cast<uint8>(Role));
		Blackboard->SetValueAsBool(Keys.HasAttackToken, Role == EZonefallAISquadRole::Attacker || Role == EZonefallAISquadRole::Flanker);
	}
}

void UZonefallAIBlackboardLibrary::SetZonefallTacticalIntent(UBlackboardComponent* Blackboard, const FZonefallAIBlackboardKeys& Keys, EZonefallAITacticalIntent Intent)
{
	if (Blackboard)
	{
		Blackboard->SetValueAsEnum(Keys.TacticalIntent, static_cast<uint8>(Intent));
	}
}

void UZonefallAIBlackboardLibrary::SetZonefallCover(UBlackboardComponent* Blackboard, const FZonefallAIBlackboardKeys& Keys, FVector Location, FVector Facing, float Score)
{
	if (!Blackboard)
	{
		return;
	}
	Blackboard->SetValueAsVector(Keys.CoverLocation, Location);
	Blackboard->SetValueAsVector(Keys.CoverFacing, Facing.GetSafeNormal());
	Blackboard->SetValueAsFloat(Keys.CoverScore, Score);
	Blackboard->SetValueAsBool(Keys.HasCover, true);
}

void UZonefallAIBlackboardLibrary::ClearZonefallCover(UBlackboardComponent* Blackboard, const FZonefallAIBlackboardKeys& Keys)
{
	if (!Blackboard)
	{
		return;
	}
	Blackboard->ClearValue(Keys.CoverLocation);
	Blackboard->ClearValue(Keys.CoverFacing);
	Blackboard->SetValueAsFloat(Keys.CoverScore, 0.0f);
	Blackboard->SetValueAsBool(Keys.HasCover, false);
}

void UZonefallAIBlackboardLibrary::SetZonefallFlank(UBlackboardComponent* Blackboard, const FZonefallAIBlackboardKeys& Keys, FVector PivotLocation, EZonefallAIFlankSide Side)
{
	if (!Blackboard)
	{
		return;
	}
	Blackboard->SetValueAsVector(Keys.FlankPivotLocation, PivotLocation);
	Blackboard->SetValueAsEnum(Keys.FlankSide, static_cast<uint8>(Side));
}

void UZonefallAIBlackboardLibrary::SetZonefallSuppression(UBlackboardComponent* Blackboard, const FZonefallAIBlackboardKeys& Keys, float Level, AActor* Source)
{
	if (!Blackboard)
	{
		return;
	}
	const float ClampedLevel = FMath::Clamp(Level, 0.0f, 1.0f);
	Blackboard->SetValueAsFloat(Keys.SuppressionLevel, ClampedLevel);
	Blackboard->SetValueAsBool(Keys.IsSuppressed, ClampedLevel >= 0.5f);
	Blackboard->SetValueAsObject(Keys.SuppressionSource, Source);
}

EZonefallAISquadRole UZonefallAIBlackboardLibrary::GetZonefallSquadRole(const UBlackboardComponent* Blackboard, const FZonefallAIBlackboardKeys& Keys)
{
	return Blackboard ? static_cast<EZonefallAISquadRole>(Blackboard->GetValueAsEnum(Keys.SquadRole)) : EZonefallAISquadRole::None;
}

EZonefallAITacticalIntent UZonefallAIBlackboardLibrary::GetZonefallTacticalIntent(const UBlackboardComponent* Blackboard, const FZonefallAIBlackboardKeys& Keys)
{
	return Blackboard ? static_cast<EZonefallAITacticalIntent>(Blackboard->GetValueAsEnum(Keys.TacticalIntent)) : EZonefallAITacticalIntent::None;
}
