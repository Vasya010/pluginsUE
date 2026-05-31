#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "ZonefallBootLogosWidget.generated.h"

class UBorder;
class UImage;
class UTextBlock;
class UVerticalBox;
class UTexture2D;

/** One full-screen boot card (engine logo, studio logo, ...). */
USTRUCT(BlueprintType)
struct FZonefallBootLogo
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zonefall|Boot")
	FText Title;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zonefall|Boot")
	FText Subtitle;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zonefall|Boot")
	FLinearColor Accent = FLinearColor(0.32f, 0.86f, 0.99f, 1.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zonefall|Boot")
	float HoldSeconds = 2.2f;

	// Optional logo image shown above the title. If null and bDrawBadge is true, a self-drawn badge is used.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zonefall|Boot")
	TSoftObjectPtr<UTexture2D> LogoImage;

	// Draw a stylised round logo badge (e.g. the "UE5" engine mark) when no LogoImage is set.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zonefall|Boot")
	bool bDrawBadge = false;

	// Text inside the self-drawn badge (e.g. "UE5").
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zonefall|Boot")
	FString BadgeText = TEXT("UE5");
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FZonefallBootLogosFinished);

/**
 * AAA-style boot/legal logo sequence shown AFTER shader compilation and BEFORE the main menu.
 * Plays a series of fading full-screen cards (e.g. "Made with Unreal Engine X.Y", then the
 * studio). Fully self-assembling; press any key / click to skip. Broadcasts OnFinished when done.
 */
UCLASS(BlueprintType, Blueprintable)
class UIWORLD_API UZonefallBootLogosWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UZonefallBootLogosWidget(const FObjectInitializer& ObjectInitializer);

	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeConstruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
	virtual FReply NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;
	virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;

	UPROPERTY(BlueprintAssignable, Category = "Zonefall|Boot")
	FZonefallBootLogosFinished OnFinished;

	// If left empty, two default cards are built at runtime (engine + studio).
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zonefall|Boot")
	TArray<FZonefallBootLogo> Logos;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zonefall|Boot")
	float FadeSeconds = 0.7f;

	// Used by the engine card's subtitle (set by the game instance).
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zonefall|Boot")
	FString EngineVersionString = TEXT("Unreal Engine");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zonefall|Boot")
	FText StudioName = FText::FromString(TEXT("ZONEFALL"));

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zonefall|Boot")
	FText StudioTagline = FText::FromString(TEXT("A game by Kuzmenko Vasiliy"));

	// Sound played when each card appears (assign a whoosh/sting). Optional.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zonefall|Boot|Audio")
	TSoftObjectPtr<class USoundBase> CardSound;

	// Sound played once when the whole sequence finishes. Optional.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zonefall|Boot|Audio")
	TSoftObjectPtr<class USoundBase> FinishSound;

	UFUNCTION(BlueprintCallable, Category = "Zonefall|Boot")
	void SetEngineVersionString(const FString& InEngine) { EngineVersionString = InEngine; }

private:
	void BuildLayout();
	void BuildDefaultLogosIfNeeded();
	void ShowCard(int32 Index);
	void Finish();

	UPROPERTY(Transient) TObjectPtr<UVerticalBox> ContentBox;
	UPROPERTY(Transient) TObjectPtr<UImage> LogoImageWidget;
	UPROPERTY(Transient) TObjectPtr<class USizeBox> LogoBadgeBox;
	UPROPERTY(Transient) TObjectPtr<UBorder> LogoBadgeRing;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> LogoBadgeText;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> TitleText;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> SubtitleText;
	UPROPERTY(Transient) TObjectPtr<UImage> AccentLine;

	int32 CurrentIndex = INDEX_NONE;
	int32 Phase = 0;           // 0 fade-in, 1 hold, 2 fade-out
	float PhaseTime = 0.0f;
	bool bFinished = false;
};
