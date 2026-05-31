#include "UI/ZonefallTitleSplashWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Brushes/SlateRoundedBoxBrush.h"
#include "Components/Border.h"
#include "Components/Image.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"

namespace
{
	FSlateFontInfo MakeSplashFont(int32 Size)
	{
		FSlateFontInfo F;
		F.Size = FMath::Clamp(Size, 8, 130);
		F.FontObject = LoadObject<UObject>(nullptr, TEXT("/Engine/EngineFonts/Roboto.Roboto"));
		return F;
	}
}

UZonefallTitleSplashWidget::UZonefallTitleSplashWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

TSharedRef<SWidget> UZonefallTitleSplashWidget::RebuildWidget()
{
	if (!WidgetTree || !WidgetTree->RootWidget)
	{
		BuildLayout();
	}
	return Super::RebuildWidget();
}

void UZonefallTitleSplashWidget::NativeConstruct()
{
	Super::NativeConstruct();
	AnimTime = 0.0f;
}

void UZonefallTitleSplashWidget::BuildLayout()
{
	if (!WidgetTree)
	{
		return;
	}

	WidgetTree->RootWidget = nullptr;

	RootBorder = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("SplashRoot"));
	RootBorder->SetBrush(FSlateRoundedBoxBrush(BackgroundColor, 0.0f));
	WidgetTree->RootWidget = RootBorder;

	UOverlay* RootOverlay = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass(), TEXT("SplashOverlay"));
	RootBorder->SetContent(RootOverlay);

	// Centered title block.
	UVerticalBox* CenterBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("SplashCenter"));

	TitleText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("SplashTitle"));
	TitleText->SetText(GameTitle);
	TitleText->SetFont(MakeSplashFont(TitleFontSize));
	TitleText->SetJustification(ETextJustify::Center);
	TitleText->SetColorAndOpacity(FSlateColor(FLinearColor::White));
	TitleText->SetShadowOffset(FVector2D(2.0f, 2.0f));
	TitleText->SetShadowColorAndOpacity(FLinearColor(0.0f, 0.0f, 0.0f, 0.9f));
	TitleText->SetRenderOpacity(0.0f);
	if (UVerticalBoxSlot* TitleSlot = CenterBox->AddChildToVerticalBox(TitleText))
	{
		TitleSlot->SetHorizontalAlignment(HAlign_Center);
		TitleSlot->SetPadding(FMargin(0, 0, 0, 16));
	}

	AccentLine = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("SplashAccentLine"));
	{
		FSlateRoundedBoxBrush LineBrush(FLinearColor::White, 2.0f);
		LineBrush.ImageSize = FVector2D(340.0f, 3.0f);
		AccentLine->SetBrush(LineBrush);
		AccentLine->SetColorAndOpacity(AccentColor);
		AccentLine->SetRenderScale(FVector2D(0.0f, 1.0f));
	}
	if (UVerticalBoxSlot* LineSlot = CenterBox->AddChildToVerticalBox(AccentLine))
	{
		LineSlot->SetHorizontalAlignment(HAlign_Center);
		LineSlot->SetPadding(FMargin(0, 0, 0, 16));
	}

	SubtitleText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("SplashSubtitle"));
	SubtitleText->SetText(Subtitle);
	SubtitleText->SetFont(MakeSplashFont(FMath::Max(12, TitleFontSize / 3)));
	SubtitleText->SetJustification(ETextJustify::Center);
	SubtitleText->SetColorAndOpacity(FSlateColor(AccentColor * FLinearColor(1, 1, 1, 0.9f)));
	SubtitleText->SetRenderOpacity(0.0f);
	if (UVerticalBoxSlot* SubSlot = CenterBox->AddChildToVerticalBox(SubtitleText))
	{
		SubSlot->SetHorizontalAlignment(HAlign_Center);
	}

	if (UOverlaySlot* CenterSlot = RootOverlay->AddChildToOverlay(CenterBox))
	{
		CenterSlot->SetHorizontalAlignment(HAlign_Center);
		CenterSlot->SetVerticalAlignment(VAlign_Center);
	}

	// Scan sweep bar.
	ScanBar = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("SplashScanBar"));
	{
		FSlateRoundedBoxBrush Brush(FLinearColor::White, 0.0f);
		Brush.ImageSize = FVector2D(3.0f, 8.0f);
		ScanBar->SetBrush(Brush);
		ScanBar->SetColorAndOpacity(AccentColor * FLinearColor(1, 1, 1, 0.16f));
	}
	if (UOverlaySlot* ScanSlot = RootOverlay->AddChildToOverlay(ScanBar))
	{
		ScanSlot->SetHorizontalAlignment(HAlign_Left);
		ScanSlot->SetVerticalAlignment(VAlign_Fill);
	}
}

void UZonefallTitleSplashWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	AnimTime += InDeltaTime;

	// Title fades + settles in over ~1.0s.
	if (TitleText)
	{
		const float In = FMath::Clamp(AnimTime / 1.0f, 0.0f, 1.0f);
		const float Eased = 1.0f - FMath::Pow(1.0f - In, 3.0f); // ease-out cubic
		TitleText->SetRenderOpacity(Eased);
		const float Scale = FMath::Lerp(1.08f, 1.0f, Eased);
		TitleText->SetRenderScale(FVector2D(Scale, Scale));
	}

	// Accent line "draws" in horizontally over [0.4 .. 1.3]s.
	if (AccentLine)
	{
		const float LineT = FMath::Clamp((AnimTime - 0.4f) / 0.9f, 0.0f, 1.0f);
		const float LineEased = 1.0f - FMath::Pow(1.0f - LineT, 3.0f);
		AccentLine->SetRenderScale(FVector2D(LineEased, 1.0f));
		const float Glow = 0.6f + 0.4f * (0.5f + 0.5f * FMath::Sin(AnimTime * 3.0f));
		AccentLine->SetColorAndOpacity(AccentColor * FLinearColor(1, 1, 1, FMath::Min(LineEased, Glow)));
	}

	// Subtitle fades in after the title.
	if (SubtitleText)
	{
		SubtitleText->SetRenderOpacity(FMath::Clamp((AnimTime - 0.9f) / 0.8f, 0.0f, 1.0f));
	}

	// Scan sweep.
	if (ScanBar)
	{
		const float Width = MyGeometry.GetLocalSize().X;
		if (Width > 1.0f)
		{
			const float Period = 5.0f;
			const float T = FMath::Fmod(AnimTime, Period) / Period;
			ScanBar->SetRenderTranslation(FVector2D(T * Width, 0.0f));
		}
	}
}
