#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/Button.h"
#include "ZonefallMainMenuWidget.generated.h"

class UBorder;
class UButton;
class UImage;
class UTextBlock;
class UVerticalBox;
class UHorizontalBox;
class UOverlay;
class USizeBox;
class UScrollBox;
class UTexture2D;

UENUM()
enum class EZonefallMenuAction : uint8
{
	GoHome,
	GoStory,
	GoOnline,
	GoWhatsNew,
	GoDLC,
	GoSettingsTab,
	PlayStory,
	OpenOnlineLobby,
	OpenSettings,
	QuitGame,
	None
};

/** A single patch-notes entry shown on the WHAT'S NEW tab. */
USTRUCT(BlueprintType)
struct FZonefallPatchEntry
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zonefall|Menu")
	FString Version;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zonefall|Menu")
	FString Date;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zonefall|Menu")
	TArray<FString> Changes;
};

/** A future / DLC entry shown on the DLC tab. */
USTRUCT(BlueprintType)
struct FZonefallDLCEntry
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zonefall|Menu")
	FString Title;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zonefall|Menu")
	FString Description;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zonefall|Menu")
	FString Status = TEXT("COMING SOON");
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnZonefallMenuItemClicked, int32, ItemId);

/** A clickable tile/tab that remembers its id and re-broadcasts the click. */
UCLASS()
class UIWORLD_API UZonefallMenuItemButton : public UButton
{
	GENERATED_BODY()

public:
	UZonefallMenuItemButton();

	UPROPERTY(Transient)
	int32 ItemId = INDEX_NONE;

	UPROPERTY(BlueprintAssignable)
	FOnZonefallMenuItemClicked OnItemClicked;

private:
	UFUNCTION()
	void HandleInternalClicked();
};

/**
 * Self-assembling AAA-style main menu (GTA-like): top category tabs, a hero banner with
 * title + description, a bottom row of large action cards, and on-screen control hints.
 * Assign as UUIWorldMenuGameInstance::MainMenuWidgetClass.
 */
UCLASS(BlueprintType, Blueprintable)
class UIWORLD_API UZonefallMainMenuWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UZonefallMainMenuWidget(const FObjectInitializer& ObjectInitializer);

	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeConstruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
	virtual FReply NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;

	// --- Content configuration ---
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zonefall|Menu")
	FText GameTitle = FText::FromString(TEXT("ZONEFALL PROTOCOL"));

	// Shown bottom-left next to the engine version. Edit freely.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zonefall|Menu")
	FText GameVersionText = FText::FromString(TEXT("v1.0.0"));

	// Animated cyberpunk-style background effects (scan sweep + pulsing accent lines).
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zonefall|Menu|Style")
	bool bEnableCyberEffects = true;

	// Patch notes shown on the WHAT'S NEW tab (newest first). Edit freely.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zonefall|Menu|Content")
	TArray<FZonefallPatchEntry> PatchNotes;

	// Future content / DLC shown on the DLC tab.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zonefall|Menu|Content")
	TArray<FZonefallDLCEntry> UpcomingDLC;

	// Level opened by the STORY "PLAY" card.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zonefall|Menu")
	FName StoryLevelName = TEXT("Lvl_ThirdPerson");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zonefall|Menu|Style")
	TSoftObjectPtr<UTexture2D> BackgroundImage;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zonefall|Menu|Style")
	FLinearColor BackgroundTint = FLinearColor(1.0f, 1.0f, 1.0f, 1.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zonefall|Menu|Style")
	FLinearColor AccentColor = FLinearColor(0.27f, 0.85f, 0.96f, 1.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zonefall|Menu|Style")
	int32 TitleFontSize = 46;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zonefall|Menu|Style")
	int32 BodyFontSize = 17;

private:
	struct FMenuTab
	{
		FText Label;
		EZonefallMenuAction TabAction; // GoHome/GoStory/GoOnline/GoSettingsTab
	};

	struct FMenuCard
	{
		FText Title;
		FText Subtitle;
		EZonefallMenuAction Action;
	};

	void BuildLayout();
	void UpdateOnlineIndicator();
	void ShowPage(int32 PageIndex);
	void RebuildCards();
	void RebuildInfoPanel(int32 PageIndex);
	void BuildPatchNotesInto(UVerticalBox* Box);
	void BuildDLCInto(UVerticalBox* Box);
	void PerformAction(EZonefallMenuAction Action);
	class UUIWorldMenuGameInstance* ResolveGameInstance() const;

	UFUNCTION() void HandleTabClicked(int32 TabId);
	UFUNCTION() void HandleCardClicked(int32 CardId);

	void GetCardsForPage(int32 PageIndex, FText& OutTitle, FText& OutDesc, TArray<FMenuCard>& OutCards) const;

	UPROPERTY(Transient) TObjectPtr<UOverlay> RootOverlay;
	UPROPERTY(Transient) TObjectPtr<UImage> BackgroundImageWidget;
	UPROPERTY(Transient) TObjectPtr<UImage> ScanBar;
	UPROPERTY(Transient) TObjectPtr<UImage> AccentLineTop;
	UPROPERTY(Transient) TObjectPtr<UImage> AccentLineBottom;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> VersionText;
	UPROPERTY(Transient) TObjectPtr<UImage> OnlineDot;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> OnlineStatusText;
	UPROPERTY(Transient) TObjectPtr<UHorizontalBox> TabBar;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> HeroTitle;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> HeroDescription;
	UPROPERTY(Transient) TObjectPtr<USizeBox> HeroSizeBox;
	UPROPERTY(Transient) TObjectPtr<UHorizontalBox> CardRow;
	UPROPERTY(Transient) TObjectPtr<UBorder> InfoPanel;
	UPROPERTY(Transient) TObjectPtr<UScrollBox> InfoScroll;

	float MenuAnimTime = 0.0f;
	float OnlinePollTimer = 0.0f;

	UPROPERTY(Transient) TArray<TObjectPtr<UZonefallMenuItemButton>> TabButtons;
	UPROPERTY(Transient) TArray<TObjectPtr<UZonefallMenuItemButton>> CardButtons;

	UPROPERTY(Transient) int32 ActivePage = 0;

	TArray<FMenuTab> Tabs;
	TArray<FMenuCard> CurrentCards;
};
