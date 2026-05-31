#pragma once

#include "CoreMinimal.h"
#include "Components/SplineComponent.h"
#include "ZonefallAIPatrolComponent.generated.h"

class AActor;

UENUM(BlueprintType)
enum class EZonefallPatrolMode : uint8
{
	Radius UMETA(DisplayName = "Radius"),
	Loop UMETA(DisplayName = "Loop"),
	PingPong UMETA(DisplayName = "Ping Pong"),
	RandomPoint UMETA(DisplayName = "Random Point")
};

USTRUCT(BlueprintType)
struct ZONEFALLAI_API FZonefallPatrolPoint
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Patrol")
	FVector LocalOffset = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Patrol")
	TObjectPtr<AActor> PointActor;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Patrol", meta = (ClampMin = "0.0"))
	float WaitTime = 1.25f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Patrol", meta = (ClampMin = "0.0"))
	float AcceptanceRadius = 120.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Patrol")
	FName Label = NAME_None;
};

UCLASS(ClassGroup = (ZonefallAI), Blueprintable, meta = (BlueprintSpawnableComponent, DisplayName = "Zonefall AI Patrol Spline"))
class ZONEFALLAI_API UZonefallAIPatrolComponent : public USplineComponent
{
	GENERATED_BODY()

public:
	UZonefallAIPatrolComponent();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Patrol")
	EZonefallPatrolMode PatrolMode = EZonefallPatrolMode::Radius;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Patrol")
	bool bStartAtOwnerLocation = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Patrol|Spline")
	bool bUseSplineRoute = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Patrol|Spline")
	bool bCloseSplineWhenLooping = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Patrol|Spline")
	FLinearColor PatrolSplineColor = FLinearColor(0.1f, 0.7f, 1.0f, 1.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Patrol|Spline")
	FLinearColor PatrolSplineSelectedColor = FLinearColor(1.0f, 0.78f, 0.05f, 1.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Patrol|Spline")
	FLinearColor PatrolSplineTangentColor = FLinearColor(0.25f, 1.0f, 0.35f, 1.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Patrol|Spline", meta = (ClampMin = "1.0"))
	float PatrolSplineVisualizationWidth = 24.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Patrol", meta = (ClampMin = "0.0"))
	float PatrolRadius = 1600.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Patrol", meta = (ClampMin = "0.0"))
	float SearchRadius = 2400.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Patrol", meta = (ClampMin = "0.0"))
	float DefaultWaitTime = 1.25f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Patrol", meta = (ClampMin = "0.0"))
	float DefaultAcceptanceRadius = 120.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Patrol|Route")
	TArray<FZonefallPatrolPoint> PatrolPoints;

	UFUNCTION(BlueprintCallable, Category = "Zonefall AI|Patrol|Spline")
	void ApplyPatrolSplineStyle();

	UFUNCTION(BlueprintPure, Category = "Zonefall AI|Patrol|Spline")
	int32 GetPatrolRoutePointCount() const;

	UFUNCTION(BlueprintCallable, Category = "Zonefall AI|Patrol")
	void ResetPatrol();

	UFUNCTION(BlueprintCallable, Category = "Zonefall AI|Patrol")
	void SetPatrolAnchor(FVector NewAnchor);

	UFUNCTION(BlueprintPure, Category = "Zonefall AI|Patrol")
	FVector GetPatrolAnchor() const;

	UFUNCTION(BlueprintCallable, Category = "Zonefall AI|Patrol")
	bool GetNextPatrolLocation(FVector& OutLocation, float& OutAcceptanceRadius, float& OutWaitTime);

	UFUNCTION(BlueprintPure, Category = "Zonefall AI|Patrol")
	bool HasRoute() const;

protected:
	virtual void OnRegister() override;
	virtual void BeginPlay() override;
#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

private:
	UPROPERTY(Transient)
	FVector PatrolAnchor = FVector::ZeroVector;

	UPROPERTY(Transient)
	int32 CurrentPatrolIndex = INDEX_NONE;

	UPROPERTY(Transient)
	int32 PatrolDirection = 1;

	FVector ResolvePointLocation(int32 PointIndex) const;
	const FZonefallPatrolPoint* GetPointSettings(int32 PointIndex) const;
	int32 SelectNextIndex();
};
