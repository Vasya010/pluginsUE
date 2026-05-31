#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "ZonefallModernComboBoxWidget.generated.h"

class UBorder;
class UComboBoxString;
class UTextBlock;
class UVerticalBox;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnZonefallModernComboSelectionChanged, const FString&, SelectedItem);

UENUM(BlueprintType)
enum class EZonefallModernComboTheme : uint8
{
	Dark UMETA(DisplayName = "Dark"),
	Neon UMETA(DisplayName = "Neon")
};

UENUM(BlueprintType)
enum class EZonefallModernComboPreset : uint8
{
	Manual UMETA(DisplayName = "Manual"),
	OverallQuality UMETA(DisplayName = "Overall Quality"),
	DisplayMode UMETA(DisplayName = "Display Mode"),
	VSync UMETA(DisplayName = "VSync"),
	FPSLimit UMETA(DisplayName = "FPS Limit"),
	ScreenResolution UMETA(DisplayName = "Screen Resolution"),
	DLSS UMETA(DisplayName = "Zonefall DLSS"),
	DLSSFrameGeneration UMETA(DisplayName = "Zonefall DLSS Frame Gen"),
	FSR UMETA(DisplayName = "Zonefall FSR"),
	FSRFrameGeneration UMETA(DisplayName = "Zonefall FSR Frame Gen")
};

UCLASS(BlueprintType, Blueprintable)
class ZONEFALL_API UZonefallModernComboBoxWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UZonefallModernComboBoxWidget(const FObjectInitializer& ObjectInitializer);

	virtual void NativePreConstruct() override;
	virtual void NativeConstruct() override;
	virtual void SynchronizeProperties() override;

	UPROPERTY(BlueprintAssignable, Category = "Zonefall|UI|ModernCombo")
	FOnZonefallModernComboSelectionChanged OnSelectionChanged;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zonefall|UI|ModernCombo|Text")
	FText Label;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zonefall|UI|ModernCombo|Text")
	FText PlaceholderText;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zonefall|UI|ModernCombo|Text")
	FSlateFontInfo LabelFont;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zonefall|UI|ModernCombo|Text", meta = (ClampMin = "10", ClampMax = "64"))
	int32 LabelFontSize;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zonefall|UI|ModernCombo|Text", meta = (ClampMin = "10", ClampMax = "64"))
	int32 ValueFontSize;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zonefall|UI|ModernCombo|Style")
	EZonefallModernComboTheme ThemePreset;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zonefall|UI|ModernCombo|Colors")
	FLinearColor PanelColor;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zonefall|UI|ModernCombo|Colors")
	FLinearColor LabelColor;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zonefall|UI|ModernCombo|Colors")
	FLinearColor ValueColor;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zonefall|UI|ModernCombo|Data")
	TArray<FString> Options;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zonefall|UI|ModernCombo|Data")
	bool bAutoPopulateFromPreset;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zonefall|UI|ModernCombo|Data")
	EZonefallModernComboPreset AutoPreset;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zonefall|UI|ModernCombo|Binding")
	FName TargetComboWidgetName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zonefall|UI|ModernCombo|Binding")
	FName TargetLabelWidgetName;

	UFUNCTION(BlueprintCallable, Category = "Zonefall|UI|ModernCombo|Data")
	void SetOptions(const TArray<FString>& InOptions, bool bSelectFirst = true);

	UFUNCTION(BlueprintCallable, Category = "Zonefall|UI|ModernCombo|Data")
	void AddOption(const FString& InOption);

	UFUNCTION(BlueprintCallable, Category = "Zonefall|UI|ModernCombo|Data")
	void ClearOptions();

	UFUNCTION(BlueprintCallable, Category = "Zonefall|UI|ModernCombo|Data")
	void SetSelectedOption(const FString& InOption);

	UFUNCTION(BlueprintPure, Category = "Zonefall|UI|ModernCombo|Data")
	FString GetSelectedOption() const;

	UFUNCTION(BlueprintCallable, Category = "Zonefall|UI|ModernCombo|Style")
	void ApplyVisualStyle();

	UFUNCTION(BlueprintCallable, Category = "Zonefall|UI|ModernCombo|Binding")
	bool ValidateAndBindWidgets();

	UFUNCTION(BlueprintImplementableEvent, Category = "Zonefall|UI|ModernCombo|Events")
	void BP_OnSelectionChanged(const FString& SelectedItem);

protected:
	UPROPERTY(Transient, BlueprintReadOnly, Category = "Zonefall|UI|ModernCombo", meta = (BindWidgetOptional))
	TObjectPtr<UBorder> RootBorder;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "Zonefall|UI|ModernCombo", meta = (BindWidgetOptional))
	TObjectPtr<UVerticalBox> RootBox;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "Zonefall|UI|ModernCombo", meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> LabelTextBlock;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "Zonefall|UI|ModernCombo", meta = (BindWidgetOptional))
	TObjectPtr<UComboBoxString> ComboBox;

private:
	void TryBindFromWidgetTree();
	void BuildWidgetTree();
	void SyncThemeColors();
	void RebuildOptions(bool bSelectFirst);
	void BindComboEvents();

	UFUNCTION()
	void HandleSelectionChanged(FString SelectedItem, ESelectInfo::Type SelectionType);
};


