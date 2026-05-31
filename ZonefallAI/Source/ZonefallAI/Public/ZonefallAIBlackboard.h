#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "ZonefallAIBlackboard.generated.h"

class AActor;
class UBlackboardComponent;

UENUM(BlueprintType)
enum class EZonefallAIThreatState : uint8
{
	Passive UMETA(DisplayName = "Passive"),
	Suspicious UMETA(DisplayName = "Suspicious"),
	Alert UMETA(DisplayName = "Alert"),
	Combat UMETA(DisplayName = "Combat"),
	LostTarget UMETA(DisplayName = "Lost Target")
};

UENUM(BlueprintType)
enum class EZonefallAIStimulusType : uint8
{
	None UMETA(DisplayName = "None"),
	Sight UMETA(DisplayName = "Sight"),
	Hearing UMETA(DisplayName = "Hearing"),
	Damage UMETA(DisplayName = "Damage")
};

UENUM(BlueprintType)
enum class EZonefallAISquadRole : uint8
{
	None UMETA(DisplayName = "None"),
	Attacker UMETA(DisplayName = "Attacker"),
	Flanker UMETA(DisplayName = "Flanker"),
	Suppressor UMETA(DisplayName = "Suppressor"),
	Support UMETA(DisplayName = "Support"),
	Coverer UMETA(DisplayName = "Coverer"),
	Retreat UMETA(DisplayName = "Retreat")
};

UENUM(BlueprintType)
enum class EZonefallAIFlankSide : uint8
{
	None UMETA(DisplayName = "None"),
	Left UMETA(DisplayName = "Left"),
	Right UMETA(DisplayName = "Right"),
	Rear UMETA(DisplayName = "Rear")
};

UENUM(BlueprintType)
enum class EZonefallAITacticalIntent : uint8
{
	None UMETA(DisplayName = "None"),
	HoldCover UMETA(DisplayName = "Hold Cover"),
	AdvanceToCover UMETA(DisplayName = "Advance To Cover"),
	Flank UMETA(DisplayName = "Flank"),
	Suppress UMETA(DisplayName = "Suppress"),
	Engage UMETA(DisplayName = "Engage"),
	Reposition UMETA(DisplayName = "Reposition"),
	Regroup UMETA(DisplayName = "Regroup"),
	Cower UMETA(DisplayName = "Cower")
};

USTRUCT(BlueprintType)
struct ZONEFALLAI_API FZonefallAIBlackboardKeys
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Target")
	FName TargetActor = TEXT("TargetActor");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Target")
	FName FocusActor = TEXT("FocusActor");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Target")
	FName HasTarget = TEXT("HasTarget");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Target")
	FName LastKnownTargetLocation = TEXT("LastKnownTargetLocation");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Target")
	FName LastHeardLocation = TEXT("LastHeardLocation");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Target")
	FName InvestigationLocation = TEXT("InvestigationLocation");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Target")
	FName LastSeenTime = TEXT("LastSeenTime");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Target")
	FName HasLineOfSight = TEXT("HasLineOfSight");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement")
	FName MoveLocation = TEXT("MoveLocation");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement")
	FName HomeLocation = TEXT("HomeLocation");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement")
	FName PatrolAnchor = TEXT("PatrolAnchor");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement")
	FName PatrolRadius = TEXT("PatrolRadius");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement")
	FName SearchRadius = TEXT("SearchRadius");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement")
	FName PatrolAcceptanceRadius = TEXT("PatrolAcceptanceRadius");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement")
	FName PatrolWaitTime = TEXT("PatrolWaitTime");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "State")
	FName ThreatState = TEXT("ThreatState");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "State")
	FName Alertness = TEXT("Alertness");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "State")
	FName MemoryConfidence = TEXT("MemoryConfidence");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "State")
	FName Suspicion = TEXT("Suspicion");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "State")
	FName TimeSinceLastStimulus = TEXT("TimeSinceLastStimulus");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "State")
	FName StimulusType = TEXT("StimulusType");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "State")
	FName ShouldInvestigate = TEXT("ShouldInvestigate");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "State")
	FName IsInvestigating = TEXT("IsInvestigating");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "State")
	FName DesiredSpeed = TEXT("DesiredSpeed");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tags")
	FName SelfArchetypeTag = TEXT("SelfArchetypeTag");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tags")
	FName SelfRoleTag = TEXT("SelfRoleTag");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tags")
	FName SelfFactionTag = TEXT("SelfFactionTag");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tags")
	FName SelfDispositionTag = TEXT("SelfDispositionTag");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tags")
	FName SelfRelationshipTag = TEXT("SelfRelationshipTag");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tags")
	FName TargetArchetypeTag = TEXT("TargetArchetypeTag");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tags")
	FName TargetRoleTag = TEXT("TargetRoleTag");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tags")
	FName TargetFactionTag = TEXT("TargetFactionTag");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tags")
	FName TargetDispositionTag = TEXT("TargetDispositionTag");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tags")
	FName TargetRelationshipTag = TEXT("TargetRelationshipTag");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tags")
	FName TargetThreatScore = TEXT("TargetThreatScore");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Squad")
	FName SquadRole = TEXT("SquadRole");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Squad")
	FName TacticalIntent = TEXT("TacticalIntent");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Squad")
	FName HasAttackToken = TEXT("HasAttackToken");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Squad")
	FName SquadCohesion = TEXT("SquadCohesion");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Squad")
	FName SquadLeader = TEXT("SquadLeader");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Squad")
	FName SquadMemberCount = TEXT("SquadMemberCount");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cover")
	FName CoverLocation = TEXT("CoverLocation");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cover")
	FName CoverFacing = TEXT("CoverFacing");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cover")
	FName HasCover = TEXT("HasCover");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cover")
	FName CoverScore = TEXT("CoverScore");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flank")
	FName FlankPivotLocation = TEXT("FlankPivotLocation");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flank")
	FName FlankSide = TEXT("FlankSide");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Suppression")
	FName IsSuppressed = TEXT("IsSuppressed");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Suppression")
	FName SuppressionLevel = TEXT("SuppressionLevel");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Suppression")
	FName SuppressionSource = TEXT("SuppressionSource");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Health")
	FName HealthFraction = TEXT("HealthFraction");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Health")
	FName IsRetreating = TEXT("IsRetreating");
};

UCLASS()
class ZONEFALLAI_API UZonefallAIBlackboardLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintPure, Category = "Zonefall AI|Blackboard")
	static FZonefallAIBlackboardKeys MakeDefaultZonefallBlackboardKeys();

	UFUNCTION(BlueprintCallable, Category = "Zonefall AI|Blackboard")
	static void SetZonefallTarget(UBlackboardComponent* Blackboard, const FZonefallAIBlackboardKeys& Keys, AActor* TargetActor, bool bHasLineOfSight);

	UFUNCTION(BlueprintCallable, Category = "Zonefall AI|Blackboard")
	static void ClearZonefallTarget(UBlackboardComponent* Blackboard, const FZonefallAIBlackboardKeys& Keys, bool bKeepLastKnownLocation);

	UFUNCTION(BlueprintCallable, Category = "Zonefall AI|Blackboard")
	static void SetZonefallThreatState(UBlackboardComponent* Blackboard, const FZonefallAIBlackboardKeys& Keys, EZonefallAIThreatState ThreatState);

	UFUNCTION(BlueprintCallable, Category = "Zonefall AI|Blackboard")
	static void SetZonefallSquadRole(UBlackboardComponent* Blackboard, const FZonefallAIBlackboardKeys& Keys, EZonefallAISquadRole Role);

	UFUNCTION(BlueprintCallable, Category = "Zonefall AI|Blackboard")
	static void SetZonefallTacticalIntent(UBlackboardComponent* Blackboard, const FZonefallAIBlackboardKeys& Keys, EZonefallAITacticalIntent Intent);

	UFUNCTION(BlueprintCallable, Category = "Zonefall AI|Blackboard")
	static void SetZonefallCover(UBlackboardComponent* Blackboard, const FZonefallAIBlackboardKeys& Keys, FVector Location, FVector Facing, float Score);

	UFUNCTION(BlueprintCallable, Category = "Zonefall AI|Blackboard")
	static void ClearZonefallCover(UBlackboardComponent* Blackboard, const FZonefallAIBlackboardKeys& Keys);

	UFUNCTION(BlueprintCallable, Category = "Zonefall AI|Blackboard")
	static void SetZonefallFlank(UBlackboardComponent* Blackboard, const FZonefallAIBlackboardKeys& Keys, FVector PivotLocation, EZonefallAIFlankSide Side);

	UFUNCTION(BlueprintCallable, Category = "Zonefall AI|Blackboard")
	static void SetZonefallSuppression(UBlackboardComponent* Blackboard, const FZonefallAIBlackboardKeys& Keys, float Level, AActor* Source);

	UFUNCTION(BlueprintPure, Category = "Zonefall AI|Blackboard")
	static EZonefallAISquadRole GetZonefallSquadRole(const UBlackboardComponent* Blackboard, const FZonefallAIBlackboardKeys& Keys);

	UFUNCTION(BlueprintPure, Category = "Zonefall AI|Blackboard")
	static EZonefallAITacticalIntent GetZonefallTacticalIntent(const UBlackboardComponent* Blackboard, const FZonefallAIBlackboardKeys& Keys);
};
