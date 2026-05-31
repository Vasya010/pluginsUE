#include "UI/ZonefallBootLogosWidget.h"

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
#include "Engine/Texture2D.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"

namespace
{
	FSlateFontInfo MakeBootFont(int32 Size, bool bMono = false)
	{
		FSlateFontInfo Font;
		Font.Size = FMath::Clamp(Size, 8, 120);
		Font.FontObject = LoadObject<UObject>(nullptr, TEXT("/Engine/EngineFonts/Roboto.Roboto"));
		Font.LetterSpacing = bMono ? 0 : 220;
		return Font;
	}
}

UZonefallBootLogosWidget::UZonefallBootLogosWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

TSharedRef<SWidget> UZonefallBootLogosWidget::RebuildWidget()
{
	if (!WidgetTree || !WidgetTree->RootWidget)
	{
		BuildLayout();
	}
	return Super::RebuildWidget();
}

void UZonefallBootLogosWidget::BuildLayout()
{
	if (!WidgetTree)
	{
		return;
	}

	WidgetTree->RootWidget = nullptr;

	// Solid black backdrop (cinematic).
	UBorder* Bg = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("BootBg"));
	Bg->SetBrush(FSlateRoundedBoxBrush(FLinearColor(0.0f, 0.0f, 0.0f, 1.0f), 0.0f));
	Bg->SetHorizontalAlignment(HAlign_Fill);
	Bg->SetVerticalAlignment(VAlign_Fill);
	WidgetTree->RootWidget = Bg;

	UOverlay* Root = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass(), TEXT("BootOverlay"));
	Bg->SetContent(Root);

	ContentBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("BootContent"));
	ContentBox->SetRenderOpacity(0.0f);
	if (UOverlaySlot* ContentSlot = Root->AddChildToOverlay(ContentBox))
	{
		ContentSlot->SetHorizontalAlignment(HAlign_Center);
		ContentSlot->SetVerticalAlignment(VAlign_Center);
	}

	// Optional image logo.
	LogoImageWidget = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("BootLogoImg"));
	LogoImageWidget->SetVisibility(ESlateVisibility::Collapsed);
	LogoImageWidget->SetDesiredSizeOverride(FVector2D(160.0f, 160.0f));
	if (UVerticalBoxSlot* LogoSlot = ContentBox->AddChildToVerticalBox(LogoImageWidget))
	{
		LogoSlot->SetHorizontalAlignment(HAlign_Center);
		LogoSlot->SetPadding(FMargin(0, 0, 0, 24));
	}

	// Self-drawn round logo badge (UE5 mark) used when no image is assigned.
	{
		UOverlay* BadgeOverlay = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass(), TEXT("BootBadgeOverlay"));

		// Outer ring.
		LogoBadgeRing = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("BootBadgeRing"));
		LogoBadgeRing->SetBrush(FSlateRoundedBoxBrush(FLinearColor(0.04f, 0.05f, 0.07f, 1.0f), 70.0f, FLinearColor(0.32f, 0.86f, 0.99f, 1.0f), 3.0f));
		if (UOverlaySlot* RingSlot = BadgeOverlay->AddChildToOverlay(LogoBadgeRing))
		{
			RingSlot->SetHorizontalAlignment(HAlign_Fill);
			RingSlot->SetVerticalAlignment(VAlign_Fill);
		}

		LogoBadgeText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("BootBadgeText"));
		LogoBadgeText->SetText(FText::FromString(TEXT("UE5")));
		LogoBadgeText->SetFont(MakeBootFont(46, /*bMono*/ true));
		LogoBadgeText->SetColorAndOpacity(FSlateColor(FLinearColor::White));
		LogoBadgeText->SetJustification(ETextJustify::Center);
		if (UOverlaySlot* TxtSlot = BadgeOverlay->AddChildToOverlay(LogoBadgeText))
		{
			TxtSlot->SetHorizontalAlignment(HAlign_Center);
			TxtSlot->SetVerticalAlignment(VAlign_Center);
		}

		LogoBadgeBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("BootBadgeBox"));
		LogoBadgeBox->SetWidthOverride(140.0f);
		LogoBadgeBox->SetHeightOverride(140.0f);
		LogoBadgeBox->AddChild(BadgeOverlay);
		LogoBadgeBox->SetVisibility(ESlateVisibility::Collapsed);
		if (UVerticalBoxSlot* BadgeSlot = ContentBox->AddChildToVerticalBox(LogoBadgeBox))
		{
			BadgeSlot->SetHorizontalAlignment(HAlign_Center);
			BadgeSlot->SetPadding(FMargin(0, 0, 0, 24));
		}
	}

	TitleText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("BootTitle"));
	TitleText->SetFont(MakeBootFont(40));
	TitleText->SetColorAndOpacity(FSlateColor(FLinearColor::White));
	TitleText->SetJustification(ETextJustify::Center);
	if (UVerticalBoxSlot* TitleSlot = ContentBox->AddChildToVerticalBox(TitleText))
	{
		TitleSlot->SetHorizontalAlignment(HAlign_Center);
	}

	// Thin accent divider.
	AccentLine = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("BootAccent"));
	AccentLine->SetBrush(FSlateRoundedBoxBrush(FLinearColor(0.32f, 0.86f, 0.99f, 1.0f), 1.0f));
	AccentLine->SetDesiredSizeOverride(FVector2D(120.0f, 2.0f));
	if (UVerticalBoxSlot* LineSlot = ContentBox->AddChildToVerticalBox(AccentLine))
	{
		LineSlot->SetHorizontalAlignment(HAlign_Center);
		LineSlot->SetPadding(FMargin(0, 16, 0, 16));
	}

	SubtitleText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("BootSubtitle"));
	SubtitleText->SetFont(MakeBootFont(16, /*bMono*/ true));
	SubtitleText->SetColorAndOpacity(FSlateColor(FLinearColor(0.72f, 0.78f, 0.85f, 1.0f)));
	SubtitleText->SetJustification(ETextJustify::Center);
	if (UVerticalBoxSlot* SubSlot = ContentBox->AddChildToVerticalBox(SubtitleText))
	{
		SubSlot->SetHorizontalAlignment(HAlign_Center);
	}
}

void UZonefallBootLogosWidget::BuildDefaultLogosIfNeeded()
{
	if (Logos.Num() > 0)
	{
		return;
	}

	// Card 1 — engine.
	{
		FZonefallBootLogo Engine;
		Engine.Title = NSLOCTEXT("ZonefallBoot", "Engine", "MADE WITH UNREAL ENGINE");
		Engine.Subtitle = FText::FromString(EngineVersionString);
		Engine.Accent = FLinearColor(0.10f, 0.55f, 0.95f, 1.0f); // UE blue
		Engine.HoldSeconds = 2.6f;
		Engine.bDrawBadge = true;
		Engine.BadgeText = TEXT("UE5");
		Logos.Add(Engine);
	}

	// Card 2 — studio.
	{
		FZonefallBootLogo Studio;
		Studio.Title = StudioName;
		Studio.Subtitle = StudioTagline;
		Studio.Accent = FLinearColor(0.95f, 0.82f, 0.35f, 1.0f);
		Studio.HoldSeconds = 2.4f;
		Logos.Add(Studio);
	}
}

void UZonefallBootLogosWidget::NativeConstruct()
{
	Super::NativeConstruct();
	SetIsFocusable(true);
	BuildDefaultLogosIfNeeded();
	ShowCard(0);
	SetKeyboardFocus();
}

void UZonefallBootLogosWidget::ShowCard(int32 Index)
{
	CurrentIndex = Index;
	Phase = 0;
	PhaseTime = 0.0f;

	if (!Logos.IsValidIndex(Index))
	{
		Finish();
		return;
	}

	const FZonefallBootLogo& Card = Logos[Index];

	if (TitleText) { TitleText->SetText(Card.Title); }
	if (SubtitleText) { SubtitleText->SetText(Card.Subtitle); }
	if (AccentLine) { AccentLine->SetBrush(FSlateRoundedBoxBrush(Card.Accent, 1.0f)); AccentLine->SetDesiredSizeOverride(FVector2D(120.0f, 2.0f)); }

	// Logo: prefer an assigned image; otherwise draw the stylised badge (UE5 mark).
	UTexture2D* Tex = Card.LogoImage.IsNull() ? nullptr : Card.LogoImage.LoadSynchronous();
	if (LogoImageWidget)
	{
		if (Tex)
		{
			LogoImageWidget->SetBrushFromTexture(Tex, false);
			LogoImageWidget->SetDesiredSizeOverride(FVector2D(160.0f, 160.0f));
			LogoImageWidget->SetVisibility(ESlateVisibility::HitTestInvisible);
		}
		else
		{
			LogoImageWidget->SetVisibility(ESlateVisibility::Collapsed);
		}
	}
	if (LogoBadgeBox && LogoBadgeRing && LogoBadgeText)
	{
		const bool bShowBadge = (Tex == nullptr) && Card.bDrawBadge;
		LogoBadgeBox->SetVisibility(bShowBadge ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
		if (bShowBadge)
		{
			LogoBadgeRing->SetBrush(FSlateRoundedBoxBrush(FLinearColor(0.03f, 0.05f, 0.08f, 1.0f), 70.0f, Card.Accent, 3.0f));
			LogoBadgeText->SetText(FText::FromString(Card.BadgeText.IsEmpty() ? TEXT("UE5") : Card.BadgeText));
		}
	}

	// Whoosh / sting when the card appears.
	if (!CardSound.IsNull())
	{
		if (USoundBase* Snd = CardSound.LoadSynchronous())
		{
			UGameplayStatics::PlaySound2D(this, Snd);
		}
	}

	if (ContentBox)
	{
		ContentBox->SetRenderOpacity(0.0f);
	}
}

void UZonefallBootLogosWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	if (bFinished || !ContentBox || !Logos.IsValidIndex(CurrentIndex))
	{
		return;
	}

	PhaseTime += InDeltaTime;
	const float Fade = FMath::Max(0.05f, FadeSeconds);

	switch (Phase)
	{
	case 0: // fade in
	{
		const float A = FMath::Clamp(PhaseTime / Fade, 0.0f, 1.0f);
		ContentBox->SetRenderOpacity(A);
		if (A >= 1.0f) { Phase = 1; PhaseTime = 0.0f; }
		break;
	}
	case 1: // hold
	{
		ContentBox->SetRenderOpacity(1.0f);
		if (PhaseTime >= Logos[CurrentIndex].HoldSeconds) { Phase = 2; PhaseTime = 0.0f; }
		break;
	}
	case 2: // fade out -> next card
	{
		const float A = 1.0f - FMath::Clamp(PhaseTime / Fade, 0.0f, 1.0f);
		ContentBox->SetRenderOpacity(A);
		if (A <= 0.0f)
		{
			if (Logos.IsValidIndex(CurrentIndex + 1))
			{
				ShowCard(CurrentIndex + 1);
			}
			else
			{
				Finish();
			}
		}
		break;
	}
	default:
		break;
	}
}

FReply UZonefallBootLogosWidget::NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
	Finish();
	return FReply::Handled();
}

FReply UZonefallBootLogosWidget::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	Finish();
	return FReply::Handled();
}

void UZonefallBootLogosWidget::Finish()
{
	if (bFinished)
	{
		return;
	}
	bFinished = true;

	if (!FinishSound.IsNull())
	{
		if (USoundBase* Snd = FinishSound.LoadSynchronous())
		{
			UGameplayStatics::PlaySound2D(this, Snd);
		}
	}

	OnFinished.Broadcast();
	RemoveFromParent();
}
