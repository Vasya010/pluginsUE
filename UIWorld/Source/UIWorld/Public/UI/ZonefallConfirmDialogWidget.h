#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "ZonefallConfirmDialogWidget.generated.h"

class UBorder;
class UButton;
class UTextBlock;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnZonefallDialogConfirmed);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnZonefallDialogCancelled);

/** Reusable, self-assembling modal confirm dialog (dim backdrop + panel + two buttons). */
UCLASS(BlueprintType, Blueprintable)
class UIWORLD_API UZonefallConfirmDialogWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UZonefallConfirmDialogWidget(const FObjectInitializer& ObjectInitializer);

	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeConstruct() override;

	UFUNCTION(BlueprintCallable, Category = "Zonefall|Dialog")
	void Setup(const FText& Title, const FText& Message, const FText& ConfirmLabel, const FText& CancelLabel);

	UPROPERTY(BlueprintAssignable, Category = "Zonefall|Dialog")
	FOnZonefallDialogConfirmed OnConfirmed;

	UPROPERTY(BlueprintAssignable, Category = "Zonefall|Dialog")
	FOnZonefallDialogCancelled OnCancelled;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zonefall|Dialog|Style")
	FLinearColor PanelTint = FLinearColor(0.06f, 0.09f, 0.13f, 0.98f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zonefall|Dialog|Style")
	FLinearColor AccentColor = FLinearColor(0.27f, 0.85f, 0.96f, 1.0f);

private:
	void BuildLayout();

	UFUNCTION() void HandleConfirm();
	UFUNCTION() void HandleCancel();

	UPROPERTY(Transient) TObjectPtr<UTextBlock> TitleText;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> MessageText;
	UPROPERTY(Transient) TObjectPtr<UButton> ConfirmButton;
	UPROPERTY(Transient) TObjectPtr<UButton> CancelButton;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> ConfirmLabelText;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> CancelLabelText;
};
