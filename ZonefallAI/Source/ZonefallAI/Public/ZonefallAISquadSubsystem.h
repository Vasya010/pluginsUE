#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Subsystems/WorldSubsystem.h"
#include "ZonefallAIBlackboard.h"
#include "ZonefallAISquadSubsystem.generated.h"

class AActor;
class AAIController;
class AZonefallAIController;
class UBlackboardComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_FourParams(FZonefallSquadRoleChangedEvent, int32, SquadId, AAIController*, Member, EZonefallAISquadRole, OldRole, EZonefallAISquadRole, NewRole);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FZonefallSquadTargetEvent, int32, SquadId, AActor*, NewTarget, AAIController*, Reporter);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FZonefallSquadCalloutEvent, int32, SquadId, FName, BarkId, AAIController*, Speaker);

USTRUCT(BlueprintType)
struct ZONEFALLAI_API FZonefallSquadMemberState
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Zonefall AI|Squad")
	TWeakObjectPtr<AAIController> Controller;

	UPROPERTY(BlueprintReadOnly, Category = "Zonefall AI|Squad")
	EZonefallAISquadRole Role = EZonefallAISquadRole::None;

	UPROPERTY(BlueprintReadOnly, Category = "Zonefall AI|Squad")
	float TimeRoleAssigned = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Zonefall AI|Squad")
	float LastBarkTime = -100.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Zonefall AI|Squad")
	bool bIsAlive = true;
};

USTRUCT(BlueprintType)
struct ZONEFALLAI_API FZonefallSquadRecord
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Zonefall AI|Squad")
	int32 SquadId = INDEX_NONE;

	UPROPERTY(BlueprintReadOnly, Category = "Zonefall AI|Squad")
	FName SquadName = NAME_None;

	UPROPERTY(BlueprintReadOnly, Category = "Zonefall AI|Squad")
	FGameplayTag FactionTag;

	UPROPERTY(BlueprintReadOnly, Category = "Zonefall AI|Squad")
	TArray<FZonefallSquadMemberState> Members;

	UPROPERTY(BlueprintReadOnly, Category = "Zonefall AI|Squad")
	TWeakObjectPtr<AAIController> Leader;

	UPROPERTY(BlueprintReadOnly, Category = "Zonefall AI|Squad")
	TWeakObjectPtr<AActor> SharedTarget;

	UPROPERTY(BlueprintReadOnly, Category = "Zonefall AI|Squad")
	FVector SharedLastKnownLocation = FVector::ZeroVector;

	UPROPERTY(BlueprintReadOnly, Category = "Zonefall AI|Squad")
	float SharedLastSeenTime = -1.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Zonefall AI|Squad")
	float LastRoleEvaluationTime = -1.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Zonefall AI|Squad")
	float LastSquadBarkTime = -100.0f;
};

USTRUCT(BlueprintType)
struct ZONEFALLAI_API FZonefallSquadConfig
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Squad", meta = (ClampMin = "1"))
	int32 MaxConcurrentAttackers = 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Squad", meta = (ClampMin = "0"))
	int32 MaxConcurrentFlankers = 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Squad", meta = (ClampMin = "0"))
	int32 MaxConcurrentSuppressors = 2;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Squad", meta = (ClampMin = "0.0"))
	float RoleEvaluationInterval = 1.25f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Squad", meta = (ClampMin = "0.0"))
	float MinTimeInRoleBeforeReassign = 4.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Squad", meta = (ClampMin = "0.0"))
	float SquadBarkCooldown = 2.5f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Squad", meta = (ClampMin = "0.0"))
	float CloseRangeForRoles = 600.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Squad", meta = (ClampMin = "0.0"))
	float MediumRangeForRoles = 1800.0f;
};

UCLASS()
class ZONEFALLAI_API UZonefallAISquadSubsystem : public UTickableWorldSubsystem
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintAssignable, Category = "Zonefall AI|Squad")
	FZonefallSquadRoleChangedEvent OnSquadRoleChanged;

	UPROPERTY(BlueprintAssignable, Category = "Zonefall AI|Squad")
	FZonefallSquadTargetEvent OnSquadTargetSpotted;

	UPROPERTY(BlueprintAssignable, Category = "Zonefall AI|Squad")
	FZonefallSquadTargetEvent OnSquadTargetLost;

	UPROPERTY(BlueprintAssignable, Category = "Zonefall AI|Squad")
	FZonefallSquadCalloutEvent OnSquadCallout;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zonefall AI|Squad")
	FZonefallSquadConfig DefaultConfig;

	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;
	virtual TStatId GetStatId() const override;
	virtual void Tick(float DeltaTime) override;
	virtual bool DoesSupportWorldType(const EWorldType::Type WorldType) const override;

	UFUNCTION(BlueprintCallable, Category = "Zonefall AI|Squad")
	int32 RegisterAgent(AAIController* Controller, FName SquadName, FGameplayTag FactionTag);

	UFUNCTION(BlueprintCallable, Category = "Zonefall AI|Squad")
	void UnregisterAgent(AAIController* Controller);

	UFUNCTION(BlueprintCallable, Category = "Zonefall AI|Squad")
	void ReportTargetSpotted(AAIController* Reporter, AActor* TargetActor);

	UFUNCTION(BlueprintCallable, Category = "Zonefall AI|Squad")
	void ReportTargetLost(AAIController* Reporter, AActor* TargetActor, FVector LastKnownLocation);

	UFUNCTION(BlueprintCallable, Category = "Zonefall AI|Squad")
	bool TryEmitSquadCallout(AAIController* Speaker, FName BarkId, float MinSquadCooldown = 2.0f, float MinSpeakerCooldown = 4.0f);

	UFUNCTION(BlueprintCallable, Category = "Zonefall AI|Squad")
	bool TryAssignSquadRoles(int32 SquadId);

	UFUNCTION(BlueprintPure, Category = "Zonefall AI|Squad")
	EZonefallAISquadRole GetAssignedRole(const AAIController* Controller) const;

	UFUNCTION(BlueprintPure, Category = "Zonefall AI|Squad")
	int32 GetSquadId(const AAIController* Controller) const;

	UFUNCTION(BlueprintPure, Category = "Zonefall AI|Squad")
	int32 GetSquadMemberCount(int32 SquadId) const;

	UFUNCTION(BlueprintPure, Category = "Zonefall AI|Squad")
	FVector GetSharedLastKnownLocation(int32 SquadId) const;

	UFUNCTION(BlueprintPure, Category = "Zonefall AI|Squad")
	AActor* GetSharedTarget(int32 SquadId) const;

	UFUNCTION(BlueprintPure, Category = "Zonefall AI|Squad")
	bool HasAttackToken(const AAIController* Controller) const;

	UFUNCTION(BlueprintCallable, Category = "Zonefall AI|Squad")
	void NotifyMemberDeath(AAIController* Controller);

private:
	struct FInternalSquad
	{
		FZonefallSquadRecord Record;
		FZonefallSquadConfig Config;
	};

	TMap<int32, FInternalSquad> Squads;
	TMap<FName, int32> SquadNameToId;
	TMap<TWeakObjectPtr<AAIController>, int32> ControllerToSquad;
	int32 NextSquadId = 1;
	float AccumulatedTime = 0.0f;

	FInternalSquad* FindOrCreateSquad(FName SquadName, FGameplayTag FactionTag);
	FInternalSquad* FindSquadByController(const AAIController* Controller);
	const FInternalSquad* FindSquadByController(const AAIController* Controller) const;
	FZonefallSquadMemberState* FindMemberState(FInternalSquad& Squad, AAIController* Controller);
	void EvaluateRoles(FInternalSquad& Squad, float CurrentTime);
	void AssignRoleToMember(FInternalSquad& Squad, FZonefallSquadMemberState& Member, EZonefallAISquadRole NewRole, float CurrentTime);
	void WriteRoleToBlackboard(AAIController* Controller, EZonefallAISquadRole Role) const;
	void WriteSquadStateToBlackboard(AAIController* Controller, const FInternalSquad& Squad) const;
	void PropagateSharedTarget(FInternalSquad& Squad);
	float NowSeconds() const;
};
