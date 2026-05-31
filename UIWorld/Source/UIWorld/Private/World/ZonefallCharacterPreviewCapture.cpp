#include "World/ZonefallCharacterPreviewCapture.h"

#include "Components/SceneCaptureComponent2D.h"
#include "Components/PrimitiveComponent.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetRenderingLibrary.h"

AZonefallCharacterPreviewCapture::AZonefallCharacterPreviewCapture()
{
	PrimaryActorTick.bCanEverTick = true;

	Capture = CreateDefaultSubobject<USceneCaptureComponent2D>(TEXT("PreviewCapture"));
	SetRootComponent(Capture);

	Capture->ProjectionType = ECameraProjectionMode::Perspective;
	Capture->FOVAngle = FieldOfView;
	Capture->CaptureSource = ESceneCaptureSource::SCS_FinalColorLDR;
	Capture->bCaptureEveryFrame = true;
	Capture->bCaptureOnMovement = false;
	Capture->bAlwaysPersistRenderingState = true;
	Capture->PrimitiveRenderMode = ESceneCapturePrimitiveRenderMode::PRM_UseShowOnlyList;
}

void AZonefallCharacterPreviewCapture::SetTrackedActor(AActor* InActor)
{
	TrackedActor = InActor;
	LastShownActor = nullptr;
}

void AZonefallCharacterPreviewCapture::RefreshShowOnlyList(AActor* Target)
{
	if (!Capture || !Target)
	{
		return;
	}

	Capture->ClearShowOnlyComponents();
	Capture->ShowOnlyActors.Reset();
	Capture->ShowOnlyActors.Add(Target);

	TInlineComponentArray<UPrimitiveComponent*> Primitives;
	Target->GetComponents(Primitives);
	for (UPrimitiveComponent* Prim : Primitives)
	{
		if (Prim && Prim->IsVisible())
		{
			Capture->ShowOnlyComponent(Prim);
		}
	}
}

void AZonefallCharacterPreviewCapture::BeginPlay()
{
	Super::BeginPlay();

	if (!RenderTarget)
	{
		const int32 Size = FMath::Clamp(RenderTargetSize, 256, 2048);
		RenderTarget = UKismetRenderingLibrary::CreateRenderTarget2D(this, Size, Size, RTF_RGBA8, BackgroundColor, false);
	}

	if (Capture)
	{
		Capture->TextureTarget = RenderTarget;
		Capture->FOVAngle = FieldOfView;
		Capture->bCaptureEveryFrame = true;
	}
}

AActor* AZonefallCharacterPreviewCapture::ResolveTrackedActor() const
{
	if (TrackedActor)
	{
		return TrackedActor;
	}
	if (const UWorld* World = GetWorld())
	{
		if (APlayerController* PC = World->GetFirstPlayerController())
		{
			return PC->GetPawn();
		}
	}
	return nullptr;
}

void AZonefallCharacterPreviewCapture::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	AActor* Target = ResolveTrackedActor();
	if (!Target || !Capture)
	{
		return;
	}

	if (Target != LastShownActor)
	{
		RefreshShowOnlyList(Target);
		LastShownActor = Target;
	}

	const FVector Focus = Target->GetActorLocation() + FVector(0.0f, 0.0f, FocusHeight);
	// Orbit around the focus point by YawOffset (0 = behind, 180 = front of the character).
	const FRotator OrbitRot(0.0f, Target->GetActorRotation().Yaw + YawOffset, 0.0f);
	const FVector Offset = OrbitRot.RotateVector(FVector(CameraDistance, 0.0f, 0.0f));
	const FVector CameraLocation = Focus + Offset;
	const FRotator CameraRotation = (Focus - CameraLocation).Rotation();

	SetActorLocationAndRotation(CameraLocation, CameraRotation);
	Capture->FOVAngle = FieldOfView;
}

AZonefallCharacterPreviewCapture* AZonefallCharacterPreviewCapture::Get(UWorld* World)
{
	if (!World)
	{
		return nullptr;
	}
	return Cast<AZonefallCharacterPreviewCapture>(
		UGameplayStatics::GetActorOfClass(World, AZonefallCharacterPreviewCapture::StaticClass()));
}
