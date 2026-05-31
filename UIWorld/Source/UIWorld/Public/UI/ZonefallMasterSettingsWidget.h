#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/Button.h"
#include "ZonefallMasterSettingsWidget.generated.h"

class UBorder;
class UButton;
class UCheckBox;
class UComboBoxString;
class USlider;
class UTextBlock;
class UVerticalBox;
class UHorizontalBox;
class UScrollBox;
class UWidgetSwitcher;
class UZonefallSettingsDataObject;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnZonefallMasterSettingsBack);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnZonefallKeybindClicked, FName, ActionId);

/** UButton that remembers a control's ActionId and re-broadcasts clicks with it. */
UCLASS()
class UIWORLD_API UZonefallKeybindButton : public UButton
{
	GENERATED_BODY()

public:
	UZonefallKeybindButton();

	UPROPERTY(Transient)
	FName ActionId;

	UPROPERTY(BlueprintAssignable)
	FOnZonefallKeybindClicked OnKeybindClicked;

private:
	UFUNCTION()
	void HandleInternalClicked();
};

/**
 * Fully self-assembling master settings screen (no Blueprint needed).
 * Tabs: GRAPHICS | AUDIO | CONTROLS.
 *  - Graphics & Audio are driven by UZonefallSettingsDataObject.
 *  - Controls rebind the player character's Enhanced Input keys live (click + press a key).
 */
UCLASS(BlueprintType, Blueprintable)
class UIWORLD_API UZonefallMasterSettingsWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UZonefallMasterSettingsWidget(const FObjectInitializer& ObjectInitializer);

	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeConstruct() override;
	virtual FReply NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;
	virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;

	UPROPERTY(BlueprintAssignable, Category = "Zonefall|Settings")
	FOnZonefallMasterSettingsBack OnBackRequested;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zonefall|Settings|Style")
	FLinearColor BackgroundTint = FLinearColor(0.03f, 0.05f, 0.08f, 0.96f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zonefall|Settings|Style")
	FLinearColor AccentColor = FLinearColor(0.27f, 0.85f, 0.96f, 1.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zonefall|Settings|Style")
	FLinearColor PanelTint = FLinearColor(0.07f, 0.11f, 0.16f, 0.96f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zonefall|Settings|Style")
	int32 TitleFontSize = 34;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zonefall|Settings|Style")
	int32 BodyFontSize = 17;

	UFUNCTION(BlueprintCallable, Category = "Zonefall|Settings")
	void RefreshFromSettings();

	UFUNCTION(BlueprintCallable, Category = "Zonefall|Settings")
	void ApplyNow();

	UFUNCTION(BlueprintCallable, Category = "Zonefall|Settings")
	void ResetToDefaults();

private:
	void BuildLayout();
	void BuildGraphicsTab(UScrollBox* Body);
	void BuildAudioTab(UScrollBox* Body);
	void BuildControlsTab(UScrollBox* Body);
	void BuildLanguageTab(UScrollBox* Body);
	void RebuildControlsRows();
	void ShowTab(int32 Index);
	void RefreshSliderValueLabels();
	void SetStatus(const FString& Message, bool bSuccess = true);

	class AZonefallPlayerCharacter* ResolvePlayerCharacter() const;
	UComboBoxString* AddComboRow(UVerticalBox* Parent, const FText& Label, const TArray<FString>& Options, const FString& Selected);
	USlider* AddSliderRow(UVerticalBox* Parent, const FText& Label, float MinValue, float MaxValue, float Value, FName ValueLabelName);
	void AddSectionHeader(UVerticalBox* Parent, const FText& Text);
	UCheckBox* AddCheckRow(UVerticalBox* Parent, const FText& Label, bool bChecked);

	UFUNCTION() void HandleTabGraphics();
	UFUNCTION() void HandleTabAudio();
	UFUNCTION() void HandleTabControls();
	UFUNCTION() void HandleTabLanguage();
	UFUNCTION() void HandleLanguageComboChanged(FString SelectedItem, ESelectInfo::Type SelectionType);
	UFUNCTION() void HandlePresetChanged(FString SelectedItem, ESelectInfo::Type SelectionType);
	UFUNCTION() void HandleApplyClicked();
	UFUNCTION() void HandleResetClicked();
	UFUNCTION() void HandleBackClicked();
	UFUNCTION() void HandleSliderChanged(float Value);
	UFUNCTION() void HandleKeybindClicked(FName ActionId);
	UFUNCTION() void HandleInvertYChanged(bool bIsChecked);
	UFUNCTION() void HandleRestartConfirmed();

	UPROPERTY(Transient) TObjectPtr<UZonefallSettingsDataObject> SettingsObject;

	UPROPERTY(Transient) TObjectPtr<UBorder> RootBorder;
	UPROPERTY(Transient) TObjectPtr<UWidgetSwitcher> BodySwitcher;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> StatusText;
	UPROPERTY(Transient) TObjectPtr<UButton> TabGraphicsButton;
	UPROPERTY(Transient) TObjectPtr<UButton> TabAudioButton;
	UPROPERTY(Transient) TObjectPtr<UButton> TabControlsButton;
	UPROPERTY(Transient) TObjectPtr<UButton> TabLanguageButton;

	// Language tab.
	UPROPERTY(Transient) TObjectPtr<UComboBoxString> LanguageCombo;

	// Graphics controls.
	UPROPERTY(Transient) TObjectPtr<UComboBoxString> PresetCombo;
	UPROPERTY(Transient) TObjectPtr<UComboBoxString> DisplayModeCombo;
	UPROPERTY(Transient) TObjectPtr<UComboBoxString> QualityCombo;
	UPROPERTY(Transient) TObjectPtr<UComboBoxString> ResolutionCombo;
	UPROPERTY(Transient) TObjectPtr<UComboBoxString> VSyncCombo;
	UPROPERTY(Transient) TObjectPtr<UComboBoxString> FPSLimitCombo;
	UPROPERTY(Transient) TObjectPtr<UComboBoxString> LumenCombo;
	UPROPERTY(Transient) TObjectPtr<UComboBoxString> CloudsCombo;
	UPROPERTY(Transient) TObjectPtr<UComboBoxString> ResScaleCombo;
	UPROPERTY(Transient) TObjectPtr<UComboBoxString> DLSSCombo;
	UPROPERTY(Transient) TObjectPtr<UComboBoxString> FrameGenCombo;
	UPROPERTY(Transient) TObjectPtr<UComboBoxString> FSRCombo;
	UPROPERTY(Transient) TObjectPtr<UComboBoxString> FSRFrameGenCombo;
	UPROPERTY(Transient) TObjectPtr<UComboBoxString> DirectXCombo;
	UPROPERTY(Transient) TObjectPtr<UComboBoxString> RayTracingCombo;
	UPROPERTY(Transient) TObjectPtr<USlider> BrightnessSlider;
	UPROPERTY(Transient) TObjectPtr<USlider> FOVSlider;

	// Advanced per-group quality combos (parallel arrays: combo[i] <-> group[i]).
	UPROPERTY(Transient) TArray<TObjectPtr<UComboBoxString>> AdvancedQualityCombos;
	UPROPERTY(Transient) TArray<FName> AdvancedQualityGroups;

	// Look / mouse controls.
	UPROPERTY(Transient) TObjectPtr<USlider> SensitivitySlider;
	UPROPERTY(Transient) TObjectPtr<UCheckBox> InvertYCheck;

	// Audio controls.
	UPROPERTY(Transient) TObjectPtr<USlider> MasterSlider;
	UPROPERTY(Transient) TObjectPtr<USlider> SfxSlider;
	UPROPERTY(Transient) TObjectPtr<USlider> MusicSlider;
	UPROPERTY(Transient) TObjectPtr<USlider> VoiceSlider;

	// Controls tab.
	UPROPERTY(Transient) TObjectPtr<UVerticalBox> ControlsBox;
	UPROPERTY(Transient) TArray<TObjectPtr<UZonefallKeybindButton>> KeybindButtons;

	// Map of slider -> value label for live numeric readouts.
	UPROPERTY(Transient) TMap<TObjectPtr<USlider>, TObjectPtr<UTextBlock>> SliderValueLabels;

	UPROPERTY(Transient) bool bListeningForKey = false;
	UPROPERTY(Transient) FName ListeningActionId;
};
