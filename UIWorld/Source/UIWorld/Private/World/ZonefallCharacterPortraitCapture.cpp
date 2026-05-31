#include "World/ZonefallCharacterPortraitCapture.h"

#include "Components/SceneCaptureComponent2D.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetRenderingLibrary.h"

AZonefallCharacterPortraitCapture::AZonefallCharacterPortraitCapture()
{
	PrimaryActorTick.bCanEverTick = true;

	Capture = CreateDefaultSubobject<USceneCaptureComponent2D>(TEXT("PortraitCapture"));
	SetRootComponent(Capture);

	Capture->ProjectionType = ECameraProjectionMode::Perspective;
	Capture->FOVAngle = FieldOfView;
	// Fully-composited opaque image so the character is lit by the level the same way it looks in-game.
	Capture->CaptureSource = ESceneCaptureSource::SCS_FinalColorLDR;
	Capture->bCaptureEveryFrame = true;
	Capture->bCaptureOnMovement = false;
	Capture->bAlwaysPersistRenderingState = true;
	// Only draw the player (and anything attached to them) — the rest is the solid card background.
	Capture->PrimitiveRenderMode = ESceneCapturePrimitiveRenderMode::PRM_UseShowOnlyList;
}

void AZonefallCharacterPortraitCapture::BeginPlay()
{
	Super::BeginPlay();

	if (!RenderTarget)
	{
		const int32 Size = FMath::Clamp(RenderTargetSize, 128, 1024);
		RenderTarget = UKismetRenderingLibrary::CreateRenderTarget2D(this, Size, Size, RTF_RGBA8, BackgroundColor, false);
	}

	if (Capture)
	{
		Capture->TextureTarget = RenderTarget;
		Capture->FOVAngle = FieldOfView;
		Capture->bCaptureEveryFrame = true;
	}
}

AActor* AZonefallCharacterPortraitCapture::ResolveTrackedActor() const
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

void AZonefallCharacterPortraitCapture::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	AActor* Target = ResolveTrackedActor();
	if (!Target || !Capture)
	{
		return;
	}

	// Keep the show-only list pointed at the current pawn (handles respawns / repossession).
	if (Target != LastShownActor)
	{
		Capture->ClearShowOnlyComponents(); // clears the per-component show-only list
		Capture->ShowOnlyActors.Reset();
		Capture->ShowOnlyActors.Add(Target);
		LastShownActor = Target;
	}

	// Frame the head/chest from the front: stand in front of the character, facing back at them.
	const FVector Origin = Target->GetActorLocation();
	const FVector Focus = Origin + FVector(0.0f, 0.0f, FocusHeight);
	const FVector Forward = Target->GetActorForwardVector();
	const FVector CameraLocation = Focus + Forward * CameraDistance + FVector(0.0f, 0.0f, 8.0f);
	const FRotator CameraRotation = (Focus - CameraLocation).Rotation();

	SetActorLocationAndRotation(CameraLocation, CameraRotation);
	Capture->FOVAngle = FieldOfView;
}

AZonefallCharacterPortraitCapture* AZonefallCharacterPortraitCapture::Get(UWorld* World)
{
	if (!World)
	{
		return nullptr;
	}
	return Cast<AZonefallCharacterPortraitCapture>(
		UGameplayStatics::GetActorOfClass(World, AZonefallCharacterPortraitCapture::StaticClass()));
}
