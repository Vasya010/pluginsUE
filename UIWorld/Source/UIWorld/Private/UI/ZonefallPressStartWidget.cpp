#include "UI/ZonefallPressStartWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Brushes/SlateRoundedBoxBrush.h"
#include "Components/Border.h"
#include "Components/Image.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Engine/Font.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"

namespace
{
	FSlateFontInfo MakePSFont(int32 Size, int32 Spacing)
	{
		FSlateFontInfo Font;
		Font.Size = FMath::Clamp(Size, 8, 120);
		Font.FontObject = LoadObject<UObject>(nullptr, TEXT("/Engine/EngineFonts/Roboto.Roboto"));
		Font.LetterSpacing = Spacing;
		return Font;
	}
}

UZonefallPressStartWidget::UZonefallPressStartWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

TSharedRef<SWidget> UZonefallPressStartWidget::RebuildWidget()
{
	if (!WidgetTree || !WidgetTree->RootWidget)
	{
		BuildLayout();
	}
	return Super::RebuildWidget();
}

void UZonefallPressStartWidget::BuildLayout()
{
	if (!WidgetTree)
	{
		return;
	}

	WidgetTree->RootWidget = nullptr;

	UOverlay* Root = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass(), TEXT("PressStartRoot"));
	WidgetTree->RootWidget = Root;

	// --- Cinematic dark backdrop (RDR2/GTA loading look; the scene shows faintly behind) ---
	UBorder* Backdrop = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("PressStartBackdrop"));
	Backdrop->SetBrush(FSlateRoundedBoxBrush(BackdropTint, 0.0f));
	if (UOverlaySlot* BSlot = Root->AddChildToOverlay(Backdrop))
	{
		BSlot->SetHorizontalAlignment(HAlign_Fill);
		BSlot->SetVerticalAlignment(VAlign_Fill);
	}

	// --- Single centred block: title + thin warm divider + one prompt (RDR2-style, elegant) ---
	UVerticalBox* Col = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("PressStartCol"));
	if (UOverlaySlot* ColSlot = Root->AddChildToOverlay(Col))
	{
		ColSlot->SetHorizontalAlignment(HAlign_Center);
		ColSlot->SetVerticalAlignment(VAlign_Center);
	}

	TitleLabel = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("PressStartTitle"));
	TitleLabel->SetText(TitleText);
	TitleLabel->SetFont(MakePSFont(58, 240));
	TitleLabel->SetColorAndOpacity(FSlateColor(TextColor));
	TitleLabel->SetJustification(ETextJustify::Center);
	TitleLabel->SetShadowOffset(FVector2D(0.0f, 3.0f));
	TitleLabel->SetShadowColorAndOpacity(FLinearColor(0.0f, 0.0f, 0.0f, 0.85f));
	if (UVerticalBoxSlot* TSlot = Col->AddChildToVerticalBox(TitleLabel))
	{
		TSlot->SetHorizontalAlignment(HAlign_Center);
	}

	// Thin, understated warm divider (no neon glow).
	TitleUnderline = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("PressStartUnderline"));
	{
		FSlateRoundedBoxBrush UB(AccentColor, 1.0f);
		UB.ImageSize = FVector2D(300.0f, 2.0f);
		TitleUnderline->SetBrush(UB);
		TitleUnderline->SetColorAndOpacity(AccentColor * FLinearColor(1, 1, 1, 0.75f));
	}
	if (UVerticalBoxSlot* USlot = Col->AddChildToVerticalBox(TitleUnderline))
	{
		USlot->SetHorizontalAlignment(HAlign_Center);
		USlot->SetPadding(FMargin(0, 26.0f, 0, 40.0f));
	}

	// The single prompt line, centred — calm warm text, gently breathing.
	PromptLabel = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("PressStartPrompt"));
	PromptLabel->SetText(PromptText);
	PromptLabel->SetFont(MakePSFont(20, 340));
	PromptLabel->SetColorAndOpacity(FSlateColor(TextColor));
	PromptLabel->SetJustification(ETextJustify::Center);
	PromptLabel->SetShadowOffset(FVector2D(0.0f, 1.0f));
	PromptLabel->SetShadowColorAndOpacity(FLinearColor(0.0f, 0.0f, 0.0f, 0.7f));
	if (UVerticalBoxSlot* PSlot = Col->AddChildToVerticalBox(PromptLabel))
	{
		PSlot->SetHorizontalAlignment(HAlign_Center);
	}

	// --- Cinematic letterbox bars (top + bottom), solid black, on top of everything ---
	if (LetterboxHeight > 0.0f)
	{
		auto MakeLetterbox = [this, Root](FName Name, bool bTop)
		{
			UBorder* Bar = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), Name);
			Bar->SetBrush(FSlateRoundedBoxBrush(FLinearColor(0.0f, 0.0f, 0.0f, 1.0f), 0.0f));
			USizeBox* Box = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), FName(*(Name.ToString() + TEXT("_Box"))));
			Box->SetHeightOverride(LetterboxHeight);
			Box->AddChild(Bar);
			if (UOverlaySlot* S = Root->AddChildToOverlay(Box))
			{
				S->SetHorizontalAlignment(HAlign_Fill);
				S->SetVerticalAlignment(bTop ? VAlign_Top : VAlign_Bottom);
			}
		};
		MakeLetterbox(TEXT("PressStartLetterTop"), true);
		MakeLetterbox(TEXT("PressStartLetterBottom"), false);
	}
}

void UZonefallPressStartWidget::NativeConstruct()
{
	Super::NativeConstruct();
	SetIsFocusable(true);
	if (TitleLabel) { TitleLabel->SetText(TitleText); }
	if (PromptLabel) { PromptLabel->SetText(PromptText); }
	SetKeyboardFocus();
}

void UZonefallPressStartWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	if (bConfirmed)
	{
		return;
	}

	AnimTime += InDeltaTime;

	if (PromptLabel)
	{
		// Slow, calm cinematic fade (RDR2-style) — no scale jitter.
		const float Pulse = 0.35f + 0.65f * (0.5f + 0.5f * FMath::Sin(AnimTime * 1.9f));
		PromptLabel->SetRenderOpacity(Pulse);
	}
}

FReply UZonefallPressStartWidget::NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
	const FKey Key = InKeyEvent.GetKey();
	if (Key == EKeys::F || Key == EKeys::Enter || Key == EKeys::SpaceBar ||
		Key == EKeys::Gamepad_FaceButton_Bottom)
	{
		Confirm();
		return FReply::Handled();
	}
	// Any other key also continues (classic "press any key").
	Confirm();
	return FReply::Handled();
}

FReply UZonefallPressStartWidget::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	Confirm();
	return FReply::Handled();
}

void UZonefallPressStartWidget::Confirm()
{
	if (bConfirmed)
	{
		return;
	}
	bConfirmed = true;

	// Vanish instantly — collapse + remove BEFORE doing anything else so there is no lingering frame.
	SetVisibility(ESlateVisibility::Collapsed);
	RemoveFromParent();

	if (!ContinueSound.IsNull())
	{
		if (USoundBase* Snd = ContinueSound.LoadSynchronous())
		{
			UGameplayStatics::PlaySound2D(this, Snd);
		}
	}

	OnContinue.Broadcast();
}
