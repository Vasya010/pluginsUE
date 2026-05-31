#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ZonefallAIBlackboard.h"
#include "ZonefallAITacticalQuery.h"
#include "ZonefallAITacticalCoverComponent.generated.h"

class AActor;
class APawn;
class UBlackboardComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FZonefallCoverChangedEvent, FVector, NewCoverLocation, float, Score);

UCLASS(ClassGroup = (ZonefallAI), Blueprintable, meta = (BlueprintSpawnableComponent, DisplayName = "Zonefall AI Tactical Cover"))
class ZONEFALLAI_API UZonefallAITacticalCoverComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UZonefallAITacticalCoverComponent();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cover|Query")
	FZonefallTacticalQueryParams CoverQueryParams;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cover|Behavior", meta = (ClampMin = "0.0"))
	float MinTimeBetweenQueries = 0.4f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cover|Behavior", meta = (ClampMin = "0.0"))
	float MinCoverDistanceToReposition = 250.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cover|Behavior", meta = (ClampMin = "0.0"))
	float CompromisedRecheckInterval = 0.75f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cover|Behavior")
	bool bAutoUpdateBlackboard = true;

	UPROPERTY(BlueprintReadOnly, Category = "Cover|State")
	FVector CurrentCoverLocation = FVector::ZeroVector;

	UPROPERTY(BlueprintReadOnly, Category = "Cover|State")
	FVector CurrentCoverFacing = FVector::ZeroVector;

	UPROPERTY(BlueprintReadOnly, Category = "Cover|State")
	float CurrentCoverScore = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Cover|State")
	bool bHasCover = false;

	UPROPERTY(BlueprintReadOnly, Category = "Cover|State")
	float LastQueryTime = -100.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Cover|State")
	float LastCompromisedCheckTime = -100.0f;

	UPROPERTY(BlueprintAssignable, Category = "Cover|Events")
	FZonefallCoverChangedEvent OnCoverChanged;

	UFUNCTION(BlueprintCallable, Category = "Zonefall AI|Cover")
	bool RequestCoverFromTarget(AActor* TargetActor, bool bForceRefresh = false);

	UFUNCTION(BlueprintCallable, Category = "Zonefall AI|Cover")
	bool RequestFlankPosition(AActor* TargetActor, EZonefallAIFlankSide PreferredSide, FVector& OutFlankLocation);

	UFUNCTION(BlueprintCallable, Category = "Zonefall AI|Cover")
	bool RequestRetreatPoint(AActor* ThreatActor, FVector& OutRetreatLocation);

	UFUNCTION(BlueprintPure, Category = "Zonefall AI|Cover")
	bool IsCurrentCoverCompromisedBy(const AActor* TargetActor) const;

	UFUNCTION(BlueprintCallable, Category = "Zonefall AI|Cover")
	void ClearCover();

	UFUNCTION(BlueprintPure, Category = "Zonefall AI|Cover")
	APawn* GetAgentPawn() const;
};
