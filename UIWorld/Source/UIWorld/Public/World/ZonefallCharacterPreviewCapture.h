#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ZonefallCharacterPreviewCapture.generated.h"

class USceneCaptureComponent2D;
class UTextureRenderTarget2D;

/**
 * Full-body preview render of the local player for the character-creator widget. A perspective
 * SceneCapture2D orbits the player (controllable yaw) with a show-only list so only the player is
 * drawn over a solid backdrop. The character spawns one when it opens the creator UI.
 */
UCLASS(BlueprintType, Blueprintable)
class UIWORLD_API AZonefallCharacterPreviewCapture : public AActor
{
	GENERATED_BODY()

public:
	AZonefallCharacterPreviewCapture();

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Preview")
	TObjectPtr<USceneCaptureComponent2D> Capture;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Preview")
	float CameraDistance = 320.0f;

	// Height (relative to the actor origin) the orbit looks at — roughly mid-torso.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Preview")
	float FocusHeight = 90.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Preview", meta = (ClampMin = "10.0", ClampMax = "90.0"))
	float FieldOfView = 38.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Preview", meta = (ClampMin = "256", ClampMax = "2048"))
	int32 RenderTargetSize = 768;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Preview")
	FLinearColor BackgroundColor = FLinearColor(0.03f, 0.045f, 0.07f, 1.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Preview")
	TObjectPtr<AActor> TrackedActor;

	UFUNCTION(BlueprintPure, Category = "Preview")
	UTextureRenderTarget2D* GetRenderTarget() const { return RenderTarget; }

	UFUNCTION(BlueprintCallable, Category = "Preview")
	void AddYaw(float DeltaDegrees) { YawOffset = FMath::UnwindDegrees(YawOffset + DeltaDegrees); }

	UFUNCTION(BlueprintCallable, Category = "Preview")
	void SetTrackedActor(AActor* InActor);

	UFUNCTION(BlueprintCallable, Category = "Preview")
	void SetYaw(float NewYaw) { YawOffset = FMath::UnwindDegrees(NewYaw); }

	UFUNCTION(BlueprintCallable, Category = "Preview")
	void RefreshShowOnlyList(AActor* Target);

	static AZonefallCharacterPreviewCapture* Get(UWorld* World);

private:
	AActor* ResolveTrackedActor() const;

	UPROPERTY(Transient) TObjectPtr<UTextureRenderTarget2D> RenderTarget;
	UPROPERTY(Transient) TObjectPtr<AActor> LastShownActor;

	float YawOffset = 180.0f; // start facing the character's front
};
