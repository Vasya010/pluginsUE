#include "UI/UIWorldSimpleMainMenuWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Button.h"
#include "Components/ButtonSlot.h"
#include "Components/ContentWidget.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Math/UnrealMathUtility.h"
#include "UIWorldMenuGameInstance.h"

void UUIWorldSimpleMainMenuWidget::NativeConstruct()
{
	Super::NativeConstruct();

	BuildLayoutIfNeeded();
	ResolveButtons();
	BindEvents();
	ApplyBaseVisualStyle();
}

void UUIWorldSimpleMainMenuWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
	UpdateAnimatedVisuals(InDeltaTime);
}

void UUIWorldSimpleMainMenuWidget::BuildLayoutIfNeeded()
{
	if (!WidgetTree || (WidgetTree->RootWidget && !bBuildFallbackLayout))
	{
		return;
	}

	UVerticalBox* RootBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("SimpleMainMenuRoot"));
	WidgetTree->RootWidget = RootBox;

	NewGameButton = CreateMenuButton(TEXT("NewGameButton"), FText::FromString(TEXT("NEW GAME")), RootBox);
	ContinueButton = CreateMenuButton(TEXT("ContinueButton"), FText::FromString(TEXT("CONTINUE")), RootBox);
	SettingsButton = CreateMenuButton(TEXT("SettingsButton"), FText::FromString(TEXT("SETTINGS")), RootBox);
	OnlineButton = CreateMenuButton(TEXT("OnlineButton"), FText::FromString(TEXT("ONLINE")), RootBox);
	QuitButton = CreateMenuButton(TEXT("QuitButton"), FText::FromString(TEXT("QUIT")), RootBox);
}

void UUIWorldSimpleMainMenuWidget::ResolveButtons()
{
	if (!WidgetTree)
	{
		return;
	}

	if (!NewGameButton)
	{
		NewGameButton = Cast<UButton>(WidgetTree->FindWidget(NewGameButtonName.IsNone() ? TEXT("NewGameButton") : NewGameButtonName));
	}
	if (!ContinueButton)
	{
		ContinueButton = Cast<UButton>(WidgetTree->FindWidget(ContinueButtonName.IsNone() ? TEXT("ContinueButton") : ContinueButtonName));
	}
	if (!SettingsButton)
	{
		SettingsButton = Cast<UButton>(WidgetTree->FindWidget(SettingsButtonName.IsNone() ? TEXT("SettingsButton") : SettingsButtonName));
	}
	if (!OnlineButton)
	{
		OnlineButton = Cast<UButton>(WidgetTree->FindWidget(OnlineButtonName.IsNone() ? TEXT("OnlineButton") : OnlineButtonName));
	}
	if (!QuitButton)
	{
		QuitButton = Cast<UButton>(WidgetTree->FindWidget(QuitButtonName.IsNone() ? TEXT("QuitButton") : QuitButtonName));
	}
}

void UUIWorldSimpleMainMenuWidget::BindEvents()
{
	if (NewGameButton)
	{
		NewGameButton->OnClicked.RemoveDynamic(this, &UUIWorldSimpleMainMenuWidget::HandleNewGameClicked);
		NewGameButton->OnClicked.AddDynamic(this, &UUIWorldSimpleMainMenuWidget::HandleNewGameClicked);
		NewGameButton->OnHovered.RemoveDynamic(this, &UUIWorldSimpleMainMenuWidget::HandleNewGameHovered);
		NewGameButton->OnHovered.AddDynamic(this, &UUIWorldSimpleMainMenuWidget::HandleNewGameHovered);
		NewGameButton->OnUnhovered.RemoveDynamic(this, &UUIWorldSimpleMainMenuWidget::HandleNewGameUnhovered);
		NewGameButton->OnUnhovered.AddDynamic(this, &UUIWorldSimpleMainMenuWidget::HandleNewGameUnhovered);
	}
	if (ContinueButton)
	{
		ContinueButton->OnClicked.RemoveDynamic(this, &UUIWorldSimpleMainMenuWidget::HandleContinueClicked);
		ContinueButton->OnClicked.AddDynamic(this, &UUIWorldSimpleMainMenuWidget::HandleContinueClicked);
		ContinueButton->OnHovered.RemoveDynamic(this, &UUIWorldSimpleMainMenuWidget::HandleContinueHovered);
		ContinueButton->OnHovered.AddDynamic(this, &UUIWorldSimpleMainMenuWidget::HandleContinueHovered);
		ContinueButton->OnUnhovered.RemoveDynamic(this, &UUIWorldSimpleMainMenuWidget::HandleContinueUnhovered);
		ContinueButton->OnUnhovered.AddDynamic(this, &UUIWorldSimpleMainMenuWidget::HandleContinueUnhovered);
	}
	if (SettingsButton)
	{
		SettingsButton->OnClicked.RemoveDynamic(this, &UUIWorldSimpleMainMenuWidget::HandleSettingsClicked);
		SettingsButton->OnClicked.AddDynamic(this, &UUIWorldSimpleMainMenuWidget::HandleSettingsClicked);
		SettingsButton->OnHovered.RemoveDynamic(this, &UUIWorldSimpleMainMenuWidget::HandleSettingsHovered);
		SettingsButton->OnHovered.AddDynamic(this, &UUIWorldSimpleMainMenuWidget::HandleSettingsHovered);
		SettingsButton->OnUnhovered.RemoveDynamic(this, &UUIWorldSimpleMainMenuWidget::HandleSettingsUnhovered);
		SettingsButton->OnUnhovered.AddDynamic(this, &UUIWorldSimpleMainMenuWidget::HandleSettingsUnhovered);
	}
	if (OnlineButton)
	{
		OnlineButton->OnClicked.RemoveDynamic(this, &UUIWorldSimpleMainMenuWidget::HandleOnlineClicked);
		OnlineButton->OnClicked.AddDynamic(this, &UUIWorldSimpleMainMenuWidget::HandleOnlineClicked);
		OnlineButton->OnHovered.RemoveDynamic(this, &UUIWorldSimpleMainMenuWidget::HandleOnlineHovered);
		OnlineButton->OnHovered.AddDynamic(this, &UUIWorldSimpleMainMenuWidget::HandleOnlineHovered);
		OnlineButton->OnUnhovered.RemoveDynamic(this, &UUIWorldSimpleMainMenuWidget::HandleOnlineUnhovered);
		OnlineButton->OnUnhovered.AddDynamic(this, &UUIWorldSimpleMainMenuWidget::HandleOnlineUnhovered);
	}
	if (QuitButton)
	{
		QuitButton->OnClicked.RemoveDynamic(this, &UUIWorldSimpleMainMenuWidget::HandleQuitClicked);
		QuitButton->OnClicked.AddDynamic(this, &UUIWorldSimpleMainMenuWidget::HandleQuitClicked);
		QuitButton->OnHovered.RemoveDynamic(this, &UUIWorldSimpleMainMenuWidget::HandleQuitHovered);
		QuitButton->OnHovered.AddDynamic(this, &UUIWorldSimpleMainMenuWidget::HandleQuitHovered);
		QuitButton->OnUnhovered.RemoveDynamic(this, &UUIWorldSimpleMainMenuWidget::HandleQuitUnhovered);
		QuitButton->OnUnhovered.AddDynamic(this, &UUIWorldSimpleMainMenuWidget::HandleQuitUnhovered);
	}
}

void UUIWorldSimpleMainMenuWidget::ApplyBaseVisualStyle()
{
	NewGameText = GetButtonText(NewGameButton);
	ContinueText = GetButtonText(ContinueButton);
	SettingsText = GetButtonText(SettingsButton);
	OnlineText = GetButtonText(OnlineButton);
	QuitText = GetButtonText(QuitButton);

	IntroProgress = 0.0f;
	SetRenderOpacity(0.0f);
	FWidgetTransform IntroTransform;
	IntroTransform.Translation = FVector2D(-34.0f, 0.0f);
	SetRenderTransform(IntroTransform);

	const FLinearColor BaseButtonColor(0.02f, 0.03f, 0.02f, 0.78f);
	const FLinearColor HoverButtonColor(0.12f, 0.14f, 0.07f, 0.90f);

	TArray<UButton*> Buttons = { NewGameButton, ContinueButton, SettingsButton, OnlineButton, QuitButton };
	for (UButton* Button : Buttons)
	{
		if (!Button)
		{
			continue;
		}

		Button->SetColorAndOpacity(BaseButtonColor);
		FButtonStyle Style = Button->GetStyle();
		Style.Normal.SetImageSize(FVector2D(300.0f, 44.0f));
		Style.Hovered.SetImageSize(FVector2D(300.0f, 44.0f));
		Style.Pressed.SetImageSize(FVector2D(300.0f, 44.0f));
		Style.Normal.TintColor = FSlateColor(BaseButtonColor);
		Style.Hovered.TintColor = FSlateColor(HoverButtonColor);
		Style.Pressed.TintColor = FSlateColor(HoverButtonColor * 0.85f);
		Button->SetStyle(Style);
		Button->SetRenderScale(FVector2D(1.0f, 1.0f));
	}

	TArray<UTextBlock*> Texts = { NewGameText, ContinueText, SettingsText, OnlineText, QuitText };
	for (UTextBlock* Text : Texts)
	{
		if (Text)
		{
			Text->SetColorAndOpacity(FSlateColor(IdleTextColor));
			Text->SetFont(FSlateFontInfo(FCoreStyle::GetDefaultFontStyle("Regular", 26)));
			Text->SetShadowColorAndOpacity(FLinearColor(0.0f, 0.0f, 0.0f, 0.85f));
			Text->SetShadowOffset(FVector2D(1.0f, 1.0f));
		}
	}
}

void UUIWorldSimpleMainMenuWidget::UpdateAnimatedVisuals(float DeltaSeconds)
{
	const float SafeDuration = FMath::Max(0.01f, IntroDurationSeconds);
	IntroProgress = FMath::Clamp(IntroProgress + (DeltaSeconds / SafeDuration), 0.0f, 1.0f);
	const float IntroAlpha = FMath::InterpEaseOut(0.0f, 1.0f, IntroProgress, 2.0f);
	SetRenderOpacity(IntroAlpha);
	FWidgetTransform IntroTransform;
	IntroTransform.Translation = FVector2D(FMath::Lerp(-34.0f, 0.0f, IntroAlpha), 0.0f);
	SetRenderTransform(IntroTransform);

	auto UpdateButtonAnim = [this, DeltaSeconds](UButton* Button, UTextBlock* Text, bool bHovered, float& HoverAlpha, float PulseOffset)
	{
		if (!Button || !Text)
		{
			return;
		}

		HoverAlpha = FMath::FInterpTo(HoverAlpha, bHovered ? 1.0f : 0.0f, DeltaSeconds, HoverInterpSpeed);
		const float Pulse = bHovered ? (FMath::Sin(GetWorld()->TimeSeconds * PulseSpeed + PulseOffset) * PulseAmplitude) : 0.0f;
		const float TargetScale = FMath::Lerp(1.0f, HoverScale, HoverAlpha) + Pulse;
		Button->SetRenderScale(FVector2D(TargetScale, TargetScale));

		const FLinearColor TextColor = FLinearColor::LerpUsingHSV(IdleTextColor, HoverTextColor, HoverAlpha);
		const FLinearColor FinalTextColor = FLinearColor::LerpUsingHSV(TextColor, AccentTextColor, HoverAlpha * 0.35f);
		Text->SetColorAndOpacity(FSlateColor(FinalTextColor));

		const FLinearColor ButtonColor = FLinearColor::LerpUsingHSV(
			FLinearColor(0.02f, 0.03f, 0.02f, 0.78f),
			FLinearColor(0.10f, 0.12f, 0.05f, 0.92f),
			HoverAlpha);
		Button->SetColorAndOpacity(ButtonColor);
	};

	UpdateButtonAnim(NewGameButton, NewGameText, bNewGameHovered, HoverNewGame, 0.0f);
	UpdateButtonAnim(ContinueButton, ContinueText, bContinueHovered, HoverContinue, 1.1f);
	UpdateButtonAnim(SettingsButton, SettingsText, bSettingsHovered, HoverSettings, 2.3f);
	UpdateButtonAnim(OnlineButton, OnlineText, bOnlineHovered, HoverOnline, 3.7f);
	UpdateButtonAnim(QuitButton, QuitText, bQuitHovered, HoverQuit, 4.9f);
}

void UUIWorldSimpleMainMenuWidget::SetButtonHoveredState(UButton* Button, bool bHovered)
{
	if (Button == NewGameButton)
	{
		bNewGameHovered = bHovered;
	}
	else if (Button == ContinueButton)
	{
		bContinueHovered = bHovered;
	}
	else if (Button == SettingsButton)
	{
		bSettingsHovered = bHovered;
	}
	else if (Button == OnlineButton)
	{
		bOnlineHovered = bHovered;
	}
	else if (Button == QuitButton)
	{
		bQuitHovered = bHovered;
	}
}

UTextBlock* UUIWorldSimpleMainMenuWidget::GetButtonText(UButton* Button) const
{
	if (!Button)
	{
		return nullptr;
	}
	return Cast<UTextBlock>(Button->GetContent());
}

UButton* UUIWorldSimpleMainMenuWidget::CreateMenuButton(const FName Name, const FText& Label, UVerticalBox* ParentBox)
{
	if (!WidgetTree || !ParentBox)
	{
		return nullptr;
	}

	UButton* Button = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), Name);
	if (UVerticalBoxSlot* VBoxSlot = ParentBox->AddChildToVerticalBox(Button))
	{
		VBoxSlot->SetPadding(FMargin(0.0f, 8.0f, 0.0f, 8.0f));
		VBoxSlot->SetHorizontalAlignment(HAlign_Center);
	}

	UTextBlock* Text = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	Text->SetText(Label);
	Text->SetJustification(ETextJustify::Center);
	Text->SetColorAndOpacity(FSlateColor(FLinearColor::White));
	Button->AddChild(Text);

	return Button;
}

UUIWorldMenuGameInstance* UUIWorldSimpleMainMenuWidget::GetMenuGameInstance() const
{
	if (UWorld* World = GetWorld())
	{
		return Cast<UUIWorldMenuGameInstance>(World->GetGameInstance());
	}
	return nullptr;
}

void UUIWorldSimpleMainMenuWidget::HandleNewGameClicked()
{
	if (UUIWorldMenuGameInstance* GI = GetMenuGameInstance())
	{
		// New Game opens the character creator; its CREATE & START button begins gameplay.
		GI->OpenCharacterCreator();
	}
}

void UUIWorldSimpleMainMenuWidget::HandleContinueClicked()
{
	if (UUIWorldMenuGameInstance* GI = GetMenuGameInstance())
	{
		// From the main menu, "Continue" loads the saved level and restores the full snapshot
		// (health, weapons + ammo, picked-up items, position). Falls back to a plain resume
		// if there is no save on disk.
		GI->ContinueSavedGame();
	}
}

void UUIWorldSimpleMainMenuWidget::HandleSettingsClicked()
{
	if (UUIWorldMenuGameInstance* GI = GetMenuGameInstance())
	{
		GI->ShowMenuFromList(EUIWorldMenuScreen::SettingsMenu, false);
	}
}

void UUIWorldSimpleMainMenuWidget::HandleOnlineClicked()
{
	if (UUIWorldMenuGameInstance* GI = GetMenuGameInstance())
	{
		GI->ShowMenuFromList(EUIWorldMenuScreen::OnlineMenu, false);
	}
}

void UUIWorldSimpleMainMenuWidget::HandleQuitClicked()
{
	if (UUIWorldMenuGameInstance* GI = GetMenuGameInstance())
	{
		GI->QuitGameNow(false);
	}
}

void UUIWorldSimpleMainMenuWidget::HandleNewGameHovered()
{
	SetButtonHoveredState(NewGameButton, true);
}

void UUIWorldSimpleMainMenuWidget::HandleNewGameUnhovered()
{
	SetButtonHoveredState(NewGameButton, false);
}

void UUIWorldSimpleMainMenuWidget::HandleContinueHovered()
{
	SetButtonHoveredState(ContinueButton, true);
}

void UUIWorldSimpleMainMenuWidget::HandleContinueUnhovered()
{
	SetButtonHoveredState(ContinueButton, false);
}

void UUIWorldSimpleMainMenuWidget::HandleSettingsHovered()
{
	SetButtonHoveredState(SettingsButton, true);
}

void UUIWorldSimpleMainMenuWidget::HandleSettingsUnhovered()
{
	SetButtonHoveredState(SettingsButton, false);
}

void UUIWorldSimpleMainMenuWidget::HandleOnlineHovered()
{
	SetButtonHoveredState(OnlineButton, true);
}

void UUIWorldSimpleMainMenuWidget::HandleOnlineUnhovered()
{
	SetButtonHoveredState(OnlineButton, false);
}

void UUIWorldSimpleMainMenuWidget::HandleQuitHovered()
{
	SetButtonHoveredState(QuitButton, true);
}

void UUIWorldSimpleMainMenuWidget::HandleQuitUnhovered()
{
	SetButtonHoveredState(QuitButton, false);
}
