#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "ZonefallTitleSplashWidget.generated.h"

class UBorder;
class UImage;
class UTextBlock;
class UVerticalBox;

/**
 * Self-assembling animated title splash shown BEFORE shader compilation.
 * Big game name reveal + accent line draw-in + subtitle fade + scan sweep.
 * The game instance controls how long it stays on screen.
 */
UCLASS(BlueprintType, Blueprintable)
class UIWORLD_API UZonefallTitleSplashWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UZonefallTitleSplashWidget(const FObjectInitializer& ObjectInitializer);

	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeConstruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	// Editable game name shown large in the center.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zonefall|Splash")
	FText GameTitle = FText::FromString(TEXT("ZONEFALL PROTOCOL"));

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zonefall|Splash")
	FText Subtitle = FText::FromString(TEXT("A SURVIVAL EXPERIENCE"));

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zonefall|Splash|Style")
	FLinearColor BackgroundColor = FLinearColor(0.01f, 0.02f, 0.04f, 1.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zonefall|Splash|Style")
	FLinearColor AccentColor = FLinearColor(0.27f, 0.85f, 0.96f, 1.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zonefall|Splash|Style", meta = (ClampMin = "20", ClampMax = "120"))
	int32 TitleFontSize = 64;

private:
	void BuildLayout();

	UPROPERTY(Transient) TObjectPtr<UBorder> RootBorder;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> TitleText;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> SubtitleText;
	UPROPERTY(Transient) TObjectPtr<UImage> AccentLine;
	UPROPERTY(Transient) TObjectPtr<UImage> ScanBar;

	float AnimTime = 0.0f;
};
