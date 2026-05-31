#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "ZonefallDeadEyeWidget.generated.h"

class UBorder;
class UProgressBar;
class UTextBlock;
class AZonefallPlayerCharacter;

/**
 * Self-assembling Dead Eye meter HUD. Reads the bound character's Dead Eye fraction every
 * tick and draws a draining bar (RDR2-style). No Blueprint required.
 */
UCLASS(BlueprintType, Blueprintable)
class UIWORLD_API UZonefallDeadEyeWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UZonefallDeadEyeWidget(const FObjectInitializer& ObjectInitializer);

	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	UFUNCTION(BlueprintCallable, Category = "Zonefall|DeadEye")
	void BindToCharacter(AZonefallPlayerCharacter* InCharacter);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zonefall|DeadEye|Style")
	FLinearColor FillColor = FLinearColor(0.92f, 0.18f, 0.18f, 1.0f);

private:
	void BuildLayout();

	UPROPERTY(Transient) TObjectPtr<AZonefallPlayerCharacter> Character;
	UPROPERTY(Transient) TObjectPtr<UProgressBar> Meter;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> Label;
};
