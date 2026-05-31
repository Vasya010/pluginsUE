#include "ZonefallAIController.h"

#include "BehaviorTree/BehaviorTree.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "BehaviorTree/BlackboardData.h"
#include "Components/TextRenderComponent.h"
#include "GameFramework/PlayerController.h"
#include "Perception/AIPerceptionComponent.h"
#include "Perception/AISense.h"
#include "Perception/AISense_Hearing.h"
#include "Perception/AISense_Sight.h"
#include "Perception/AISenseConfig_Hearing.h"
#include "Perception/AISenseConfig_Sight.h"
#include "ZonefallAI.h"
#include "ZonefallAIBarkComponent.h"
#include "ZonefallAICharacterComponent.h"
#include "ZonefallAIPerceptionMemoryComponent.h"
#include "ZonefallAIPatrolComponent.h"
#include "ZonefallAISquadSubsystem.h"
#include "ZonefallAISuppressionComponent.h"
#include "ZonefallAITacticalCoverComponent.h"

namespace
{
	FName ZonefallTagName(const FGameplayTag& Tag)
	{
		return Tag.IsValid() ? Tag.GetTagName() : NAME_None;
	}
}

AZonefallAIController::AZonefallAIController(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	PrimaryActorTick.bCanEverTick = true;

	ZonefallPerceptionComponent = CreateDefaultSubobject<UAIPerceptionComponent>(TEXT("ZonefallPerception"));
	SetPerceptionComponent(*ZonefallPerceptionComponent);
	PerceptionMemoryComponent = CreateDefaultSubobject<UZonefallAIPerceptionMemoryComponent>(TEXT("ZonefallPerceptionMemory"));
	CoverComponent = CreateDefaultSubobject<UZonefallAITacticalCoverComponent>(TEXT("ZonefallCover"));
	SuppressionComponent = CreateDefaultSubobject<UZonefallAISuppressionComponent>(TEXT("ZonefallSuppression"));
	BarkComponent = CreateDefaultSubobject<UZonefallAIBarkComponent>(TEXT("ZonefallBark"));
	AlertIndicatorComponent = CreateDefaultSubobject<UTextRenderComponent>(TEXT("BanditAlertIndicator"));
	AlertIndicatorComponent->SetHiddenInGame(true);
	AlertIndicatorComponent->SetTextRenderColor(FColor::Yellow);
	AlertIndicatorComponent->SetHorizontalAlignment(EHTA_Center);
	AlertIndicatorComponent->SetVerticalAlignment(EVRTA_TextCenter);
	AlertIndicatorComponent->SetWorldSize(34.0f);
	AlertIndicatorComponent->SetText(FText::FromString(TEXT("[          ]")));

	SightConfig = CreateDefaultSubobject<UAISenseConfig_Sight>(TEXT("SightConfig"));
	SightConfig->SightRadius = 3500.0f;
	SightConfig->LoseSightRadius = 4200.0f;
	SightConfig->PeripheralVisionAngleDegrees = 85.0f;
	SightConfig->SetMaxAge(6.0f);
	SightConfig->AutoSuccessRangeFromLastSeenLocation = 600.0f;
	SightConfig->DetectionByAffiliation.bDetectEnemies = true;
	SightConfig->DetectionByAffiliation.bDetectFriendlies = false;
	SightConfig->DetectionByAffiliation.bDetectNeutrals = true;

	HearingConfig = CreateDefaultSubobject<UAISenseConfig_Hearing>(TEXT("HearingConfig"));
	HearingConfig->HearingRange = 2600.0f;
	HearingConfig->SetMaxAge(8.0f);
	HearingConfig->DetectionByAffiliation.bDetectEnemies = true;
	HearingConfig->DetectionByAffiliation.bDetectFriendlies = true;
	HearingConfig->DetectionByAffiliation.bDetectNeutrals = true;

	ZonefallPerceptionComponent->ConfigureSense(*SightConfig);
	ZonefallPerceptionComponent->ConfigureSense(*HearingConfig);
	ZonefallPerceptionComponent->SetDominantSense(SightConfig->GetSenseImplementation());
	ZonefallPerceptionComponent->OnTargetPerceptionUpdated.AddDynamic(this, &AZonefallAIController::HandleTargetPerceptionUpdated);

	bAttachToPawn = true;
}

void AZonefallAIController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);
	AttachAlertIndicator(InPawn);
	SeedBlackboard(InPawn);
	StartZonefallBehavior();

	if (bAutoJoinSquad)
	{
		RegisterWithSquad(SquadOverrideName);
	}
}

void AZonefallAIController::OnUnPossess()
{
	UnregisterFromSquad();

	StopZonefallBehavior(TEXT("AI controller unpossessed"));
	if (AlertIndicatorComponent)
	{
		AlertIndicatorComponent->DetachFromComponent(FDetachmentTransformRules::KeepWorldTransform);
		AlertIndicatorComponent->SetHiddenInGame(true);
	}
	Super::OnUnPossess();
}

int32 AZonefallAIController::RegisterWithSquad(FName SquadName)
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return INDEX_NONE;
	}

	UZonefallAISquadSubsystem* SquadSubsystem = World->GetSubsystem<UZonefallAISquadSubsystem>();
	if (!SquadSubsystem)
	{
		return INDEX_NONE;
	}

	FGameplayTag FactionTag;
	if (const APawn* ControlledPawn = GetPawn())
	{
		if (const UZonefallAICharacterComponent* Identity = UZonefallAICharacterComponent::FindAICharacterComponent(ControlledPawn))
		{
			FactionTag = Identity->FactionTag;
		}
	}

	return SquadSubsystem->RegisterAgent(this, SquadName, FactionTag);
}

void AZonefallAIController::UnregisterFromSquad()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	if (UZonefallAISquadSubsystem* SquadSubsystem = World->GetSubsystem<UZonefallAISquadSubsystem>())
	{
		SquadSubsystem->UnregisterAgent(this);
	}
}

void AZonefallAIController::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	UpdateAlertIndicator();
}

bool AZonefallAIController::StartZonefallBehavior()
{
	UBehaviorTree* BehaviorTreeToRun = DefaultBehaviorTree;
	UBlackboardData* BlackboardAsset = DefaultBlackboardAsset;

	if (!BehaviorTreeToRun && bUseCodedBehaviorTreeWhenAssetMissing)
	{
		FZonefallCodedBehaviorTreeSettings RuntimeSettings = CodedBehaviorSettings;
		RuntimeSettings.BlackboardKeys = BlackboardKeys;

		if (!RuntimeCodedBlackboardAsset)
		{
			RuntimeCodedBlackboardAsset = UZonefallCodedBehaviorTreeLibrary::CreateAdventureBlackboard(this, BlackboardKeys);
		}

		if (!RuntimeCodedBehaviorTree)
		{
			RuntimeCodedBehaviorTree = UZonefallCodedBehaviorTreeLibrary::CreateAdventureBehaviorTree(this, RuntimeCodedBlackboardAsset, RuntimeSettings);
		}

		BehaviorTreeToRun = RuntimeCodedBehaviorTree;
	}

	if (!BlackboardAsset && BehaviorTreeToRun)
	{
		BlackboardAsset = BehaviorTreeToRun->BlackboardAsset;
	}

	if (BlackboardAsset)
	{
		UBlackboardComponent* BlackboardComponent = nullptr;
		if (!UseBlackboard(BlackboardAsset, BlackboardComponent))
		{
			UE_LOG(LogZonefallAI, Warning, TEXT("%s failed to initialize blackboard %s"), *GetName(), *GetNameSafe(BlackboardAsset));
			return false;
		}

		SeedBlackboard(GetPawn());
	}

	if (BehaviorTreeToRun)
	{
		return RunBehaviorTree(BehaviorTreeToRun);
	}

	return BlackboardAsset != nullptr;
}

void AZonefallAIController::StopZonefallBehavior(const FString& Reason)
{
	if (BrainComponent)
	{
		BrainComponent->StopLogic(Reason);
	}

	StopMovement();
}

void AZonefallAIController::HandleTargetPerceptionUpdated(AActor* Actor, FAIStimulus Stimulus)
{
	UBlackboardComponent* BlackboardComponent = GetBlackboardComponent();
	if (!BlackboardComponent || !PerceptionMemoryComponent || !Actor || Actor == GetPawn())
	{
		return;
	}

	const FAISenseID SightID = UAISense::GetSenseID<UAISense_Sight>();
	const FAISenseID HearingID = UAISense::GetSenseID<UAISense_Hearing>();

	UZonefallAISquadSubsystem* SquadSubsystem = GetWorld() ? GetWorld()->GetSubsystem<UZonefallAISquadSubsystem>() : nullptr;

	if (Stimulus.Type == SightID)
	{
		if (Stimulus.WasSuccessfullySensed() && CanAcceptPerceivedTarget(Actor))
		{
			PerceptionMemoryComponent->RecordSightStimulus(Actor, Actor->GetActorLocation(), Stimulus.Strength);
			PushMemoryToBlackboard();
			if (PerceptionMemoryComponent->IsDetectionComplete())
			{
				UZonefallAIBlackboardLibrary::SetZonefallTarget(BlackboardComponent, BlackboardKeys, Actor, true);
				OnZonefallTargetSpotted(Actor);
				if (SquadSubsystem)
				{
					SquadSubsystem->ReportTargetSpotted(this, Actor);
				}
				if (BarkComponent)
				{
					BarkComponent->TryBarkCategory(EZonefallAIBarkCategory::Contact);
				}
			}
		}
		else if (BlackboardComponent->GetValueAsObject(BlackboardKeys.TargetActor) == Actor)
		{
			const FVector LastKnownLocation = Stimulus.StimulusLocation.IsNearlyZero() ? Actor->GetActorLocation() : Stimulus.StimulusLocation;
			PerceptionMemoryComponent->RecordLostSight(Actor, LastKnownLocation);
			BlackboardComponent->SetValueAsVector(BlackboardKeys.LastKnownTargetLocation, LastKnownLocation);
			UZonefallAIBlackboardLibrary::ClearZonefallTarget(BlackboardComponent, BlackboardKeys, true);
			PushMemoryToBlackboard();
			OnZonefallTargetLost(Actor, LastKnownLocation);
			if (SquadSubsystem)
			{
				SquadSubsystem->ReportTargetLost(this, Actor, LastKnownLocation);
			}
			if (BarkComponent)
			{
				BarkComponent->TryBarkCategory(EZonefallAIBarkCategory::LostTarget);
			}
			if (PerceptionMemoryComponent->bShouldInvestigate)
			{
				OnZonefallInvestigationStarted(PerceptionMemoryComponent->InvestigationLocation, PerceptionMemoryComponent->LastStimulusType);
			}
		}
	}
	else if (Stimulus.Type == HearingID && Stimulus.WasSuccessfullySensed())
	{
		PerceptionMemoryComponent->RecordHearingStimulus(Actor, Stimulus.StimulusLocation, Stimulus.Strength);
		PushMemoryToBlackboard();
		OnZonefallSuspiciousSoundHeard(Stimulus.StimulusLocation, Actor);
		if (PerceptionMemoryComponent->bShouldInvestigate)
		{
			OnZonefallInvestigationStarted(PerceptionMemoryComponent->InvestigationLocation, EZonefallAIStimulusType::Hearing);
		}
	}
}

void AZonefallAIController::OnZonefallTargetSpotted_Implementation(AActor* TargetActor)
{
}

void AZonefallAIController::OnZonefallSuspiciousSoundHeard_Implementation(FVector SoundLocation, AActor* SourceActor)
{
}

void AZonefallAIController::OnZonefallTargetLost_Implementation(AActor* LostActor, FVector LastKnownLocation)
{
}

void AZonefallAIController::OnZonefallInvestigationStarted_Implementation(FVector InvestigationLocation, EZonefallAIStimulusType StimulusType)
{
}

bool AZonefallAIController::CanAcceptPerceivedTarget(AActor* Actor) const
{
	const APawn* ControlledPawn = GetPawn();
	const UZonefallAICharacterComponent* SelfIdentity = ControlledPawn ? ControlledPawn->FindComponentByClass<UZonefallAICharacterComponent>() : nullptr;
	return !SelfIdentity || SelfIdentity->CanTargetActor(Actor);
}

void AZonefallAIController::PushMemoryToBlackboard()
{
	if (PerceptionMemoryComponent)
	{
		PerceptionMemoryComponent->WriteToBlackboard(GetBlackboardComponent(), BlackboardKeys);
	}
}

void AZonefallAIController::AttachAlertIndicator(APawn* InPawn)
{
	if (!AlertIndicatorComponent || !InPawn || !InPawn->GetRootComponent())
	{
		return;
	}

	AlertIndicatorComponent->AttachToComponent(InPawn->GetRootComponent(), FAttachmentTransformRules::KeepRelativeTransform);
	AlertIndicatorComponent->SetRelativeLocation(FVector(0.0f, 0.0f, AlertIndicatorHeight));
	AlertIndicatorComponent->SetHiddenInGame(true);
}

void AZonefallAIController::UpdateAlertIndicator()
{
	if (!AlertIndicatorComponent || !PerceptionMemoryComponent)
	{
		return;
	}

	const bool bShouldShow = ShouldShowAlertIndicator();
	AlertIndicatorComponent->SetHiddenInGame(!bShouldShow);
	if (!bShouldShow)
	{
		return;
	}

	const float Alpha = FMath::Clamp(PerceptionMemoryComponent->DetectionProgress / FMath::Max(PerceptionMemoryComponent->DetectionThreshold, KINDA_SMALL_NUMBER), 0.0f, 1.0f);
	const int32 FilledBars = FMath::Clamp(FMath::RoundToInt(Alpha * 10.0f), 0, 10);
	const FString Bar = FString::ChrN(FilledBars, TCHAR('|')) + FString::ChrN(10 - FilledBars, TCHAR('.'));
	const FString Prefix = PerceptionMemoryComponent->IsDetectionComplete() ? TEXT("! ") : TEXT("? ");
	AlertIndicatorComponent->SetText(FText::FromString(Prefix + TEXT("[") + Bar + TEXT("]")));
	AlertIndicatorComponent->SetTextRenderColor(PerceptionMemoryComponent->IsDetectionComplete() ? FColor::Red : FColor::Yellow);
	AlertIndicatorComponent->SetRelativeLocation(FVector(0.0f, 0.0f, AlertIndicatorHeight));

	if (APlayerController* PlayerController = GetWorld() ? GetWorld()->GetFirstPlayerController() : nullptr)
	{
		FVector CameraLocation;
		FRotator CameraRotation;
		PlayerController->GetPlayerViewPoint(CameraLocation, CameraRotation);
		const FVector ToCamera = CameraLocation - AlertIndicatorComponent->GetComponentLocation();
		if (!ToCamera.IsNearlyZero())
		{
			AlertIndicatorComponent->SetWorldRotation(ToCamera.Rotation());
		}
	}
}

bool AZonefallAIController::ShouldShowAlertIndicator() const
{
	if (!bUseBanditAlertIndicator || !PerceptionMemoryComponent || PerceptionMemoryComponent->DetectionProgress <= 0.0f)
	{
		return false;
	}

	const APawn* ControlledPawn = GetPawn();
	const UZonefallAICharacterComponent* Identity = ControlledPawn ? ControlledPawn->FindComponentByClass<UZonefallAICharacterComponent>() : nullptr;
	return Identity && Identity->bAlwaysChaseMainHero;
}

void AZonefallAIController::SeedBlackboard(APawn* InPawn)
{
	UBlackboardComponent* BlackboardComponent = GetBlackboardComponent();
	if (!BlackboardComponent || !InPawn)
	{
		return;
	}

	const FVector PawnLocation = InPawn->GetActorLocation();
	BlackboardComponent->SetValueAsVector(BlackboardKeys.HomeLocation, PawnLocation);
	BlackboardComponent->SetValueAsVector(BlackboardKeys.PatrolAnchor, PawnLocation);
	BlackboardComponent->SetValueAsFloat(BlackboardKeys.PatrolRadius, DefaultPatrolRadius);
	BlackboardComponent->SetValueAsFloat(BlackboardKeys.SearchRadius, DefaultSearchRadius);
	BlackboardComponent->SetValueAsFloat(BlackboardKeys.PatrolAcceptanceRadius, CodedBehaviorSettings.PatrolAcceptanceRadius);
	BlackboardComponent->SetValueAsFloat(BlackboardKeys.PatrolWaitTime, CodedBehaviorSettings.PatrolWaitTime);
	BlackboardComponent->SetValueAsBool(BlackboardKeys.HasTarget, false);
	BlackboardComponent->SetValueAsBool(BlackboardKeys.HasLineOfSight, false);
	BlackboardComponent->SetValueAsBool(BlackboardKeys.IsInvestigating, false);
	BlackboardComponent->SetValueAsBool(BlackboardKeys.ShouldInvestigate, false);
	BlackboardComponent->SetValueAsFloat(BlackboardKeys.MemoryConfidence, 0.0f);
	BlackboardComponent->SetValueAsFloat(BlackboardKeys.Suspicion, 0.0f);
	BlackboardComponent->SetValueAsFloat(BlackboardKeys.TimeSinceLastStimulus, 0.0f);
	BlackboardComponent->SetValueAsFloat(BlackboardKeys.Alertness, 0.0f);
	UZonefallAIBlackboardLibrary::SetZonefallThreatState(BlackboardComponent, BlackboardKeys, EZonefallAIThreatState::Passive);

	if (const UZonefallAICharacterComponent* IdentityComponent = InPawn->FindComponentByClass<UZonefallAICharacterComponent>())
	{
		BlackboardComponent->SetValueAsName(BlackboardKeys.SelfArchetypeTag, ZonefallTagName(IdentityComponent->ArchetypeTag));
		BlackboardComponent->SetValueAsName(BlackboardKeys.SelfRoleTag, ZonefallTagName(IdentityComponent->RoleTag));
		BlackboardComponent->SetValueAsName(BlackboardKeys.SelfFactionTag, ZonefallTagName(IdentityComponent->FactionTag));
		BlackboardComponent->SetValueAsName(BlackboardKeys.SelfDispositionTag, ZonefallTagName(IdentityComponent->DispositionTag));
		BlackboardComponent->SetValueAsName(BlackboardKeys.SelfRelationshipTag, ZonefallTagName(IdentityComponent->RelationshipTag));
	}

	if (UZonefallAIPatrolComponent* PatrolComponent = InPawn->FindComponentByClass<UZonefallAIPatrolComponent>())
	{
		if (PatrolComponent->GetPatrolAnchor().IsNearlyZero())
		{
			PatrolComponent->SetPatrolAnchor(PawnLocation);
		}

		BlackboardComponent->SetValueAsVector(BlackboardKeys.PatrolAnchor, PatrolComponent->GetPatrolAnchor());
		BlackboardComponent->SetValueAsFloat(BlackboardKeys.PatrolRadius, PatrolComponent->PatrolRadius);
		BlackboardComponent->SetValueAsFloat(BlackboardKeys.SearchRadius, PatrolComponent->SearchRadius);
		BlackboardComponent->SetValueAsFloat(BlackboardKeys.PatrolAcceptanceRadius, PatrolComponent->DefaultAcceptanceRadius);
		BlackboardComponent->SetValueAsFloat(BlackboardKeys.PatrolWaitTime, PatrolComponent->DefaultWaitTime);
	}
}
