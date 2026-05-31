#include "ZonefallPatrolPath.h"

#include "Components/SplineComponent.h"
#if WITH_EDITORONLY_DATA
#include "Components/BillboardComponent.h"
#endif

AZonefallPatrolPath::AZonefallPatrolPath()
{
	PrimaryActorTick.bCanEverTick = false;

	Spline = CreateDefaultSubobject<USplineComponent>(TEXT("PatrolSpline"));
	SetRootComponent(Spline);
	Spline->SetClosedLoop(true);

	// A small default square route so it's usable the moment it's placed; drag points to edit.
	Spline->ClearSplinePoints(false);
	Spline->AddSplinePoint(FVector(0.0f, 0.0f, 0.0f), ESplineCoordinateSpace::Local, false);
	Spline->AddSplinePoint(FVector(600.0f, 0.0f, 0.0f), ESplineCoordinateSpace::Local, false);
	Spline->AddSplinePoint(FVector(600.0f, 600.0f, 0.0f), ESplineCoordinateSpace::Local, false);
	Spline->AddSplinePoint(FVector(0.0f, 600.0f, 0.0f), ESplineCoordinateSpace::Local, false);
	Spline->UpdateSpline();

#if WITH_EDITORONLY_DATA
	EditorIcon = CreateDefaultSubobject<UBillboardComponent>(TEXT("EditorIcon"));
	if (EditorIcon)
	{
		EditorIcon->SetupAttachment(Spline);
		EditorIcon->bIsScreenSizeScaled = true;
	}
#endif
}
