#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "ZonefallModernProgressBarWidget.generated.h"

class UBorder;
class UProgressBar;
class UTextBlock;
class UVerticalBox;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnZonefallProgressChanged, float, NormalizedValue);

UCLASS(BlueprintType, Blueprintable)
class ZONEFALL_API UZonefallModernProgressBarWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UZonefallModernProgressBarWidget(const FObjectInitializer& ObjectInitializer);

	virtual void NativePreConstruct() override;
	virtual void NativeConstruct() override;

	UPROPERTY(BlueprintAssignable, Category = "Zonefall|UI|Progress")
	FOnZonefallProgressChanged OnProgressChanged;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zonefall|UI|Progress")
	FText TitleText;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zonefall|UI|Progress")
	bool bAutoProgress;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zonefall|UI|Progress", meta = (ClampMin = "0.001", ClampMax = "1.0"))
	float AutoProgressStep;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zonefall|UI|Progress", meta = (ClampMin = "0.005", ClampMax = "1.0"))
	float AutoProgressInterval;

	// Optional UMG widget names for auto-binding.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zonefall|UI|Progress|Binding")
	FName ProgressBarName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zonefall|UI|Progress|Binding")
	FName RootBorderName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zonefall|UI|Progress|Binding")
	FName RootBoxName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zonefall|UI|Progress|Binding")
	FName TitleLabelName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zonefall|UI|Progress|Binding")
	FName PercentLabelName;

	UFUNCTION(BlueprintCallable, Category = "Zonefall|UI|Progress")
	void SetProgressNormalized(float InProgress);

	UFUNCTION(BlueprintCallable, Category = "Zonefall|UI|Progress")
	void StartAutoProgress();

	UFUNCTION(BlueprintCallable, Category = "Zonefall|UI|Progress")
	void StopAutoProgress();

	UFUNCTION(BlueprintPure, Category = "Zonefall|UI|Progress")
	float GetProgressNormalized() const { return ProgressValue; }

protected:
	UPROPERTY(Transient, BlueprintReadOnly, Category = "Zonefall|UI|Progress", meta = (BindWidgetOptional))
	TObjectPtr<UBorder> RootBorder;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "Zonefall|UI|Progress", meta = (BindWidgetOptional))
	TObjectPtr<UVerticalBox> RootBox;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "Zonefall|UI|Progress", meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> TitleLabel;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "Zonefall|UI|Progress", meta = (BindWidgetOptional))
	TObjectPtr<UProgressBar> ProgressBar;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "Zonefall|UI|Progress", meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> PercentLabel;

private:
	void ResolveNamedWidgets();
	void BuildWidgetTree();
	void UpdateVisuals();

	UFUNCTION()
	void HandleAutoProgressTick();

	float ProgressValue;
	FTimerHandle AutoProgressTimerHandle;
};


