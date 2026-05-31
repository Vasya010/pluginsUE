#include "ZonefallAICharacterComponent.h"

#include "ZonefallAI.h"
#include "ZonefallAIBarkComponent.h"
#include "ZonefallAIController.h"
#include "ZonefallAIPatrolComponent.h"
#include "ZonefallAIPerceptionMemoryComponent.h"
#include "ZonefallAISuppressionComponent.h"
#include "ZonefallAITacticalCoverComponent.h"
#include "ZonefallNpcVitalsComponent.h"
#include "AIController.h"
#include "Components/SceneComponent.h"
#include "GameFramework/Actor.h"
#include "GameFramework/Pawn.h"
#include "GameplayTagsManager.h"
#include "TimerManager.h"
#include "UObject/UnrealType.h"
#include <type_traits>

namespace ZonefallAIComponentSetup
{
	template<typename TComponent>
	void RegisterNewComponent(AActor* Owner, TComponent* Component)
	{
		if constexpr (std::is_base_of_v<USceneComponent, TComponent>)
		{
			USceneComponent* SceneComponent = Component;
			if (USceneComponent* Root = Owner->GetRootComponent())
			{
				SceneComponent->SetupAttachment(Root);
			}
			SceneComponent->RegisterComponent();
		}
		else
		{
			Component->RegisterComponent();
		}
	}

	template<typename TComponent>
	TComponent* EnsureComponentOnActor(AActor* Owner, TObjectPtr<TComponent>& Cache, bool bShouldAdd)
	{
		if (!Owner || !bShouldAdd)
		{
			return Cache;
		}

		if (!Cache)
		{
			Cache = Owner->FindComponentByClass<TComponent>();
		}

		if (!Cache)
		{
			Cache = NewObject<TComponent>(Owner, TComponent::StaticClass(), NAME_None, RF_Transactional);
			if (!Cache)
			{
				return nullptr;
			}

			RegisterNewComponent(Owner, Cache.Get());
			Owner->AddInstanceComponent(Cache);
		}

		return Cache;
	}

	template<typename TComponent>
	TComponent* EnsureComponentOnActor(AActor* Owner, bool bShouldAdd)
	{
		TObjectPtr<TComponent> TransientCache;
		return EnsureComponentOnActor(Owner, TransientCache, bShouldAdd);
	}
}

namespace ZonefallAITags
{
	const FName Human(TEXT("Zonefall.AI.Archetype.Human"));
	const FName Animal(TEXT("Zonefall.AI.Archetype.Animal"));
	const FName Hero(TEXT("Zonefall.AI.Role.Hero"));
	const FName Resident(TEXT("Zonefall.AI.Role.Resident"));
	const FName Civilian(TEXT("Zonefall.AI.Role.Civilian"));
	const FName Companion(TEXT("Zonefall.AI.Role.Companion"));
	const FName Bandit(TEXT("Zonefall.AI.Role.Bandit"));
	const FName Creature(TEXT("Zonefall.AI.Role.Creature"));
	const FName Predator(TEXT("Zonefall.AI.Role.Predator"));
	const FName FactionPlayer(TEXT("Zonefall.AI.Faction.Player"));
	const FName FactionFriendly(TEXT("Zonefall.AI.Faction.Friendly"));
	const FName FactionCivilian(TEXT("Zonefall.AI.Faction.Civilian"));
	const FName FactionBandit(TEXT("Zonefall.AI.Faction.Bandit"));
	const FName FactionWildlife(TEXT("Zonefall.AI.Faction.Wildlife"));
	const FName Harmless(TEXT("Zonefall.AI.Disposition.Harmless"));
	const FName Friendly(TEXT("Zonefall.AI.Disposition.Friendly"));
	const FName Neutral(TEXT("Zonefall.AI.Disposition.Neutral"));
	const FName Defensive(TEXT("Zonefall.AI.Disposition.Defensive"));
	const FName Hostile(TEXT("Zonefall.AI.Disposition.Hostile"));
	const FName RelationshipFriendly(TEXT("Zonefall.AI.Relationship.Friendly"));
	const FName RelationshipAlly(TEXT("Zonefall.AI.Relationship.Ally"));
	const FName RelationshipEnemy(TEXT("Zonefall.AI.Relationship.Enemy"));
	const FName RelationshipNeutral(TEXT("Zonefall.AI.Relationship.Neutral"));
}

UZonefallAICharacterComponent::UZonefallAICharacterComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UZonefallAICharacterComponent::BeginPlay()
{
	Super::BeginPlay();
	if (bAutoApplyProfileOnBeginPlay)
	{
		ApplyCharacterProfile();
	}
	EnsureDefaultIdentity();
	EnsureAutoZonefallComponents();

	if (APawn* Pawn = Cast<APawn>(GetOwner()))
	{
		Pawn->ReceiveControllerChangedDelegate.AddUniqueDynamic(this, &UZonefallAICharacterComponent::HandleControllerChanged);

		if (UWorld* World = GetWorld())
		{
			World->GetTimerManager().SetTimerForNextTick(FTimerDelegate::CreateUObject(this, &UZonefallAICharacterComponent::EnsureAutoZonefallComponents));
		}
	}
}

void UZonefallAICharacterComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (APawn* Pawn = Cast<APawn>(GetOwner()))
	{
		Pawn->ReceiveControllerChangedDelegate.RemoveDynamic(this, &UZonefallAICharacterComponent::HandleControllerChanged);
	}

	Super::EndPlay(EndPlayReason);
}

void UZonefallAICharacterComponent::HandleControllerChanged(APawn* /*Pawn*/, AController* /*OldController*/, AController* /*NewController*/)
{
	EnsureAutoZonefallComponents();
}

void UZonefallAICharacterComponent::EnsureAutoZonefallComponents()
{
	if (bAutoAddNpcVitals)
	{
		EnsureNpcVitalsComponent();
	}

	if (ShouldAutoAddPatrol())
	{
		EnsurePatrolComponent();
	}

	EnsureControllerZonefallComponents();
}

bool UZonefallAICharacterComponent::ShouldAutoAddPatrol() const
{
	if (!bAutoAddPatrolSpline)
	{
		return false;
	}

	return !bAutoAddPatrolOnlyForHostiles || IsHostile();
}

UZonefallAIPatrolComponent* UZonefallAICharacterComponent::EnsurePatrolComponent()
{
	AActor* Owner = GetOwner();
	if (!Owner)
	{
		return nullptr;
	}

	return ZonefallAIComponentSetup::EnsureComponentOnActor<UZonefallAIPatrolComponent>(Owner, CachedPatrol, true);
}

void UZonefallAICharacterComponent::EnsureControllerZonefallComponents()
{
	if (!bAutoAddMissingControllerComponents)
	{
		return;
	}

	APawn* Pawn = Cast<APawn>(GetOwner());
	if (!Pawn)
	{
		return;
	}

	AAIController* AIController = Cast<AAIController>(Pawn->GetController());
	if (!AIController)
	{
		return;
	}

	if (Cast<AZonefallAIController>(AIController))
	{
		return;
	}

	if (bWarnWhenNotUsingZonefallAIController && IsHostile())
	{
		UE_LOG(LogZonefallAI, Warning, TEXT("NPC '%s' uses %s — set AI Controller Class to ZonefallAIController for sight/hearing + behavior tree."),
			*Pawn->GetName(),
			*AIController->GetClass()->GetName());
	}

	ZonefallAIComponentSetup::EnsureComponentOnActor<UZonefallAIPerceptionMemoryComponent>(AIController, true);
	ZonefallAIComponentSetup::EnsureComponentOnActor<UZonefallAITacticalCoverComponent>(AIController, true);
	ZonefallAIComponentSetup::EnsureComponentOnActor<UZonefallAISuppressionComponent>(AIController, true);
	ZonefallAIComponentSetup::EnsureComponentOnActor<UZonefallAIBarkComponent>(AIController, true);
}

#if WITH_EDITOR
void UZonefallAICharacterComponent::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	const FName PropertyName = PropertyChangedEvent.GetPropertyName();
	if (PropertyName == GET_MEMBER_NAME_CHECKED(UZonefallAICharacterComponent, CharacterProfile))
	{
		ApplyCharacterProfile();
		SyncNpcVitalsFromIdentity();
	}
}
#endif

void UZonefallAICharacterComponent::GetOwnedGameplayTags(FGameplayTagContainer& TagContainer) const
{
	TagContainer = GetCombinedTags();
}

void UZonefallAICharacterComponent::ApplyCharacterProfile()
{
	switch (CharacterProfile)
	{
	case EZonefallAICharacterProfile::MainHero:
		ConfigureAsMainHero();
		break;
	case EZonefallAICharacterProfile::FriendlyResident:
		ConfigureAsFriendlyResident();
		break;
	case EZonefallAICharacterProfile::HarmlessResident:
		ConfigureAsHarmlessHuman();
		break;
	case EZonefallAICharacterProfile::FriendlyCompanion:
		ConfigureAsFriendlyCompanion();
		break;
	case EZonefallAICharacterProfile::EnemyBandit:
		ConfigureAsBandit();
		break;
	case EZonefallAICharacterProfile::PassiveAnimal:
		ConfigureAsAnimal(false);
		break;
	case EZonefallAICharacterProfile::DefensiveAnimal:
		ConfigureAsAnimal(true);
		break;
	case EZonefallAICharacterProfile::HostileAnimal:
		ConfigureAsHostileAnimal();
		break;
	case EZonefallAICharacterProfile::Custom:
	default:
		break;
	}
}

void UZonefallAICharacterComponent::ConfigureAsMainHero()
{
	ArchetypeTag = GetHumanArchetypeTag();
	RoleTag = GetHeroRoleTag();
	FactionTag = RequestTag(ZonefallAITags::FactionPlayer);
	DispositionTag = GetFriendlyDispositionTag();
	RelationshipTag = GetFriendlyRelationshipTag();
	CombatIntent = EZonefallAICombatIntent::Defensive;
	bCanInitiateCombat = true;
	bCanTargetHarmlessActors = false;
	bIgnorePlayerAsTarget = false;
	bAlwaysChaseMainHero = false;
	MainHeroChaseSpeed = 420.0f;
	BaseThreatScore = 0.9f;
}

void UZonefallAICharacterComponent::ConfigureAsFriendlyResident()
{
	ArchetypeTag = GetHumanArchetypeTag();
	RoleTag = GetResidentRoleTag();
	FactionTag = RequestTag(ZonefallAITags::FactionFriendly);
	DispositionTag = GetFriendlyDispositionTag();
	RelationshipTag = GetFriendlyRelationshipTag();
	CombatIntent = EZonefallAICombatIntent::None;
	bCanInitiateCombat = false;
	bCanTargetHarmlessActors = false;
	bIgnorePlayerAsTarget = true;
	bAlwaysChaseMainHero = false;
	MainHeroChaseSpeed = 220.0f;
	BaseThreatScore = 0.35f;
}

void UZonefallAICharacterComponent::ConfigureAsHarmlessHuman()
{
	ArchetypeTag = GetHumanArchetypeTag();
	RoleTag = GetResidentRoleTag();
	FactionTag = RequestTag(ZonefallAITags::FactionCivilian);
	DispositionTag = GetHarmlessDispositionTag();
	RelationshipTag = RequestTag(ZonefallAITags::RelationshipNeutral);
	CombatIntent = EZonefallAICombatIntent::None;
	bCanInitiateCombat = false;
	bCanTargetHarmlessActors = false;
	bIgnorePlayerAsTarget = true;
	bAlwaysChaseMainHero = false;
	MainHeroChaseSpeed = 165.0f;
	BaseThreatScore = 0.05f;
}

void UZonefallAICharacterComponent::ConfigureAsFriendlyCompanion()
{
	ArchetypeTag = GetHumanArchetypeTag();
	RoleTag = RequestTag(ZonefallAITags::Companion);
	FactionTag = RequestTag(ZonefallAITags::FactionPlayer);
	DispositionTag = GetFriendlyDispositionTag();
	RelationshipTag = RequestTag(ZonefallAITags::RelationshipAlly);
	CombatIntent = EZonefallAICombatIntent::Defensive;
	bCanInitiateCombat = true;
	bCanTargetHarmlessActors = false;
	bIgnorePlayerAsTarget = false;
	bAlwaysChaseMainHero = false;
	MainHeroChaseSpeed = 420.0f;
	BaseThreatScore = 0.65f;
}

void UZonefallAICharacterComponent::ConfigureAsBandit()
{
	ArchetypeTag = GetHumanArchetypeTag();
	RoleTag = GetBanditRoleTag();
	FactionTag = RequestTag(ZonefallAITags::FactionBandit);
	DispositionTag = GetHostileDispositionTag();
	RelationshipTag = GetEnemyRelationshipTag();
	CombatIntent = EZonefallAICombatIntent::Aggressive;
	bCanInitiateCombat = true;
	bCanTargetHarmlessActors = true;
	bIgnorePlayerAsTarget = false;
	bAlwaysChaseMainHero = true;
	MainHeroChaseSpeed = 600.0f;
	BaseThreatScore = 0.85f;
}

void UZonefallAICharacterComponent::ConfigureAsAnimal(bool bDefensiveAnimal)
{
	ArchetypeTag = GetAnimalArchetypeTag();
	RoleTag = RequestTag(ZonefallAITags::Creature);
	FactionTag = RequestTag(ZonefallAITags::FactionWildlife);
	DispositionTag = RequestTag(bDefensiveAnimal ? ZonefallAITags::Defensive : ZonefallAITags::Neutral);
	RelationshipTag = RequestTag(ZonefallAITags::RelationshipNeutral);
	CombatIntent = bDefensiveAnimal ? EZonefallAICombatIntent::Defensive : EZonefallAICombatIntent::None;
	bCanInitiateCombat = false;
	bCanTargetHarmlessActors = false;
	bIgnorePlayerAsTarget = true;
	bAlwaysChaseMainHero = false;
	MainHeroChaseSpeed = bDefensiveAnimal ? 360.0f : 220.0f;
	BaseThreatScore = bDefensiveAnimal ? 0.45f : 0.15f;
}

void UZonefallAICharacterComponent::ConfigureAsHostileAnimal()
{
	ArchetypeTag = GetAnimalArchetypeTag();
	RoleTag = RequestTag(ZonefallAITags::Predator);
	FactionTag = RequestTag(ZonefallAITags::FactionWildlife);
	DispositionTag = GetHostileDispositionTag();
	RelationshipTag = GetEnemyRelationshipTag();
	CombatIntent = EZonefallAICombatIntent::Aggressive;
	bCanInitiateCombat = true;
	bCanTargetHarmlessActors = false;
	bIgnorePlayerAsTarget = false;
	bAlwaysChaseMainHero = false;
	MainHeroChaseSpeed = 520.0f;
	BaseThreatScore = 0.75f;
}

FGameplayTagContainer UZonefallAICharacterComponent::GetCombinedTags() const
{
	FGameplayTagContainer Tags = AdditionalTags;
	Tags.AddTag(ArchetypeTag);
	Tags.AddTag(RoleTag);
	Tags.AddTag(FactionTag);
	Tags.AddTag(DispositionTag);
	Tags.AddTag(RelationshipTag);
	return Tags;
}

bool UZonefallAICharacterComponent::HasZonefallTag(FGameplayTag TagToCheck) const
{
	return TagToCheck.IsValid() && GetCombinedTags().HasTag(TagToCheck);
}

bool UZonefallAICharacterComponent::IsHarmless() const
{
	return HasZonefallTag(GetHarmlessDispositionTag()) || CombatIntent == EZonefallAICombatIntent::None || !bCanInitiateCombat;
}

bool UZonefallAICharacterComponent::IsFriendly() const
{
	return HasZonefallTag(GetFriendlyDispositionTag()) || HasZonefallTag(GetFriendlyRelationshipTag());
}

bool UZonefallAICharacterComponent::IsHostile() const
{
	return HasZonefallTag(GetHostileDispositionTag()) || HasZonefallTag(GetEnemyRelationshipTag());
}

bool UZonefallAICharacterComponent::IsSameFactionAs(const UZonefallAICharacterComponent* OtherComponent) const
{
	return OtherComponent && FactionTag.IsValid() && OtherComponent->FactionTag.IsValid() && FactionTag.MatchesTagExact(OtherComponent->FactionTag);
}

bool UZonefallAICharacterComponent::IsEnemyFor(const UZonefallAICharacterComponent* OtherComponent) const
{
	if (!OtherComponent || IsSameFactionAs(OtherComponent))
	{
		return false;
	}

	const bool bSelfEnemy = IsHostile() || HasZonefallTag(GetEnemyRelationshipTag());
	const bool bOtherEnemy = OtherComponent->IsHostile() || OtherComponent->HasZonefallTag(GetEnemyRelationshipTag());
	const bool bSelfFriendly = IsFriendly() || FactionTag.MatchesTagExact(RequestTag(ZonefallAITags::FactionPlayer));
	const bool bOtherFriendly = OtherComponent->IsFriendly() || OtherComponent->FactionTag.MatchesTagExact(RequestTag(ZonefallAITags::FactionPlayer));

	return (bSelfEnemy && bOtherFriendly) || (bSelfFriendly && bOtherEnemy);
}

bool UZonefallAICharacterComponent::IsMainHeroActor(const AActor* CandidateActor) const
{
	const UZonefallAICharacterComponent* CandidateIdentity = FindAICharacterComponent(CandidateActor);
	return CandidateIdentity && CandidateIdentity->HasZonefallTag(GetHeroRoleTag());
}

bool UZonefallAICharacterComponent::ShouldForceChaseActor(const AActor* CandidateActor) const
{
	return bAlwaysChaseMainHero && IsMainHeroActor(CandidateActor) && CanTargetActor(CandidateActor);
}

bool UZonefallAICharacterComponent::CanTargetActor(const AActor* CandidateActor) const
{
	const UZonefallAICharacterComponent* CandidateIdentity = FindAICharacterComponent(CandidateActor);
	if (!bCanInitiateCombat || CombatIntent == EZonefallAICombatIntent::None || !CandidateActor)
	{
		return false;
	}

	if (CandidateIdentity)
	{
		if (bIgnorePlayerAsTarget && CandidateIdentity->HasZonefallTag(GetHeroRoleTag()))
		{
			return false;
		}

		if (IsSameFactionAs(CandidateIdentity))
		{
			return false;
		}

		if (IsEnemyFor(CandidateIdentity))
		{
			return true;
		}

		if (CandidateIdentity->IsHarmless() && !bCanTargetHarmlessActors)
		{
			return false;
		}
	}

	return true;
}

float UZonefallAICharacterComponent::GetThreatScoreAgainst(const AActor* CandidateActor) const
{
	if (!CanTargetActor(CandidateActor))
	{
		return 0.0f;
	}

	const UZonefallAICharacterComponent* CandidateIdentity = FindAICharacterComponent(CandidateActor);
	float Score = BaseThreatScore;
	if (CandidateIdentity)
	{
		if (CandidateIdentity->HasZonefallTag(GetHostileDispositionTag()))
		{
			Score += 0.35f;
		}
		if (CandidateIdentity->HasZonefallTag(GetBanditRoleTag()))
		{
			Score += 0.2f;
		}
		if (IsEnemyFor(CandidateIdentity))
		{
			Score += 0.5f;
		}
		if (CandidateIdentity->IsHarmless())
		{
			Score -= 0.25f;
		}
	}

	return FMath::Clamp(Score, 0.0f, 1.5f);
}

FGameplayTag UZonefallAICharacterComponent::GetHumanArchetypeTag()
{
	return RequestTag(ZonefallAITags::Human);
}

FGameplayTag UZonefallAICharacterComponent::GetAnimalArchetypeTag()
{
	return RequestTag(ZonefallAITags::Animal);
}

FGameplayTag UZonefallAICharacterComponent::GetBanditRoleTag()
{
	return RequestTag(ZonefallAITags::Bandit);
}

FGameplayTag UZonefallAICharacterComponent::GetHeroRoleTag()
{
	return RequestTag(ZonefallAITags::Hero);
}

FGameplayTag UZonefallAICharacterComponent::GetResidentRoleTag()
{
	return RequestTag(ZonefallAITags::Resident);
}

FGameplayTag UZonefallAICharacterComponent::GetHarmlessDispositionTag()
{
	return RequestTag(ZonefallAITags::Harmless);
}

FGameplayTag UZonefallAICharacterComponent::GetFriendlyDispositionTag()
{
	return RequestTag(ZonefallAITags::Friendly);
}

FGameplayTag UZonefallAICharacterComponent::GetHostileDispositionTag()
{
	return RequestTag(ZonefallAITags::Hostile);
}

FGameplayTag UZonefallAICharacterComponent::GetFriendlyRelationshipTag()
{
	return RequestTag(ZonefallAITags::RelationshipFriendly);
}

FGameplayTag UZonefallAICharacterComponent::GetEnemyRelationshipTag()
{
	return RequestTag(ZonefallAITags::RelationshipEnemy);
}

UZonefallAICharacterComponent* UZonefallAICharacterComponent::FindAICharacterComponent(const AActor* Actor)
{
	return Actor ? Actor->FindComponentByClass<UZonefallAICharacterComponent>() : nullptr;
}

void UZonefallAICharacterComponent::EnsureDefaultIdentity()
{
	if (!ArchetypeTag.IsValid() && !RoleTag.IsValid() && !FactionTag.IsValid() && !DispositionTag.IsValid())
	{
		ApplyCharacterProfile();
	}
}

UZonefallNpcVitalsComponent* UZonefallAICharacterComponent::EnsureNpcVitalsComponent()
{
	AActor* Owner = GetOwner();
	if (!Owner)
	{
		return nullptr;
	}

	if (!CachedNpcVitals)
	{
		CachedNpcVitals = Owner->FindComponentByClass<UZonefallNpcVitalsComponent>();
	}

	if (!CachedNpcVitals && bAutoAddNpcVitals)
	{
		CachedNpcVitals = NewObject<UZonefallNpcVitalsComponent>(
			Owner,
			UZonefallNpcVitalsComponent::StaticClass(),
			NAME_None,
			RF_Transactional);
		if (CachedNpcVitals)
		{
			Owner->AddInstanceComponent(CachedNpcVitals);
			CachedNpcVitals->RegisterComponent();
		}
	}

	if (CachedNpcVitals)
	{
		CachedNpcVitals->ApplyWidgetSettingsFromAICharacter(this);
		CachedNpcVitals->SetupStatusWidget();
	}

	return CachedNpcVitals;
}

void UZonefallAICharacterComponent::SyncNpcVitalsFromIdentity()
{
	EnsureAutoZonefallComponents();
}

FGameplayTag UZonefallAICharacterComponent::RequestTag(FName TagName)
{
	return UGameplayTagsManager::Get().RequestGameplayTag(TagName, false);
}
