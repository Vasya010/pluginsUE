#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "ZonefallModernButtonWidget.generated.h"

class UBorder;
class UButton;
class UOverlay;
class USoundBase;
class UTextBlock;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnZonefallModernButtonClicked);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnZonefallStartGameRequested);

UENUM(BlueprintType)
enum class EZonefallModernButtonTheme : uint8
{
	Dark UMETA(DisplayName = "Dark"),
	Neon UMETA(DisplayName = "Neon")
};

UCLASS(BlueprintType, Blueprintable)
class ZONEFALL_API UZonefallModernButtonWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UZonefallModernButtonWidget(const FObjectInitializer& ObjectInitializer);

	virtual void NativePreConstruct() override;
	virtual void NativeConstruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
	virtual void SynchronizeProperties() override;

	UPROPERTY(BlueprintAssignable, Category = "Zonefall|UI|ModernButton")
	FOnZonefallModernButtonClicked OnClicked;

	UPROPERTY(BlueprintAssignable, Category = "Zonefall|UI|ModernButton")
	FOnZonefallStartGameRequested OnStartGameRequested;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zonefall|UI|ModernButton")
	FText Label;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zonefall|UI|ModernButton")
	FText FallbackLabel;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zonefall|UI|ModernButton")
	FSlateFontInfo LabelFont;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zonefall|UI|ModernButton", meta = (ClampMin = "10", ClampMax = "128"))
	int32 LabelFontSize;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zonefall|UI|ModernButton|Colors")
	FLinearColor LabelColorNormal;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zonefall|UI|ModernButton|Colors")
	FLinearColor LabelColorHover;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zonefall|UI|ModernButton|Colors")
	FLinearColor LabelColorPressed;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zonefall|UI|ModernButton|Colors")
	FLinearColor BackgroundNormal;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zonefall|UI|ModernButton|Colors")
	FLinearColor BackgroundHover;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zonefall|UI|ModernButton|Colors")
	FLinearColor BackgroundPressed;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zonefall|UI|ModernButton|Style")
	EZonefallModernButtonTheme ThemePreset;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zonefall|UI|ModernButton|Style")
	bool bUseGlassGradientStyle;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zonefall|UI|ModernButton|Behavior")
	bool bEmitStartGameOnClick;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zonefall|UI|ModernButton|Audio")
	TObjectPtr<USoundBase> HoverSound;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zonefall|UI|ModernButton|Audio")
	TObjectPtr<USoundBase> PressSound;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zonefall|UI|ModernButton|Audio")
	TObjectPtr<USoundBase> ClickSound;

	UFUNCTION(BlueprintCallable, Category = "Zonefall|UI|ModernButton")
	void SetLabel(const FText& InLabel);

	UFUNCTION(BlueprintCallable, Category = "Zonefall|UI|ModernButton")
	void ApplyVisualStyle();

	UFUNCTION(BlueprintCallable, Category = "Zonefall|UI|ModernButton")
	void ApplyThemePreset(EZonefallModernButtonTheme NewTheme);

	UFUNCTION(BlueprintCallable, Category = "Zonefall|UI|ModernButton")
	void RequestStartGame();

	// Direct BP events (appear as regular Event nodes in widget graph).
	UFUNCTION(BlueprintImplementableEvent, Category = "Zonefall|UI|ModernButton|Events")
	void BP_OnButtonClicked();

	UFUNCTION(BlueprintImplementableEvent, Category = "Zonefall|UI|ModernButton|Events")
	void BP_OnButtonHovered();

	UFUNCTION(BlueprintImplementableEvent, Category = "Zonefall|UI|ModernButton|Events")
	void BP_OnButtonPressed();

	UFUNCTION(BlueprintImplementableEvent, Category = "Zonefall|UI|ModernButton|Events")
	void BP_OnStartGameRequested();

	// Optional explicit names to bind widgets from a designer-authored blueprint.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zonefall|UI|ModernButton|Binding")
	FName TargetButtonWidgetName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zonefall|UI|ModernButton|Binding")
	FName TargetTextWidgetName;

	UFUNCTION(BlueprintCallable, Category = "Zonefall|UI|ModernButton|Binding")
	bool ValidateAndBindWidgets();

protected:
	UPROPERTY(Transient, BlueprintReadOnly, Category = "Zonefall|UI|ModernButton", meta = (BindWidgetOptional))
	TObjectPtr<UBorder> RootBorder;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "Zonefall|UI|ModernButton", meta = (BindWidgetOptional))
	TObjectPtr<UButton> ClickButton;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "Zonefall|UI|ModernButton", meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> LabelText;

private:
	void BuildWidgetTree();
	void UpdateVisualState(const FLinearColor& InBackground, const FLinearColor& InTextColor, const FVector2D& ShadowOffset);
	void SyncThemeColors();
	void ApplyButtonCoreStyle();
	FText GetEffectiveLabel() const;
	void TryBindFromWidgetTree();

	UFUNCTION()
	void HandleClicked();

	UFUNCTION()
	void HandleHovered();

	UFUNCTION()
	void HandleUnhovered();

	UFUNCTION()
	void HandlePressed();

	UFUNCTION()
	void HandleReleased();

	bool bHoverPulseActive = false;
	float HoverPulseTime = 0.0f;
};

