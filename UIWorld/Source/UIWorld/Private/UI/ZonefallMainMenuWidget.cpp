#include "UI/ZonefallMainMenuWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Brushes/SlateRoundedBoxBrush.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/ButtonSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Image.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/ScrollBox.h"
#include "Components/SizeBox.h"
#include "Components/SizeBoxSlot.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Engine/Font.h"
#include "Engine/Texture2D.h"

#include "Localization/ZonefallLocalizationSubsystem.h"
#include "UIWorldMenuGameInstance.h"

namespace
{
	FSlateFontInfo MakeMenuFont(int32 Size)
	{
		FSlateFontInfo Font;
		Font.Size = FMath::Clamp(Size, 8, 120);
		Font.FontObject = LoadObject<UObject>(nullptr, TEXT("/Engine/EngineFonts/Roboto.Roboto"));
		return Font;
	}

	UTextBlock* MakeMenuText(UWidgetTree* Tree, const FText& InText, int32 Size, FLinearColor Color)
	{
		UTextBlock* T = Tree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
		T->SetText(InText);
		T->SetFont(MakeMenuFont(Size));
		T->SetColorAndOpacity(FSlateColor(Color));
		return T;
	}

	void StyleMenuButton(UButton* B, FLinearColor Base, FLinearColor Hover, FLinearColor Pressed, float Radius = 4.0f)
	{
		if (!B) { return; }
		FButtonStyle Style = B->GetStyle();
		Style.Normal = FSlateRoundedBoxBrush(Base, Radius);
		Style.Hovered = FSlateRoundedBoxBrush(Hover, Radius);
		Style.Pressed = FSlateRoundedBoxBrush(Pressed, Radius);
		Style.Disabled = FSlateRoundedBoxBrush(FLinearColor(0.2f, 0.2f, 0.2f, 0.5f), Radius);
		B->SetStyle(Style);
	}
}

UZonefallMenuItemButton::UZonefallMenuItemButton()
{
	OnClicked.AddDynamic(this, &UZonefallMenuItemButton::HandleInternalClicked);
}

void UZonefallMenuItemButton::HandleInternalClicked()
{
	OnItemClicked.Broadcast(ItemId);
}

UZonefallMainMenuWidget::UZonefallMainMenuWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

TSharedRef<SWidget> UZonefallMainMenuWidget::RebuildWidget()
{
	if (!WidgetTree || !WidgetTree->RootWidget)
	{
		BuildLayout();
	}
	return Super::RebuildWidget();
}

void UZonefallMainMenuWidget::NativeConstruct()
{
	Super::NativeConstruct();

	// Build the version + engine line (engine string comes from the game instance).
	if (VersionText)
	{
		FString EngineStr = TEXT("Unreal Engine");
		if (const UUIWorldMenuGameInstance* GI = ResolveGameInstance())
		{
			EngineStr = GI->GetEngineVersionString();
		}
		VersionText->SetText(FText::FromString(FString::Printf(TEXT("%s   •   %s"), *GameVersionText.ToString(), *EngineStr)));
	}

	// Allow keyboard shortcuts (P = settings, Esc = quit).
	SetIsFocusable(true);
	SetKeyboardFocus();

	// Kick a Steam login attempt and paint the online indicator.
	if (UUIWorldMenuGameInstance* GI = ResolveGameInstance())
	{
		GI->RequestOnlineLogin();
	}
	UpdateOnlineIndicator();

	ShowPage(0);
}

void UZonefallMainMenuWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	// Refresh the online indicator about once a second (Steam login completes asynchronously).
	OnlinePollTimer += InDeltaTime;
	if (OnlinePollTimer >= 1.0f)
	{
		OnlinePollTimer = 0.0f;
		UpdateOnlineIndicator();
	}

	if (!bEnableCyberEffects)
	{
		return;
	}

	MenuAnimTime += InDeltaTime;

	// Pulsing accent lines (offset phases for a layered feel).
	const float PulseA = 0.45f + 0.55f * (0.5f + 0.5f * FMath::Sin(MenuAnimTime * 2.2f));
	const float PulseB = 0.45f + 0.55f * (0.5f + 0.5f * FMath::Sin(MenuAnimTime * 2.2f + PI));
	if (AccentLineTop) { AccentLineTop->SetColorAndOpacity(AccentColor * FLinearColor(1, 1, 1, PulseA)); }
	if (AccentLineBottom) { AccentLineBottom->SetColorAndOpacity(AccentColor * FLinearColor(1, 1, 1, PulseB)); }

	// Scan bar sweeps left to right and loops.
	if (ScanBar)
	{
		const float Width = MyGeometry.GetLocalSize().X;
		if (Width > 1.0f)
		{
			const float Period = 7.0f;
			const float T = FMath::Fmod(MenuAnimTime, Period) / Period;
			ScanBar->SetRenderTranslation(FVector2D(T * Width, 0.0f));
			const float ScanAlpha = 0.10f + 0.12f * (0.5f + 0.5f * FMath::Sin(MenuAnimTime * 5.0f));
			ScanBar->SetColorAndOpacity(AccentColor * FLinearColor(1, 1, 1, ScanAlpha));
		}
	}
}

FReply UZonefallMainMenuWidget::NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
	const FKey Key = InKeyEvent.GetKey();

	if (Key == EKeys::P)
	{
		PerformAction(EZonefallMenuAction::OpenSettings);
		return FReply::Handled();
	}
	if (Key == EKeys::Escape)
	{
		PerformAction(EZonefallMenuAction::QuitGame);
		return FReply::Handled();
	}

	// --- Controller / keyboard tab navigation (AAA feel) ---
	const int32 TabCount = FMath::Max(1, Tabs.Num());

	// Previous tab: Left / Q / Gamepad Left Shoulder.
	if (Key == EKeys::Left || Key == EKeys::Q || Key == EKeys::Gamepad_LeftShoulder)
	{
		ShowPage((ActivePage - 1 + TabCount) % TabCount);
		return FReply::Handled();
	}
	// Next tab: Right / E / Gamepad Right Shoulder.
	if (Key == EKeys::Right || Key == EKeys::E || Key == EKeys::Gamepad_RightShoulder)
	{
		ShowPage((ActivePage + 1) % TabCount);
		return FReply::Handled();
	}

	// Number keys 1..N jump straight to a tab.
	static const FKey NumberKeys[] = { EKeys::One, EKeys::Two, EKeys::Three, EKeys::Four, EKeys::Five, EKeys::Six };
	for (int32 i = 0; i < UE_ARRAY_COUNT(NumberKeys) && i < TabCount; ++i)
	{
		if (Key == NumberKeys[i])
		{
			ShowPage(i);
			return FReply::Handled();
		}
	}

	// Enter / Space / gamepad A = activate the page's primary action (PLAY, PLAY ONLINE, ...).
	if (Key == EKeys::Enter || Key == EKeys::SpaceBar || Key == EKeys::Gamepad_FaceButton_Bottom)
	{
		if (CurrentCards.Num() > 0)
		{
			PerformAction(CurrentCards[0].Action);
			return FReply::Handled();
		}
	}

	return Super::NativeOnKeyDown(InGeometry, InKeyEvent);
}

UUIWorldMenuGameInstance* UZonefallMainMenuWidget::ResolveGameInstance() const
{
	if (UWorld* World = GetWorld())
	{
		return World->GetGameInstance<UUIWorldMenuGameInstance>();
	}
	return nullptr;
}

void UZonefallMainMenuWidget::BuildLayout()
{
	if (!WidgetTree)
	{
		return;
	}

	UZonefallLocalizationSubsystem* Loc = UZonefallLocalizationSubsystem::Get(this);
	auto LT = [Loc](const TCHAR* Key, const FText& Fallback) -> FText
	{
		return Loc ? Loc->GetText(FName(Key)) : Fallback;
	};

	Tabs.Reset();
	Tabs.Add({ LT(TEXT("menu.home"),     NSLOCTEXT("ZonefallMenu", "TabHome", "HOME")), EZonefallMenuAction::GoHome });
	Tabs.Add({ LT(TEXT("menu.story"),    NSLOCTEXT("ZonefallMenu", "TabStory", "STORY")), EZonefallMenuAction::GoStory });
	Tabs.Add({ LT(TEXT("menu.online"),   NSLOCTEXT("ZonefallMenu", "TabOnline", "ONLINE")), EZonefallMenuAction::GoOnline });
	Tabs.Add({ LT(TEXT("menu.whatsnew"), NSLOCTEXT("ZonefallMenu", "TabWhatsNew", "WHAT'S NEW")), EZonefallMenuAction::GoWhatsNew });
	Tabs.Add({ LT(TEXT("menu.dlc"),      NSLOCTEXT("ZonefallMenu", "TabDLC", "DLC")), EZonefallMenuAction::GoDLC });
	Tabs.Add({ LT(TEXT("menu.settings"), NSLOCTEXT("ZonefallMenu", "TabSettings", "SETTINGS")), EZonefallMenuAction::GoSettingsTab });

	WidgetTree->RootWidget = nullptr;

	UBorder* RootBorder = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("MenuRoot"));
	RootBorder->SetBrush(FSlateRoundedBoxBrush(FLinearColor(0.02f, 0.03f, 0.05f, 1.0f), 0.0f));
	RootBorder->SetPadding(FMargin(0.0f));
	WidgetTree->RootWidget = RootBorder;

	RootOverlay = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass(), TEXT("MenuOverlay"));
	RootBorder->SetContent(RootOverlay);

	// --- Background image ---
	BackgroundImageWidget = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("MenuBg"));
	BackgroundImageWidget->SetColorAndOpacity(BackgroundTint);
	if (!BackgroundImage.IsNull())
	{
		if (UTexture2D* Tex = BackgroundImage.LoadSynchronous())
		{
			BackgroundImageWidget->SetBrushFromTexture(Tex, true);
		}
	}
	if (UOverlaySlot* BgSlot = RootOverlay->AddChildToOverlay(BackgroundImageWidget))
	{
		BgSlot->SetHorizontalAlignment(HAlign_Fill);
		BgSlot->SetVerticalAlignment(VAlign_Fill);
	}

	// --- Dim layer for legibility ---
	UBorder* Dim = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("MenuDim"));
	Dim->SetBrush(FSlateRoundedBoxBrush(FLinearColor(0.0f, 0.0f, 0.0f, 0.45f), 0.0f));
	if (UOverlaySlot* DimSlot = RootOverlay->AddChildToOverlay(Dim))
	{
		DimSlot->SetHorizontalAlignment(HAlign_Fill);
		DimSlot->SetVerticalAlignment(VAlign_Fill);
	}

	// --- Cyberpunk accent lines + scan sweep (animated in NativeTick) ---
	auto MakeLine = [this](FName Name, float Thickness) -> UImage*
	{
		UImage* Line = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), Name);
		FSlateRoundedBoxBrush Brush(FLinearColor::White, 1.0f);
		Brush.ImageSize = FVector2D(512.0f, Thickness);
		Line->SetBrush(Brush);
		Line->SetColorAndOpacity(AccentColor);
		return Line;
	};

	AccentLineTop = MakeLine(TEXT("MenuAccentTop"), 2.0f);
	if (UOverlaySlot* S = RootOverlay->AddChildToOverlay(AccentLineTop))
	{
		S->SetHorizontalAlignment(HAlign_Fill);
		S->SetVerticalAlignment(VAlign_Top);
		S->SetPadding(FMargin(0.0f, 100.0f, 0.0f, 0.0f));
	}

	AccentLineBottom = MakeLine(TEXT("MenuAccentBottom"), 2.0f);
	if (UOverlaySlot* S = RootOverlay->AddChildToOverlay(AccentLineBottom))
	{
		S->SetHorizontalAlignment(HAlign_Fill);
		S->SetVerticalAlignment(VAlign_Bottom);
		S->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 220.0f));
	}

	// Vertical scan bar that sweeps across the screen.
	ScanBar = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("MenuScanBar"));
	{
		FSlateRoundedBoxBrush Brush(FLinearColor::White, 0.0f);
		Brush.ImageSize = FVector2D(3.0f, 8.0f);
		ScanBar->SetBrush(Brush);
		ScanBar->SetColorAndOpacity(AccentColor * FLinearColor(1, 1, 1, 0.18f));
	}
	if (UOverlaySlot* S = RootOverlay->AddChildToOverlay(ScanBar))
	{
		S->SetHorizontalAlignment(HAlign_Left);
		S->SetVerticalAlignment(VAlign_Fill);
	}

	// --- Top tab bar (top-left) ---
	TabBar = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("MenuTabBar"));
	for (int32 i = 0; i < Tabs.Num(); ++i)
	{
		UZonefallMenuItemButton* Tab = WidgetTree->ConstructWidget<UZonefallMenuItemButton>(UZonefallMenuItemButton::StaticClass());
		Tab->ItemId = i;
		StyleMenuButton(Tab, FLinearColor(0.06f, 0.08f, 0.11f, 0.6f), AccentColor * 0.5f, AccentColor * 0.7f);
		UTextBlock* TabLabel = MakeMenuText(WidgetTree, Tabs[i].Label, BodyFontSize + 2, FLinearColor::White);
		Tab->AddChild(TabLabel);
		if (UButtonSlot* LSlot = Cast<UButtonSlot>(TabLabel->Slot))
		{
			LSlot->SetHorizontalAlignment(HAlign_Center);
			LSlot->SetVerticalAlignment(VAlign_Center);
			LSlot->SetPadding(FMargin(22.0f, 10.0f));
		}
		Tab->OnItemClicked.AddDynamic(this, &UZonefallMainMenuWidget::HandleTabClicked);
		TabButtons.Add(Tab);
		if (UHorizontalBoxSlot* TS = TabBar->AddChildToHorizontalBox(Tab))
		{
			TS->SetPadding(FMargin(0, 0, 6, 0));
		}
	}
	if (UOverlaySlot* TabSlot = RootOverlay->AddChildToOverlay(TabBar))
	{
		TabSlot->SetHorizontalAlignment(HAlign_Left);
		TabSlot->SetVerticalAlignment(VAlign_Top);
		TabSlot->SetPadding(FMargin(48.0f, 40.0f, 0.0f, 0.0f));
	}

	// --- Hero (left-center) ---
	UVerticalBox* HeroBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("MenuHero"));

	HeroTitle = MakeMenuText(WidgetTree, GameTitle, TitleFontSize, FLinearColor::White);
	{
		// Wide letter spacing reads as a premium AAA title.
		FSlateFontInfo TitleFont = HeroTitle->GetFont();
		TitleFont.LetterSpacing = 160;
		HeroTitle->SetFont(TitleFont);
	}
	HeroTitle->SetShadowOffset(FVector2D(3.0f, 4.0f));
	HeroTitle->SetShadowColorAndOpacity(FLinearColor(0, 0, 0, 0.85f));
	HeroBox->AddChildToVerticalBox(HeroTitle);

	// Glowing accent underline beneath the title.
	{
		UImage* TitleUnderline = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("MenuHeroUnderline"));
		FSlateRoundedBoxBrush UnderlineBrush(FLinearColor::White, 2.0f);
		UnderlineBrush.ImageSize = FVector2D(220.0f, 4.0f);
		TitleUnderline->SetBrush(UnderlineBrush);
		TitleUnderline->SetColorAndOpacity(AccentColor);
		if (UVerticalBoxSlot* UnderlineSlot = HeroBox->AddChildToVerticalBox(TitleUnderline))
		{
			UnderlineSlot->SetPadding(FMargin(2, 14, 0, 0));
			UnderlineSlot->SetHorizontalAlignment(HAlign_Left);
		}
	}

	HeroDescription = MakeMenuText(WidgetTree, FText::GetEmpty(), BodyFontSize, FLinearColor(0.82f, 0.88f, 0.94f, 1.0f));
	HeroDescription->SetAutoWrapText(true);
	if (UVerticalBoxSlot* DescSlot = HeroBox->AddChildToVerticalBox(HeroDescription))
	{
		DescSlot->SetPadding(FMargin(0, 14, 0, 0));
	}

	HeroSizeBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("MenuHeroSize"));
	HeroSizeBox->SetWidthOverride(620.0f);
	HeroSizeBox->AddChild(HeroBox);
	if (UOverlaySlot* HeroSlot = RootOverlay->AddChildToOverlay(HeroSizeBox))
	{
		HeroSlot->SetHorizontalAlignment(HAlign_Left);
		HeroSlot->SetVerticalAlignment(VAlign_Center);
		HeroSlot->SetPadding(FMargin(56.0f, 0.0f, 0.0f, 80.0f));
	}

	// --- Info panel (WHAT'S NEW / DLC) — hidden unless those tabs are active ---
	InfoPanel = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("MenuInfoPanel"));
	InfoPanel->SetBrush(FSlateRoundedBoxBrush(FLinearColor(0.04f, 0.07f, 0.11f, 0.92f), 12.0f));
	InfoPanel->SetPadding(FMargin(26.0f, 22.0f));
	InfoScroll = WidgetTree->ConstructWidget<UScrollBox>(UScrollBox::StaticClass(), TEXT("MenuInfoScroll"));
	InfoPanel->SetContent(InfoScroll);

	USizeBox* InfoSize = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("MenuInfoSize"));
	InfoSize->SetWidthOverride(760.0f);
	InfoSize->AddChild(InfoPanel);
	if (UOverlaySlot* InfoSlot = RootOverlay->AddChildToOverlay(InfoSize))
	{
		InfoSlot->SetHorizontalAlignment(HAlign_Left);
		InfoSlot->SetVerticalAlignment(VAlign_Fill);
		InfoSlot->SetPadding(FMargin(56.0f, 150.0f, 0.0f, 240.0f));
	}
	InfoPanel->SetVisibility(ESlateVisibility::Collapsed);

	// --- Bottom card row ---
	CardRow = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("MenuCardRow"));
	USizeBox* CardSize = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("MenuCardSize"));
	CardSize->SetHeightOverride(168.0f);
	CardSize->AddChild(CardRow);
	if (UOverlaySlot* CardSlot = RootOverlay->AddChildToOverlay(CardSize))
	{
		CardSlot->SetHorizontalAlignment(HAlign_Fill);
		CardSlot->SetVerticalAlignment(VAlign_Bottom);
		CardSlot->SetPadding(FMargin(48.0f, 0.0f, 48.0f, 36.0f));
	}

	// --- Control hints (bottom-right) ---
	UTextBlock* Hints = MakeMenuText(WidgetTree,
		NSLOCTEXT("ZonefallMenu", "Hints", "Select  [Enter]      Settings  [P]      Quit  [Esc]"),
		BodyFontSize - 2, FLinearColor(0.7f, 0.78f, 0.86f, 1.0f));
	if (UOverlaySlot* HintSlot = RootOverlay->AddChildToOverlay(Hints))
	{
		HintSlot->SetHorizontalAlignment(HAlign_Right);
		HintSlot->SetVerticalAlignment(VAlign_Bottom);
		HintSlot->SetPadding(FMargin(0.0f, 0.0f, 52.0f, 12.0f));
	}

	// --- Version + engine (bottom-left) ---
	VersionText = MakeMenuText(WidgetTree, FText::GetEmpty(), BodyFontSize - 3, FLinearColor(0.6f, 0.7f, 0.8f, 1.0f));
	if (UOverlaySlot* VerSlot = RootOverlay->AddChildToOverlay(VersionText))
	{
		VerSlot->SetHorizontalAlignment(HAlign_Left);
		VerSlot->SetVerticalAlignment(VAlign_Bottom);
		VerSlot->SetPadding(FMargin(52.0f, 0.0f, 0.0f, 12.0f));
	}

	// --- Online status indicator (top-right): coloured dot + ONLINE/OFFLINE + account name ---
	{
		UHorizontalBox* OnlineRow = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("OnlineRow"));

		OnlineDot = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("OnlineDot"));
		OnlineDot->SetBrush(FSlateRoundedBoxBrush(FLinearColor(0.5f, 0.5f, 0.5f, 1.0f), 8.0f));
		OnlineDot->SetDesiredSizeOverride(FVector2D(14.0f, 14.0f));
		if (UHorizontalBoxSlot* DotSlot = OnlineRow->AddChildToHorizontalBox(OnlineDot))
		{
			DotSlot->SetVerticalAlignment(VAlign_Center);
			DotSlot->SetPadding(FMargin(0.0f, 0.0f, 8.0f, 0.0f));
		}

		OnlineStatusText = MakeMenuText(WidgetTree, FText::GetEmpty(), BodyFontSize - 2, FLinearColor(0.8f, 0.85f, 0.9f, 1.0f));
		if (UHorizontalBoxSlot* TxtSlot = OnlineRow->AddChildToHorizontalBox(OnlineStatusText))
		{
			TxtSlot->SetVerticalAlignment(VAlign_Center);
		}

		if (UOverlaySlot* OnlineSlot = RootOverlay->AddChildToOverlay(OnlineRow))
		{
			OnlineSlot->SetHorizontalAlignment(HAlign_Right);
			OnlineSlot->SetVerticalAlignment(VAlign_Top);
			OnlineSlot->SetPadding(FMargin(0.0f, 18.0f, 52.0f, 0.0f));
		}
	}
}

void UZonefallMainMenuWidget::UpdateOnlineIndicator()
{
	if (!OnlineStatusText && !OnlineDot)
	{
		return;
	}

	UUIWorldMenuGameInstance* GI = ResolveGameInstance();

	const bool bLoggedIn = GI && GI->IsOnlineLoggedIn();
	const bool bAvailable = GI && GI->IsOnlineAvailable();

	// Keep it clean: just the Steam account name when signed in (no verbose status text).
	FString Display;
	if (bLoggedIn && GI)
	{
		Display = GI->GetOnlinePlayerNickname();
	}
	else if (bAvailable)
	{
		Display = TEXT("Connecting to Steam…");
	}
	else
	{
		Display = TEXT("Offline");
	}

	if (OnlineStatusText)
	{
		OnlineStatusText->SetText(FText::FromString(Display));
	}
	if (OnlineDot)
	{
		const FLinearColor DotColor = bLoggedIn
			? FLinearColor(0.30f, 0.92f, 0.45f, 1.0f)   // green = signed in to Steam
			: (bAvailable ? FLinearColor(0.96f, 0.78f, 0.30f, 1.0f)  // amber = Steam up, signing in
						  : FLinearColor(0.65f, 0.65f, 0.65f, 1.0f)); // grey = offline
		OnlineDot->SetBrush(FSlateRoundedBoxBrush(DotColor, 8.0f));
		OnlineDot->SetDesiredSizeOverride(FVector2D(14.0f, 14.0f));
	}
}

void UZonefallMainMenuWidget::GetCardsForPage(int32 PageIndex, FText& OutTitle, FText& OutDesc, TArray<FMenuCard>& OutCards) const
{
	OutCards.Reset();
	switch (PageIndex)
	{
	case 1: // STORY
		OutTitle = NSLOCTEXT("ZonefallMenu", "StoryTitle", "STORY MODE");
		OutDesc = NSLOCTEXT("ZonefallMenu", "StoryDesc", "Drop into the campaign. Survive the zone, manage your gear and push deeper into hostile territory.");
		OutCards.Add({ NSLOCTEXT("ZonefallMenu", "Play", "PLAY"), NSLOCTEXT("ZonefallMenu", "PlaySub", "Start the campaign"), EZonefallMenuAction::PlayStory });
		OutCards.Add({ NSLOCTEXT("ZonefallMenu", "Settings", "SETTINGS"), NSLOCTEXT("ZonefallMenu", "SettingsSub", "Graphics, audio, controls"), EZonefallMenuAction::OpenSettings });
		OutCards.Add({ NSLOCTEXT("ZonefallMenu", "Home", "HOME"), NSLOCTEXT("ZonefallMenu", "HomeSub", "Back to main"), EZonefallMenuAction::GoHome });
		OutCards.Add({ NSLOCTEXT("ZonefallMenu", "Quit", "QUIT"), NSLOCTEXT("ZonefallMenu", "QuitSub", "Exit to desktop"), EZonefallMenuAction::QuitGame });
		break;

	case 2: // ONLINE
		OutTitle = NSLOCTEXT("ZonefallMenu", "OnlineTitle", "ZONEFALL ONLINE");
		OutDesc = NSLOCTEXT("ZonefallMenu", "OnlineDesc", "Host or join sessions over LAN and online. Team up, fight and survive together in the zone.");
		OutCards.Add({ NSLOCTEXT("ZonefallMenu", "PlayOnline", "PLAY ONLINE"), NSLOCTEXT("ZonefallMenu", "PlayOnlineSub", "Browse & host games"), EZonefallMenuAction::OpenOnlineLobby });
		OutCards.Add({ NSLOCTEXT("ZonefallMenu", "Settings", "SETTINGS"), NSLOCTEXT("ZonefallMenu", "SettingsSub", "Graphics, audio, controls"), EZonefallMenuAction::OpenSettings });
		OutCards.Add({ NSLOCTEXT("ZonefallMenu", "Home", "HOME"), NSLOCTEXT("ZonefallMenu", "HomeSub", "Back to main"), EZonefallMenuAction::GoHome });
		OutCards.Add({ NSLOCTEXT("ZonefallMenu", "Quit", "QUIT"), NSLOCTEXT("ZonefallMenu", "QuitSub", "Exit to desktop"), EZonefallMenuAction::QuitGame });
		break;

	case 3: // WHAT'S NEW
		OutTitle = NSLOCTEXT("ZonefallMenu", "WhatsNewTitle", "WHAT'S NEW");
		OutDesc = NSLOCTEXT("ZonefallMenu", "WhatsNewDesc", "Latest updates and patch notes.");
		OutCards.Add({ NSLOCTEXT("ZonefallMenu", "Home", "HOME"), NSLOCTEXT("ZonefallMenu", "HomeSub", "Back to main"), EZonefallMenuAction::GoHome });
		OutCards.Add({ NSLOCTEXT("ZonefallMenu", "DLC", "DLC"), NSLOCTEXT("ZonefallMenu", "DLCSub", "Upcoming content"), EZonefallMenuAction::GoDLC });
		OutCards.Add({ NSLOCTEXT("ZonefallMenu", "Online", "ONLINE"), NSLOCTEXT("ZonefallMenu", "OnlineSub", "Multiplayer"), EZonefallMenuAction::GoOnline });
		OutCards.Add({ NSLOCTEXT("ZonefallMenu", "Quit", "QUIT"), NSLOCTEXT("ZonefallMenu", "QuitSub", "Exit to desktop"), EZonefallMenuAction::QuitGame });
		break;

	case 4: // DLC
		OutTitle = NSLOCTEXT("ZonefallMenu", "DLCTitle", "DOWNLOADABLE CONTENT");
		OutDesc = NSLOCTEXT("ZonefallMenu", "DLCDesc", "Expansions and add-ons coming to Zonefall.");
		OutCards.Add({ NSLOCTEXT("ZonefallMenu", "Home", "HOME"), NSLOCTEXT("ZonefallMenu", "HomeSub", "Back to main"), EZonefallMenuAction::GoHome });
		OutCards.Add({ NSLOCTEXT("ZonefallMenu", "WhatsNew", "WHAT'S NEW"), NSLOCTEXT("ZonefallMenu", "WhatsNewSub", "Patch notes"), EZonefallMenuAction::GoWhatsNew });
		OutCards.Add({ NSLOCTEXT("ZonefallMenu", "Online", "ONLINE"), NSLOCTEXT("ZonefallMenu", "OnlineSub", "Multiplayer"), EZonefallMenuAction::GoOnline });
		OutCards.Add({ NSLOCTEXT("ZonefallMenu", "Quit", "QUIT"), NSLOCTEXT("ZonefallMenu", "QuitSub", "Exit to desktop"), EZonefallMenuAction::QuitGame });
		break;

	case 5: // SETTINGS
		OutTitle = NSLOCTEXT("ZonefallMenu", "SettingsTitle", "SETTINGS");
		OutDesc = NSLOCTEXT("ZonefallMenu", "SettingsPageDesc", "Tune graphics quality, upscaling, audio levels and rebind your controls.");
		OutCards.Add({ NSLOCTEXT("ZonefallMenu", "OpenSettings", "OPEN SETTINGS"), NSLOCTEXT("ZonefallMenu", "OpenSettingsSub", "All options"), EZonefallMenuAction::OpenSettings });
		OutCards.Add({ NSLOCTEXT("ZonefallMenu", "Online", "ONLINE"), NSLOCTEXT("ZonefallMenu", "OnlineSub", "Multiplayer"), EZonefallMenuAction::GoOnline });
		OutCards.Add({ NSLOCTEXT("ZonefallMenu", "Home", "HOME"), NSLOCTEXT("ZonefallMenu", "HomeSub", "Back to main"), EZonefallMenuAction::GoHome });
		OutCards.Add({ NSLOCTEXT("ZonefallMenu", "Quit", "QUIT"), NSLOCTEXT("ZonefallMenu", "QuitSub", "Exit to desktop"), EZonefallMenuAction::QuitGame });
		break;

	default: // HOME
		OutTitle = GameTitle;
		OutDesc = NSLOCTEXT("ZonefallMenu", "HomeDesc", "Welcome to Zonefall Protocol. Choose Story to play the campaign, jump into Online with friends, or fine-tune your experience in Settings.");
		OutCards.Add({ NSLOCTEXT("ZonefallMenu", "Story", "STORY"), NSLOCTEXT("ZonefallMenu", "StorySub", "Play the campaign"), EZonefallMenuAction::GoStory });
		OutCards.Add({ NSLOCTEXT("ZonefallMenu", "Online", "ONLINE"), NSLOCTEXT("ZonefallMenu", "OnlineSub", "Host & join games"), EZonefallMenuAction::GoOnline });
		OutCards.Add({ NSLOCTEXT("ZonefallMenu", "Settings", "SETTINGS"), NSLOCTEXT("ZonefallMenu", "SettingsSub", "Graphics, audio, controls"), EZonefallMenuAction::OpenSettings });
		OutCards.Add({ NSLOCTEXT("ZonefallMenu", "Quit", "QUIT"), NSLOCTEXT("ZonefallMenu", "QuitSub", "Exit to desktop"), EZonefallMenuAction::QuitGame });
		break;
	}
}

void UZonefallMainMenuWidget::ShowPage(int32 PageIndex)
{
	ActivePage = FMath::Clamp(PageIndex, 0, Tabs.Num() - 1);

	FText Title, Desc;
	GetCardsForPage(ActivePage, Title, Desc, CurrentCards);

	if (HeroTitle) { HeroTitle->SetText(Title); }
	if (HeroDescription) { HeroDescription->SetText(Desc); }

	// WHAT'S NEW (3) and DLC (4) show the info panel instead of the hero banner.
	const bool bInfoPage = (ActivePage == 3 || ActivePage == 4);
	if (HeroSizeBox) { HeroSizeBox->SetVisibility(bInfoPage ? ESlateVisibility::Collapsed : ESlateVisibility::SelfHitTestInvisible); }
	if (InfoPanel) { InfoPanel->SetVisibility(bInfoPage ? ESlateVisibility::Visible : ESlateVisibility::Collapsed); }
	if (bInfoPage) { RebuildInfoPanel(ActivePage); }

	// Highlight the active tab.
	for (int32 i = 0; i < TabButtons.Num(); ++i)
	{
		if (TabButtons[i])
		{
			const bool bActive = (i == ActivePage);
			StyleMenuButton(TabButtons[i],
				bActive ? AccentColor * 0.65f : FLinearColor(0.06f, 0.08f, 0.11f, 0.6f),
				AccentColor * 0.5f, AccentColor * 0.7f);
		}
	}

	RebuildCards();
}

void UZonefallMainMenuWidget::RebuildCards()
{
	if (!CardRow)
	{
		return;
	}

	CardRow->ClearChildren();
	CardButtons.Reset();

	for (int32 i = 0; i < CurrentCards.Num(); ++i)
	{
		const FMenuCard& Card = CurrentCards[i];

		UZonefallMenuItemButton* CardBtn = WidgetTree->ConstructWidget<UZonefallMenuItemButton>(UZonefallMenuItemButton::StaticClass());
		CardBtn->ItemId = i;
		StyleMenuButton(CardBtn, FLinearColor(0.06f, 0.09f, 0.13f, 0.92f), FLinearColor(0.10f, 0.16f, 0.22f, 0.98f), AccentColor * 0.5f, 6.0f);

		UVerticalBox* CardContent = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());

		// Accent bar on top of each card.
		UImage* AccentBar = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass());
		{
			FSlateRoundedBoxBrush BarBrush(FLinearColor::White, 2.0f);
			BarBrush.ImageSize = FVector2D(64.0f, 4.0f);
			AccentBar->SetBrush(BarBrush);
			AccentBar->SetColorAndOpacity(AccentColor);
		}
		if (UVerticalBoxSlot* BarSlot = CardContent->AddChildToVerticalBox(AccentBar))
		{
			BarSlot->SetHorizontalAlignment(HAlign_Left);
			BarSlot->SetPadding(FMargin(0, 0, 0, 0));
		}

		// Spacer that pushes title to the bottom.
		UVerticalBox* Spacer = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
		if (UVerticalBoxSlot* SpSlot = CardContent->AddChildToVerticalBox(Spacer))
		{
			SpSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
		}

		UTextBlock* TitleText = MakeMenuText(WidgetTree, Card.Title, BodyFontSize + 4, FLinearColor::White);
		CardContent->AddChildToVerticalBox(TitleText);

		UTextBlock* SubText = MakeMenuText(WidgetTree, Card.Subtitle, BodyFontSize - 3, FLinearColor(0.7f, 0.78f, 0.86f, 1.0f));
		CardContent->AddChildToVerticalBox(SubText);

		CardBtn->AddChild(CardContent);
		if (UButtonSlot* CSlot = Cast<UButtonSlot>(CardContent->Slot))
		{
			CSlot->SetHorizontalAlignment(HAlign_Fill);
			CSlot->SetVerticalAlignment(VAlign_Fill);
			CSlot->SetPadding(FMargin(18.0f, 16.0f));
		}

		CardBtn->OnItemClicked.AddDynamic(this, &UZonefallMainMenuWidget::HandleCardClicked);
		CardButtons.Add(CardBtn);

		if (UHorizontalBoxSlot* HS = CardRow->AddChildToHorizontalBox(CardBtn))
		{
			HS->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
			HS->SetPadding(FMargin(i == 0 ? 0.0f : 8.0f, 0.0f, 0.0f, 0.0f));
		}
	}
}

void UZonefallMainMenuWidget::HandleTabClicked(int32 TabId)
{
	if (Tabs.IsValidIndex(TabId))
	{
		PerformAction(Tabs[TabId].TabAction);
	}
}

void UZonefallMainMenuWidget::HandleCardClicked(int32 CardId)
{
	if (CurrentCards.IsValidIndex(CardId))
	{
		PerformAction(CurrentCards[CardId].Action);
	}
}

void UZonefallMainMenuWidget::PerformAction(EZonefallMenuAction Action)
{
	UUIWorldMenuGameInstance* GI = ResolveGameInstance();

	switch (Action)
	{
	case EZonefallMenuAction::GoHome:        ShowPage(0); break;
	case EZonefallMenuAction::GoStory:       ShowPage(1); break;
	case EZonefallMenuAction::GoOnline:      ShowPage(2); break;
	case EZonefallMenuAction::GoWhatsNew:    ShowPage(3); break;
	case EZonefallMenuAction::GoDLC:         ShowPage(4); break;
	case EZonefallMenuAction::GoSettingsTab: ShowPage(5); break;

	case EZonefallMenuAction::PlayStory:
		if (GI) { GI->OpenCharacterCreator(); }
		break;
	case EZonefallMenuAction::OpenOnlineLobby:
		if (GI) { GI->ShowMenuFromList(EUIWorldMenuScreen::OnlineMenu, false); }
		break;
	case EZonefallMenuAction::OpenSettings:
		if (GI) { GI->ShowMenuFromList(EUIWorldMenuScreen::SettingsMenu, false); }
		break;
	case EZonefallMenuAction::QuitGame:
		if (GI) { GI->QuitGameNow(false); }
		break;

	default:
		break;
	}
}

void UZonefallMainMenuWidget::RebuildInfoPanel(int32 PageIndex)
{
	if (!InfoScroll)
	{
		return;
	}
	InfoScroll->ClearChildren();

	UVerticalBox* Box = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
	InfoScroll->AddChild(Box);

	if (PageIndex == 4)
	{
		BuildDLCInto(Box);
	}
	else
	{
		BuildPatchNotesInto(Box);
	}
}

void UZonefallMainMenuWidget::BuildPatchNotesInto(UVerticalBox* Box)
{
	if (!Box)
	{
		return;
	}

	// Header.
	UTextBlock* Header = MakeMenuText(WidgetTree, NSLOCTEXT("ZonefallMenu", "PatchHeader", "WHAT'S NEW"), TitleFontSize - 10, FLinearColor::White);
	if (UVerticalBoxSlot* HS = Box->AddChildToVerticalBox(Header)) { HS->SetPadding(FMargin(0, 0, 0, 14)); }

	// Use configured notes, or sensible defaults so the tab is never empty.
	TArray<FZonefallPatchEntry> Entries = PatchNotes;
	if (Entries.Num() == 0)
	{
		FZonefallPatchEntry E1;
		E1.Version = TEXT("v1.0.0");
		E1.Date = TEXT("Latest");
		E1.Changes = {
			TEXT("New GTA-style main menu with tabs, hero banner and animated background"),
			TEXT("Full settings: per-group graphics quality, DLSS / FSR, RTX, DirectX 11/12"),
			TEXT("Volumetric cloud quality option"),
			TEXT("Replicated inventory with world pickups and drop"),
			TEXT("Rebuilt pause menu with in-game saving + side save toast"),
			TEXT("Redesigned shader-compilation and loading screens")
		};
		FZonefallPatchEntry E2;
		E2.Version = TEXT("v0.9.0");
		E2.Date = TEXT("Beta");
		E2.Changes = {
			TEXT("Online lobby: host and join LAN/online sessions with map select"),
			TEXT("Weather system performance optimization"),
			TEXT("Third / first person player character with Enhanced Input")
		};
		Entries = { E1, E2 };
	}

	for (const FZonefallPatchEntry& Entry : Entries)
	{
		// Version + date row.
		UHorizontalBox* VerRow = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
		UTextBlock* Ver = MakeMenuText(WidgetTree, FText::FromString(Entry.Version), BodyFontSize + 6, AccentColor);
		if (UHorizontalBoxSlot* VS = VerRow->AddChildToHorizontalBox(Ver)) { VS->SetVerticalAlignment(VAlign_Bottom); }
		UTextBlock* Date = MakeMenuText(WidgetTree, FText::FromString(FString::Printf(TEXT("   %s"), *Entry.Date)), BodyFontSize - 2, FLinearColor(0.6f, 0.7f, 0.8f, 1.0f));
		if (UHorizontalBoxSlot* DS = VerRow->AddChildToHorizontalBox(Date)) { DS->SetVerticalAlignment(VAlign_Bottom); }
		if (UVerticalBoxSlot* VRS = Box->AddChildToVerticalBox(VerRow)) { VRS->SetPadding(FMargin(0, 6, 0, 6)); }

		// Accent divider.
		UImage* Divider = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass());
		{
			FSlateRoundedBoxBrush DivBrush(FLinearColor::White, 1.0f);
			DivBrush.ImageSize = FVector2D(680.0f, 2.0f);
			Divider->SetBrush(DivBrush);
			Divider->SetColorAndOpacity(AccentColor * FLinearColor(1, 1, 1, 0.5f));
		}
		if (UVerticalBoxSlot* DivS = Box->AddChildToVerticalBox(Divider)) { DivS->SetPadding(FMargin(0, 0, 0, 8)); }

		for (const FString& Change : Entry.Changes)
		{
			UTextBlock* Line = MakeMenuText(WidgetTree, FText::FromString(FString::Printf(TEXT("•  %s"), *Change)), BodyFontSize - 1, FLinearColor(0.85f, 0.9f, 0.96f, 1.0f));
			Line->SetAutoWrapText(true);
			if (UVerticalBoxSlot* LS = Box->AddChildToVerticalBox(Line)) { LS->SetPadding(FMargin(8, 2, 0, 2)); }
		}

		// Spacer between versions.
		UTextBlock* Spacer = MakeMenuText(WidgetTree, FText::GetEmpty(), BodyFontSize, FLinearColor::White);
		if (UVerticalBoxSlot* SS = Box->AddChildToVerticalBox(Spacer)) { SS->SetPadding(FMargin(0, 8, 0, 8)); }
	}
}

void UZonefallMainMenuWidget::BuildDLCInto(UVerticalBox* Box)
{
	if (!Box)
	{
		return;
	}

	UTextBlock* Header = MakeMenuText(WidgetTree, NSLOCTEXT("ZonefallMenu", "DLCHeader", "DOWNLOADABLE CONTENT"), TitleFontSize - 10, FLinearColor::White);
	if (UVerticalBoxSlot* HS = Box->AddChildToVerticalBox(Header)) { HS->SetPadding(FMargin(0, 0, 0, 16)); }

	TArray<FZonefallDLCEntry> Entries = UpcomingDLC;
	if (Entries.Num() == 0)
	{
		FZonefallDLCEntry D1; D1.Title = TEXT("Episode 1: The Breach");   D1.Status = TEXT("COMING SOON"); D1.Description = TEXT("A new region with fresh threats, gear and a story chapter.");
		FZonefallDLCEntry D2; D2.Title = TEXT("Co-op Raids");             D2.Status = TEXT("PLANNED");     D2.Description = TEXT("Four-player cooperative endgame missions.");
		FZonefallDLCEntry D3; D3.Title = TEXT("Vehicles Update");         D3.Status = TEXT("PLANNED");     D3.Description = TEXT("Drivable vehicles to traverse the zone.");
		Entries = { D1, D2, D3 };
	}

	for (const FZonefallDLCEntry& Entry : Entries)
	{
		UBorder* Card = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass());
		Card->SetBrush(FSlateRoundedBoxBrush(FLinearColor(0.07f, 0.11f, 0.16f, 0.95f), 8.0f));
		Card->SetPadding(FMargin(16.0f, 14.0f));

		UVerticalBox* CardBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
		Card->SetContent(CardBox);

		UHorizontalBox* TitleRow = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
		UTextBlock* Title = MakeMenuText(WidgetTree, FText::FromString(Entry.Title), BodyFontSize + 5, FLinearColor::White);
		if (UHorizontalBoxSlot* TS = TitleRow->AddChildToHorizontalBox(Title)) { TS->SetSize(FSlateChildSize(ESlateSizeRule::Fill)); TS->SetVerticalAlignment(VAlign_Center); }
		UTextBlock* Status = MakeMenuText(WidgetTree, FText::FromString(Entry.Status), BodyFontSize - 2, AccentColor);
		if (UHorizontalBoxSlot* StS = TitleRow->AddChildToHorizontalBox(Status)) { StS->SetVerticalAlignment(VAlign_Center); }
		CardBox->AddChildToVerticalBox(TitleRow);

		UTextBlock* Desc = MakeMenuText(WidgetTree, FText::FromString(Entry.Description), BodyFontSize - 1, FLinearColor(0.78f, 0.85f, 0.92f, 1.0f));
		Desc->SetAutoWrapText(true);
		if (UVerticalBoxSlot* DS = CardBox->AddChildToVerticalBox(Desc)) { DS->SetPadding(FMargin(0, 6, 0, 0)); }

		if (UVerticalBoxSlot* CardSlot = Box->AddChildToVerticalBox(Card)) { CardSlot->SetPadding(FMargin(0, 0, 0, 12)); }
	}
}
