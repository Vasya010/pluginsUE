#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ZonefallMinimapCapture.generated.h"

class USceneCaptureComponent2D;
class UTextureRenderTarget2D;

/**
 * Level object that drives the round minimap. A top-down orthographic SceneCapture2D
 * renders into a runtime render target which the HUD displays. It follows the local
 * player every frame. Place one in the level, or the player character auto-spawns one.
 */
UCLASS(BlueprintType, Blueprintable)
class UIWORLD_API AZonefallMinimapCapture : public AActor
{
	GENERATED_BODY()

public:
	AZonefallMinimapCapture();

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Minimap")
	TObjectPtr<USceneCaptureComponent2D> Capture;

	// Height above the tracked actor the camera sits at.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Minimap")
	float CaptureHeight = 8000.0f;

	// World units visible across the minimap (smaller = more zoomed in / closer to the player).
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Minimap", meta = (ClampMin = "500.0"))
	float OrthoWidth = 3600.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Minimap", meta = (ClampMin = "128", ClampMax = "2048"))
	int32 RenderTargetSize = 512;

	// If true the map rotates with the player's facing; otherwise it stays north-up.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Minimap")
	bool bRotateWithPlayer = false;

	// Optional explicit actor to follow; defaults to the local player pawn.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Minimap")
	TObjectPtr<AActor> TrackedActor;

	UFUNCTION(BlueprintPure, Category = "Minimap")
	UTextureRenderTarget2D* GetRenderTarget() const { return RenderTarget; }

	UFUNCTION(BlueprintPure, Category = "Minimap")
	float GetOrthoWidth() const { return OrthoWidth; }

	/** First active minimap capture in the world (or null). */
	static AZonefallMinimapCapture* Get(UWorld* World);

private:
	AActor* ResolveTrackedActor() const;

	UPROPERTY(Transient)
	TObjectPtr<UTextureRenderTarget2D> RenderTarget;
};
