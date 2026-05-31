#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "ZonefallShaderLoadingWidget.generated.h"

class UBorder;
class UImage;
class UProgressBar;
class UTextBlock;
class UVerticalBox;
class UTexture2D;
class UFont;
class UCircularThrobber;

UCLASS(BlueprintType, Blueprintable)
class ZONEFALL_API UZonefallShaderLoadingWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UZonefallShaderLoadingWidget(const FObjectInitializer& ObjectInitializer);

	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	// Game title shown large in the center. Edit freely to change the game name.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zonefall|UI|Startup")
	FText StartupTitleText;

	// Sub-line under the title (studio / tagline).
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zonefall|UI|Startup")
	FText StartupSubtitleText;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zonefall|UI|Startup")
	FText ShaderLoadingBaseText;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zonefall|UI|Startup")
	FText ShaderCheckingCacheText;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zonefall|UI|Startup")
	FText ShaderFinalizingText;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zonefall|UI|Startup")
	FText PleaseWaitText;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zonefall|UI|Startup")
	FText ShaderCacheHintText;

	// Rotating loading tips shown below the hint line. Cycles every TipRotateSeconds.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zonefall|UI|Startup")
	TArray<FText> RotatingTips;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zonefall|UI|Startup", meta = (ClampMin = "1.0", ClampMax = "30.0"))
	float TipRotateSeconds;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zonefall|UI|Startup", meta = (ClampMin = "0.0", ClampMax = "100.0"))
	float ShaderCompileProgressPercent;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zonefall|UI|Startup", meta = (ClampMin = "0.1", ClampMax = "2.0"))
	float TextAnimInterval;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zonefall|UI|Startup")
	bool bUseRealShaderCompilerProgress;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zonefall|UI|Style")
	TSoftObjectPtr<UTexture2D> BackgroundImageTexture;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zonefall|UI|Style")
	FLinearColor BackgroundImageTint;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zonefall|UI|Style")
	FLinearColor BottomPanelTint;

	// Accent color used for the glowing divider, progress bar and title sheen.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zonefall|UI|Style")
	FLinearColor AccentColor;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zonefall|UI|Style", meta = (ClampMin = "10", ClampMax = "72"))
	int32 TitleFontSize;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zonefall|UI|Style", meta = (ClampMin = "10", ClampMax = "48"))
	int32 BodyFontSize;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zonefall|UI|Style", meta = (ClampMin = "0.0", ClampMax = "25.0"))
	float SmoothProgressSpeed;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zonefall|UI|Style", meta = (ClampMin = "5.0", ClampMax = "120.0"))
	float ProgressUnitsPerSecond;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zonefall|UI|Style")
	bool bEnableUiAnimations;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zonefall|UI|Style", meta = (ClampMin = "0.1", ClampMax = "8.0"))
	float PanelPulseSpeed;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zonefall|UI|Style", meta = (ClampMin = "0.1", ClampMax = "12.0"))
	float BarGlowSpeed;

	// Optional custom title font. If empty, engine default Roboto is used.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zonefall|UI|Style")
	TSoftObjectPtr<UFont> CustomTitleFont;

	// Optional custom body font for status/progress lines.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zonefall|UI|Style")
	TSoftObjectPtr<UFont> CustomBodyFont;

	UFUNCTION(BlueprintCallable, Category = "Zonefall|UI|Startup")
	void SetShaderCompileProgress(float NewPercent);

	UFUNCTION(BlueprintCallable, Category = "Zonefall|UI|Startup")
	void SetInitialShaderJobCount(int32 InInitialJobCount);

	// Number of render pipelines (PSOs) still precompiling — the real anti-stutter metric.
	UFUNCTION(BlueprintCallable, Category = "Zonefall|UI|Startup")
	void SetPipelinesRemaining(int32 InRemaining);

	UFUNCTION(BlueprintPure, Category = "Zonefall|UI|Startup")
	bool IsShaderCompilationLikelyFinished() const;

protected:
	UPROPERTY(Transient, BlueprintReadOnly, Category = "Zonefall|UI|Startup", meta = (BindWidgetOptional))
	TObjectPtr<UBorder> RootBorder;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "Zonefall|UI|Startup", meta = (BindWidgetOptional))
	TObjectPtr<UImage> BackgroundImageWidget;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "Zonefall|UI|Startup", meta = (BindWidgetOptional))
	TObjectPtr<UVerticalBox> RootBox;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "Zonefall|UI|Startup", meta = (BindWidgetOptional))
	TObjectPtr<UBorder> BottomPanel;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "Zonefall|UI|Startup", meta = (BindWidgetOptional))
	TObjectPtr<UImage> AccentDivider;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "Zonefall|UI|Startup", meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> TipText;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "Zonefall|UI|Startup", meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> TitleText;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "Zonefall|UI|Startup", meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> SubtitleText;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "Zonefall|UI|Startup", meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> PercentText;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "Zonefall|UI|Startup", meta = (BindWidgetOptional))
	TObjectPtr<UCircularThrobber> Spinner;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "Zonefall|UI|Startup", meta = (BindWidgetOptional))
	TObjectPtr<UImage> TopAccentLine;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "Zonefall|UI|Startup", meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> ShaderText;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "Zonefall|UI|Startup", meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> ProgressText;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "Zonefall|UI|Startup", meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> WaitText;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "Zonefall|UI|Startup", meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> CacheHintText;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "Zonefall|UI|Startup", meta = (BindWidgetOptional))
	TObjectPtr<UProgressBar> CompileProgressBar;

private:
	void BuildLayoutIfNeeded(bool bForceRebuild);
	void BindExistingWidgetsFromTree();
	void ApplyRuntimeFonts();
	void UpdateTexts();

	UFUNCTION()
	void HandleTextAnimTick();

	FTimerHandle TextAnimTimerHandle;
	int32 DotCount;
	int32 InitialShaderJobCount;
	int32 LastKnownRemainingJobs;
	int32 PipelinesRemaining = 0;
	int32 ConsecutiveZeroJobTicks;
	bool bShaderProgressInitialized;
	bool bEnteredFinalizingState;
	float SmoothedProgressPercent;
	float AnimationTimeSeconds;
	int32 CurrentTipIndex;
	float TipElapsedSeconds;
};

