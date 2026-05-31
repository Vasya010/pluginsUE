#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameplayTagAssetInterface.h"
#include "GameplayTagContainer.h"
#include "ZonefallAICharacterComponent.generated.h"

class AActor;
class AController;
class APawn;
class UUserWidget;
class UZonefallAIPatrolComponent;
class UZonefallNpcVitalsComponent;

UENUM(BlueprintType)
enum class EZonefallAICombatIntent : uint8
{
	None UMETA(DisplayName = "None"),
	Defensive UMETA(DisplayName = "Defensive"),
	Aggressive UMETA(DisplayName = "Aggressive")
};

UENUM(BlueprintType)
enum class EZonefallAICharacterProfile : uint8
{
	Custom UMETA(DisplayName = "Custom"),
	MainHero UMETA(DisplayName = "Main Hero"),
	FriendlyResident UMETA(DisplayName = "Friendly Resident"),
	HarmlessResident UMETA(DisplayName = "Harmless Resident"),
	FriendlyCompanion UMETA(DisplayName = "Friendly Companion"),
	EnemyBandit UMETA(DisplayName = "Enemy Bandit"),
	PassiveAnimal UMETA(DisplayName = "Passive Animal"),
	DefensiveAnimal UMETA(DisplayName = "Defensive Animal"),
	HostileAnimal UMETA(DisplayName = "Hostile Animal")
};

UCLASS(ClassGroup = (ZonefallAI), Blueprintable, meta = (BlueprintSpawnableComponent, DisplayName = "Zonefall AI Character"))
class ZONEFALLAI_API UZonefallAICharacterComponent : public UActorComponent, public IGameplayTagAssetInterface
{
	GENERATED_BODY()

public:
	UZonefallAICharacterComponent();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Identity|Profile")
	EZonefallAICharacterProfile CharacterProfile = EZonefallAICharacterProfile::HarmlessResident;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Identity|Profile")
	bool bAutoApplyProfileOnBeginPlay = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Identity|Tags")
	FGameplayTag ArchetypeTag;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Identity|Tags")
	FGameplayTag RoleTag;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Identity|Tags")
	FGameplayTag FactionTag;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Identity|Tags")
	FGameplayTag DispositionTag;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Identity|Tags")
	FGameplayTag RelationshipTag;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Identity|Tags")
	FGameplayTagContainer AdditionalTags;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat")
	EZonefallAICombatIntent CombatIntent = EZonefallAICombatIntent::Defensive;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat")
	bool bCanInitiateCombat = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat")
	bool bCanTargetHarmlessActors = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Player")
	bool bIgnorePlayerAsTarget = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Player")
	bool bAlwaysChaseMainHero = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Player", meta = (ClampMin = "0.0"))
	float MainHeroChaseSpeed = 600.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat", meta = (ClampMin = "0.0"))
	float BaseThreatScore = 0.5f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Perception", meta = (ClampMin = "0.0"))
	float HearingSuspicionMultiplier = 0.35f;

	/** Adds Zonefall NPC Vitals + floating status widget automatically (no second component in the editor). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zonefall AI|Status Widget")
	bool bAutoAddNpcVitals = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zonefall AI|Status Widget", meta = (EditCondition = "bAutoAddNpcVitals"))
	bool bEnableFloatingStatusWidget = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zonefall AI|Status Widget", meta = (EditCondition = "bAutoAddNpcVitals && bEnableFloatingStatusWidget"))
	TSubclassOf<UUserWidget> StatusWidgetClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zonefall AI|Status Widget", meta = (EditCondition = "bAutoAddNpcVitals && bEnableFloatingStatusWidget"))
	float WidgetHeightOffset = 110.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zonefall AI|Status Widget", meta = (EditCondition = "bAutoAddNpcVitals && bEnableFloatingStatusWidget"))
	FVector2D WidgetDrawSize = FVector2D(150.0f, 30.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zonefall AI|Status Widget", meta = (ClampMin = "0.0", EditCondition = "bAutoAddNpcVitals && bEnableFloatingStatusWidget"))
	float WidgetVisibleDistance = 4500.0f;

	/** One component on the pawn: vitals, optional patrol, and controller helpers (see tooltips). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zonefall AI|Auto Setup")
	bool bAutoAddPatrolSpline = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zonefall AI|Auto Setup", meta = (EditCondition = "bAutoAddPatrolSpline"))
	bool bAutoAddPatrolOnlyForHostiles = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zonefall AI|Auto Setup")
	bool bAutoAddMissingControllerComponents = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zonefall AI|Auto Setup")
	bool bWarnWhenNotUsingZonefallAIController = true;

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void GetOwnedGameplayTags(FGameplayTagContainer& TagContainer) const override;
#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

	UFUNCTION(BlueprintCallable, CallInEditor, Category = "Zonefall AI|Identity")
	void ApplyCharacterProfile();

	UFUNCTION(BlueprintCallable, Category = "Zonefall AI|Identity")
	void ConfigureAsMainHero();

	UFUNCTION(BlueprintCallable, Category = "Zonefall AI|Identity")
	void ConfigureAsFriendlyResident();

	UFUNCTION(BlueprintCallable, Category = "Zonefall AI|Identity")
	void ConfigureAsHarmlessHuman();

	UFUNCTION(BlueprintCallable, Category = "Zonefall AI|Identity")
	void ConfigureAsFriendlyCompanion();

	UFUNCTION(BlueprintCallable, Category = "Zonefall AI|Identity")
	void ConfigureAsBandit();

	UFUNCTION(BlueprintCallable, Category = "Zonefall AI|Identity")
	void ConfigureAsAnimal(bool bDefensiveAnimal = true);

	UFUNCTION(BlueprintCallable, Category = "Zonefall AI|Identity")
	void ConfigureAsHostileAnimal();

	UFUNCTION(BlueprintPure, Category = "Zonefall AI|Identity")
	FGameplayTagContainer GetCombinedTags() const;

	UFUNCTION(BlueprintPure, Category = "Zonefall AI|Identity")
	bool HasZonefallTag(FGameplayTag TagToCheck) const;

	UFUNCTION(BlueprintPure, Category = "Zonefall AI|Combat")
	bool IsHarmless() const;

	UFUNCTION(BlueprintPure, Category = "Zonefall AI|Combat")
	bool IsFriendly() const;

	UFUNCTION(BlueprintPure, Category = "Zonefall AI|Combat")
	bool IsHostile() const;

	UFUNCTION(BlueprintPure, Category = "Zonefall AI|Combat")
	bool IsSameFactionAs(const UZonefallAICharacterComponent* OtherComponent) const;

	UFUNCTION(BlueprintPure, Category = "Zonefall AI|Combat")
	bool IsEnemyFor(const UZonefallAICharacterComponent* OtherComponent) const;

	UFUNCTION(BlueprintPure, Category = "Zonefall AI|Combat")
	bool IsMainHeroActor(const AActor* CandidateActor) const;

	UFUNCTION(BlueprintPure, Category = "Zonefall AI|Combat")
	bool ShouldForceChaseActor(const AActor* CandidateActor) const;

	UFUNCTION(BlueprintPure, Category = "Zonefall AI|Combat")
	bool CanTargetActor(const AActor* CandidateActor) const;

	UFUNCTION(BlueprintPure, Category = "Zonefall AI|Combat")
	float GetThreatScoreAgainst(const AActor* CandidateActor) const;

	UFUNCTION(BlueprintPure, Category = "Zonefall AI|Tags")
	static FGameplayTag GetHumanArchetypeTag();

	UFUNCTION(BlueprintPure, Category = "Zonefall AI|Tags")
	static FGameplayTag GetAnimalArchetypeTag();

	UFUNCTION(BlueprintPure, Category = "Zonefall AI|Tags")
	static FGameplayTag GetBanditRoleTag();

	UFUNCTION(BlueprintPure, Category = "Zonefall AI|Tags")
	static FGameplayTag GetHeroRoleTag();

	UFUNCTION(BlueprintPure, Category = "Zonefall AI|Tags")
	static FGameplayTag GetResidentRoleTag();

	UFUNCTION(BlueprintPure, Category = "Zonefall AI|Tags")
	static FGameplayTag GetHarmlessDispositionTag();

	UFUNCTION(BlueprintPure, Category = "Zonefall AI|Tags")
	static FGameplayTag GetFriendlyDispositionTag();

	UFUNCTION(BlueprintPure, Category = "Zonefall AI|Tags")
	static FGameplayTag GetHostileDispositionTag();

	UFUNCTION(BlueprintPure, Category = "Zonefall AI|Tags")
	static FGameplayTag GetFriendlyRelationshipTag();

	UFUNCTION(BlueprintPure, Category = "Zonefall AI|Tags")
	static FGameplayTag GetEnemyRelationshipTag();

	UFUNCTION(BlueprintPure, Category = "Zonefall AI|Tags")
	static UZonefallAICharacterComponent* FindAICharacterComponent(const AActor* Actor);

	/** Vitals created/found by this component (health, detection, status widget). */
	UFUNCTION(BlueprintPure, Category = "Zonefall AI|Status Widget")
	UZonefallNpcVitalsComponent* GetNpcVitalsComponent() const { return CachedNpcVitals; }

	UFUNCTION(BlueprintCallable, Category = "Zonefall AI|Status Widget")
	UZonefallNpcVitalsComponent* EnsureNpcVitalsComponent();

	UFUNCTION(BlueprintPure, Category = "Zonefall AI|Auto Setup")
	UZonefallAIPatrolComponent* GetPatrolComponent() const { return CachedPatrol; }

	UFUNCTION(BlueprintCallable, Category = "Zonefall AI|Auto Setup")
	UZonefallAIPatrolComponent* EnsurePatrolComponent();

	/** Vitals + patrol + controller components (safe to call again after possess). */
	UFUNCTION(BlueprintCallable, Category = "Zonefall AI|Auto Setup")
	void EnsureAutoZonefallComponents();

private:
	void EnsureDefaultIdentity();
	void SyncNpcVitalsFromIdentity();
	void EnsureControllerZonefallComponents();
	bool ShouldAutoAddPatrol() const;

	UFUNCTION()
	void HandleControllerChanged(APawn* Pawn, AController* OldController, AController* NewController);

	static FGameplayTag RequestTag(FName TagName);

	UPROPERTY(Transient)
	TObjectPtr<UZonefallNpcVitalsComponent> CachedNpcVitals;

	UPROPERTY(Transient)
	TObjectPtr<UZonefallAIPatrolComponent> CachedPatrol;
};
