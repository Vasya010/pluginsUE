#include "UI/ZonefallPauseMenuWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Brushes/SlateRoundedBoxBrush.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/ButtonSlot.h"
#include "Components/Image.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"

#include "Localization/ZonefallLocalizationSubsystem.h"
#include "UIWorldMenuGameInstance.h"

namespace
{
	enum EPauseItem : int32 { Item_Resume = 0, Item_Save = 1, Item_Settings = 2, Item_MainMenu = 3, Item_Quit = 4 };

	FSlateFontInfo MakePauseFont(int32 Size)
	{
		FSlateFontInfo F;
		F.Size = FMath::Clamp(Size, 8, 72);
		F.FontObject = LoadObject<UObject>(nullptr, TEXT("/Engine/EngineFonts/Roboto.Roboto"));
		return F;
	}
}

UZonefallPauseItemButton::UZonefallPauseItemButton()
{
	OnClicked.AddDynamic(this, &UZonefallPauseItemButton::HandleInternalClicked);
}

void UZonefallPauseItemButton::HandleInternalClicked()
{
	OnItemClicked.Broadcast(ItemId);
}

UZonefallPauseMenuWidget::UZonefallPauseMenuWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

TSharedRef<SWidget> UZonefallPauseMenuWidget::RebuildWidget()
{
	if (!WidgetTree || !WidgetTree->RootWidget)
	{
		BuildLayout();
	}
	return Super::RebuildWidget();
}

void UZonefallPauseMenuWidget::NativeConstruct()
{
	Super::NativeConstruct();
	IntroTime = 0.0f;
	SetIsFocusable(true);
	SetKeyboardFocus();
	SetStatus(FText::GetEmpty());
}

UUIWorldMenuGameInstance* UZonefallPauseMenuWidget::ResolveGameInstance() const
{
	return GetGameInstance<UUIWorldMenuGameInstance>();
}

void UZonefallPauseMenuWidget::BuildLayout()
{
	if (!WidgetTree)
	{
		return;
	}

	WidgetTree->RootWidget = nullptr;

	// Full-screen dim backdrop.
	Backdrop = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("PauseBackdrop"));
	Backdrop->SetBrush(FSlateRoundedBoxBrush(BackdropTint, 0.0f));
	Backdrop->SetHorizontalAlignment(HAlign_Fill);
	Backdrop->SetVerticalAlignment(VAlign_Fill);
	WidgetTree->RootWidget = Backdrop;

	UOverlay* Root = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass(), TEXT("PauseOverlay"));
	Backdrop->SetContent(Root);

	// Centered fixed-width panel.
	USizeBox* PanelSize = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("PausePanelSize"));
	PanelSize->SetWidthOverride(440.0f);
	if (UOverlaySlot* PSlot = Root->AddChildToOverlay(PanelSize))
	{
		PSlot->SetHorizontalAlignment(HAlign_Center);
		PSlot->SetVerticalAlignment(VAlign_Center);
	}

	Panel = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("PausePanel"));
	Panel->SetBrush(FSlateRoundedBoxBrush(PanelTint, 14.0f));
	Panel->SetPadding(FMargin(30.0f, 28.0f));
	PanelSize->AddChild(Panel);

	UVerticalBox* Box = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("PauseBox"));
	Panel->SetContent(Box);

	UZonefallLocalizationSubsystem* Loc = UZonefallLocalizationSubsystem::Get(this);
	auto LT = [Loc](const TCHAR* Key, const FText& Fallback) -> FText
	{
		return Loc ? Loc->GetText(FName(Key)) : Fallback;
	};

	// Title.
	TitleLabel = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("PauseTitle"));
	TitleLabel->SetText(Loc ? LT(TEXT("pause.title"), TitleText) : TitleText);
	TitleLabel->SetFont(MakePauseFont(TitleFontSize));
	TitleLabel->SetColorAndOpacity(FSlateColor(FLinearColor::White));
	if (UVerticalBoxSlot* TS = Box->AddChildToVerticalBox(TitleLabel))
	{
		TS->SetHorizontalAlignment(HAlign_Center);
		TS->SetPadding(FMargin(0, 0, 0, 8));
	}

	// Accent line.
	AccentLine = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("PauseAccentLine"));
	{
		FSlateRoundedBoxBrush LineBrush(FLinearColor::White, 2.0f);
		LineBrush.ImageSize = FVector2D(180.0f, 3.0f);
		AccentLine->SetBrush(LineBrush);
		AccentLine->SetColorAndOpacity(AccentColor);
	}
	if (UVerticalBoxSlot* LS = Box->AddChildToVerticalBox(AccentLine))
	{
		LS->SetHorizontalAlignment(HAlign_Center);
		LS->SetPadding(FMargin(0, 0, 0, 22));
	}

	// Menu buttons.
	MenuButtons.Reset();
	AddMenuButton(Box, Item_Resume,   LT(TEXT("pause.resume"),   NSLOCTEXT("ZonefallPause", "Resume", "RESUME")));
	AddMenuButton(Box, Item_Save,     LT(TEXT("pause.save"),     NSLOCTEXT("ZonefallPause", "Save", "SAVE GAME")));
	AddMenuButton(Box, Item_Settings, LT(TEXT("pause.settings"), NSLOCTEXT("ZonefallPause", "Settings", "SETTINGS")));
	AddMenuButton(Box, Item_MainMenu, LT(TEXT("pause.mainmenu"), NSLOCTEXT("ZonefallPause", "MainMenu", "MAIN MENU")));
	AddMenuButton(Box, Item_Quit,     LT(TEXT("pause.quit"),     NSLOCTEXT("ZonefallPause", "Quit", "QUIT")));

	// Status line.
	StatusLabel = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("PauseStatus"));
	StatusLabel->SetFont(MakePauseFont(BodyFontSize - 4));
	StatusLabel->SetColorAndOpacity(FSlateColor(FLinearColor(0.7f, 0.85f, 0.95f, 1.0f)));
	StatusLabel->SetJustification(ETextJustify::Center);
	if (UVerticalBoxSlot* SS = Box->AddChildToVerticalBox(StatusLabel))
	{
		SS->SetHorizontalAlignment(HAlign_Center);
		SS->SetPadding(FMargin(0, 14, 0, 0));
	}
}

UZonefallPauseItemButton* UZonefallPauseMenuWidget::AddMenuButton(UVerticalBox* Parent, int32 ItemId, const FText& Label)
{
	UZonefallPauseItemButton* Btn = WidgetTree->ConstructWidget<UZonefallPauseItemButton>(UZonefallPauseItemButton::StaticClass());
	Btn->ItemId = ItemId;

	FButtonStyle Style = Btn->GetStyle();
	Style.Normal = FSlateRoundedBoxBrush(FLinearColor(0.09f, 0.13f, 0.18f, 0.95f), 6.0f);
	Style.Hovered = FSlateRoundedBoxBrush(AccentColor * 0.5f, 6.0f);
	Style.Pressed = FSlateRoundedBoxBrush(AccentColor * 0.7f, 6.0f);
	Btn->SetStyle(Style);

	UTextBlock* ButtonLabel = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	ButtonLabel->SetText(Label);
	ButtonLabel->SetFont(MakePauseFont(BodyFontSize));
	ButtonLabel->SetColorAndOpacity(FSlateColor(FLinearColor::White));
	Btn->AddChild(ButtonLabel);
	if (UButtonSlot* LabelSlot = Cast<UButtonSlot>(ButtonLabel->Slot))
	{
		LabelSlot->SetHorizontalAlignment(HAlign_Center);
		LabelSlot->SetVerticalAlignment(VAlign_Center);
		LabelSlot->SetPadding(FMargin(14.0f, 11.0f));
	}

	Btn->OnItemClicked.AddDynamic(this, &UZonefallPauseMenuWidget::HandleItemClicked);
	if (UVerticalBoxSlot* BtnSlot = Parent->AddChildToVerticalBox(Btn))
	{
		BtnSlot->SetHorizontalAlignment(HAlign_Fill);
		BtnSlot->SetPadding(FMargin(0, 0, 0, 8));
	}

	MenuButtons.Add(Btn);
	return Btn;
}

void UZonefallPauseMenuWidget::SetStatus(const FText& Text)
{
	if (StatusLabel)
	{
		StatusLabel->SetText(Text);
	}
}

void UZonefallPauseMenuWidget::HandleItemClicked(int32 ItemId)
{
	UUIWorldMenuGameInstance* GI = ResolveGameInstance();

	switch (ItemId)
	{
	case Item_Resume:
		OnResumeRequested.Broadcast();
		if (GI) { GI->ContinueGame(true); }
		RemoveFromParent();
		break;

	case Item_Save:
		OnSaveRequested.Broadcast();
		if (GI)
		{
			const bool bOk = GI->SaveGame();
			SetStatus(bOk
				? NSLOCTEXT("ZonefallPause", "Saved", "Progress saved.")
				: NSLOCTEXT("ZonefallPause", "SaveFail", "Save failed."));
		}
		break;

	case Item_Settings:
		OnSettingsRequested.Broadcast();
		if (GI) { GI->ShowMenuFromList(EUIWorldMenuScreen::SettingsMenu, false); }
		break;

	case Item_MainMenu:
		OnMainMenuRequested.Broadcast();
		if (GI) { GI->LoadMainMenuLevel(true); }
		break;

	case Item_Quit:
		OnQuitRequested.Broadcast();
		if (GI) { GI->QuitGameNow(false); }
		break;

	default:
		break;
	}
}

void UZonefallPauseMenuWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	IntroTime += InDeltaTime;

	// Panel fades + scales in over ~0.3s.
	if (Panel)
	{
		const float In = FMath::Clamp(IntroTime / 0.3f, 0.0f, 1.0f);
		const float Eased = 1.0f - FMath::Pow(1.0f - In, 3.0f);
		Panel->SetRenderOpacity(Eased);
		const float Scale = FMath::Lerp(0.94f, 1.0f, Eased);
		Panel->SetRenderScale(FVector2D(Scale, Scale));
	}

	// Accent line breathes.
	if (AccentLine)
	{
		const float Glow = 0.6f + 0.4f * (0.5f + 0.5f * FMath::Sin(IntroTime * 2.4f));
		AccentLine->SetColorAndOpacity(AccentColor * FLinearColor(1, 1, 1, Glow));
	}
}

FReply UZonefallPauseMenuWidget::NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
	if (InKeyEvent.GetKey() == EKeys::Escape)
	{
		HandleItemClicked(Item_Resume);
		return FReply::Handled();
	}
	return Super::NativeOnKeyDown(InGeometry, InKeyEvent);
}
