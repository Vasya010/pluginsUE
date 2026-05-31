#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ZonefallCharacterPortraitCapture.generated.h"

class USceneCaptureComponent2D;
class UTextureRenderTarget2D;

/**
 * Renders a live bust portrait of the local player into a render target for the HUD vitals card
 * (the "he draws himself" panel). A perspective SceneCapture2D sits in front of the player's
 * head looking back at them, with a show-only list so only the player (and the weapon attached
 * to them) is drawn over a solid card-coloured background. The player auto-spawns one.
 */
UCLASS(BlueprintType, Blueprintable)
class UIWORLD_API AZonefallCharacterPortraitCapture : public AActor
{
	GENERATED_BODY()

public:
	AZonefallCharacterPortraitCapture();

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Portrait")
	TObjectPtr<USceneCaptureComponent2D> Capture;

	// Distance in front of the head the camera sits at.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Portrait")
	float CameraDistance = 165.0f;

	// Height (relative to the actor origin) the portrait frames — roughly the head/chest.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Portrait")
	float FocusHeight = 60.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Portrait", meta = (ClampMin = "10.0", ClampMax = "90.0"))
	float FieldOfView = 36.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Portrait", meta = (ClampMin = "128", ClampMax = "1024"))
	int32 RenderTargetSize = 384;

	// Solid background colour behind the cut-out character (matches the vitals card glass).
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Portrait")
	FLinearColor BackgroundColor = FLinearColor(0.02f, 0.04f, 0.07f, 1.0f);

	// Optional explicit actor to portray; defaults to the local player pawn.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Portrait")
	TObjectPtr<AActor> TrackedActor;

	UFUNCTION(BlueprintPure, Category = "Portrait")
	UTextureRenderTarget2D* GetRenderTarget() const { return RenderTarget; }

	/** First active portrait capture in the world (or null). */
	static AZonefallCharacterPortraitCapture* Get(UWorld* World);

private:
	AActor* ResolveTrackedActor() const;

	UPROPERTY(Transient)
	TObjectPtr<UTextureRenderTarget2D> RenderTarget;

	UPROPERTY(Transient)
	TObjectPtr<AActor> LastShownActor;
};
