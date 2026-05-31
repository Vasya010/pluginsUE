#include "World/ZonefallMinimapCapture.h"

#include "Components/SceneCaptureComponent2D.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetRenderingLibrary.h"

AZonefallMinimapCapture::AZonefallMinimapCapture()
{
	PrimaryActorTick.bCanEverTick = true;

	Capture = CreateDefaultSubobject<USceneCaptureComponent2D>(TEXT("MinimapCapture"));
	SetRootComponent(Capture);

	Capture->ProjectionType = ECameraProjectionMode::Orthographic;
	Capture->OrthoWidth = OrthoWidth;
	// FinalColorLDR gives a fully-composited opaque image (sky/ground), ideal for a minimap.
	Capture->CaptureSource = ESceneCaptureSource::SCS_FinalColorLDR;
	Capture->bCaptureEveryFrame = true;
	Capture->bCaptureOnMovement = false;
	Capture->bAlwaysPersistRenderingState = true;
	// Look straight down.
	Capture->SetRelativeRotation(FRotator(-90.0f, 0.0f, 0.0f));
}

void AZonefallMinimapCapture::BeginPlay()
{
	Super::BeginPlay();

	if (!RenderTarget)
	{
		const int32 Size = FMath::Clamp(RenderTargetSize, 128, 2048);
		// KismetRendering creates a properly-initialised RGBA8 target (opaque clear).
		RenderTarget = UKismetRenderingLibrary::CreateRenderTarget2D(
			this, Size, Size, RTF_RGBA8, FLinearColor(0.02f, 0.03f, 0.04f, 1.0f), false);
	}

	if (Capture)
	{
		Capture->TextureTarget = RenderTarget;
		Capture->OrthoWidth = OrthoWidth;
		Capture->bCaptureEveryFrame = true;
	}
}

AActor* AZonefallMinimapCapture::ResolveTrackedActor() const
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

void AZonefallMinimapCapture::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	AActor* Target = ResolveTrackedActor();
	if (!Target)
	{
		return;
	}

	const FVector Loc = Target->GetActorLocation() + FVector(0.0f, 0.0f, CaptureHeight);

	float Yaw = 0.0f;
	if (bRotateWithPlayer)
	{
		// Rotate the map so the player's facing points up.
		Yaw = Target->GetActorRotation().Yaw;
	}
	SetActorLocationAndRotation(Loc, FRotator(-90.0f, Yaw, 0.0f));

	if (Capture)
	{
		Capture->OrthoWidth = OrthoWidth;
	}
}

AZonefallMinimapCapture* AZonefallMinimapCapture::Get(UWorld* World)
{
	if (!World)
	{
		return nullptr;
	}
	return Cast<AZonefallMinimapCapture>(UGameplayStatics::GetActorOfClass(World, AZonefallMinimapCapture::StaticClass()));
}
