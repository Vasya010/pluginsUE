#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "ZonefallAICharacterComponent.h"
#include "ZonefallAICharacter.generated.h"

class UZonefallNpcVitalsComponent;
class AZonefallPatrolPath;

/**
 * Ready-to-drop AI pawn for Zonefall. Everything is wired in C++ so you can just drag it
 * into the level (or set it as a spawner class) and it works:
 *   - UZonefallAICharacterComponent (identity / faction / profile)
 *   - UZonefallNpcVitalsComponent  (health, detection meter, floating status widget)
 *   - Possessed by AZonefallAIController (perception + behavior)
 *
 * Pick the Profile in the details panel (e.g. Enemy Bandit shows a health bar + detection
 * meter; Harmless Resident shows no health bar, only a brief detect flash). Assign a
 * SkeletalMesh + AnimClass like any character.
 */
UCLASS(Blueprintable)
class ZONEFALLAI_API AZonefallAICharacter : public ACharacter
{
	GENERATED_BODY()

public:
	AZonefallAICharacter();

	virtual void BeginPlay() override;
#if WITH_EDITOR
	virtual void OnConstruction(const FTransform& Transform) override;
#endif

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Zonefall AI")
	TObjectPtr<UZonefallAICharacterComponent> AICharacter;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Zonefall AI")
	TObjectPtr<UZonefallNpcVitalsComponent> Vitals;

	// Quick role picker — pushed into the AI character component on spawn.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zonefall AI")
	EZonefallAICharacterProfile Profile = EZonefallAICharacterProfile::EnemyBandit;

	// Starting health for this NPC (applied to the vitals component).
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zonefall AI", meta = (ClampMin = "1.0"))
	float StartingHealth = 100.0f;

	// --- Patrol ---
	// Assign a Zonefall Patrol Path actor and the NPC will walk that spline route.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zonefall AI|Patrol")
	TObjectPtr<AZonefallPatrolPath> PatrolPathActor;

	// If no patrol path is assigned, auto-build a wander route around the spawn so the NPC
	// never just stands still (requires a NavMeshBoundsVolume in the level).
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zonefall AI|Patrol")
	bool bAutoGeneratePatrolRoute = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zonefall AI|Patrol", meta = (ClampMin = "200.0"))
	float PatrolRouteRadius = 1400.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zonefall AI|Patrol", meta = (ClampMin = "2", ClampMax = "12"))
	int32 PatrolRoutePointCount = 5;

	// Walk speed used while patrolling (also drives the locomotion animation).
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zonefall AI|Patrol", meta = (ClampMin = "50.0"))
	float PatrolWalkSpeed = 230.0f;

private:
	void SetupPatrol();
};
