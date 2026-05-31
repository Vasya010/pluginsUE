#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "ZonefallSettingsMenuWidget.generated.h"

class UBorder;
class UButton;
class UComboBoxString;
class UProgressBar;
class UTextBlock;
class UVerticalBox;
class UZonefallSettingsDataObject;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnSettingsBackRequested);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnSettingsApplied);

UCLASS(BlueprintType, Blueprintable)
class ZONEFALL_API UZonefallSettingsMenuWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UZonefallSettingsMenuWidget(const FObjectInitializer& ObjectInitializer);

	virtual void NativePreConstruct() override;
	virtual void NativeConstruct() override;

	UPROPERTY(BlueprintAssignable, Category = "Zonefall|UI|Settings")
	FOnSettingsBackRequested OnBackRequested;

	UPROPERTY(BlueprintAssignable, Category = "Zonefall|UI|Settings")
	FOnSettingsApplied OnSettingsApplied;

	UFUNCTION(BlueprintCallable, Category = "Zonefall|UI|Settings")
	void RefreshFromSettings();

	UFUNCTION(BlueprintCallable, Category = "Zonefall|UI|Settings")
	void ApplySettingsNow();

	UFUNCTION(BlueprintCallable, Category = "Zonefall|UI|Settings")
	void ResetToDefaults();

protected:
	UPROPERTY(Transient, BlueprintReadOnly, Category = "Zonefall|UI|Settings")
	TObjectPtr<UBorder> RootBorder;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "Zonefall|UI|Settings")
	TObjectPtr<UVerticalBox> RootBox;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "Zonefall|UI|Settings")
	TObjectPtr<UTextBlock> TitleText;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "Zonefall|UI|Settings")
	TObjectPtr<UComboBoxString> DisplayModeComboBox;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "Zonefall|UI|Settings")
	TObjectPtr<UTextBlock> DisplayModeText;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "Zonefall|UI|Settings")
	TObjectPtr<UComboBoxString> GraphicsPresetComboBox;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "Zonefall|UI|Settings")
	TObjectPtr<UTextBlock> GraphicsPresetText;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "Zonefall|UI|Settings")
	TObjectPtr<UComboBoxString> OverallQualityComboBox;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "Zonefall|UI|Settings")
	TObjectPtr<UTextBlock> OverallQualityText;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "Zonefall|UI|Settings")
	TObjectPtr<UComboBoxString> ResolutionScaleComboBox;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "Zonefall|UI|Settings")
	TObjectPtr<UTextBlock> ResolutionScaleText;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "Zonefall|UI|Settings")
	TObjectPtr<UComboBoxString> ScreenResolutionComboBox;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "Zonefall|UI|Settings")
	TObjectPtr<UTextBlock> ScreenResolutionText;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "Zonefall|UI|Settings")
	TObjectPtr<UComboBoxString> VSyncComboBox;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "Zonefall|UI|Settings")
	TObjectPtr<UTextBlock> VSyncText;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "Zonefall|UI|Settings")
	TObjectPtr<UComboBoxString> FPSLimitComboBox;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "Zonefall|UI|Settings")
	TObjectPtr<UTextBlock> FPSLimitText;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "Zonefall|UI|Settings")
	TObjectPtr<UComboBoxString> LumenComboBox;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "Zonefall|UI|Settings")
	TObjectPtr<UTextBlock> LumenText;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "Zonefall|UI|Settings")
	TObjectPtr<UComboBoxString> DLSSComboBox;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "Zonefall|UI|Settings")
	TObjectPtr<UTextBlock> DLSSText;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "Zonefall|UI|Settings")
	TObjectPtr<UComboBoxString> FrameGenerationComboBox;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "Zonefall|UI|Settings")
	TObjectPtr<UTextBlock> FrameGenerationText;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "Zonefall|UI|Settings")
	TObjectPtr<UComboBoxString> FSRComboBox;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "Zonefall|UI|Settings")
	TObjectPtr<UTextBlock> FSRText;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "Zonefall|UI|Settings")
	TObjectPtr<UComboBoxString> FSRFrameGenerationComboBox;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "Zonefall|UI|Settings")
	TObjectPtr<UTextBlock> FSRFrameGenerationText;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "Zonefall|UI|Settings")
	TObjectPtr<UProgressBar> MemoryUsageProgressBar;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "Zonefall|UI|Settings")
	TObjectPtr<UTextBlock> MemoryUsageText;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "Zonefall|UI|Settings")
	TObjectPtr<UTextBlock> ApplyStatusText;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "Zonefall|UI|Settings")
	TObjectPtr<UButton> ApplyButton;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "Zonefall|UI|Settings")
	TObjectPtr<UButton> ResetButton;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "Zonefall|UI|Settings")
	TObjectPtr<UButton> BackButton;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "Zonefall|UI|Settings")
	TObjectPtr<UZonefallSettingsDataObject> SettingsObject;

	// Assign widget names from your UMG in Details to auto-bind everything.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zonefall|UI|Settings|Binding")
	FName DisplayModeComboBoxName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zonefall|UI|Settings|Binding")
	FName GraphicsPresetComboBoxName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zonefall|UI|Settings|Binding")
	FName OverallQualityComboBoxName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zonefall|UI|Settings|Binding")
	FName ResolutionScaleComboBoxName;

	// Your Fullscreen combo can be used here for screen resolutions (example: "Fullscreen").
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zonefall|UI|Settings|Binding")
	FName ScreenResolutionComboBoxName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zonefall|UI|Settings|Binding")
	FName VSyncComboBoxName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zonefall|UI|Settings|Binding")
	FName FPSLimitComboBoxName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zonefall|UI|Settings|Binding")
	FName LumenComboBoxName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zonefall|UI|Settings|Binding")
	FName DLSSComboBoxName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zonefall|UI|Settings|Binding")
	FName FrameGenerationComboBoxName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zonefall|UI|Settings|Binding")
	FName FSRComboBoxName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zonefall|UI|Settings|Binding")
	FName FSRFrameGenerationComboBoxName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zonefall|UI|Settings|Binding")
	FName ApplyButtonName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zonefall|UI|Settings|Binding")
	FName ResetButtonName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zonefall|UI|Settings|Binding")
	FName BackButtonName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zonefall|UI|Settings|Binding")
	FName MemoryUsageProgressBarName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zonefall|UI|Settings|Binding")
	FName MemoryUsageTextName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zonefall|UI|Settings|Binding")
	FName ApplyStatusTextName;

	// If enabled, screen mode/resolution are applied immediately when selected.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zonefall|UI|Settings|Behavior")
	bool bAutoApplyDisplayModeAndResolution;

private:
	void BuildLayoutIfNeeded();
	void ResolveNamedWidgets();
	void PopulateComboOptionsIfNeeded();
	void PopulateScreenResolutionsIfNeeded();
	void BindEvents();
	void ApplyModernVisualTheme();
	void ApplyButtonStyle(UButton* Button) const;
	void ApplyComboBoxStyle(UComboBoxString* ComboBox) const;
	void ApplyLabelStyle(UTextBlock* LabelTextBlock, int32 FontSize = 21) const;
	UComboBoxString* CreateOptionCombo(const FName Name, TObjectPtr<UTextBlock>& OutTextBlock);
	void EnsureComboHasOption(UComboBoxString* ComboBox, const FString& Option) const;
	void UpdateFeatureAvailabilityUI();
	void UpdatePerformanceEstimateUI();
	float CalculateEstimatedMemoryUsageMB() const;
	void UpdateTexts();
	void MarkSettingsDirty();
	void SetApplyStatusMessage(const FText& InMessage, const FLinearColor& InColor);
	void UpdateApplyButtonState();

	UFUNCTION()
	void HandleDisplayModeSelectionChanged(FString SelectedItem, ESelectInfo::Type SelectionType);

	UFUNCTION()
	void HandleGraphicsPresetSelectionChanged(FString SelectedItem, ESelectInfo::Type SelectionType);

	UFUNCTION()
	void HandleOverallQualitySelectionChanged(FString SelectedItem, ESelectInfo::Type SelectionType);

	UFUNCTION()
	void HandleResolutionScaleSelectionChanged(FString SelectedItem, ESelectInfo::Type SelectionType);

	UFUNCTION()
	void HandleScreenResolutionSelectionChanged(FString SelectedItem, ESelectInfo::Type SelectionType);

	UFUNCTION()
	void HandleVSyncSelectionChanged(FString SelectedItem, ESelectInfo::Type SelectionType);

	UFUNCTION()
	void HandleFPSLimitSelectionChanged(FString SelectedItem, ESelectInfo::Type SelectionType);

	UFUNCTION()
	void HandleLumenSelectionChanged(FString SelectedItem, ESelectInfo::Type SelectionType);

	UFUNCTION()
	void HandleDLSSSelectionChanged(FString SelectedItem, ESelectInfo::Type SelectionType);

	UFUNCTION()
	void HandleFrameGenerationSelectionChanged(FString SelectedItem, ESelectInfo::Type SelectionType);

	UFUNCTION()
	void HandleFSRSelectionChanged(FString SelectedItem, ESelectInfo::Type SelectionType);

	UFUNCTION()
	void HandleFSRFrameGenerationSelectionChanged(FString SelectedItem, ESelectInfo::Type SelectionType);

	UFUNCTION()
	void HandleApplyClicked();

	UFUNCTION()
	void HandleResetClicked();

	UFUNCTION()
	void HandleBackClicked();

	bool bHasPendingChanges;
	bool bIsRefreshingFromSettings;
};

