#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "UIWorldMenuGameInstance.h"
#include "ZonefallLoadingScreenWidget.generated.h"

class UBorder;
class UProgressBar;
class UTextBlock;
class UVerticalBox;
class UTexture2D;

UCLASS(BlueprintType, Blueprintable)
class ZONEFALL_API UZonefallLoadingScreenWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UZonefallLoadingScreenWidget(const FObjectInitializer& ObjectInitializer);

	virtual void NativePreConstruct() override;
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zonefall|UI|Loading")
	FText LoadingTitle;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zonefall|UI|Loading")
	FText LoadingSubtitle;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zonefall|UI|Loading")
	bool bAutoStartProgress;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zonefall|UI|Loading|Background")
	bool bAutoRotateImages;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zonefall|UI|Loading|Background", meta = (ClampMin = "0.2", ClampMax = "30.0"))
	float ImageRotateInterval;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zonefall|UI|Loading|Background")
	TArray<TObjectPtr<UTexture2D>> LoadingImages;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zonefall|UI|Loading|Background")
	TObjectPtr<UTexture2D> DefaultLoadingImage;

	// Optional UMG widget names for auto-binding.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zonefall|UI|Loading|Binding")
	FName ImageBorderName;


	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zonefall|UI|Loading|Binding")
	FName RootBorderName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zonefall|UI|Loading|Binding")
	FName RootBoxName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zonefall|UI|Loading|Binding")
	FName TitleTextName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zonefall|UI|Loading|Binding")
	FName SubtitleTextName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zonefall|UI|Loading|Binding")
	FName ProgressBarName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zonefall|UI|Loading|Binding")
	FName ProgressTextName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zonefall|UI|Loading|Text")
	bool bAnimateLoadingText;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zonefall|UI|Loading|Text", meta = (ClampMin = "0.1", ClampMax = "2.0"))
	float LoadingTextAnimInterval;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zonefall|UI|Loading|Text")
	bool bAnimateStatusText;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zonefall|UI|Loading|Text", meta = (ClampMin = "0.2", ClampMax = "4.0"))
	float StatusTextAnimInterval;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zonefall|UI|Loading|Text")
	TArray<FText> LoadingStatusPhrases;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zonefall|UI|Loading|Text")
	bool bAnimateSpinner;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zonefall|UI|Loading|Progress")
	bool bAnimateProgress;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zonefall|UI|Loading|Progress", meta = (ClampMin = "0.2", ClampMax = "30.0"))
	float ProgressDurationMinSeconds;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zonefall|UI|Loading|Progress", meta = (ClampMin = "0.5", ClampMax = "60.0"))
	float ProgressDurationMaxSeconds;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zonefall|UI|Loading|Progress", meta = (ClampMin = "0.02", ClampMax = "1.0"))
	float ProgressTickInterval;

	UFUNCTION(BlueprintCallable, Category = "Zonefall|UI|Loading")
	void StartLoading();

	/** Legacy entry — prefer ConfigureOnlineTravelLoading for host/join. */
	UFUNCTION(BlueprintCallable, Category = "Zonefall|UI|Loading")
	void StartOnlineTravelLoading(const FText& Subtitle);

	/** Slower progress + online-specific status lines (host / join / sync). */
	UFUNCTION(BlueprintCallable, Category = "Zonefall|UI|Loading")
	void ConfigureOnlineTravelLoading(EZonefallOnlineTravelPhase Phase, const FText& StatusHint);

	UFUNCTION(BlueprintCallable, Category = "Zonefall|UI|Loading")
	void SetOnlineTravelStatus(const FText& Status);

	UFUNCTION(BlueprintCallable, Category = "Zonefall|UI|Loading")
	void CompleteLoading();

	// Pushes text progress near the end before blocking map load starts.
	UFUNCTION(BlueprintCallable, Category = "Zonefall|UI|Loading")
	void EnterFinalizingPhase(float InPercent = 90.0f);

	UFUNCTION(BlueprintCallable, Category = "Zonefall|UI|Loading")
	void StartImageRotation();

	UFUNCTION(BlueprintCallable, Category = "Zonefall|UI|Loading")
	void StopImageRotation();

protected:
	UPROPERTY(Transient, BlueprintReadOnly, Category = "Zonefall|UI|Loading", meta = (BindWidgetOptional))
	TObjectPtr<UBorder> RootBorder;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "Zonefall|UI|Loading", meta = (BindWidgetOptional))
	TObjectPtr<UVerticalBox> RootBox;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "Zonefall|UI|Loading", meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> TitleText;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "Zonefall|UI|Loading", meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> SubtitleText;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "Zonefall|UI|Loading", meta = (BindWidgetOptional))
	TObjectPtr<UBorder> ImageBorder;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "Zonefall|UI|Loading", meta = (BindWidgetOptional))
	TObjectPtr<UProgressBar> ProgressBar;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "Zonefall|UI|Loading", meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> ProgressText;

private:
	void ResolveNamedWidgets();
	void BuildWidgetTree();
	void UpdateTexts();
	void UpdateImageVisual();

	UFUNCTION()
	void HandleImageRotateTick();

	UFUNCTION()
	void HandleLoadingTextTick();

	UFUNCTION()
	void HandleStatusTextTick();

	UFUNCTION()
	void HandleProgressTick();

	float EstimateHardwareProgressDuration() const;

	int32 CurrentImageIndex;
	FTimerHandle ImageRotateTimerHandle;
	FTimerHandle LoadingTextTimerHandle;
	FTimerHandle StatusTextTimerHandle;
	FTimerHandle ProgressTimerHandle;
	int32 LoadingTextDotCount;
	int32 StatusPhraseIndex;
	int32 SpinnerFrameIndex;
	float ProgressAlpha;
	float ProgressDurationSeconds;
	FText BaseAnimatedTitleText;
	FText BaseAnimatedSubtitleText;

	bool bOnlineTravelMode = false;
	float OnlineProgressCap = 92.0f;
	EZonefallOnlineTravelPhase OnlineTravelPhase = EZonefallOnlineTravelPhase::Joining;
};


