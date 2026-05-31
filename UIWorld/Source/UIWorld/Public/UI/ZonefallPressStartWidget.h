#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "ZonefallPressStartWidget.generated.h"

class UBorder;
class UImage;
class UTextBlock;
class UVerticalBox;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FZonefallPressStartContinue);

/**
 * "Press any key to continue" screen shown over the live game scene AFTER the boot logos
 * and BEFORE the main menu (AAA-style). Transparent so the world shows through, with an
 * animated pulsing prompt. F / Enter / Space / click / gamepad confirm proceeds.
 */
UCLASS(BlueprintType, Blueprintable)
class UIWORLD_API UZonefallPressStartWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UZonefallPressStartWidget(const FObjectInitializer& ObjectInitializer);

	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeConstruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
	virtual FReply NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;
	virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;

	UPROPERTY(BlueprintAssignable, Category = "Zonefall|PressStart")
	FZonefallPressStartContinue OnContinue;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zonefall|PressStart")
	FText TitleText = FText::FromString(TEXT("ZONEFALL PROTOCOL"));

	// Single clean prompt line, shown centred. Any key / click still continues.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zonefall|PressStart")
	FText PromptText = FText::FromString(TEXT("PRESS ANY KEY TO CONTINUE"));

	// Warm cinematic accent (RDR2/GTA-style), not neon.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zonefall|PressStart")
	FLinearColor AccentColor = FLinearColor(0.86f, 0.74f, 0.48f, 1.0f);

	// Warm cream text colour for the title/prompt.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zonefall|PressStart")
	FLinearColor TextColor = FLinearColor(0.94f, 0.91f, 0.84f, 1.0f);

	// Cinematic dark backdrop (the game scene shows faintly behind it).
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zonefall|PressStart|Style")
	FLinearColor BackdropTint = FLinearColor(0.015f, 0.014f, 0.012f, 0.90f);

	// Cinematic letterbox bars (top/bottom) like RDR2/GTA loading screens.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zonefall|PressStart|Style", meta = (ClampMin = "0.0", ClampMax = "200.0"))
	float LetterboxHeight = 80.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zonefall|PressStart|Style")
	bool bEnableBackdropAnimation = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zonefall|PressStart|Audio")
	TSoftObjectPtr<class USoundBase> ContinueSound;

private:
	void BuildLayout();
	void Confirm();

	UPROPERTY(Transient) TObjectPtr<UTextBlock> TitleLabel;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> PromptLabel;
	UPROPERTY(Transient) TObjectPtr<UImage> AccentTop;
	UPROPERTY(Transient) TObjectPtr<UImage> AccentBottom;
	UPROPERTY(Transient) TObjectPtr<UImage> TitleUnderline;

	float AnimTime = 0.0f;
	bool bConfirmed = false;
};
