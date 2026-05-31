#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/Button.h"
#include "Character/ZonefallCharacterAppearance.h"
#include "ZonefallCharacterCreatorWidget.generated.h"

class UBorder;
class UEditableTextBox;
class UImage;
class UPanelWidget;
class USlider;
class UTextBlock;
class UVerticalBox;
class AZonefallPlayerCharacter;
class AZonefallCharacterPreviewCapture;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnZonefallCreatorAction, int32, ActionId, int32, Param);

/** A UButton that carries an action id + parameter and re-broadcasts clicks with them. */
UCLASS()
class UIWORLD_API UZonefallCreatorButton : public UButton
{
	GENERATED_BODY()

public:
	UZonefallCreatorButton();

	UPROPERTY(Transient) int32 ActionId = 0;
	UPROPERTY(Transient) int32 Param = 0;

	UPROPERTY(BlueprintAssignable) FOnZonefallCreatorAction OnCreatorAction;

private:
	UFUNCTION()
	void HandleClicked();
};

/**
 * Self-assembling, RDR2/GTA-style character creator (no Blueprint required). Left side shows a
 * live full-body preview of the player; the right side has identity (name), body/face/hair
 * selectors, height/build sliders and colour palettes for skin/hair/primary/secondary. Every
 * change is applied to the live character instantly. CREATE & START commits the look and begins
 * the game; BACK returns to the main menu.
 */
UCLASS(BlueprintType, Blueprintable)
class UIWORLD_API UZonefallCharacterCreatorWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UZonefallCharacterCreatorWidget(const FObjectInitializer& ObjectInitializer);

	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	UFUNCTION(BlueprintCallable, Category = "Zonefall|Creator")
	void SetupForCharacter(AZonefallPlayerCharacter* InCharacter);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zonefall|Creator|Style")
	FLinearColor AccentColor = FLinearColor(0.92f, 0.78f, 0.42f, 1.0f);

private:
	void BuildLayout();
	void ApplyToPreview();
	void RefreshDynamicLabels();
	void RefreshPreviewBrush();
	void Randomize();

	// Builders.
	UZonefallCreatorButton* MakeButton(FName Name, const FText& Label, int32 ActionId, int32 Param, int32 FontSize);
	void AddSection(UVerticalBox* Col, const FText& Title);
	UTextBlock* AddSelectorRow(UVerticalBox* Col, const FText& Label, int32 PrevAction, int32 NextAction);
	void AddSwatchRow(UVerticalBox* Col, const FText& Label, int32 SwatchAction, const TArray<FLinearColor>& Palette);
	USlider* AddSliderRow(UVerticalBox* Col, const FText& Label, float MinV, float MaxV, float Value);

	UFUNCTION() void HandleAction(int32 ActionId, int32 Param);
	UFUNCTION() void HandleNameChanged(const FText& Text);
	UFUNCTION() void HandleHeightChanged(float Value);
	UFUNCTION() void HandleBuildChanged(float Value);

	UPROPERTY(Transient) TObjectPtr<AZonefallPlayerCharacter> Character;
	UPROPERTY(Transient) TObjectPtr<AZonefallCharacterPreviewCapture> PreviewCapture;

	UPROPERTY(Transient) TObjectPtr<UImage> PreviewImage;
	UPROPERTY(Transient) TObjectPtr<UEditableTextBox> NameBox;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> BodyValueText;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> FaceValueText;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> HairValueText;
	UPROPERTY(Transient) TObjectPtr<USlider> HeightSlider;
	UPROPERTY(Transient) TObjectPtr<USlider> BuildSlider;

	TArray<FLinearColor> SkinPalette;
	TArray<FLinearColor> HairPalette;
	TArray<FLinearColor> ClothPalette;

	FZonefallCharacterAppearance Working;

	int32 BodyOptionCount = 1;
	static constexpr int32 FaceCount = 6;
	static constexpr int32 HairCount = 8;

	bool bPreviewBrushSet = false;
};
