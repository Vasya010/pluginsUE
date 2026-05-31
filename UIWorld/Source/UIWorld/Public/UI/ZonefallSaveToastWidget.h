#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "ZonefallSaveToastWidget.generated.h"

class UBorder;
class UTextBlock;
class UCircularThrobber;

/**
 * Self-managing "saved" toast that slides in from the right edge, holds, then slides
 * out and removes itself. GTA-style save indicator. Add to viewport and call ShowToast.
 */
UCLASS(BlueprintType, Blueprintable)
class UIWORLD_API UZonefallSaveToastWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UZonefallSaveToastWidget(const FObjectInitializer& ObjectInitializer);

	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeConstruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	UFUNCTION(BlueprintCallable, Category = "Zonefall|Save")
	void ShowToast(const FText& Title, const FText& Message);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zonefall|Save|Toast")
	FText DefaultTitle = FText::FromString(TEXT("GAME SAVED"));

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zonefall|Save|Toast", meta = (ClampMin = "0.5", ClampMax = "10.0"))
	float DisplayDuration = 2.5f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zonefall|Save|Toast|Style")
	FLinearColor PanelTint = FLinearColor(0.05f, 0.08f, 0.12f, 0.96f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zonefall|Save|Toast|Style")
	FLinearColor AccentColor = FLinearColor(0.27f, 0.85f, 0.96f, 1.0f);

private:
	void BuildLayout();

	UPROPERTY(Transient) TObjectPtr<UBorder> Panel;
	UPROPERTY(Transient) TObjectPtr<UCircularThrobber> Spinner;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> TitleText;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> MessageText;

	float AnimTime = 0.0f;
	bool bRemoved = false;
};
