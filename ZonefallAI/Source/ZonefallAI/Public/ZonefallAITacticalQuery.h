#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "ZonefallAIBlackboard.h"
#include "ZonefallAITacticalQuery.generated.h"

class AActor;
class APawn;
class UWorld;

USTRUCT(BlueprintType)
struct ZONEFALLAI_API FZonefallTacticalCandidate
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Zonefall AI|Tactical")
	FVector Location = FVector::ZeroVector;

	UPROPERTY(BlueprintReadOnly, Category = "Zonefall AI|Tactical")
	FVector FacingDirection = FVector::ZeroVector;

	UPROPERTY(BlueprintReadOnly, Category = "Zonefall AI|Tactical")
	float Score = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Zonefall AI|Tactical")
	bool bHasCoverFromTarget = false;

	UPROPERTY(BlueprintReadOnly, Category = "Zonefall AI|Tactical")
	bool bHasLineOfSightToTarget = false;

	UPROPERTY(BlueprintReadOnly, Category = "Zonefall AI|Tactical")
	float DistanceFromAgent = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Zonefall AI|Tactical")
	float DistanceFromTarget = 0.0f;
};

USTRUCT(BlueprintType)
struct ZONEFALLAI_API FZonefallTacticalQueryParams
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sampling", meta = (ClampMin = "4", ClampMax = "256"))
	int32 SampleCount = 24;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sampling", meta = (ClampMin = "100.0"))
	float SampleRadius = 1200.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sampling", meta = (ClampMin = "0.0"))
	float MinSampleRadius = 250.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sampling", meta = (ClampMin = "50.0"))
	float NavProjectionExtent = 200.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sampling")
	bool bMustProjectToNav = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Trace")
	float EyeHeight = 60.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scoring", meta = (ClampMin = "0.0"))
	float CoverWeight = 6.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scoring")
	float LineOfSightWeight = 2.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scoring")
	float DistanceFromAgentPenalty = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scoring")
	float IdealDistanceFromTarget = 900.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scoring")
	float DistanceFromTargetWeight = 1.5f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scoring")
	float FlankWeight = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scoring")
	float TooCloseToTargetPenalty = 4.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scoring", meta = (ClampMin = "0.0"))
	float MinDistanceFromTarget = 300.0f;
};

UCLASS()
class ZONEFALLAI_API UZonefallAITacticalQueryLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Zonefall AI|Tactical", meta = (WorldContext = "WorldContextObject"))
	static bool FindBestCoverPoint(
		UObject* WorldContextObject,
		APawn* Agent,
		AActor* TargetActor,
		const FZonefallTacticalQueryParams& Params,
		FZonefallTacticalCandidate& OutBestCandidate);

	UFUNCTION(BlueprintCallable, Category = "Zonefall AI|Tactical", meta = (WorldContext = "WorldContextObject"))
	static bool FindBestFlankPoint(
		UObject* WorldContextObject,
		APawn* Agent,
		AActor* TargetActor,
		EZonefallAIFlankSide PreferredSide,
		const FZonefallTacticalQueryParams& Params,
		FZonefallTacticalCandidate& OutBestCandidate);

	UFUNCTION(BlueprintCallable, Category = "Zonefall AI|Tactical", meta = (WorldContext = "WorldContextObject"))
	static bool FindBestRetreatPoint(
		UObject* WorldContextObject,
		APawn* Agent,
		AActor* ThreatActor,
		const FZonefallTacticalQueryParams& Params,
		FZonefallTacticalCandidate& OutBestCandidate);

	UFUNCTION(BlueprintCallable, Category = "Zonefall AI|Tactical", meta = (WorldContext = "WorldContextObject"))
	static bool GatherTacticalCandidates(
		UObject* WorldContextObject,
		APawn* Agent,
		AActor* TargetActor,
		const FZonefallTacticalQueryParams& Params,
		TArray<FZonefallTacticalCandidate>& OutCandidates);

	UFUNCTION(BlueprintPure, Category = "Zonefall AI|Tactical")
	static bool HasLineOfSightBetween(
		UObject* WorldContextObject,
		FVector ObserverEyeLocation,
		AActor* TargetActor,
		AActor* ObserverActorToIgnore);
};
