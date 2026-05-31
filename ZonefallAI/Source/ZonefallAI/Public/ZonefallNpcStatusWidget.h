#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "ZonefallNpcStatusWidget.generated.h"

class UBorder;
class UProgressBar;
class UTextBlock;
class UVerticalBox;
class UZonefallNpcVitalsComponent;

/**
 * Self-assembling status bar that floats above an NPC (shown via a WidgetComponent):
 *  - Health bar (only for hostiles / bandits)
 *  - Detection meter that fills as the NPC spots the hero, turning red + "!" when alerted.
 * Built entirely in C++. It also self-binds to the owning actor's vitals component, so it
 * works even when the WidgetComponent creates the widget on its own.
 */
UCLASS(BlueprintType, Blueprintable)
class ZONEFALLAI_API UZonefallNpcStatusWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UZonefallNpcStatusWidget(const FObjectInitializer& ObjectInitializer);

	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeConstruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	UFUNCTION(BlueprintCallable, Category = "Zonefall AI|NPC")
	void BindToVitals(UZonefallNpcVitalsComponent* InVitals);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zonefall AI|NPC|Style")
	FLinearColor HealthColor = FLinearColor(0.85f, 0.22f, 0.18f, 1.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zonefall AI|NPC|Style")
	FLinearColor DetectionColor = FLinearColor(0.96f, 0.82f, 0.28f, 1.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zonefall AI|NPC|Style")
	FLinearColor AlertColor = FLinearColor(0.95f, 0.20f, 0.16f, 1.0f);

private:
	void EnsureVitalsBound();
	void BuildLayout();

	UPROPERTY(Transient) TObjectPtr<UZonefallNpcVitalsComponent> Vitals;

	UPROPERTY(Transient) TObjectPtr<UBorder> HealthRow;
	UPROPERTY(Transient) TObjectPtr<UProgressBar> HealthBar;
	UPROPERTY(Transient) TObjectPtr<UBorder> DetectRow;
	UPROPERTY(Transient) TObjectPtr<UProgressBar> DetectBar;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> AlertText;

	float PulseTime = 0.0f;
};
