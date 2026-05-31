#include "ZonefallFancyMenuWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Button.h"
#include "Components/ButtonSlot.h"
#include "Components/Border.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/ComboBoxString.h"
#include "Components/TextBlock.h"
#include "Components/Widget.h"
#include "Framework/Application/SlateApplication.h"
#include "GameFramework/PlayerController.h"
#include "Input/Reply.h"
#include "InputCoreTypes.h"
#include "TimerManager.h"
#include "Brushes/SlateRoundedBoxBrush.h"
#include "Styling/SlateTypes.h"
#include "UIWorldMenuGameInstance.h"
#include "ZonefallModernButtonWidget.h"

UZonefallFancyMenuWidget::UZonefallFancyMenuWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
	, ButtonText(NSLOCTEXT("ZonefallUI", "PlayButtonLabel", "START ADVENTURE"))
	, ContinueButtonText(NSLOCTEXT("ZonefallUI", "ContinueButtonLabel", "CONTINUE"))
	, NormalTextColor(FLinearColor(0.90f, 0.93f, 1.00f, 1.00f))
	, HoverTextColor(FLinearColor(1.00f, 0.77f, 0.28f, 1.00f))
	, PressedTextColor(FLinearColor(1.00f, 0.60f, 0.12f, 1.00f))
	, bAllowFallbackButtonBuild(false)
	, FontSize(34)
	, MoodPreset(EZonefallMenuMoodPreset::WastelandHorror)
	, bEnableHorrorFX(true)
	, PulseOpacityStrength(0.10f)
	, PulseSpeed(0.90f)
	, FlickerChancePerSecond(0.70f)
	, FlickerDarkenStrength(0.35f)
	, JitterPixels(1.2f)
	, HorrorBackgroundWidgetName(TEXT("HorrorBackground"))
	, HostButtonWidgetName(TEXT("OnlineHostButton"))
	, FindButtonWidgetName(TEXT("OnlineFindButton"))
	, JoinButtonWidgetName(TEXT("OnlineJoinButton"))
	, LeaveButtonWidgetName(TEXT("OnlineLeaveButton"))
	, SessionsComboBoxWidgetName(TEXT("OnlineSessionsComboBox"))
	, OnlineStatusTextWidgetName(TEXT("OnlineStatusText"))
	, MainMenuPanelWidgetName(TEXT("MainMenuPanel"))
	, OnlinePanelWidgetName(TEXT("OnlinePanel"))
	, OpenOnlineButtonWidgetName(TEXT("OpenOnlineButton"))
	, BackToMainMenuButtonWidgetName(TEXT("BackToMainMenuButton"))
	, ServerNameTextWidgetName(TEXT("SelectedServerNameText"))
	, ServerPlayersTextWidgetName(TEXT("SelectedServerPlayersText"))
	, ServerPingTextWidgetName(TEXT("SelectedServerPingText"))
	, LocalIpTextWidgetName(TEXT("LocalIpText"))
	, bApplyAaaAutoStyleToOnlineButtons(true)
	, OnlineHostButtonText(NSLOCTEXT("ZonefallUI", "OnlineHostButtonText", "HOST"))
	, OnlineFindButtonText(NSLOCTEXT("ZonefallUI", "OnlineFindButtonText", "FIND"))
	, OnlineJoinButtonText(NSLOCTEXT("ZonefallUI", "OnlineJoinButtonText", "JOIN"))
	, OnlineLeaveButtonText(NSLOCTEXT("ZonefallUI", "OnlineLeaveButtonText", "LEAVE"))
	, OnlineStatusIdleText(NSLOCTEXT("ZonefallUI", "OnlineStatusIdle", "Online ready"))
	, OnlineStatusHostingText(NSLOCTEXT("ZonefallUI", "OnlineStatusHosting", "Hosting session..."))
	, OnlineStatusSearchingText(NSLOCTEXT("ZonefallUI", "OnlineStatusSearching", "Searching sessions..."))
	, OnlineStatusJoiningText(NSLOCTEXT("ZonefallUI", "OnlineStatusJoining", "Joining session..."))
	, OnlineStatusLeavingText(NSLOCTEXT("ZonefallUI", "OnlineStatusLeaving", "Leaving session..."))
	, OnlineStatusHostSuccessText(NSLOCTEXT("ZonefallUI", "OnlineStatusHostOk", "Session hosted. Travel to host map..."))
	, OnlineStatusJoinSuccessText(NSLOCTEXT("ZonefallUI", "OnlineStatusJoinOk", "Joined server"))
	, OnlineStatusLeaveSuccessText(NSLOCTEXT("ZonefallUI", "OnlineStatusLeaveOk", "Disconnected"))
	, OnlineStatusSelectServerText(NSLOCTEXT("ZonefallUI", "OnlineStatusSelectServer", "Select a valid server first"))
	, OnlineNoSessionsText(NSLOCTEXT("ZonefallUI", "OnlineNoSessionsText", "No sessions found"))
	, ServerNamePrefixText(NSLOCTEXT("ZonefallUI", "ServerNamePrefixText", "Server"))
	, ServerPlayersPrefixText(NSLOCTEXT("ZonefallUI", "ServerPlayersPrefixText", "Players"))
	, ServerPingPrefixText(NSLOCTEXT("ZonefallUI", "ServerPingPrefixText", "Ping"))
	, ServerLocalIpPrefixText(NSLOCTEXT("ZonefallUI", "ServerLocalIpPrefixText", "Local IP"))
	, bUseLanForOnline(true)
	, OnlineHostMaxPlayers(4)
	, HorrorFXTimeSeconds(0.0f)
	, FlickerDarkenAlpha(0.0f)
	, FlickerRecoverSpeed(2.5f)
	, bMainActionInProgress(false)
	, bContinueActionInProgress(false)
	, CachedSessionCount(0)
{
}

void UZonefallFancyMenuWidget::NativeConstruct()
{
	Super::NativeConstruct();

	BuildDefaultLayout();
	ApplyMoodPreset();
	ApplyStyleNow();
	SetContinueButtonVisible(false);
	RefreshOnlineSessionsUI();
	BindOnlineUiEvents();
	SetIsFocusable(true);

	if (GetOwningPlayer())
	{
		if (MainButton && MainButton->GetVisibility() == ESlateVisibility::Visible)
		{
			MainButton->SetKeyboardFocus();
		}
	}
}

void UZonefallFancyMenuWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
	UpdateHorrorFX(InDeltaTime);
}

FReply UZonefallFancyMenuWidget::NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
	const FKey PressedKey = InKeyEvent.GetKey();

	if ((PressedKey == EKeys::Up || PressedKey == EKeys::Gamepad_DPad_Up) && SessionsComboBox && CachedSessionCount > 0)
	{
		const int32 Current = FMath::Max(0, SessionsComboBox->GetSelectedIndex());
		const int32 Prev = (Current - 1 + CachedSessionCount) % CachedSessionCount;
		SessionsComboBox->SetSelectedIndex(Prev);
		UpdateSelectedServerCard();
		return FReply::Handled();
	}

	if ((PressedKey == EKeys::Down || PressedKey == EKeys::Gamepad_DPad_Down) && SessionsComboBox && CachedSessionCount > 0)
	{
		const int32 Current = FMath::Max(0, SessionsComboBox->GetSelectedIndex());
		const int32 Next = (Current + 1) % CachedSessionCount;
		SessionsComboBox->SetSelectedIndex(Next);
		UpdateSelectedServerCard();
		return FReply::Handled();
	}

	if (PressedKey == EKeys::Enter && CachedSessionCount > 0 && SessionsComboBox && SessionsComboBox->GetSelectedIndex() >= 0)
	{
		HandleJoinButtonClicked();
		return FReply::Handled();
	}

	if (PressedKey == EKeys::Enter && MainButton && MainButton->GetVisibility() == ESlateVisibility::Visible)
	{
		MainButton->OnClicked.Broadcast();
		return FReply::Handled();
	}

	if (PressedKey == EKeys::C && ContinueButton && ContinueButton->GetVisibility() == ESlateVisibility::Visible)
	{
		ContinueButton->OnClicked.Broadcast();
		return FReply::Handled();
	}

	return Super::NativeOnKeyDown(InGeometry, InKeyEvent);
}

void UZonefallFancyMenuWidget::SetButtonLabel(const FText& NewText)
{
	ButtonText = NewText;
	if (MainButtonText)
	{
		MainButtonText->SetText(ButtonText);
	}
}

void UZonefallFancyMenuWidget::ApplyStyleNow()
{
	ApplyMoodPreset();

	if (MainButtonText)
	{
		ApplyTextStyle(NormalTextColor);
		MainButtonText->SetText(ButtonText);
	}
	if (ContinueButtonTextBlock)
	{
		FSlateFontInfo EffectiveFont = ButtonFont;
		EffectiveFont.Size = FontSize;
		ContinueButtonTextBlock->SetFont(EffectiveFont);
		ContinueButtonTextBlock->SetColorAndOpacity(FSlateColor(NormalTextColor));
		ContinueButtonTextBlock->SetText(ContinueButtonText);
	}
	ApplyOnlineButtonTexts();
}

void UZonefallFancyMenuWidget::ApplyMoodPreset()
{
	if (MoodPreset == EZonefallMenuMoodPreset::WastelandHorror)
	{
		ApplyWastelandHorrorPreset();
	}
}

void UZonefallFancyMenuWidget::BuildDefaultLayout()
{
	if (!WidgetTree)
	{
		return;
	}

	if (!HorrorBackgroundWidget)
	{
		if (!HorrorBackgroundWidgetName.IsNone())
		{
			HorrorBackgroundWidget = Cast<UBorder>(WidgetTree->FindWidget(HorrorBackgroundWidgetName));
		}
		if (!HorrorBackgroundWidget)
		{
			HorrorBackgroundWidget = Cast<UBorder>(WidgetTree->FindWidget(TEXT("HorrorBackground_01")));
		}
	}

	if (!MainButton)
	{
		MainButton = Cast<UButton>(WidgetTree->FindWidget(TEXT("MainActionButton")));
	}
	if (!MainButtonText)
	{
		MainButtonText = Cast<UTextBlock>(WidgetTree->FindWidget(TEXT("MainActionText")));
	}
	if (!ContinueButton)
	{
		ContinueButton = Cast<UButton>(WidgetTree->FindWidget(TEXT("ContinueButton")));
	}
	if (!ContinueButton)
	{
		ContinueButton = Cast<UButton>(WidgetTree->FindWidget(TEXT("ContinueActionButton")));
	}
	if (!ContinueButtonTextBlock)
	{
		ContinueButtonTextBlock = Cast<UTextBlock>(WidgetTree->FindWidget(TEXT("ContinueText")));
	}
	if (!ContinueButtonTextBlock)
	{
		ContinueButtonTextBlock = Cast<UTextBlock>(WidgetTree->FindWidget(TEXT("ContinueActionText")));
	}
	if (!ContinueWidgetFallback)
	{
		ContinueWidgetFallback = WidgetTree->FindWidget(TEXT("Continue"));
	}
	if (!ContinueWidgetFallback)
	{
		ContinueWidgetFallback = WidgetTree->FindWidget(TEXT("ContinueText"));
	}
	if (!ContinueWidgetFallback)
	{
		ContinueWidgetFallback = WidgetTree->FindWidget(TEXT("ContinueButton"));
	}
	if (!HostButton && !HostButtonWidgetName.IsNone())
	{
		HostButton = Cast<UButton>(WidgetTree->FindWidget(HostButtonWidgetName));
	}
	if (!HostModernButton && !HostButtonWidgetName.IsNone())
	{
		HostModernButton = Cast<UZonefallModernButtonWidget>(WidgetTree->FindWidget(HostButtonWidgetName));
	}
	if (!FindButton && !FindButtonWidgetName.IsNone())
	{
		FindButton = Cast<UButton>(WidgetTree->FindWidget(FindButtonWidgetName));
	}
	if (!FindModernButton && !FindButtonWidgetName.IsNone())
	{
		FindModernButton = Cast<UZonefallModernButtonWidget>(WidgetTree->FindWidget(FindButtonWidgetName));
	}
	if (!JoinButton && !JoinButtonWidgetName.IsNone())
	{
		JoinButton = Cast<UButton>(WidgetTree->FindWidget(JoinButtonWidgetName));
	}
	if (!JoinModernButton && !JoinButtonWidgetName.IsNone())
	{
		JoinModernButton = Cast<UZonefallModernButtonWidget>(WidgetTree->FindWidget(JoinButtonWidgetName));
	}
	if (!LeaveButton && !LeaveButtonWidgetName.IsNone())
	{
		LeaveButton = Cast<UButton>(WidgetTree->FindWidget(LeaveButtonWidgetName));
	}
	if (!LeaveModernButton && !LeaveButtonWidgetName.IsNone())
	{
		LeaveModernButton = Cast<UZonefallModernButtonWidget>(WidgetTree->FindWidget(LeaveButtonWidgetName));
	}
	if (!SessionsComboBox && !SessionsComboBoxWidgetName.IsNone())
	{
		SessionsComboBox = Cast<UComboBoxString>(WidgetTree->FindWidget(SessionsComboBoxWidgetName));
	}
	if (!SessionsComboBox)
	{
		SessionsComboBox = Cast<UComboBoxString>(WidgetTree->FindWidget(TEXT("OnlineSessions")));
	}
	if (!OnlineStatusText && !OnlineStatusTextWidgetName.IsNone())
	{
		OnlineStatusText = Cast<UTextBlock>(WidgetTree->FindWidget(OnlineStatusTextWidgetName));
	}
	if (!OnlineStatusText)
	{
		OnlineStatusText = Cast<UTextBlock>(WidgetTree->FindWidget(TEXT("OnlineText")));
	}
	if (!ServerNameText && !ServerNameTextWidgetName.IsNone())
	{
		ServerNameText = Cast<UTextBlock>(WidgetTree->FindWidget(ServerNameTextWidgetName));
	}
	if (!ServerPlayersText && !ServerPlayersTextWidgetName.IsNone())
	{
		ServerPlayersText = Cast<UTextBlock>(WidgetTree->FindWidget(ServerPlayersTextWidgetName));
	}
	if (!ServerPingText && !ServerPingTextWidgetName.IsNone())
	{
		ServerPingText = Cast<UTextBlock>(WidgetTree->FindWidget(ServerPingTextWidgetName));
	}
	if (!LocalIpText && !LocalIpTextWidgetName.IsNone())
	{
		LocalIpText = Cast<UTextBlock>(WidgetTree->FindWidget(LocalIpTextWidgetName));
	}
	if (!MainMenuPanelWidget && !MainMenuPanelWidgetName.IsNone())
	{
		MainMenuPanelWidget = WidgetTree->FindWidget(MainMenuPanelWidgetName);
	}
	if (!OnlinePanelWidget && !OnlinePanelWidgetName.IsNone())
	{
		OnlinePanelWidget = WidgetTree->FindWidget(OnlinePanelWidgetName);
	}
	if (!OpenOnlineButton && !OpenOnlineButtonWidgetName.IsNone())
	{
		OpenOnlineButton = Cast<UButton>(WidgetTree->FindWidget(OpenOnlineButtonWidgetName));
	}
	if (!BackToMainMenuButton && !BackToMainMenuButtonWidgetName.IsNone())
	{
		BackToMainMenuButton = Cast<UButton>(WidgetTree->FindWidget(BackToMainMenuButtonWidgetName));
	}

	// Respect designer-authored layout. Build fallback only when explicitly allowed and root is empty.
	if (WidgetTree->RootWidget && !bAllowFallbackButtonBuild)
	{
		if (MainButton)
		{
			MainButton->OnClicked.RemoveDynamic(this, &UZonefallFancyMenuWidget::HandleMainButtonClicked);
			MainButton->OnHovered.RemoveDynamic(this, &UZonefallFancyMenuWidget::HandleMainButtonHovered);
			MainButton->OnUnhovered.RemoveDynamic(this, &UZonefallFancyMenuWidget::HandleMainButtonUnhovered);
			MainButton->OnPressed.RemoveDynamic(this, &UZonefallFancyMenuWidget::HandleMainButtonPressed);
			MainButton->OnReleased.RemoveDynamic(this, &UZonefallFancyMenuWidget::HandleMainButtonReleased);
			MainButton->OnClicked.AddDynamic(this, &UZonefallFancyMenuWidget::HandleMainButtonClicked);
			MainButton->OnHovered.AddDynamic(this, &UZonefallFancyMenuWidget::HandleMainButtonHovered);
			MainButton->OnUnhovered.AddDynamic(this, &UZonefallFancyMenuWidget::HandleMainButtonUnhovered);
			MainButton->OnPressed.AddDynamic(this, &UZonefallFancyMenuWidget::HandleMainButtonPressed);
			MainButton->OnReleased.AddDynamic(this, &UZonefallFancyMenuWidget::HandleMainButtonReleased);
		}
		if (ContinueButton)
		{
			ContinueButton->OnClicked.RemoveDynamic(this, &UZonefallFancyMenuWidget::HandleContinueButtonClicked);
			ContinueButton->OnHovered.RemoveDynamic(this, &UZonefallFancyMenuWidget::HandleContinueButtonHovered);
			ContinueButton->OnUnhovered.RemoveDynamic(this, &UZonefallFancyMenuWidget::HandleContinueButtonUnhovered);
			ContinueButton->OnClicked.AddDynamic(this, &UZonefallFancyMenuWidget::HandleContinueButtonClicked);
			ContinueButton->OnHovered.AddDynamic(this, &UZonefallFancyMenuWidget::HandleContinueButtonHovered);
			ContinueButton->OnUnhovered.AddDynamic(this, &UZonefallFancyMenuWidget::HandleContinueButtonUnhovered);
		}
		return;
	}

	if (!RootPanel)
	{
		RootPanel = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("RootPanel"));
		WidgetTree->RootWidget = RootPanel;
	}

	if (!MainButton)
	{
		MainButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("MainActionButton"));
		MainButton->SetToolTipText(NSLOCTEXT("ZonefallUI", "MainActionButtonTooltip", "Main menu action"));
		FButtonStyle ButtonStyle = MainButton->GetStyle();
		ButtonStyle.SetNormal(FSlateRoundedBoxBrush(FLinearColor(0.03f, 0.05f, 0.10f, 0.88f), 12.0f));
		ButtonStyle.SetHovered(FSlateRoundedBoxBrush(FLinearColor(0.09f, 0.12f, 0.20f, 1.00f), 12.0f));
		ButtonStyle.SetPressed(FSlateRoundedBoxBrush(FLinearColor(0.02f, 0.03f, 0.07f, 1.00f), 12.0f));
		ButtonStyle.NormalPadding = FMargin(18.f, 10.f);
		ButtonStyle.PressedPadding = FMargin(18.f, 12.f, 18.f, 8.f);
		MainButton->SetStyle(ButtonStyle);

		if (UCanvasPanelSlot* CanvasSlot = RootPanel->AddChildToCanvas(MainButton))
		{
			CanvasSlot->SetAutoSize(true);
			CanvasSlot->SetAlignment(FVector2D(0.5f, 0.5f));
			CanvasSlot->SetAnchors(FAnchors(0.5f, 0.5f));
		}
	}

	if (!MainButtonText)
	{
		MainButtonText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("MainActionText"));
		MainButtonText->SetJustification(ETextJustify::Center);

		MainButton->AddChild(MainButtonText);
	}

	if (!ContinueButton)
	{
		ContinueButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("ContinueActionButton"));
		ContinueButton->SetToolTipText(NSLOCTEXT("ZonefallUI", "ContinueActionButtonTooltip", "Continue from latest save"));
		FButtonStyle ContinueStyle = ContinueButton->GetStyle();
		ContinueStyle.SetNormal(FSlateRoundedBoxBrush(FLinearColor(0.03f, 0.05f, 0.10f, 0.82f), 12.0f));
		ContinueStyle.SetHovered(FSlateRoundedBoxBrush(FLinearColor(0.08f, 0.11f, 0.18f, 1.00f), 12.0f));
		ContinueStyle.SetPressed(FSlateRoundedBoxBrush(FLinearColor(0.02f, 0.03f, 0.07f, 1.00f), 12.0f));
		ContinueStyle.NormalPadding = FMargin(18.f, 10.f);
		ContinueStyle.PressedPadding = FMargin(18.f, 12.f, 18.f, 8.f);
		ContinueButton->SetStyle(ContinueStyle);

		if (UCanvasPanelSlot* ContinueSlot = RootPanel->AddChildToCanvas(ContinueButton))
		{
			ContinueSlot->SetAutoSize(true);
			ContinueSlot->SetAlignment(FVector2D(0.5f, 0.5f));
			ContinueSlot->SetAnchors(FAnchors(0.5f, 0.5f));
			ContinueSlot->SetPosition(FVector2D(0.0f, 74.0f));
		}
	}

	if (!ContinueButtonTextBlock)
	{
		ContinueButtonTextBlock = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("ContinueActionText"));
		ContinueButtonTextBlock->SetJustification(ETextJustify::Center);
		ContinueButton->AddChild(ContinueButtonTextBlock);
	}

	MainButton->OnClicked.RemoveDynamic(this, &UZonefallFancyMenuWidget::HandleMainButtonClicked);
	MainButton->OnHovered.RemoveDynamic(this, &UZonefallFancyMenuWidget::HandleMainButtonHovered);
	MainButton->OnUnhovered.RemoveDynamic(this, &UZonefallFancyMenuWidget::HandleMainButtonUnhovered);
	MainButton->OnPressed.RemoveDynamic(this, &UZonefallFancyMenuWidget::HandleMainButtonPressed);
	MainButton->OnReleased.RemoveDynamic(this, &UZonefallFancyMenuWidget::HandleMainButtonReleased);

	MainButton->OnClicked.AddDynamic(this, &UZonefallFancyMenuWidget::HandleMainButtonClicked);
	MainButton->OnHovered.AddDynamic(this, &UZonefallFancyMenuWidget::HandleMainButtonHovered);
	MainButton->OnUnhovered.AddDynamic(this, &UZonefallFancyMenuWidget::HandleMainButtonUnhovered);
	MainButton->OnPressed.AddDynamic(this, &UZonefallFancyMenuWidget::HandleMainButtonPressed);
	MainButton->OnReleased.AddDynamic(this, &UZonefallFancyMenuWidget::HandleMainButtonReleased);

	ContinueButton->OnClicked.RemoveDynamic(this, &UZonefallFancyMenuWidget::HandleContinueButtonClicked);
	ContinueButton->OnHovered.RemoveDynamic(this, &UZonefallFancyMenuWidget::HandleContinueButtonHovered);
	ContinueButton->OnUnhovered.RemoveDynamic(this, &UZonefallFancyMenuWidget::HandleContinueButtonUnhovered);
	ContinueButton->OnClicked.AddDynamic(this, &UZonefallFancyMenuWidget::HandleContinueButtonClicked);
	ContinueButton->OnHovered.AddDynamic(this, &UZonefallFancyMenuWidget::HandleContinueButtonHovered);
	ContinueButton->OnUnhovered.AddDynamic(this, &UZonefallFancyMenuWidget::HandleContinueButtonUnhovered);
}

void UZonefallFancyMenuWidget::ApplyTextStyle(const FLinearColor& Color)
{
	if (!MainButtonText)
	{
		return;
	}

	FSlateFontInfo EffectiveFont = ButtonFont;
	EffectiveFont.Size = FontSize;
	MainButtonText->SetFont(EffectiveFont);
	MainButtonText->SetColorAndOpacity(FSlateColor(Color));
	MainButtonText->SetShadowOffset(FVector2D(1.0f, 2.0f));
	MainButtonText->SetShadowColorAndOpacity(FLinearColor(0.0f, 0.0f, 0.0f, 0.65f));
}

void UZonefallFancyMenuWidget::ApplyWastelandHorrorPreset()
{
	bEnableHorrorFX = true;
	PulseOpacityStrength = 0.18f;
	PulseSpeed = 0.62f;
	FlickerChancePerSecond = 1.45f;
	FlickerDarkenStrength = 0.52f;
	JitterPixels = 1.9f;

	NormalTextColor = FLinearColor(0.80f, 0.88f, 0.78f, 1.0f);
	HoverTextColor = FLinearColor(0.92f, 1.0f, 0.84f, 1.0f);
	PressedTextColor = FLinearColor(0.67f, 0.76f, 0.63f, 1.0f);
}

UUIWorldMenuGameInstance* UZonefallFancyMenuWidget::GetMenuGameInstance() const
{
	if (UWorld* World = GetWorld())
	{
		return Cast<UUIWorldMenuGameInstance>(World->GetGameInstance());
	}
	return nullptr;
}

void UZonefallFancyMenuWidget::BindOnlineUiEvents()
{
	if (HostButton)
	{
		HostButton->OnClicked.RemoveDynamic(this, &UZonefallFancyMenuWidget::HandleHostButtonClicked);
		HostButton->OnClicked.AddDynamic(this, &UZonefallFancyMenuWidget::HandleHostButtonClicked);
	}
	if (HostModernButton)
	{
		HostModernButton->OnClicked.RemoveDynamic(this, &UZonefallFancyMenuWidget::HandleHostButtonClicked);
		HostModernButton->OnClicked.AddDynamic(this, &UZonefallFancyMenuWidget::HandleHostButtonClicked);
	}
	if (FindButton)
	{
		FindButton->OnClicked.RemoveDynamic(this, &UZonefallFancyMenuWidget::HandleFindButtonClicked);
		FindButton->OnClicked.AddDynamic(this, &UZonefallFancyMenuWidget::HandleFindButtonClicked);
	}
	if (FindModernButton)
	{
		FindModernButton->OnClicked.RemoveDynamic(this, &UZonefallFancyMenuWidget::HandleFindButtonClicked);
		FindModernButton->OnClicked.AddDynamic(this, &UZonefallFancyMenuWidget::HandleFindButtonClicked);
	}
	if (JoinButton)
	{
		JoinButton->OnClicked.RemoveDynamic(this, &UZonefallFancyMenuWidget::HandleJoinButtonClicked);
		JoinButton->OnClicked.AddDynamic(this, &UZonefallFancyMenuWidget::HandleJoinButtonClicked);
	}
	if (JoinModernButton)
	{
		JoinModernButton->OnClicked.RemoveDynamic(this, &UZonefallFancyMenuWidget::HandleJoinButtonClicked);
		JoinModernButton->OnClicked.AddDynamic(this, &UZonefallFancyMenuWidget::HandleJoinButtonClicked);
	}
	if (LeaveButton)
	{
		LeaveButton->OnClicked.RemoveDynamic(this, &UZonefallFancyMenuWidget::HandleLeaveButtonClicked);
		LeaveButton->OnClicked.AddDynamic(this, &UZonefallFancyMenuWidget::HandleLeaveButtonClicked);
	}
	if (LeaveModernButton)
	{
		LeaveModernButton->OnClicked.RemoveDynamic(this, &UZonefallFancyMenuWidget::HandleLeaveButtonClicked);
		LeaveModernButton->OnClicked.AddDynamic(this, &UZonefallFancyMenuWidget::HandleLeaveButtonClicked);
	}

	if (UUIWorldMenuGameInstance* GI = GetMenuGameInstance())
	{
		if (!GI->OnHostCompleted.IsAlreadyBound(this, &UZonefallFancyMenuWidget::HandleHostCompleted))
		{
			GI->OnHostCompleted.AddDynamic(this, &UZonefallFancyMenuWidget::HandleHostCompleted);
		}
		if (!GI->OnJoinCompleted.IsAlreadyBound(this, &UZonefallFancyMenuWidget::HandleJoinCompleted))
		{
			GI->OnJoinCompleted.AddDynamic(this, &UZonefallFancyMenuWidget::HandleJoinCompleted);
		}
		if (!GI->OnLeaveCompleted.IsAlreadyBound(this, &UZonefallFancyMenuWidget::HandleLeaveCompleted))
		{
			GI->OnLeaveCompleted.AddDynamic(this, &UZonefallFancyMenuWidget::HandleLeaveCompleted);
		}
		if (!GI->OnSessionsFound.IsAlreadyBound(this, &UZonefallFancyMenuWidget::HandleSessionsFound))
		{
			GI->OnSessionsFound.AddDynamic(this, &UZonefallFancyMenuWidget::HandleSessionsFound);
		}
	}

	if (SessionsComboBox)
	{
		SessionsComboBox->OnSelectionChanged.RemoveDynamic(this, &UZonefallFancyMenuWidget::HandleSessionSelectionChanged);
		SessionsComboBox->OnSelectionChanged.AddDynamic(this, &UZonefallFancyMenuWidget::HandleSessionSelectionChanged);
	}
	if (OpenOnlineButton)
	{
		OpenOnlineButton->OnClicked.RemoveDynamic(this, &UZonefallFancyMenuWidget::HandleOpenOnlineClicked);
		OpenOnlineButton->OnClicked.AddDynamic(this, &UZonefallFancyMenuWidget::HandleOpenOnlineClicked);
	}
	if (BackToMainMenuButton)
	{
		BackToMainMenuButton->OnClicked.RemoveDynamic(this, &UZonefallFancyMenuWidget::HandleBackToMainMenuClicked);
		BackToMainMenuButton->OnClicked.AddDynamic(this, &UZonefallFancyMenuWidget::HandleBackToMainMenuClicked);
	}

	ApplyAaaStyleToOnlineButtons();
}

void UZonefallFancyMenuWidget::RefreshOnlineSessionsUI()
{
	CachedSessionResults.Reset();
	if (SessionsComboBox)
	{
		SessionsComboBox->ClearOptions();
		SessionsComboBox->AddOption(OnlineNoSessionsText.ToString());
		SessionsComboBox->SetSelectedIndex(0);
	}
	CachedSessionCount = 0;
	SetJoinEnabled(false);
	UpdateSelectedServerCard();
	SetOnlineStatusText(OnlineStatusIdleText);
}

void UZonefallFancyMenuWidget::SetOnlineStatusText(const FText& NewStatus)
{
	if (OnlineStatusText)
	{
		OnlineStatusText->SetText(NewStatus);
	}
}

void UZonefallFancyMenuWidget::UpdateSelectedServerCard()
{
	const int32 SelectedIndex = SessionsComboBox ? SessionsComboBox->GetSelectedIndex() : INDEX_NONE;
	if (!CachedSessionResults.IsValidIndex(SelectedIndex))
	{
		if (ServerNameText)
		{
			ServerNameText->SetText(FText::FromString(FString::Printf(TEXT("%s: -"), *ServerNamePrefixText.ToString())));
		}
		if (ServerPlayersText)
		{
			ServerPlayersText->SetText(FText::FromString(FString::Printf(TEXT("%s: -"), *ServerPlayersPrefixText.ToString())));
		}
		if (ServerPingText)
		{
			ServerPingText->SetText(FText::FromString(FString::Printf(TEXT("%s: -"), *ServerPingPrefixText.ToString())));
		}
		if (LocalIpText)
		{
			LocalIpText->SetText(FText::FromString(FString::Printf(TEXT("%s: -"), *ServerLocalIpPrefixText.ToString())));
		}
		return;
	}

	const FUIWorldOnlineSessionResult& R = CachedSessionResults[SelectedIndex];
	if (ServerNameText)
	{
		ServerNameText->SetText(FText::FromString(FString::Printf(TEXT("%s: %s"), *ServerNamePrefixText.ToString(), *R.ServerName)));
	}
	if (ServerPlayersText)
	{
		ServerPlayersText->SetText(FText::FromString(FString::Printf(TEXT("%s: %d/%d"), *ServerPlayersPrefixText.ToString(), R.CurrentPlayers, R.MaxPlayers)));
	}
	if (ServerPingText)
	{
		ServerPingText->SetText(FText::FromString(FString::Printf(TEXT("%s: %d ms"), *ServerPingPrefixText.ToString(), R.PingMs)));
	}
	if (LocalIpText)
	{
		LocalIpText->SetText(FText::FromString(FString::Printf(TEXT("%s: -"), *ServerLocalIpPrefixText.ToString())));
	}
}

void UZonefallFancyMenuWidget::SetNativeButtonLabel(UButton* NativeButton, const FText& Label)
{
	if (!NativeButton)
	{
		return;
	}
	UTextBlock* ChildText = Cast<UTextBlock>(NativeButton->GetChildAt(0));
	if (ChildText)
	{
		ChildText->SetText(Label);
	}
}

void UZonefallFancyMenuWidget::ApplyOnlineButtonTexts()
{
	SetNativeButtonLabel(HostButton, OnlineHostButtonText);
	SetNativeButtonLabel(FindButton, OnlineFindButtonText);
	SetNativeButtonLabel(JoinButton, OnlineJoinButtonText);
	SetNativeButtonLabel(LeaveButton, OnlineLeaveButtonText);

	if (HostModernButton) { HostModernButton->SetLabel(OnlineHostButtonText); }
	if (FindModernButton) { FindModernButton->SetLabel(OnlineFindButtonText); }
	if (JoinModernButton) { JoinModernButton->SetLabel(OnlineJoinButtonText); }
	if (LeaveModernButton) { LeaveModernButton->SetLabel(OnlineLeaveButtonText); }
}

void UZonefallFancyMenuWidget::ApplyAaaStyleToOnlineButtons()
{
	if (!bApplyAaaAutoStyleToOnlineButtons)
	{
		return;
	}

	const auto StyleButton = [](UButton* Button)
	{
		if (!Button)
		{
			return;
		}
		FButtonStyle Style = Button->GetStyle();
		Style.SetNormal(FSlateRoundedBoxBrush(FLinearColor(0.04f, 0.08f, 0.16f, 0.92f), 10.0f));
		Style.SetHovered(FSlateRoundedBoxBrush(FLinearColor(0.09f, 0.18f, 0.30f, 1.0f), 10.0f));
		Style.SetPressed(FSlateRoundedBoxBrush(FLinearColor(0.03f, 0.06f, 0.12f, 1.0f), 10.0f));
		Style.NormalPadding = FMargin(16.f, 10.f);
		Style.PressedPadding = FMargin(16.f, 12.f, 16.f, 8.f);
		Button->SetStyle(Style);
	};

	StyleButton(HostButton);
	StyleButton(FindButton);
	StyleButton(JoinButton);
	StyleButton(LeaveButton);

	const auto StyleModern = [](UZonefallModernButtonWidget* Modern)
	{
		if (!Modern)
		{
			return;
		}
		Modern->bUseGlassGradientStyle = true;
		Modern->ApplyThemePreset(EZonefallModernButtonTheme::Neon);
		Modern->ApplyVisualStyle();
	};

	StyleModern(HostModernButton);
	StyleModern(FindModernButton);
	StyleModern(JoinModernButton);
	StyleModern(LeaveModernButton);
}

void UZonefallFancyMenuWidget::SetJoinEnabled(bool bEnabled)
{
	if (JoinButton)
	{
		JoinButton->SetIsEnabled(bEnabled);
	}
	if (JoinModernButton)
	{
		JoinModernButton->SetIsEnabled(bEnabled);
	}
}

void UZonefallFancyMenuWidget::OpenOnlinePanel()
{
	if (MainMenuPanelWidget)
	{
		MainMenuPanelWidget->SetVisibility(ESlateVisibility::Collapsed);
	}
	if (OnlinePanelWidget)
	{
		OnlinePanelWidget->SetVisibility(ESlateVisibility::Visible);
	}
	SetOnlineStatusText(OnlineStatusIdleText);
}

void UZonefallFancyMenuWidget::BackToMainMenuPanel()
{
	if (OnlinePanelWidget)
	{
		OnlinePanelWidget->SetVisibility(ESlateVisibility::Collapsed);
	}
	if (MainMenuPanelWidget)
	{
		MainMenuPanelWidget->SetVisibility(ESlateVisibility::Visible);
	}
}

void UZonefallFancyMenuWidget::UpdateHorrorFX(float InDeltaTime)
{
	if (!bEnableHorrorFX)
	{
		SetRenderOpacity(1.0f);
		SetRenderTranslation(FVector2D::ZeroVector);
		return;
	}

	HorrorFXTimeSeconds += InDeltaTime;
	const float Pulse = 0.5f + 0.5f * FMath::Sin(HorrorFXTimeSeconds * PulseSpeed * 2.0f * PI);

	const float FlickerRoll = FMath::FRandRange(0.0f, 1.0f);
	const float TriggerChance = FlickerChancePerSecond * InDeltaTime;
	if (FlickerRoll < TriggerChance)
	{
		FlickerDarkenAlpha = FMath::Max(FlickerDarkenAlpha, FlickerDarkenStrength);
	}
	FlickerDarkenAlpha = FMath::FInterpTo(FlickerDarkenAlpha, 0.0f, InDeltaTime, FlickerRecoverSpeed);

	const float Opacity = 1.0f - (PulseOpacityStrength * Pulse) - FlickerDarkenAlpha;
	SetRenderOpacity(FMath::Clamp(Opacity, 0.45f, 1.0f));

	const float JitterX = FMath::FRandRange(-JitterPixels, JitterPixels);
	const float JitterY = FMath::FRandRange(-JitterPixels, JitterPixels);
	SetRenderTranslation(FVector2D(JitterX, JitterY));

	if (HorrorBackgroundWidget)
	{
		const float BackAlpha = 0.65f + (0.20f * Pulse) + (0.30f * FlickerDarkenAlpha);
		HorrorBackgroundWidget->SetBrushColor(FLinearColor(0.04f, 0.07f, 0.04f, FMath::Clamp(BackAlpha, 0.2f, 0.95f)));
	}
}

void UZonefallFancyMenuWidget::HandleMainButtonClicked()
{
	if (bMainActionInProgress)
	{
		return;
	}
	bMainActionInProgress = true;
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimerForNextTick([this]()
		{
			bMainActionInProgress = false;
		});
	}
	OnMainButtonClicked.Broadcast();
}

void UZonefallFancyMenuWidget::HandleMainButtonHovered()
{
	ApplyTextStyle(HoverTextColor);
}

void UZonefallFancyMenuWidget::HandleMainButtonUnhovered()
{
	ApplyTextStyle(NormalTextColor);
}

void UZonefallFancyMenuWidget::HandleMainButtonPressed()
{
	ApplyTextStyle(PressedTextColor);
}

void UZonefallFancyMenuWidget::HandleMainButtonReleased()
{
	ApplyTextStyle(HoverTextColor);
}

void UZonefallFancyMenuWidget::SetContinueButtonVisible(bool bVisible)
{
	const ESlateVisibility TargetVisibility = bVisible ? ESlateVisibility::Visible : ESlateVisibility::Collapsed;

	if (ContinueButton)
	{
		ContinueButton->SetVisibility(TargetVisibility);
	}
	if (ContinueWidgetFallback)
	{
		ContinueWidgetFallback->SetVisibility(TargetVisibility);
	}

	if (!bVisible)
	{
		bContinueActionInProgress = false;
	}
}

void UZonefallFancyMenuWidget::HandleContinueButtonClicked()
{
	if (bContinueActionInProgress)
	{
		return;
	}
	bContinueActionInProgress = true;
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimerForNextTick([this]()
		{
			bContinueActionInProgress = false;
		});
	}
	OnContinueButtonClicked.Broadcast();
}

void UZonefallFancyMenuWidget::HandleContinueButtonHovered()
{
	if (ContinueButtonTextBlock)
	{
		ContinueButtonTextBlock->SetColorAndOpacity(FSlateColor(HoverTextColor));
	}
}

void UZonefallFancyMenuWidget::HandleContinueButtonUnhovered()
{
	if (ContinueButtonTextBlock)
	{
		ContinueButtonTextBlock->SetColorAndOpacity(FSlateColor(NormalTextColor));
	}
}

void UZonefallFancyMenuWidget::HandleHostButtonClicked()
{
	OnOnlineHostClicked.Broadcast();
	if (UUIWorldMenuGameInstance* GI = GetMenuGameInstance())
	{
		SetOnlineStatusText(OnlineStatusHostingText);
		GI->HostOnlineSession(OnlineHostMaxPlayers, bUseLanForOnline);
	}
}

void UZonefallFancyMenuWidget::HandleFindButtonClicked()
{
	OnOnlineFindClicked.Broadcast();
	if (UUIWorldMenuGameInstance* GI = GetMenuGameInstance())
	{
		SetOnlineStatusText(OnlineStatusSearchingText);
		GI->FindOnlineSessions(50, bUseLanForOnline);
	}
}

void UZonefallFancyMenuWidget::HandleJoinButtonClicked()
{
	int32 SelectedIndex = INDEX_NONE;
	if (SessionsComboBox)
	{
		SelectedIndex = SessionsComboBox->GetSelectedIndex();
	}
	if (SelectedIndex < 0 || SelectedIndex >= CachedSessionCount)
	{
		SetOnlineStatusText(OnlineStatusSelectServerText);
		return;
	}

	OnOnlineJoinClicked.Broadcast(SelectedIndex);
	if (UUIWorldMenuGameInstance* GI = GetMenuGameInstance())
	{
		SetOnlineStatusText(OnlineStatusJoiningText);
		GI->JoinOnlineSessionByIndex(SelectedIndex);
	}
}

void UZonefallFancyMenuWidget::HandleLeaveButtonClicked()
{
	OnOnlineLeaveClicked.Broadcast();
	if (UUIWorldMenuGameInstance* GI = GetMenuGameInstance())
	{
		SetOnlineStatusText(OnlineStatusLeavingText);
		GI->LeaveOnlineSessionAndReturnToMenu();
	}
}

void UZonefallFancyMenuWidget::HandleHostCompleted(bool bSuccess, const FString& Message)
{
	const FText Status = bSuccess
		? (Message.IsEmpty() ? OnlineStatusHostSuccessText : FText::FromString(Message))
		: FText::FromString(FString::Printf(TEXT("Host failed: %s"), *Message));
	SetOnlineStatusText(Status);

	if (bSuccess && LocalIpText)
	{
		FString ParsedIp = TEXT("-");
		const FString Marker = TEXT("LAN IP:");
		int32 MarkerIndex = INDEX_NONE;
		MarkerIndex = Message.Find(Marker, ESearchCase::IgnoreCase, ESearchDir::FromStart);
		if (MarkerIndex != INDEX_NONE)
		{
			ParsedIp = Message.Mid(MarkerIndex + Marker.Len()).TrimStartAndEnd();
		}
		LocalIpText->SetText(FText::FromString(FString::Printf(TEXT("%s: %s"), *ServerLocalIpPrefixText.ToString(), *ParsedIp)));
	}
}

void UZonefallFancyMenuWidget::HandleJoinCompleted(bool bSuccess, const FString& Message)
{
	const FText Status = bSuccess
		? OnlineStatusJoinSuccessText
		: FText::FromString(FString::Printf(TEXT("Join failed: %s"), *Message));
	SetOnlineStatusText(Status);
}

void UZonefallFancyMenuWidget::HandleLeaveCompleted(bool bSuccess, const FString& Message)
{
	const FText Status = bSuccess
		? OnlineStatusLeaveSuccessText
		: FText::FromString(FString::Printf(TEXT("Leave failed: %s"), *Message));
	SetOnlineStatusText(Status);
}

void UZonefallFancyMenuWidget::HandleSessionsFound(const TArray<FUIWorldOnlineSessionResult>& Results)
{
	CachedSessionCount = Results.Num();
	CachedSessionResults = Results;
	SetJoinEnabled(Results.Num() > 0);

	if (SessionsComboBox)
	{
		SessionsComboBox->ClearOptions();
		if (Results.Num() == 0)
		{
			SessionsComboBox->AddOption(OnlineNoSessionsText.ToString());
			SessionsComboBox->SetSelectedIndex(0);
		}
		else
		{
			for (int32 i = 0; i < Results.Num(); ++i)
			{
				const FUIWorldOnlineSessionResult& R = Results[i];
				const FString Label = FString::Printf(
					TEXT("%d) %s | %d/%d | %d ms"),
					i + 1,
					*R.ServerName,
					R.CurrentPlayers,
					R.MaxPlayers,
					R.PingMs
				);
				SessionsComboBox->AddOption(Label);
			}
			SessionsComboBox->SetSelectedIndex(0);
		}
	}

	UpdateSelectedServerCard();
	SetOnlineStatusText(FText::FromString(FString::Printf(TEXT("Found sessions: %d"), Results.Num())));
}

void UZonefallFancyMenuWidget::HandleSessionSelectionChanged(FString SelectedItem, ESelectInfo::Type SelectionType)
{
	UpdateSelectedServerCard();
}

void UZonefallFancyMenuWidget::HandleOpenOnlineClicked()
{
	OpenOnlinePanel();
}

void UZonefallFancyMenuWidget::HandleBackToMainMenuClicked()
{
	BackToMainMenuPanel();
}

FString UZonefallFancyMenuWidget::GetOnlineBindingReport() const
{
	return FString::Printf(
		TEXT("Host=%s Find=%s Join=%s Leave=%s Sessions=%s Status=%s ServerName=%s Players=%s Ping=%s LocalIp=%s MainPanel=%s OnlinePanel=%s OpenOnline=%s BackMain=%s"),
		(HostButton || HostModernButton) ? TEXT("OK") : TEXT("MISSING"),
		(FindButton || FindModernButton) ? TEXT("OK") : TEXT("MISSING"),
		(JoinButton || JoinModernButton) ? TEXT("OK") : TEXT("MISSING"),
		(LeaveButton || LeaveModernButton) ? TEXT("OK") : TEXT("MISSING"),
		SessionsComboBox ? TEXT("OK") : TEXT("MISSING"),
		OnlineStatusText ? TEXT("OK") : TEXT("MISSING"),
		ServerNameText ? TEXT("OK") : TEXT("MISSING"),
		ServerPlayersText ? TEXT("OK") : TEXT("MISSING"),
		ServerPingText ? TEXT("OK") : TEXT("MISSING"),
		LocalIpText ? TEXT("OK") : TEXT("MISSING"),
		MainMenuPanelWidget ? TEXT("OK") : TEXT("MISSING"),
		OnlinePanelWidget ? TEXT("OK") : TEXT("MISSING"),
		OpenOnlineButton ? TEXT("OK") : TEXT("MISSING"),
		BackToMainMenuButton ? TEXT("OK") : TEXT("MISSING")
	);
}

