#include "ZonefallAIPatrolComponent.h"

#include "GameFramework/Actor.h"

UZonefallAIPatrolComponent::UZonefallAIPatrolComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetDrawDebug(true);
}

void UZonefallAIPatrolComponent::OnRegister()
{
	Super::OnRegister();
	ApplyPatrolSplineStyle();
}

void UZonefallAIPatrolComponent::BeginPlay()
{
	Super::BeginPlay();
	ResetPatrol();
}

#if WITH_EDITOR
void UZonefallAIPatrolComponent::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);
	ApplyPatrolSplineStyle();
}
#endif

void UZonefallAIPatrolComponent::ApplyPatrolSplineStyle()
{
	SetUnselectedSplineSegmentColor(PatrolSplineColor);
	SetSelectedSplineSegmentColor(PatrolSplineSelectedColor);
	SetTangentColor(PatrolSplineTangentColor);

#if WITH_EDITORONLY_DATA
	bShouldVisualizeScale = true;
	ScaleVisualizationWidth = PatrolSplineVisualizationWidth;
#endif

	if (bUseSplineRoute && bCloseSplineWhenLooping)
	{
		SetClosedLoop(PatrolMode == EZonefallPatrolMode::Loop, true);
	}
}

int32 UZonefallAIPatrolComponent::GetPatrolRoutePointCount() const
{
	if (bUseSplineRoute && GetNumberOfSplinePoints() > 0)
	{
		return GetNumberOfSplinePoints();
	}

	return PatrolPoints.Num();
}

void UZonefallAIPatrolComponent::ResetPatrol()
{
	PatrolAnchor = bStartAtOwnerLocation ? GetComponentLocation() : PatrolAnchor;
	CurrentPatrolIndex = INDEX_NONE;
	PatrolDirection = 1;
	ApplyPatrolSplineStyle();
}

void UZonefallAIPatrolComponent::SetPatrolAnchor(FVector NewAnchor)
{
	PatrolAnchor = NewAnchor;
}

FVector UZonefallAIPatrolComponent::GetPatrolAnchor() const
{
	if (!PatrolAnchor.IsNearlyZero())
	{
		return PatrolAnchor;
	}

	return GetComponentLocation();
}

bool UZonefallAIPatrolComponent::GetNextPatrolLocation(FVector& OutLocation, float& OutAcceptanceRadius, float& OutWaitTime)
{
	if (!HasRoute())
	{
		return false;
	}

	const int32 NextIndex = SelectNextIndex();
	if (NextIndex == INDEX_NONE)
	{
		return false;
	}

	CurrentPatrolIndex = NextIndex;
	const FZonefallPatrolPoint* PointSettings = GetPointSettings(CurrentPatrolIndex);
	OutLocation = ResolvePointLocation(CurrentPatrolIndex);
	OutAcceptanceRadius = PointSettings && PointSettings->AcceptanceRadius > 0.0f ? PointSettings->AcceptanceRadius : DefaultAcceptanceRadius;
	OutWaitTime = PointSettings && PointSettings->WaitTime > 0.0f ? PointSettings->WaitTime : DefaultWaitTime;
	return true;
}

bool UZonefallAIPatrolComponent::HasRoute() const
{
	return PatrolMode != EZonefallPatrolMode::Radius && GetPatrolRoutePointCount() > 0;
}

FVector UZonefallAIPatrolComponent::ResolvePointLocation(int32 PointIndex) const
{
	if (bUseSplineRoute && GetNumberOfSplinePoints() > 0)
	{
		return GetLocationAtSplinePoint(PointIndex, ESplineCoordinateSpace::World);
	}

	const FZonefallPatrolPoint* Point = GetPointSettings(PointIndex);
	if (Point && Point->PointActor)
	{
		return Point->PointActor->GetActorLocation();
	}

	return Point ? GetPatrolAnchor() + Point->LocalOffset : GetPatrolAnchor();
}

const FZonefallPatrolPoint* UZonefallAIPatrolComponent::GetPointSettings(int32 PointIndex) const
{
	return PatrolPoints.IsValidIndex(PointIndex) ? &PatrolPoints[PointIndex] : nullptr;
}

int32 UZonefallAIPatrolComponent::SelectNextIndex()
{
	const int32 PointCount = GetPatrolRoutePointCount();
	if (PointCount <= 0)
	{
		return INDEX_NONE;
	}

	if (PointCount <= 1)
	{
		return 0;
	}

	switch (PatrolMode)
	{
	case EZonefallPatrolMode::RandomPoint:
		return FMath::RandRange(0, PointCount - 1);

	case EZonefallPatrolMode::PingPong:
		if (CurrentPatrolIndex == INDEX_NONE)
		{
			return 0;
		}
		if (CurrentPatrolIndex + PatrolDirection >= PointCount || CurrentPatrolIndex + PatrolDirection < 0)
		{
			PatrolDirection *= -1;
		}
		return FMath::Clamp(CurrentPatrolIndex + PatrolDirection, 0, PointCount - 1);

	case EZonefallPatrolMode::Loop:
	default:
		return CurrentPatrolIndex == INDEX_NONE ? 0 : (CurrentPatrolIndex + 1) % PointCount;
	}
}
