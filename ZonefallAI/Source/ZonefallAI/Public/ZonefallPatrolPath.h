#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ZonefallPatrolPath.generated.h"

class USplineComponent;
class UBillboardComponent;

/**
 * A placeable patrol route built on a spline. Drop it in the level, then drag the spline
 * points to shape the path. Assign it to AZonefallAICharacter::PatrolPathActor and the NPC
 * will copy the route on BeginPlay.
 *
 * The actor IS a spline (its root), so the patrol component's
 * FindComponentByClass<USplineComponent>() picks it up automatically.
 */
UCLASS(Blueprintable, meta = (DisplayName = "Zonefall Patrol Path"))
class ZONEFALLAI_API AZonefallPatrolPath : public AActor
{
	GENERATED_BODY()

public:
	AZonefallPatrolPath();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Patrol")
	TObjectPtr<USplineComponent> Spline;

#if WITH_EDITORONLY_DATA
	UPROPERTY()
	TObjectPtr<UBillboardComponent> EditorIcon;
#endif

	UFUNCTION(BlueprintPure, Category = "Patrol")
	USplineComponent* GetSpline() const { return Spline; }
};
