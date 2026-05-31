#include "ZonefallModernProgressBarWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Brushes/SlateRoundedBoxBrush.h"
#include "Components/Border.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"

UZonefallModernProgressBarWidget::UZonefallModernProgressBarWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
	, TitleText(NSLOCTEXT("ZonefallUI", "LoadingProgressTitle", "LOADING"))
	, bAutoProgress(true)
	, AutoProgressStep(0.02f)
	, AutoProgressInterval(0.03f)
	, ProgressBarName(TEXT("ProgressBar"))
	, RootBorderName(TEXT("ProgressRoot"))
	, RootBoxName(TEXT("ProgressRootBox"))
	, TitleLabelName(TEXT("ProgressTitle"))
	, PercentLabelName(TEXT("ProgressPercent"))
	, ProgressValue(0.0f)
{
}

void UZonefallModernProgressBarWidget::NativePreConstruct()
{
	Super::NativePreConstruct();
	ResolveNamedWidgets();
	BuildWidgetTree();
	UpdateVisuals();
}

void UZonefallModernProgressBarWidget::NativeConstruct()
{
	Super::NativeConstruct();
	ResolveNamedWidgets();
	BuildWidgetTree();
	UpdateVisuals();

	if (bAutoProgress)
	{
		StartAutoProgress();
	}
}

void UZonefallModernProgressBarWidget::ResolveNamedWidgets()
{
	if (!WidgetTree)
	{
		return;
	}

	if (!RootBorder && !RootBorderName.IsNone())
	{
		RootBorder = Cast<UBorder>(WidgetTree->FindWidget(RootBorderName));
	}

	if (!RootBox && !RootBoxName.IsNone())
	{
		RootBox = Cast<UVerticalBox>(WidgetTree->FindWidget(RootBoxName));
	}

	if (!ProgressBar && !ProgressBarName.IsNone())
	{
		ProgressBar = Cast<UProgressBar>(WidgetTree->FindWidget(ProgressBarName));
	}

	if (!TitleLabel && !TitleLabelName.IsNone())
	{
		TitleLabel = Cast<UTextBlock>(WidgetTree->FindWidget(TitleLabelName));
	}

	if (!PercentLabel && !PercentLabelName.IsNone())
	{
		PercentLabel = Cast<UTextBlock>(WidgetTree->FindWidget(PercentLabelName));
	}
}

void UZonefallModernProgressBarWidget::SetProgressNormalized(float InProgress)
{
	ProgressValue = FMath::Clamp(InProgress, 0.0f, 1.0f);
	UpdateVisuals();
	OnProgressChanged.Broadcast(ProgressValue);
}

void UZonefallModernProgressBarWidget::StartAutoProgress()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(
			AutoProgressTimerHandle,
			this,
			&UZonefallModernProgressBarWidget::HandleAutoProgressTick,
			AutoProgressInterval,
			true
		);
	}
}

void UZonefallModernProgressBarWidget::StopAutoProgress()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(AutoProgressTimerHandle);
	}
}

void UZonefallModernProgressBarWidget::BuildWidgetTree()
{
	if (!WidgetTree)
	{
		return;
	}

	if (!RootBorder)
	{
		RootBorder = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("ProgressRoot"));
		RootBorder->SetBrush(FSlateRoundedBoxBrush(FLinearColor(0.05f, 0.09f, 0.16f, 0.95f), 12.0f));
		RootBorder->SetPadding(FMargin(12.0f));
		if (!WidgetTree->RootWidget)
		{
			WidgetTree->RootWidget = RootBorder;
		}
	}

	if (!RootBox)
	{
		RootBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("ProgressRootBox"));
		RootBorder->SetContent(RootBox);
	}

	if (!TitleLabel)
	{
		TitleLabel = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("ProgressTitle"));
		if (UVerticalBoxSlot* TitleSlot = RootBox->AddChildToVerticalBox(TitleLabel))
		{
			TitleSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 6.0f));
		}
	}

	if (!ProgressBar)
	{
		ProgressBar = WidgetTree->ConstructWidget<UProgressBar>(UProgressBar::StaticClass(), TEXT("ProgressBar"));
		ProgressBar->SetFillColorAndOpacity(FLinearColor(0.35f, 0.76f, 1.0f, 1.0f));
		RootBox->AddChildToVerticalBox(ProgressBar);
	}

	if (!PercentLabel)
	{
		PercentLabel = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("ProgressPercent"));
		if (UVerticalBoxSlot* PercentSlot = RootBox->AddChildToVerticalBox(PercentLabel))
		{
			PercentSlot->SetPadding(FMargin(0.0f, 6.0f, 0.0f, 0.0f));
		}
	}
}

void UZonefallModernProgressBarWidget::UpdateVisuals()
{
	if (TitleLabel)
	{
		TitleLabel->SetText(TitleText);
		TitleLabel->SetColorAndOpacity(FSlateColor(FLinearColor(0.87f, 0.94f, 1.0f, 1.0f)));
	}

	if (ProgressBar)
	{
		ProgressBar->SetPercent(ProgressValue);
	}

	if (PercentLabel)
	{
		PercentLabel->SetText(FText::FromString(FString::Printf(TEXT("%d%%"), FMath::RoundToInt(ProgressValue * 100.0f))));
		PercentLabel->SetColorAndOpacity(FSlateColor(FLinearColor(0.78f, 0.88f, 1.0f, 1.0f)));
	}
}

void UZonefallModernProgressBarWidget::HandleAutoProgressTick()
{
	const float NextValue = FMath::Min(ProgressValue + AutoProgressStep, 0.96f);
	SetProgressNormalized(NextValue);
}


