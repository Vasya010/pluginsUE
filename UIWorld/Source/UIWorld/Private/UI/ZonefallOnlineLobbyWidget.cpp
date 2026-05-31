#include "ZonefallOnlineLobbyWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Brushes/SlateRoundedBoxBrush.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/ButtonSlot.h"
#include "Components/CheckBox.h"
#include "Components/ComboBoxString.h"
#include "Components/EditableTextBox.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Image.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/ProgressBar.h"
#include "Components/ScrollBox.h"
#include "Components/ScrollBoxSlot.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Engine/Font.h"
#include "Styling/SlateTypes.h"
#include "TimerManager.h"
#include "UIWorldMenuGameInstance.h"

namespace
{
	// Shared glassmorphism palette (matches the settings screen).
	namespace ZGlass
	{
		static const FLinearColor Panel       = FLinearColor(1.0f, 1.0f, 1.0f, 0.05f);
		static const FLinearColor PanelStrong = FLinearColor(1.0f, 1.0f, 1.0f, 0.08f);
		static const FLinearColor Row         = FLinearColor(1.0f, 1.0f, 1.0f, 0.035f);
		static const FLinearColor RowHover    = FLinearColor(1.0f, 1.0f, 1.0f, 0.09f);
		static const FLinearColor Outline     = FLinearColor(1.0f, 1.0f, 1.0f, 0.14f);
		static const FLinearColor OutlineSoft = FLinearColor(1.0f, 1.0f, 1.0f, 0.07f);
		static const FLinearColor TextPrimary = FLinearColor(0.94f, 0.97f, 1.0f, 1.0f);
		static const FLinearColor TextSecondary = FLinearColor(0.66f, 0.74f, 0.84f, 1.0f);
		static const FLinearColor Backdrop    = FLinearColor(0.012f, 0.022f, 0.04f, 0.92f);
		static const FLinearColor MenuFill    = FLinearColor(0.03f, 0.05f, 0.09f, 0.98f);
	}

	FSlateFontInfo MakeFont(int32 Size)
	{
		FSlateFontInfo Font;
		Font.Size = FMath::Clamp(Size, 8, 96);
		Font.FontObject = LoadObject<UObject>(nullptr, TEXT("/Engine/EngineFonts/Roboto.Roboto"));
		return Font;
	}

	UTextBlock* MakeText(UWidgetTree* Tree, FName Name, const FText& InText, int32 Size, FLinearColor Color)
	{
		UTextBlock* T = Tree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), Name);
		T->SetText(InText);
		T->SetFont(MakeFont(Size));
		T->SetColorAndOpacity(FSlateColor(Color));
		return T;
	}

	UBorder* MakeGlassPanel(UWidgetTree* Tree, FName Name, const FLinearColor& Fill, float Radius, const FLinearColor& OutlineColor, float OutlineWidth = 1.0f)
	{
		UBorder* B = Tree->ConstructWidget<UBorder>(UBorder::StaticClass(), Name);
		B->SetBrush(FSlateRoundedBoxBrush(Fill, Radius, OutlineColor, OutlineWidth));
		return B;
	}

	FLinearColor PingColor(int32 PingMs)
	{
		if (PingMs < 0)
		{
			return FLinearColor(0.7f, 0.7f, 0.7f, 1.0f);
		}
		if (PingMs <= 60) return FLinearColor(0.3f, 0.92f, 0.45f, 1.0f);   // green
		if (PingMs <= 140) return FLinearColor(0.96f, 0.84f, 0.32f, 1.0f); // yellow
		return FLinearColor(0.95f, 0.4f, 0.4f, 1.0f);                       // red
	}

	void StyleButton(UButton* B, FLinearColor Base, FLinearColor Hover, FLinearColor Pressed)
	{
		if (!B)
		{
			return;
		}
		FButtonStyle Style = B->GetStyle();
		Style.Normal   = FSlateRoundedBoxBrush(Base,    8.0f, ZGlass::OutlineSoft, 1.0f);
		Style.Hovered  = FSlateRoundedBoxBrush(Hover,   8.0f, ZGlass::Outline,     1.0f);
		Style.Pressed  = FSlateRoundedBoxBrush(Pressed, 8.0f, ZGlass::Outline,     1.0f);
		Style.Disabled = FSlateRoundedBoxBrush(FLinearColor(1.0f, 1.0f, 1.0f, 0.03f), 8.0f, ZGlass::OutlineSoft, 1.0f);
		B->SetStyle(Style);
	}

	UButton* MakeIconButton(UWidgetTree* Tree, FName Name, const FText& Label, int32 FontSize, FLinearColor BgColor, FLinearColor AccentColor)
	{
		UButton* B = Tree->ConstructWidget<UButton>(UButton::StaticClass(), Name);
		StyleButton(B, BgColor, BgColor * 1.3f + FLinearColor(1.0f, 1.0f, 1.0f, 0.04f), AccentColor);
		UTextBlock* T = Tree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), FName(*(Name.ToString() + TEXT("_Label"))));
		T->SetText(Label);
		T->SetFont(MakeFont(FontSize));
		T->SetColorAndOpacity(FSlateColor(ZGlass::TextPrimary));
		B->AddChild(T);
		if (UButtonSlot* Slot = Cast<UButtonSlot>(T->Slot))
		{
			Slot->SetHorizontalAlignment(HAlign_Center);
			Slot->SetVerticalAlignment(VAlign_Center);
			Slot->SetPadding(FMargin(16.0f, 9.0f));
		}
		return B;
	}

	// Frosted combo box matching the settings screen.
	void StyleGlassCombo(UComboBoxString* Combo)
	{
		if (!Combo) { return; }

		FComboBoxStyle ComboStyle = Combo->GetWidgetStyle();
		FButtonStyle Btn = ComboStyle.ComboButtonStyle.ButtonStyle;
		Btn.SetNormal  (FSlateRoundedBoxBrush(FLinearColor(1.0f, 1.0f, 1.0f, 0.05f), 8.0f, ZGlass::OutlineSoft, 1.0f));
		Btn.SetHovered (FSlateRoundedBoxBrush(FLinearColor(1.0f, 1.0f, 1.0f, 0.10f), 8.0f, ZGlass::Outline,     1.0f));
		Btn.SetPressed (FSlateRoundedBoxBrush(FLinearColor(1.0f, 1.0f, 1.0f, 0.07f), 8.0f, ZGlass::Outline,     1.0f));
		Btn.NormalPadding  = FMargin(14.0f, 7.0f);
		Btn.PressedPadding = FMargin(14.0f, 8.0f, 14.0f, 6.0f);
		ComboStyle.ComboButtonStyle.SetButtonStyle(Btn);
		ComboStyle.ComboButtonStyle.SetContentPadding(FMargin(10.0f, 5.0f));
		ComboStyle.ComboButtonStyle.SetMenuBorderBrush(FSlateRoundedBoxBrush(ZGlass::MenuFill, 8.0f, ZGlass::Outline, 1.0f));
		ComboStyle.ComboButtonStyle.SetMenuBorderPadding(FMargin(6.0f));
		Combo->SetWidgetStyle(ComboStyle);

		FTableRowStyle Row = Combo->GetItemStyle();
		Row.SetInactiveBrush(FSlateRoundedBoxBrush(FLinearColor(1.0f, 1.0f, 1.0f, 0.04f), 6.0f));
		Row.SetInactiveHoveredBrush(FSlateRoundedBoxBrush(FLinearColor(1.0f, 1.0f, 1.0f, 0.10f), 6.0f));
		Row.SetEvenRowBackgroundBrush(FSlateRoundedBoxBrush(ZGlass::MenuFill, 4.0f));
		Row.SetEvenRowBackgroundHoveredBrush(FSlateRoundedBoxBrush(FLinearColor(1.0f, 1.0f, 1.0f, 0.08f), 4.0f));
		Row.SetOddRowBackgroundBrush(FSlateRoundedBoxBrush(ZGlass::MenuFill, 4.0f));
		Row.SetOddRowBackgroundHoveredBrush(FSlateRoundedBoxBrush(FLinearColor(1.0f, 1.0f, 1.0f, 0.08f), 4.0f));
		Combo->SetItemStyle(Row);
	}

	// Rounded toggle for the LAN checkbox.
	void StyleGlassCheck(UCheckBox* Check, const FLinearColor& Accent)
	{
		if (!Check) { return; }
		FCheckBoxStyle Style = Check->GetWidgetStyle();
		Style.SetCheckBoxType(ESlateCheckBoxType::ToggleButton);
		Style.SetUncheckedImage(FSlateRoundedBoxBrush(FLinearColor(1.0f, 1.0f, 1.0f, 0.06f), 6.0f, ZGlass::Outline, 1.0f));
		Style.SetUncheckedHoveredImage(FSlateRoundedBoxBrush(FLinearColor(1.0f, 1.0f, 1.0f, 0.12f), 6.0f, ZGlass::Outline, 1.0f));
		Style.SetUncheckedPressedImage(FSlateRoundedBoxBrush(FLinearColor(1.0f, 1.0f, 1.0f, 0.12f), 6.0f, ZGlass::Outline, 1.0f));
		Style.SetCheckedImage(FSlateRoundedBoxBrush(Accent, 6.0f, ZGlass::Outline, 1.0f));
		Style.SetCheckedHoveredImage(FSlateRoundedBoxBrush(Accent * 1.1f, 6.0f, ZGlass::Outline, 1.0f));
		Style.SetCheckedPressedImage(FSlateRoundedBoxBrush(Accent, 6.0f, ZGlass::Outline, 1.0f));
		Style.SetPadding(FMargin(14.0f, 9.0f));
		Check->SetWidgetStyle(Style);
	}
}

UZonefallSessionCardButton::UZonefallSessionCardButton()
{
	OnClicked.AddDynamic(this, &UZonefallSessionCardButton::HandleInternalClicked);
}

void UZonefallSessionCardButton::HandleInternalClicked()
{
	OnCardClicked.Broadcast(CardIndex);
}

UZonefallOnlineLobbyWidget::UZonefallOnlineLobbyWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

FName UZonefallOnlineLobbyWidget::GetSelectedMapName() const
{
	if (MapSelectBox)
	{
		const FString Selected = MapSelectBox->GetSelectedOption();
		if (!Selected.IsEmpty())
		{
			return FName(*Selected);
		}
	}

	// Fall back to whatever the game instance is configured to host.
	if (const UUIWorldMenuGameInstance* GI = ResolveGameInstance())
	{
		if (!GI->OnlineHostMapName.IsNone())
		{
			return GI->OnlineHostMapName;
		}
	}
	return NAME_None;
}

TSharedRef<SWidget> UZonefallOnlineLobbyWidget::RebuildWidget()
{
	if (!WidgetTree || !WidgetTree->RootWidget)
	{
		BuildLayout();
	}
	return Super::RebuildWidget();
}

void UZonefallOnlineLobbyWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (UUIWorldMenuGameInstance* GI = ResolveGameInstance())
	{
		GI->OnSessionsFound.AddDynamic(this, &UZonefallOnlineLobbyWidget::HandleSessionsFound);
		GI->OnHostCompleted.AddDynamic(this, &UZonefallOnlineLobbyWidget::HandleHostCompleted);
		GI->OnJoinCompleted.AddDynamic(this, &UZonefallOnlineLobbyWidget::HandleJoinCompleted);
		GI->OnLeaveCompleted.AddDynamic(this, &UZonefallOnlineLobbyWidget::HandleLeaveCompleted);
		GI->OnOnlineMatchReady.AddDynamic(this, &UZonefallOnlineLobbyWidget::HandleOnlineMatchReady);
	}

	if (HostButton) HostButton->OnClicked.AddDynamic(this, &UZonefallOnlineLobbyWidget::HandleHostClicked);
	if (QuickJoinButton) QuickJoinButton->OnClicked.AddDynamic(this, &UZonefallOnlineLobbyWidget::HandleQuickJoinClicked);
	if (RefreshButton) RefreshButton->OnClicked.AddDynamic(this, &UZonefallOnlineLobbyWidget::HandleRefreshClicked);
	if (JoinButton) JoinButton->OnClicked.AddDynamic(this, &UZonefallOnlineLobbyWidget::HandleJoinClicked);
	if (LeaveButton) LeaveButton->OnClicked.AddDynamic(this, &UZonefallOnlineLobbyWidget::HandleLeaveClicked);
	if (OpenLevelButton) OpenLevelButton->OnClicked.AddDynamic(this, &UZonefallOnlineLobbyWidget::HandleOpenLevelClicked);
	if (JoinByIdButton) JoinByIdButton->OnClicked.AddDynamic(this, &UZonefallOnlineLobbyWidget::HandleJoinByIdClicked);
	if (BackButton) BackButton->OnClicked.AddDynamic(this, &UZonefallOnlineLobbyWidget::HandleBackClicked);
	if (ServerNameInput) ServerNameInput->OnTextCommitted.AddDynamic(this, &UZonefallOnlineLobbyWidget::HandleServerNameChanged);
	if (LanCheck) LanCheck->OnCheckStateChanged.AddDynamic(this, &UZonefallOnlineLobbyWidget::HandleLanCheckChanged);

	UpdateNetworkModeBadge();
	if (UUIWorldMenuGameInstance* GI = ResolveGameInstance())
	{
		UpdateSteamBanner();
	}
	SetStatusMessage(TEXT("Quick Join finds the best open session. Host first, then REFRESH or Quick Join."));

	if (bAutoRefreshOnConstruct)
	{
		RefreshSessionList();
	}

	if (AutoRefreshIntervalSeconds > 0.0f && GetWorld())
	{
		GetWorld()->GetTimerManager().SetTimer(
			AutoRefreshTimerHandle,
			this,
			&UZonefallOnlineLobbyWidget::HandleAutoRefreshTick,
			AutoRefreshIntervalSeconds,
			true);
	}
}

void UZonefallOnlineLobbyWidget::NativeDestruct()
{
	if (UUIWorldMenuGameInstance* GI = ResolveGameInstance())
	{
		GI->OnSessionsFound.RemoveDynamic(this, &UZonefallOnlineLobbyWidget::HandleSessionsFound);
		GI->OnHostCompleted.RemoveDynamic(this, &UZonefallOnlineLobbyWidget::HandleHostCompleted);
		GI->OnJoinCompleted.RemoveDynamic(this, &UZonefallOnlineLobbyWidget::HandleJoinCompleted);
		GI->OnLeaveCompleted.RemoveDynamic(this, &UZonefallOnlineLobbyWidget::HandleLeaveCompleted);
		GI->OnOnlineMatchReady.RemoveDynamic(this, &UZonefallOnlineLobbyWidget::HandleOnlineMatchReady);
	}

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(AutoRefreshTimerHandle);
	}

	Super::NativeDestruct();
}

UUIWorldMenuGameInstance* UZonefallOnlineLobbyWidget::ResolveGameInstance() const
{
	if (UWorld* World = GetWorld())
	{
		return World->GetGameInstance<UUIWorldMenuGameInstance>();
	}
	return nullptr;
}

void UZonefallOnlineLobbyWidget::BuildLayout()
{
	if (!WidgetTree)
	{
		return;
	}

	WidgetTree->RootWidget = nullptr;

	// Full-screen frosted backdrop.
	RootBorder = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("LobbyRoot"));
	RootBorder->SetBrush(FSlateRoundedBoxBrush(ZGlass::Backdrop, 0.0f));
	RootBorder->SetPadding(FMargin(56.0f, 40.0f));
	RootBorder->SetHorizontalAlignment(HAlign_Fill);
	RootBorder->SetVerticalAlignment(VAlign_Fill);
	WidgetTree->RootWidget = RootBorder;

	// Centered frosted glass card.
	UBorder* Card = MakeGlassPanel(WidgetTree, TEXT("LobbyCard"), ZGlass::PanelStrong, 20.0f, ZGlass::Outline, 1.0f);
	Card->SetPadding(FMargin(36.0f, 30.0f));
	RootBorder->SetContent(Card);

	UVerticalBox* MainBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("LobbyMainBox"));
	RootBox = MainBox;
	Card->SetContent(MainBox);

	// Title row: headline + mode badge (LAN / STEAM).
	UHorizontalBox* TitleRow = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("LobbyTitleRow"));
	TitleText = MakeText(WidgetTree, TEXT("LobbyTitle"), NSLOCTEXT("ZonefallLobby", "Title", "ONLINE"),
		TitleFontSize, ZGlass::TextPrimary);
	TitleText->SetShadowOffset(FVector2D(0.0f, 1.0f));
	TitleText->SetShadowColorAndOpacity(FLinearColor(0.0f, 0.0f, 0.0f, 0.6f));
	if (UHorizontalBoxSlot* TitleSlot = TitleRow->AddChildToHorizontalBox(TitleText))
	{
		TitleSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
		TitleSlot->SetVerticalAlignment(VAlign_Center);
	}
	ModeBadgeText = MakeText(
		WidgetTree,
		TEXT("ModeBadge"),
		NSLOCTEXT("ZonefallLobby", "ModeLAN", "LOCAL NETWORK"),
		BodyFontSize - 1,
		AccentColor);
	if (UHorizontalBoxSlot* BadgeSlot = TitleRow->AddChildToHorizontalBox(ModeBadgeText))
	{
		BadgeSlot->SetHorizontalAlignment(HAlign_Right);
		BadgeSlot->SetVerticalAlignment(VAlign_Center);
	}
	if (UVerticalBoxSlot* TitleRowSlot = MainBox->AddChildToVerticalBox(TitleRow))
	{
		TitleRowSlot->SetPadding(FMargin(2, 0, 0, 4));
	}

	SubtitleText = MakeText(
		WidgetTree,
		TEXT("LobbySubtitle"),
		NSLOCTEXT("ZonefallLobby", "Subtitle", "Join open worlds with up to 32 players — Quick Join, browse, or host your own session."),
		BodyFontSize,
		ZGlass::TextSecondary);
	if (UVerticalBoxSlot* SubSlot = MainBox->AddChildToVerticalBox(SubtitleText))
	{
		SubSlot->SetPadding(FMargin(2, 0, 0, 4));
	}

	// Steam status as a glass "pill" chip (modern look).
	PlayerBannerText = MakeText(
		WidgetTree,
		TEXT("PlayerBanner"),
		NSLOCTEXT("ZonefallLobby", "PlayerBanner", "Signed in"),
		BodyFontSize - 1,
		AccentColor * 0.9f);
	{
		UBorder* BannerPill = MakeGlassPanel(WidgetTree, TEXT("LobbySteamPill"), ZGlass::Panel, 10.0f, ZGlass::OutlineSoft, 1.0f);
		BannerPill->SetPadding(FMargin(12.0f, 6.0f));
		BannerPill->SetContent(PlayerBannerText);
		if (UVerticalBoxSlot* BannerSlot = MainBox->AddChildToVerticalBox(BannerPill))
		{
			BannerSlot->SetPadding(FMargin(2, 0, 0, 10));
			BannerSlot->SetHorizontalAlignment(HAlign_Left);
		}
	}

	UBorder* TitleRule = MakeGlassPanel(WidgetTree, TEXT("LobbyTitleRule"), AccentColor, 2.0f, AccentColor, 0.0f);
	if (USizeBox* RuleSize = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("LobbyTitleRuleSize")))
	{
		RuleSize->SetWidthOverride(120.0f);
		RuleSize->SetHeightOverride(3.0f);
		RuleSize->AddChild(TitleRule);
		if (UVerticalBoxSlot* RuleSlot = MainBox->AddChildToVerticalBox(RuleSize))
		{
			RuleSlot->SetPadding(FMargin(2, 0, 0, 16));
			RuleSlot->SetHorizontalAlignment(HAlign_Left);
		}
	}

	ContentRow = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("LobbyContentRow"));
	if (UVerticalBoxSlot* ContentRowSlot = MainBox->AddChildToVerticalBox(ContentRow))
	{
		ContentRowSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
		ContentRowSlot->SetPadding(FMargin(0, 0, 0, 14));
	}

	// --- Left column: host / settings ---
	UVerticalBox* LeftCol = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("LobbyLeftCol"));
	if (UHorizontalBoxSlot* LeftSlot = ContentRow->AddChildToHorizontalBox(LeftCol))
	{
		LeftSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
		LeftSlot->SetPadding(FMargin(0, 0, 14, 0));
	}

	UTextBlock* HostSectionLabel = MakeText(
		WidgetTree,
		TEXT("HostSectionLabel"),
		NSLOCTEXT("ZonefallLobby", "HostSection", "HOST MATCH"),
		BodyFontSize + 2,
		ZGlass::TextPrimary);
	if (UVerticalBoxSlot* HostLabelSlot = LeftCol->AddChildToVerticalBox(HostSectionLabel))
	{
		HostLabelSlot->SetPadding(FMargin(2, 0, 0, 8));
	}

	HostColumn = LeftCol;
	UBorder* HostBarPanel = MakeGlassPanel(WidgetTree, TEXT("HostBarPanel"), ZGlass::Panel, 12.0f, ZGlass::OutlineSoft, 1.0f);
	HostBarPanel->SetPadding(FMargin(12.0f, 10.0f));
	HostBar = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("HostBar"));
	HostBarPanel->SetContent(HostBar);

	ServerNameInput = WidgetTree->ConstructWidget<UEditableTextBox>(UEditableTextBox::StaticClass(), TEXT("ServerNameInput"));
	ServerNameInput->SetHintText(NSLOCTEXT("ZonefallLobby", "ServerNameHint", "Server name"));
	{
		FEditableTextBoxStyle Style = ServerNameInput->GetWidgetStyle();
		Style.SetBackgroundImageNormal(FSlateRoundedBoxBrush(FLinearColor(1.0f, 1.0f, 1.0f, 0.05f), 8.0f, ZGlass::OutlineSoft, 1.0f));
		Style.SetBackgroundImageHovered(FSlateRoundedBoxBrush(FLinearColor(1.0f, 1.0f, 1.0f, 0.10f), 8.0f, ZGlass::Outline, 1.0f));
		Style.SetBackgroundImageFocused(FSlateRoundedBoxBrush(FLinearColor(1.0f, 1.0f, 1.0f, 0.10f), 8.0f, AccentColor, 1.0f));
		Style.SetForegroundColor(FSlateColor(ZGlass::TextPrimary));
		Style.SetFont(MakeFont(BodyFontSize));
		ServerNameInput->SetWidgetStyle(Style);
	}
	if (UHorizontalBoxSlot* InputSlot = HostBar->AddChildToHorizontalBox(ServerNameInput))
	{
		InputSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
		InputSlot->SetPadding(FMargin(0, 0, 8, 0));
	}

	// Map selection — choose which level to host / open.
	MapSelectBox = WidgetTree->ConstructWidget<UComboBoxString>(UComboBoxString::StaticClass(), TEXT("MapSelectBox"));
	{
		// Seed from AvailableMaps, otherwise from the game instance's configured maps.
		TArray<FString> MapOptions = AvailableMaps;
		if (MapOptions.Num() == 0)
		{
			if (const UUIWorldMenuGameInstance* GI = ResolveGameInstance())
			{
				if (!GI->OnlineHostMapName.IsNone())
				{
					MapOptions.AddUnique(GI->OnlineHostMapName.ToString());
				}
				if (!GI->MainMenuLevelName.IsNone())
				{
					MapOptions.AddUnique(GI->MainMenuLevelName.ToString());
				}
			}
		}
		for (const FString& MapName : MapOptions)
		{
			MapSelectBox->AddOption(MapName);
		}
		if (MapOptions.Num() > 0)
		{
			MapSelectBox->SetSelectedOption(MapOptions[0]);
		}
		StyleGlassCombo(MapSelectBox);
	}
	if (UHorizontalBoxSlot* MapSlot = HostBar->AddChildToHorizontalBox(MapSelectBox))
	{
		MapSlot->SetPadding(FMargin(0, 0, 8, 0));
	}

	MaxPlayersBox = WidgetTree->ConstructWidget<UComboBoxString>(UComboBoxString::StaticClass(), TEXT("MaxPlayersBox"));
	for (int32 V : {2, 4, 8, 16, 30, 32})
	{
		MaxPlayersBox->AddOption(FString::FromInt(V));
	}
	MaxPlayersBox->SetSelectedOption(TEXT("16"));
	StyleGlassCombo(MaxPlayersBox);
	if (UHorizontalBoxSlot* MaxSlot = HostBar->AddChildToHorizontalBox(MaxPlayersBox))
	{
		MaxSlot->SetPadding(FMargin(0, 0, 8, 0));
	}

	LanCheck = WidgetTree->ConstructWidget<UCheckBox>(UCheckBox::StaticClass(), TEXT("LanCheck"));
	StyleGlassCheck(LanCheck, AccentColor * 0.6f);
	LanCheck->SetIsChecked(bLanByDefault);
	{
		UTextBlock* LanLabel = MakeText(
			WidgetTree,
			TEXT("LanLabel"),
			NSLOCTEXT("ZonefallLobby", "LAN", "LOCAL LAN"),
			BodyFontSize,
			ZGlass::TextPrimary);
		LanCheck->SetContent(LanLabel);
	}
	if (UHorizontalBoxSlot* LanSlot = HostBar->AddChildToHorizontalBox(LanCheck))
	{
		LanSlot->SetPadding(FMargin(0, 0, 8, 0));
		LanSlot->SetVerticalAlignment(VAlign_Center);
	}

	PrivacyBox = WidgetTree->ConstructWidget<UComboBoxString>(UComboBoxString::StaticClass(), TEXT("PrivacyBox"));
	PrivacyBox->AddOption(TEXT("Public"));
	PrivacyBox->AddOption(TEXT("Friends"));
	PrivacyBox->AddOption(TEXT("Invite Only"));
	PrivacyBox->SetSelectedOption(TEXT("Public"));
	StyleGlassCombo(PrivacyBox);
	if (UHorizontalBoxSlot* PrivacySlot = HostBar->AddChildToHorizontalBox(PrivacyBox))
	{
		PrivacySlot->SetPadding(FMargin(0, 0, 8, 0));
	}

	HostButton = MakeIconButton(WidgetTree, TEXT("HostButton"), NSLOCTEXT("ZonefallLobby", "Host", "HOST"), BodyFontSize + 2, AccentColor * 0.5f, AccentColor);
	if (UHorizontalBoxSlot* HostBtnSlot = HostBar->AddChildToHorizontalBox(HostButton))
	{
		HostBtnSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
	}

	if (UVerticalBoxSlot* HostBarSlot = LeftCol->AddChildToVerticalBox(HostBarPanel))
	{
		HostBarSlot->SetPadding(FMargin(0, 0, 0, 10));
	}

	HostPasswordInput = WidgetTree->ConstructWidget<UEditableTextBox>(UEditableTextBox::StaticClass(), TEXT("HostPasswordInput"));
	HostPasswordInput->SetHintText(NSLOCTEXT("ZonefallLobby", "HostPasswordHint", "Host password (optional)"));
	{
		FEditableTextBoxStyle Style = HostPasswordInput->GetWidgetStyle();
		Style.SetBackgroundImageNormal(FSlateRoundedBoxBrush(FLinearColor(1.0f, 1.0f, 1.0f, 0.05f), 8.0f, ZGlass::OutlineSoft, 1.0f));
		Style.SetBackgroundImageHovered(FSlateRoundedBoxBrush(FLinearColor(1.0f, 1.0f, 1.0f, 0.10f), 8.0f, ZGlass::Outline, 1.0f));
		Style.SetBackgroundImageFocused(FSlateRoundedBoxBrush(FLinearColor(1.0f, 1.0f, 1.0f, 0.10f), 8.0f, AccentColor, 1.0f));
		Style.SetForegroundColor(FSlateColor(ZGlass::TextPrimary));
		Style.SetFont(MakeFont(BodyFontSize));
		HostPasswordInput->SetWidgetStyle(Style);
		HostPasswordInput->SetIsPassword(true);
	}
	if (UVerticalBoxSlot* HostPwdSlot = LeftCol->AddChildToVerticalBox(HostPasswordInput))
	{
		HostPwdSlot->SetPadding(FMargin(0, 0, 0, 6));
	}

	JoinPasswordInput = WidgetTree->ConstructWidget<UEditableTextBox>(UEditableTextBox::StaticClass(), TEXT("JoinPasswordInput"));
	JoinPasswordInput->SetHintText(NSLOCTEXT("ZonefallLobby", "JoinPasswordHint", "Session password (if locked)"));
	{
		FEditableTextBoxStyle Style = JoinPasswordInput->GetWidgetStyle();
		Style.SetBackgroundImageNormal(FSlateRoundedBoxBrush(FLinearColor(1.0f, 1.0f, 1.0f, 0.05f), 8.0f, ZGlass::OutlineSoft, 1.0f));
		Style.SetBackgroundImageHovered(FSlateRoundedBoxBrush(FLinearColor(1.0f, 1.0f, 1.0f, 0.10f), 8.0f, ZGlass::Outline, 1.0f));
		Style.SetBackgroundImageFocused(FSlateRoundedBoxBrush(FLinearColor(1.0f, 1.0f, 1.0f, 0.10f), 8.0f, AccentColor, 1.0f));
		Style.SetForegroundColor(FSlateColor(ZGlass::TextPrimary));
		Style.SetFont(MakeFont(BodyFontSize));
		JoinPasswordInput->SetWidgetStyle(Style);
		JoinPasswordInput->SetIsPassword(true);
	}
	if (UVerticalBoxSlot* JoinPwdSlot = LeftCol->AddChildToVerticalBox(JoinPasswordInput))
	{
		JoinPwdSlot->SetPadding(FMargin(0, 0, 0, 6));
	}

	UTextBlock* HostHint = MakeText(
		WidgetTree,
		TEXT("HostHint"),
		NSLOCTEXT("ZonefallLobby", "HostHint", "LAN = same PC / Wi‑Fi. Steam = friends worldwide. Player 1 window hosts in PIE."),
		BodyFontSize - 2,
		ZGlass::TextSecondary);
	if (UVerticalBoxSlot* HintSlot = LeftCol->AddChildToVerticalBox(HostHint))
	{
		HintSlot->SetPadding(FMargin(2, 0, 0, 10));
	}

	// Direct connect row (left column).
	{
		UTextBlock* DirectLabel = MakeText(
			WidgetTree,
			TEXT("DirectLabel"),
			NSLOCTEXT("ZonefallLobby", "DirectLabel", "DIRECT CONNECT"),
			BodyFontSize + 2,
			ZGlass::TextPrimary);
		if (UVerticalBoxSlot* DLSlot = LeftCol->AddChildToVerticalBox(DirectLabel))
		{
			DLSlot->SetPadding(FMargin(2, 0, 0, 6));
		}

		UHorizontalBox* DirectRow = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("DirectConnectRow"));
		JoinByIdInput = WidgetTree->ConstructWidget<UEditableTextBox>(UEditableTextBox::StaticClass(), TEXT("JoinByIdInput"));
		JoinByIdInput->SetHintText(NSLOCTEXT("ZonefallLobby", "JoinByIdHint", "127.0.0.1:7777"));
		{
			FEditableTextBoxStyle Style = JoinByIdInput->GetWidgetStyle();
			Style.SetBackgroundImageNormal(FSlateRoundedBoxBrush(FLinearColor(1.0f, 1.0f, 1.0f, 0.05f), 8.0f, ZGlass::OutlineSoft, 1.0f));
			Style.SetBackgroundImageHovered(FSlateRoundedBoxBrush(FLinearColor(1.0f, 1.0f, 1.0f, 0.10f), 8.0f, ZGlass::Outline, 1.0f));
			Style.SetBackgroundImageFocused(FSlateRoundedBoxBrush(FLinearColor(1.0f, 1.0f, 1.0f, 0.10f), 8.0f, AccentColor, 1.0f));
			Style.SetForegroundColor(FSlateColor(ZGlass::TextPrimary));
			Style.SetFont(MakeFont(BodyFontSize));
			JoinByIdInput->SetWidgetStyle(Style);
		}
		if (UHorizontalBoxSlot* InSlot = DirectRow->AddChildToHorizontalBox(JoinByIdInput))
		{
			InSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
			InSlot->SetPadding(FMargin(0, 0, 8, 0));
		}
		JoinByIdButton = MakeIconButton(
			WidgetTree,
			TEXT("JoinByIdButton"),
			NSLOCTEXT("ZonefallLobby", "DirectConnect", "CONNECT"),
			BodyFontSize,
			AccentColor * 0.45f,
			AccentColor);
		DirectRow->AddChildToHorizontalBox(JoinByIdButton);
		if (UVerticalBoxSlot* DirectSlot = LeftCol->AddChildToVerticalBox(DirectRow))
		{
			DirectSlot->SetPadding(FMargin(0, 0, 0, 0));
		}
	}

	// --- Right column: session browser ---
	UVerticalBox* RightCol = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("LobbyRightCol"));
	if (UHorizontalBoxSlot* RightSlot = ContentRow->AddChildToHorizontalBox(RightCol))
	{
		RightSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	}

	SessionListHeaderText = MakeText(
		WidgetTree,
		TEXT("SessionListHeader"),
		NSLOCTEXT("ZonefallLobby", "SessionsHeader", "BROWSE SESSIONS"),
		BodyFontSize + 2,
		ZGlass::TextPrimary);
	if (UVerticalBoxSlot* HeaderSlot = RightCol->AddChildToVerticalBox(SessionListHeaderText))
	{
		HeaderSlot->SetPadding(FMargin(2, 0, 0, 8));
	}

	// --- Toolbar (refresh, join, leave, back) ---
	UHorizontalBox* Toolbar = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("LobbyToolbar"));

	QuickJoinButton = MakeIconButton(
		WidgetTree,
		TEXT("QuickJoinButton"),
		NSLOCTEXT("ZonefallLobby", "QuickJoin", "QUICK JOIN"),
		BodyFontSize + 1,
		FLinearColor(0.18f, 0.55f, 0.28f, 1.0f),
		FLinearColor(0.35f, 0.92f, 0.45f, 1.0f));
	if (UHorizontalBoxSlot* QJS = Toolbar->AddChildToHorizontalBox(QuickJoinButton))
	{
		QJS->SetPadding(FMargin(0, 0, 8, 0));
	}

	RefreshButton = MakeIconButton(WidgetTree, TEXT("RefreshButton"), NSLOCTEXT("ZonefallLobby", "Refresh", "REFRESH"), BodyFontSize, ZGlass::Row, AccentColor);
	if (UHorizontalBoxSlot* RS = Toolbar->AddChildToHorizontalBox(RefreshButton))
	{
		RS->SetPadding(FMargin(0, 0, 8, 0));
	}

	HideFullCheck = WidgetTree->ConstructWidget<UCheckBox>(UCheckBox::StaticClass(), TEXT("HideFullCheck"));
	StyleGlassCheck(HideFullCheck, AccentColor * 0.5f);
	HideFullCheck->SetIsChecked(true);
	{
		UTextBlock* HideFullLabel = MakeText(
			WidgetTree,
			TEXT("HideFullLabel"),
			NSLOCTEXT("ZonefallLobby", "HideFull", "Hide full"),
			BodyFontSize - 2,
			ZGlass::TextPrimary);
		HideFullCheck->SetContent(HideFullLabel);
	}
	if (UHorizontalBoxSlot* HFSlot = Toolbar->AddChildToHorizontalBox(HideFullCheck))
	{
		HFSlot->SetPadding(FMargin(0, 0, 8, 0));
		HFSlot->SetVerticalAlignment(VAlign_Center);
	}

	JoinButton = MakeIconButton(WidgetTree, TEXT("JoinButton"), NSLOCTEXT("ZonefallLobby", "Join", "JOIN"), BodyFontSize, AccentColor * 0.45f, AccentColor);
	if (UHorizontalBoxSlot* JS = Toolbar->AddChildToHorizontalBox(JoinButton))
	{
		JS->SetPadding(FMargin(0, 0, 8, 0));
	}

	LeaveButton = MakeIconButton(WidgetTree, TEXT("LeaveButton"), NSLOCTEXT("ZonefallLobby", "Leave", "LEAVE"), BodyFontSize, ZGlass::Row, FLinearColor(0.92f, 0.32f, 0.32f, 1.0f));
	if (UHorizontalBoxSlot* LS = Toolbar->AddChildToHorizontalBox(LeaveButton))
	{
		LS->SetPadding(FMargin(0, 0, 8, 0));
	}

	OpenLevelButton = MakeIconButton(WidgetTree, TEXT("OpenLevelButton"), NSLOCTEXT("ZonefallLobby", "OpenLevel", "OPEN LEVEL"), BodyFontSize, AccentColor * 0.45f, AccentColor);
	OpenLevelButton->SetVisibility(ESlateVisibility::Collapsed);

	BackButton = MakeIconButton(WidgetTree, TEXT("BackButton"), NSLOCTEXT("ZonefallLobby", "Back", "BACK"), BodyFontSize, ZGlass::Row, FLinearColor(0.6f, 0.6f, 0.6f, 1.0f));
	if (UHorizontalBoxSlot* BS = Toolbar->AddChildToHorizontalBox(BackButton))
	{
		BS->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
		BS->SetHorizontalAlignment(HAlign_Right);
	}

	if (UVerticalBoxSlot* ToolbarSlot = RightCol->AddChildToVerticalBox(Toolbar))
	{
		ToolbarSlot->SetPadding(FMargin(0, 0, 0, 10));
	}

	UBorder* ListPanel = MakeGlassPanel(WidgetTree, TEXT("SessionListPanel"), ZGlass::Panel, 12.0f, ZGlass::OutlineSoft, 1.0f);
	ListPanel->SetPadding(FMargin(8.0f, 8.0f));
	SessionList = WidgetTree->ConstructWidget<UScrollBox>(UScrollBox::StaticClass(), TEXT("SessionList"));
	ListPanel->SetContent(SessionList);
	if (UVerticalBoxSlot* ListSlot = RightCol->AddChildToVerticalBox(ListPanel))
	{
		ListSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	}

	// --- Status row ---
	UHorizontalBox* StatusRow = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("StatusRow"));

	StatusText = MakeText(WidgetTree, TEXT("StatusText"), NSLOCTEXT("ZonefallLobby", "Ready", "Ready."), BodyFontSize, ZGlass::TextSecondary);
	if (UHorizontalBoxSlot* StatusSlot = StatusRow->AddChildToHorizontalBox(StatusText))
	{
		StatusSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
		StatusSlot->SetVerticalAlignment(VAlign_Center);
	}

	BusyBar = WidgetTree->ConstructWidget<UProgressBar>(UProgressBar::StaticClass(), TEXT("LobbyBusyBar"));
	{
		FProgressBarStyle BarStyle = BusyBar->GetWidgetStyle();
		BarStyle.SetBackgroundImage(FSlateRoundedBoxBrush(FLinearColor(1.0f, 1.0f, 1.0f, 0.08f), 5.0f));
		BarStyle.SetFillImage(FSlateRoundedBoxBrush(AccentColor, 5.0f));
		BarStyle.SetMarqueeImage(FSlateRoundedBoxBrush(AccentColor * 1.1f, 5.0f));
		BusyBar->SetWidgetStyle(BarStyle);
	}
	BusyBar->SetPercent(0.0f);
	BusyBar->SetFillColorAndOpacity(AccentColor);
	BusyBar->SetVisibility(ESlateVisibility::Hidden);
	if (UHorizontalBoxSlot* BarSlot = StatusRow->AddChildToHorizontalBox(BusyBar))
	{
		BarSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
		BarSlot->SetPadding(FMargin(12, 0, 0, 0));
	}

	if (UVerticalBoxSlot* StatusRowSlot = MainBox->AddChildToVerticalBox(StatusRow))
	{
		StatusRowSlot->SetPadding(FMargin(0));
	}

	RebuildSessionList({});
}

void UZonefallOnlineLobbyWidget::RebuildSessionList(const TArray<FUIWorldOnlineSessionResult>& Results)
{
	if (!SessionList)
	{
		return;
	}

	// Remember which session was selected so the periodic refresh doesn't lose the player's pick.
	const FString PreviouslySelectedId = CachedSessions.IsValidIndex(SelectedSessionIndex)
		? CachedSessions[SelectedSessionIndex].SessionId
		: FString();

	SessionList->ClearChildren();
	SessionCardButtons.Reset();
	CachedSessions = Results;
	SelectedSessionIndex = INDEX_NONE;
	LastClickedCardIndex = INDEX_NONE;

	if (Results.Num() == 0)
	{
		// Friendly empty-state card — never an error, just "nothing to join yet".
		UBorder* EmptyPanel = MakeGlassPanel(WidgetTree, TEXT("EmptyPanel"), ZGlass::Panel, 12.0f, ZGlass::OutlineSoft, 1.0f);
		EmptyPanel->SetPadding(FMargin(24.0f, 28.0f));

		UVerticalBox* EmptyCol = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("EmptyCol"));
		EmptyPanel->SetContent(EmptyCol);

		UTextBlock* EmptyTitle = MakeText(WidgetTree, TEXT("EmptyTitle"),
			NSLOCTEXT("ZonefallLobby", "EmptyTitle", "NO SESSIONS YET"), BodyFontSize + 6, ZGlass::TextPrimary);
		EmptyTitle->SetJustification(ETextJustify::Center);
		if (UVerticalBoxSlot* ETSlot = EmptyCol->AddChildToVerticalBox(EmptyTitle))
		{
			ETSlot->SetHorizontalAlignment(HAlign_Center);
			ETSlot->SetPadding(FMargin(0, 0, 0, 8));
		}

		UTextBlock* EmptyHint = MakeText(WidgetTree, TEXT("EmptyHint"),
			NSLOCTEXT("ZonefallLobby", "EmptyHint", "Host a match or press QUICK JOIN. LAN and Steam are searched automatically — this list refreshes on its own."),
			BodyFontSize - 1, ZGlass::TextSecondary);
		EmptyHint->SetJustification(ETextJustify::Center);
		EmptyHint->SetAutoWrapText(true);
		EmptyCol->AddChildToVerticalBox(EmptyHint);

		SessionList->AddChild(EmptyPanel);
		if (UScrollBoxSlot* EmptySlot = Cast<UScrollBoxSlot>(EmptyPanel->Slot))
		{
			EmptySlot->SetPadding(FMargin(8, 18, 8, 18));
			EmptySlot->SetHorizontalAlignment(HAlign_Fill);
		}
		return;
	}

	for (int32 Index = 0; Index < Results.Num(); ++Index)
	{
		const FUIWorldOnlineSessionResult& R = Results[Index];

		UZonefallSessionCardButton* CardButton = WidgetTree->ConstructWidget<UZonefallSessionCardButton>(UZonefallSessionCardButton::StaticClass(), FName(*FString::Printf(TEXT("Card_%d"), Index)));
		CardButton->CardIndex = Index;
		StyleButton(CardButton, CardTint, CardHoverTint, AccentColor * 0.6f);
		SessionCardButtons.Add(CardButton);

		UHorizontalBox* Row = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), FName(*FString::Printf(TEXT("CardRow_%d"), Index)));
		CardButton->SetContent(Row);

		// Name + owner column
		UVerticalBox* NameCol = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), FName(*FString::Printf(TEXT("NameCol_%d"), Index)));
		UTextBlock* NameT = MakeText(WidgetTree, FName(*FString::Printf(TEXT("NameT_%d"), Index)),
			FText::FromString(R.ServerName.IsEmpty() ? FString::Printf(TEXT("Server %d"), Index + 1) : R.ServerName),
			BodyFontSize + 4, ZGlass::TextPrimary);
		UTextBlock* OwnerT = MakeText(WidgetTree, FName(*FString::Printf(TEXT("OwnerT_%d"), Index)),
			FText::FromString(R.OwningUserName.IsEmpty() ? TEXT("Unknown host") : R.OwningUserName),
			BodyFontSize - 2, ZGlass::TextSecondary);
		NameCol->AddChildToVerticalBox(NameT);
		NameCol->AddChildToVerticalBox(OwnerT);
		if (!R.MapDisplayName.IsEmpty())
		{
			UTextBlock* MapT = MakeText(
				WidgetTree,
				FName(*FString::Printf(TEXT("MapT_%d"), Index)),
				FText::FromString(R.MapDisplayName),
				BodyFontSize - 2,
				AccentColor * 0.75f);
			NameCol->AddChildToVerticalBox(MapT);
		}
		if (UHorizontalBoxSlot* NS = Row->AddChildToHorizontalBox(NameCol))
		{
			NS->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
			NS->SetPadding(FMargin(14, 10));
			NS->SetVerticalAlignment(VAlign_Center);
		}

		// Players column
		const FLinearColor PlayerColor = R.bIsFull
			? FLinearColor(0.95f, 0.4f, 0.4f, 1.0f)
			: ZGlass::TextPrimary;
		UTextBlock* PlayersT = MakeText(WidgetTree, FName(*FString::Printf(TEXT("PlayersT_%d"), Index)),
			FText::FromString(FString::Printf(TEXT("%d / %d"), R.CurrentPlayers, R.MaxPlayers)),
			BodyFontSize + 2, PlayerColor);
		if (UHorizontalBoxSlot* PS = Row->AddChildToHorizontalBox(PlayersT))
		{
			PS->SetPadding(FMargin(14, 10));
			PS->SetVerticalAlignment(VAlign_Center);
		}

		// Ping column
		UTextBlock* PingT = MakeText(WidgetTree, FName(*FString::Printf(TEXT("PingT_%d"), Index)),
			FText::FromString(R.PingMs >= 0 ? FString::Printf(TEXT("%d ms"), R.PingMs) : TEXT("--")),
			BodyFontSize + 2, PingColor(R.PingMs));
		if (UHorizontalBoxSlot* PiS = Row->AddChildToHorizontalBox(PingT))
		{
			PiS->SetPadding(FMargin(14, 10));
			PiS->SetVerticalAlignment(VAlign_Center);
		}

		if (R.bPasswordProtected)
		{
			UBorder* LockPill = MakeGlassPanel(
				WidgetTree,
				FName(*FString::Printf(TEXT("LockPill_%d"), Index)),
				FLinearColor(1.0f, 0.75f, 0.2f, 0.15f),
				6.0f,
				FLinearColor(1.0f, 0.75f, 0.2f, 0.5f),
				1.0f);
			LockPill->SetPadding(FMargin(8.0f, 4.0f));
			LockPill->SetContent(MakeText(
				WidgetTree,
				FName(*FString::Printf(TEXT("LockT_%d"), Index)),
				FText::FromString(TEXT("LOCK")),
				BodyFontSize - 3,
				FLinearColor(1.0f, 0.82f, 0.35f, 1.0f)));
			if (UHorizontalBoxSlot* LS = Row->AddChildToHorizontalBox(LockPill))
			{
				LS->SetPadding(FMargin(6, 10));
				LS->SetVerticalAlignment(VAlign_Center);
			}
		}

		if (R.bIsFull)
		{
			UBorder* FullPill = MakeGlassPanel(
				WidgetTree,
				FName(*FString::Printf(TEXT("FullPill_%d"), Index)),
				FLinearColor(0.9f, 0.2f, 0.2f, 0.15f),
				6.0f,
				FLinearColor(0.9f, 0.2f, 0.2f, 0.5f),
				1.0f);
			FullPill->SetPadding(FMargin(8.0f, 4.0f));
			FullPill->SetContent(MakeText(
				WidgetTree,
				FName(*FString::Printf(TEXT("FullT_%d"), Index)),
				FText::FromString(TEXT("FULL")),
				BodyFontSize - 3,
				FLinearColor(1.0f, 0.45f, 0.45f, 1.0f)));
			if (UHorizontalBoxSlot* FS = Row->AddChildToHorizontalBox(FullPill))
			{
				FS->SetPadding(FMargin(6, 10));
				FS->SetVerticalAlignment(VAlign_Center);
			}
		}

		if (!R.bBuildCompatible)
		{
			UBorder* VerPill = MakeGlassPanel(
				WidgetTree,
				FName(*FString::Printf(TEXT("VerPill_%d"), Index)),
				FLinearColor(0.8f, 0.4f, 0.1f, 0.15f),
				6.0f,
				FLinearColor(0.8f, 0.4f, 0.1f, 0.5f),
				1.0f);
			VerPill->SetPadding(FMargin(8.0f, 4.0f));
			VerPill->SetContent(MakeText(
				WidgetTree,
				FName(*FString::Printf(TEXT("VerT_%d"), Index)),
				FText::FromString(TEXT("VER")),
				BodyFontSize - 3,
				FLinearColor(1.0f, 0.65f, 0.3f, 1.0f)));
			if (UHorizontalBoxSlot* VS = Row->AddChildToHorizontalBox(VerPill))
			{
				VS->SetPadding(FMargin(6, 10));
				VS->SetVerticalAlignment(VAlign_Center);
			}
		}

		// LAN / Online badge as a rounded pill.
		const FLinearColor BadgeColor = R.bIsLAN ? FLinearColor(0.6f, 0.85f, 1.0f, 1.0f) : FLinearColor(1.0f, 0.78f, 0.32f, 1.0f);
		UBorder* BadgePill = MakeGlassPanel(WidgetTree, FName(*FString::Printf(TEXT("BadgePill_%d"), Index)),
			BadgeColor * 0.18f, 6.0f, BadgeColor * 0.5f, 1.0f);
		BadgePill->SetPadding(FMargin(10.0f, 4.0f));
		UTextBlock* BadgeT = MakeText(WidgetTree, FName(*FString::Printf(TEXT("BadgeT_%d"), Index)),
			FText::FromString(R.bIsLAN ? TEXT("LAN") : TEXT("ONLINE")),
			BodyFontSize - 3, BadgeColor);
		BadgePill->SetContent(BadgeT);
		if (UHorizontalBoxSlot* BS = Row->AddChildToHorizontalBox(BadgePill))
		{
			BS->SetPadding(FMargin(14, 10));
			BS->SetVerticalAlignment(VAlign_Center);
		}

		// One click = select (highlight). Click the already-selected card again
		// within DoubleClickJoinSeconds, or press JOIN, to actually join.
		CardButton->OnCardClicked.AddDynamic(this, &UZonefallOnlineLobbyWidget::HandleCardClicked);

		SessionList->AddChild(CardButton);
		if (UScrollBoxSlot* CardSlot = Cast<UScrollBoxSlot>(CardButton->Slot))
		{
			CardSlot->SetPadding(FMargin(2, 0, 2, 8));
		}
	}

	// Re-apply the previous selection by session id so a refresh doesn't deselect the player's pick.
	if (!PreviouslySelectedId.IsEmpty())
	{
		for (int32 i = 0; i < CachedSessions.Num(); ++i)
		{
			if (CachedSessions[i].SessionId == PreviouslySelectedId)
			{
				SelectedSessionIndex = i;
				LastClickedCardIndex = i;
				break;
			}
		}
	}

	UpdateCardSelectionVisuals();
	UpdateActionButtonStates();
}

void UZonefallOnlineLobbyWidget::SetStatusMessage(const FString& Message, bool bSuccess)
{
	if (StatusText)
	{
		StatusText->SetColorAndOpacity(FSlateColor(bSuccess ? FLinearColor(0.85f, 0.95f, 0.98f, 1.0f) : FLinearColor(1.0f, 0.5f, 0.5f, 1.0f)));
		StatusText->SetText(FText::FromString(Message));
	}
}

void UZonefallOnlineLobbyWidget::UpdateSteamBanner()
{
	UUIWorldMenuGameInstance* GI = ResolveGameInstance();
	if (!GI || !PlayerBannerText)
	{
		return;
	}

	const bool bLoggedIn = GI->IsOnlineLoggedIn();
	const bool bAvailable = GI->IsOnlineAvailable();
	const FString Service = GI->GetOnlineServiceName();
	const FString Nick = GI->GetOnlinePlayerNickname();

	FString Banner;
	if (bLoggedIn && !Nick.IsEmpty())
	{
		// e.g. "STEAM ● Connected — Cattleman   |   Build 1"
		Banner = FString::Printf(TEXT("%s  ●  %s   |   Build %s"), *Service.ToUpper(), *Nick, *GI->OnlineGameBuildId);
	}
	else if (bAvailable)
	{
		Banner = FString::Printf(TEXT("%s  ○  Connecting…   |   Build %s"), *Service.ToUpper(), *GI->OnlineGameBuildId);
	}
	else
	{
		Banner = FString::Printf(TEXT("OFFLINE   |   Build %s"), *GI->OnlineGameBuildId);
	}
	PlayerBannerText->SetText(FText::FromString(Banner));
}

void UZonefallOnlineLobbyWidget::RefreshSessionList()
{
	UUIWorldMenuGameInstance* GI = ResolveGameInstance();
	if (!GI)
	{
		SetStatusMessage(TEXT("No game instance."), false);
		return;
	}

	// Keep the Steam account / connection banner current (login completes asynchronously).
	UpdateSteamBanner();

	const bool bLan = LanCheck ? LanCheck->IsChecked() : bLanByDefault;
	bSearchingSessions = true;
	UpdateActionButtonStates();
	if (BusyBar)
	{
		BusyBar->SetVisibility(ESlateVisibility::Visible);
		BusyBar->SetPercent(0.25f);
	}
	SetStatusMessage(TEXT("Searching sessions..."));
	if (!GI->FindOnlineSessions(MaxSessionResults, bLan))
	{
		const FString Diag = GI->GetLastOnlineDiagnostic();
		if (!Diag.IsEmpty())
		{
			SetStatusMessage(Diag, false);
		}
		if (!Diag.Contains(TEXT("Waiting for")))
		{
			bSearchingSessions = false;
			UpdateActionButtonStates();
			if (BusyBar)
			{
				BusyBar->SetVisibility(ESlateVisibility::Hidden);
			}
		}
	}
}

void UZonefallOnlineLobbyWidget::HostFromUI()
{
	UUIWorldMenuGameInstance* GI = ResolveGameInstance();
	if (!GI)
	{
		SetStatusMessage(TEXT("No game instance."), false);
		return;
	}

	if (ServerNameInput && !ServerNameInput->GetText().IsEmpty())
	{
		GI->OnlineServerName = ServerNameInput->GetText().ToString();
	}

	// Host the level chosen in the map combo.
	const FName SelectedMap = GetSelectedMapName();
	if (!SelectedMap.IsNone())
	{
		GI->OnlineHostMapName = SelectedMap;
	}

	const bool bLan = LanCheck ? LanCheck->IsChecked() : bLanByDefault;
	int32 MaxPlayers = 16;
	if (MaxPlayersBox)
	{
		LexFromString(MaxPlayers, *MaxPlayersBox->GetSelectedOption());
		MaxPlayers = FMath::Clamp(MaxPlayers, 2, 64);
	}

	GI->PendingHostPassword = HostPasswordInput ? HostPasswordInput->GetText().ToString().TrimStartAndEnd() : FString();
	GI->PendingHostPrivacy = 0;
	if (PrivacyBox)
	{
		const FString Privacy = PrivacyBox->GetSelectedOption();
		if (Privacy.Equals(TEXT("Friends")))
		{
			GI->PendingHostPrivacy = 1;
		}
		else if (Privacy.Contains(TEXT("Invite")))
		{
			GI->PendingHostPrivacy = 2;
		}
	}

	bBusy = true;
	UpdateActionButtonStates();
	if (BusyBar)
	{
		BusyBar->SetVisibility(ESlateVisibility::Visible);
		BusyBar->SetPercent(0.5f);
	}
	SetStatusMessage(FString::Printf(TEXT("Hosting (max=%d, %s)..."), MaxPlayers, bLan ? TEXT("LAN") : TEXT("Online")));
	GI->HostOnlineSession(MaxPlayers, bLan);
}

void UZonefallOnlineLobbyWidget::JoinSelectedSession()
{
	UUIWorldMenuGameInstance* GI = ResolveGameInstance();
	if (!GI || !CachedSessions.IsValidIndex(SelectedSessionIndex))
	{
		SetStatusMessage(TEXT("Pick a session first."), false);
		return;
	}

	const FUIWorldOnlineSessionResult& Session = CachedSessions[SelectedSessionIndex];
	if (Session.bIsFull)
	{
		SetStatusMessage(TEXT("Session is full — pick another or Quick Join."), false);
		return;
	}
	if (!Session.bBuildCompatible)
	{
		SetStatusMessage(TEXT("Game version mismatch — update both players to the same build."), false);
		return;
	}

	GI->PendingJoinPassword = JoinPasswordInput ? JoinPasswordInput->GetText().ToString().TrimStartAndEnd() : FString();
	const int32 NativeIndex = Session.SearchResultIndex;
	if (NativeIndex == INDEX_NONE)
	{
		SetStatusMessage(TEXT("Session expired — press REFRESH."), false);
		return;
	}

	bBusy = true;
	UpdateActionButtonStates();
	if (BusyBar)
	{
		BusyBar->SetVisibility(ESlateVisibility::Visible);
		BusyBar->SetPercent(0.5f);
	}
	SetStatusMessage(FString::Printf(TEXT("Joining \"%s\"..."), *Session.ServerName));
	GI->JoinOnlineSessionByIndex(NativeIndex);
}

void UZonefallOnlineLobbyWidget::QuickJoinFromUI()
{
	UUIWorldMenuGameInstance* GI = ResolveGameInstance();
	if (!GI)
	{
		SetStatusMessage(TEXT("No game instance."), false);
		return;
	}

	GI->PendingJoinPassword = JoinPasswordInput ? JoinPasswordInput->GetText().ToString().TrimStartAndEnd() : FString();
	const bool bLan = LanCheck ? LanCheck->IsChecked() : bLanByDefault;

	bBusy = true;
	UpdateActionButtonStates();
	if (BusyBar)
	{
		BusyBar->SetVisibility(ESlateVisibility::Visible);
		BusyBar->SetPercent(0.35f);
	}
	SetStatusMessage(TEXT("Quick Join — searching for the best session..."));
	GI->QuickJoinOnlineSession(bLan);
}

void UZonefallOnlineLobbyWidget::LeaveCurrentSession()
{
	UUIWorldMenuGameInstance* GI = ResolveGameInstance();
	if (!GI)
	{
		return;
	}

	bBusy = true;
	UpdateActionButtonStates();
	if (BusyBar)
	{
		BusyBar->SetVisibility(ESlateVisibility::Visible);
		BusyBar->SetPercent(0.5f);
	}
	SetStatusMessage(TEXT("Leaving session..."));
	GI->LeaveOnlineSessionAndReturnToMenu();
}

void UZonefallOnlineLobbyWidget::HandleRefreshClicked()
{
	RefreshSessionList();
}

void UZonefallOnlineLobbyWidget::HandleQuickJoinClicked()
{
	QuickJoinFromUI();
}

void UZonefallOnlineLobbyWidget::HandleHostClicked()
{
	HostFromUI();
}

void UZonefallOnlineLobbyWidget::HandleJoinByIdClicked()
{
	UUIWorldMenuGameInstance* GI = ResolveGameInstance();
	if (!GI)
	{
		SetStatusMessage(TEXT("No game instance."), false);
		return;
	}

	const FString Address = JoinByIdInput ? JoinByIdInput->GetText().ToString().TrimStartAndEnd() : FString();
	if (Address.IsEmpty())
	{
		SetStatusMessage(TEXT("Enter an IP / address / ID (e.g. 192.168.0.10:7777)."), false);
		return;
	}

	bBusy = true;
	UpdateActionButtonStates();
	if (BusyBar)
	{
		BusyBar->SetVisibility(ESlateVisibility::Visible);
		BusyBar->SetPercent(0.5f);
	}
	SetStatusMessage(FString::Printf(TEXT("Connecting to %s..."), *Address));
	GI->JoinOnlineByAddress(Address);
}

void UZonefallOnlineLobbyWidget::HandleCardClicked(int32 CardIndex)
{
	if (!CachedSessions.IsValidIndex(CardIndex))
	{
		return;
	}

	const double Now = FPlatformTime::Seconds();
	const bool bIsDoubleClick =
		(CardIndex == LastClickedCardIndex) &&
		((Now - LastCardClickTime) <= DoubleClickJoinSeconds);

	LastClickedCardIndex = CardIndex;
	LastCardClickTime = Now;

	SetSelectedSessionIndex(CardIndex);

	if (bIsDoubleClick && !bBusy)
	{
		// Second click on the same card = confirm join.
		JoinSelectedSession();
		return;
	}

	const FUIWorldOnlineSessionResult& R = CachedSessions[CardIndex];
	const FString Name = R.ServerName.IsEmpty() ? FString::Printf(TEXT("Server %d"), CardIndex + 1) : R.ServerName;
	SetStatusMessage(FString::Printf(TEXT("Selected \"%s\" — press JOIN (or click again) to connect."), *Name));
}

void UZonefallOnlineLobbyWidget::SetSelectedSessionIndex(int32 NewIndex)
{
	SelectedSessionIndex = CachedSessions.IsValidIndex(NewIndex) ? NewIndex : INDEX_NONE;
	UpdateCardSelectionVisuals();
	UpdateActionButtonStates();
}

void UZonefallOnlineLobbyWidget::UpdateCardSelectionVisuals()
{
	for (int32 i = 0; i < SessionCardButtons.Num(); ++i)
	{
		UZonefallSessionCardButton* Card = SessionCardButtons[i];
		if (!Card)
		{
			continue;
		}

		if (i == SelectedSessionIndex)
		{
			// Bright accent fill so the chosen row reads clearly as "armed".
			const FLinearColor SelectedBase = AccentColor * 0.42f + FLinearColor(0.02f, 0.04f, 0.06f, 0.0f);
			const FLinearColor SelectedHover = AccentColor * 0.55f + FLinearColor(0.02f, 0.04f, 0.06f, 0.0f);
			StyleButton(Card, SelectedBase, SelectedHover, AccentColor * 0.7f);
		}
		else
		{
			StyleButton(Card, CardTint, CardHoverTint, AccentColor * 0.6f);
		}
	}
}

void UZonefallOnlineLobbyWidget::UpdateActionButtonStates()
{
	const bool bHasSelection = CachedSessions.IsValidIndex(SelectedSessionIndex);
	bool bCanJoin = bHasSelection && !bBusy;
	if (bHasSelection)
	{
		const FUIWorldOnlineSessionResult& Session = CachedSessions[SelectedSessionIndex];
		bCanJoin = bCanJoin && !Session.bIsFull && Session.bBuildCompatible;
	}
	if (JoinButton)
	{
		JoinButton->SetIsEnabled(bCanJoin);
	}
	if (QuickJoinButton)
	{
		QuickJoinButton->SetIsEnabled(!bBusy);
	}
}

void UZonefallOnlineLobbyWidget::HandleJoinClicked()
{
	if (SelectedSessionIndex == INDEX_NONE && CachedSessions.Num() == 1)
	{
		// Only one server in the list — be friendly and pick it.
		SetSelectedSessionIndex(0);
	}

	JoinSelectedSession();
}

void UZonefallOnlineLobbyWidget::HandleLeaveClicked()
{
	LeaveCurrentSession();
}

void UZonefallOnlineLobbyWidget::HandleBackClicked()
{
	if (UUIWorldMenuGameInstance* GI = ResolveGameInstance())
	{
		GI->ShowMenuFromList(EUIWorldMenuScreen::MainMenu, false);
	}
}

void UZonefallOnlineLobbyWidget::HandleOpenLevelClicked()
{
	UUIWorldMenuGameInstance* GI = ResolveGameInstance();
	if (!GI)
	{
		SetStatusMessage(TEXT("No game instance."), false);
		return;
	}

	const FName MapName = GetSelectedMapName();
	if (MapName.IsNone())
	{
		SetStatusMessage(TEXT("Pick a map to open."), false);
		return;
	}

	SetStatusMessage(FString::Printf(TEXT("Opening level \"%s\"..."), *MapName.ToString()));
	GI->CloseMenuUI(false);

	// Uses the animated loading screen (movie player / widget) while the level streams in.
	GI->LoadLevelWithLoadingScreen(MapName, true);
}

void UZonefallOnlineLobbyWidget::HandleAutoRefreshTick()
{
	// Don't auto-refresh while busy or while the player has a session selected —
	// otherwise the list rebuild fights their selection right as they try to join.
	if (bBusy || bSearchingSessions || SelectedSessionIndex != INDEX_NONE)
	{
		return;
	}
	RefreshSessionList();
}

void UZonefallOnlineLobbyWidget::HandleSessionsFound(const TArray<FUIWorldOnlineSessionResult>& Results)
{
	bSearchingSessions = false;
	if (BusyBar)
	{
		BusyBar->SetPercent(1.0f);
		BusyBar->SetVisibility(ESlateVisibility::Hidden);
	}

	if (Results.Num() == 0)
	{
		// Empty is NOT an error — show a calm neutral message, never kick out of the lobby.
		SetStatusMessage(TEXT("No sessions found yet. Host a game or press Quick Join — the list keeps refreshing."), true);
	}
	else
	{
		SetStatusMessage(FString::Printf(TEXT("Found %d session(s)."), Results.Num()), true);
	}

	TArray<FUIWorldOnlineSessionResult> DisplayResults = Results;
	if (HideFullCheck && HideFullCheck->IsChecked())
	{
		DisplayResults.RemoveAll([](const FUIWorldOnlineSessionResult& S) { return S.bIsFull; });
	}

	if (DisplayResults.Num() == 0 && Results.Num() > 0)
	{
		SetStatusMessage(TEXT("All sessions are full — host a new match or disable Hide full."), false);
	}

	RebuildSessionList(DisplayResults);

	if (DisplayResults.Num() == 1 && !bBusy)
	{
		SetSelectedSessionIndex(0);
	}

	UpdateActionButtonStates();
}

void UZonefallOnlineLobbyWidget::HandleHostCompleted(bool bSuccess, const FString& Message)
{
	UUIWorldMenuGameInstance* GI = ResolveGameInstance();
	if (bSuccess && GI && GI->IsOnlineTravelInProgress())
	{
		SetStatusMessage(Message, true);
		if (BusyBar)
		{
			BusyBar->SetVisibility(ESlateVisibility::Visible);
			BusyBar->SetPercent(0.65f);
		}
		bBusy = true;
		UpdateActionButtonStates();
		return;
	}

	bBusy = false;
	if (BusyBar)
	{
		BusyBar->SetVisibility(ESlateVisibility::Hidden);
	}
	SetStatusMessage(Message, bSuccess);
	UpdateActionButtonStates();
}

void UZonefallOnlineLobbyWidget::HandleJoinCompleted(bool bSuccess, const FString& Message)
{
	UUIWorldMenuGameInstance* GI = ResolveGameInstance();
	if (bSuccess && GI && GI->IsOnlineTravelInProgress())
	{
		SetStatusMessage(Message, true);
		if (BusyBar)
		{
			BusyBar->SetVisibility(ESlateVisibility::Visible);
			BusyBar->SetPercent(0.65f);
		}
		bBusy = true;
		UpdateActionButtonStates();
		return;
	}

	bBusy = false;
	if (BusyBar)
	{
		BusyBar->SetVisibility(ESlateVisibility::Hidden);
	}
	SetStatusMessage(Message, bSuccess);
	UpdateActionButtonStates();
}

void UZonefallOnlineLobbyWidget::HandleLeaveCompleted(bool bSuccess, const FString& Message)
{
	bBusy = false;
	if (BusyBar)
	{
		BusyBar->SetVisibility(ESlateVisibility::Hidden);
	}
	SetStatusMessage(Message, bSuccess);
	UpdateActionButtonStates();
}

void UZonefallOnlineLobbyWidget::HandleOnlineMatchReady(UWorld* /*World*/)
{
	bBusy = false;
	bSearchingSessions = false;
	if (BusyBar)
	{
		BusyBar->SetVisibility(ESlateVisibility::Hidden);
	}
	SetStatusMessage(TEXT("Connected — in match."), true);
	UpdateActionButtonStates();
}

void UZonefallOnlineLobbyWidget::HandleServerNameChanged(const FText& NewText, ETextCommit::Type CommitType)
{
	if (UUIWorldMenuGameInstance* GI = ResolveGameInstance())
	{
		GI->OnlineServerName = NewText.ToString();
	}
}

void UZonefallOnlineLobbyWidget::UpdateNetworkModeBadge()
{
	if (!ModeBadgeText)
	{
		return;
	}

	const bool bLan = LanCheck ? LanCheck->IsChecked() : bLanByDefault;
	ModeBadgeText->SetText(FText::FromString(bLan ? TEXT("LOCAL NETWORK") : TEXT("STEAM INTERNET")));
	ModeBadgeText->SetColorAndOpacity(FSlateColor(bLan ? AccentColor : FLinearColor(1.0f, 0.78f, 0.32f, 1.0f)));

	if (SubtitleText)
	{
		SubtitleText->SetText(bLan
			? NSLOCTEXT(
				"ZonefallLobby",
				"SubtitleLAN",
				"PIE: Play → 2 Players → Listen Server. Player 1: HOST. Player 2: REFRESH or CONNECT 127.0.0.1:7777")
			: NSLOCTEXT(
				"ZonefallLobby",
				"SubtitleSteam",
				"Global — Steam friends / internet. Steam must run on both PCs. LAN checkbox OFF."));
	}
}

void UZonefallOnlineLobbyWidget::HandleLanCheckChanged(bool bIsChecked)
{
	UpdateNetworkModeBadge();
	if (bIsChecked)
	{
		SetStatusMessage(TEXT("LOCAL LAN — Steam not used. HOST, then JOIN after map loads."));
	}
	else
	{
		SetStatusMessage(TEXT("STEAM INTERNET — LAN off. Steam must be running."));
	}
}
