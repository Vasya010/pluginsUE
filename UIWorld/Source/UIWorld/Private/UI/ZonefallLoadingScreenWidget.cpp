#include "ZonefallLoadingScreenWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Brushes/SlateRoundedBoxBrush.h"
#include "Components/Border.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Engine/Texture2D.h"
#include "HAL/PlatformMemory.h"
#include "GenericPlatform/GenericPlatformMisc.h"
#include "UObject/UObjectGlobals.h"

UZonefallLoadingScreenWidget::UZonefallLoadingScreenWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
	, LoadingTitle(NSLOCTEXT("ZonefallUI", "LoadingTitleDefault", "LOADING LEVEL"))
	, LoadingSubtitle(NSLOCTEXT("ZonefallUI", "LoadingSubtitleDefault", "Preparing world and gameplay systems..."))
	, bAutoStartProgress(true)
	, bAutoRotateImages(true)
	, ImageRotateInterval(2.5f)
	, DefaultLoadingImage(nullptr)
	, ImageBorderName(TEXT("LoadingImageBorder"))
	, RootBorderName(TEXT("LoadingRoot"))
	, RootBoxName(TEXT("LoadingRootBox"))
	, TitleTextName(TEXT("LoadingTitleText"))
	, SubtitleTextName(TEXT("LoadingSubtitleText"))
	, ProgressBarName(TEXT("LoadingProgressBar"))
	, ProgressTextName(TEXT("LoadingProgressText"))
	, bAnimateLoadingText(true)
	, LoadingTextAnimInterval(0.35f)
	, bAnimateStatusText(true)
	, StatusTextAnimInterval(1.15f)
	, bAnimateSpinner(true)
	, bAnimateProgress(true)
	, ProgressDurationMinSeconds(1.8f)
	, ProgressDurationMaxSeconds(9.0f)
	, ProgressTickInterval(0.06f)
	, CurrentImageIndex(0)
	, LoadingTextDotCount(0)
	, StatusPhraseIndex(0)
	, SpinnerFrameIndex(0)
	, ProgressAlpha(0.0f)
	, ProgressDurationSeconds(3.0f)
	, BaseAnimatedTitleText(NSLOCTEXT("ZonefallUI", "LoadingAnimatedBase", "Loading"))
	, BaseAnimatedSubtitleText(NSLOCTEXT("ZonefallUI", "LoadingStatusBase", "Preparing world and gameplay systems"))
{
	DefaultLoadingImage = LoadObject<UTexture2D>(nullptr, TEXT("/Engine/EngineResources/DefaultTexture.DefaultTexture"));
	LoadingStatusPhrases = {
		NSLOCTEXT("ZonefallUI", "LoadingStatus1", "Initializing world"),
		NSLOCTEXT("ZonefallUI", "LoadingStatus2", "Streaming environment"),
		NSLOCTEXT("ZonefallUI", "LoadingStatus3", "Preparing gameplay systems"),
		NSLOCTEXT("ZonefallUI", "LoadingStatus4", "Finishing loading")
	};
}

void UZonefallLoadingScreenWidget::NativePreConstruct()
{
	Super::NativePreConstruct();
	ResolveNamedWidgets();
	BuildWidgetTree();
	ResolveNamedWidgets();
	UpdateTexts();
	UpdateImageVisual();
}

void UZonefallLoadingScreenWidget::NativeConstruct()
{
	Super::NativeConstruct();
	ResolveNamedWidgets();
	BuildWidgetTree();
	ResolveNamedWidgets();
	UpdateTexts();
	UpdateImageVisual();

	if (bAutoStartProgress)
	{
		StartLoading();
	}

	if (bAutoRotateImages)
	{
		StartImageRotation();
	}

	if (bAnimateLoadingText && TitleText)
	{
		BaseAnimatedTitleText = LoadingTitle.IsEmpty() ? NSLOCTEXT("ZonefallUI", "LoadingAnimatedBase", "Loading") : LoadingTitle;
		if (UWorld* World = GetWorld())
		{
			World->GetTimerManager().SetTimer(
				LoadingTextTimerHandle,
				this,
				&UZonefallLoadingScreenWidget::HandleLoadingTextTick,
				FMath::Max(0.1f, LoadingTextAnimInterval),
				true
			);
		}
	}
}

void UZonefallLoadingScreenWidget::NativeDestruct()
{
	StopImageRotation();
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(LoadingTextTimerHandle);
		World->GetTimerManager().ClearTimer(StatusTextTimerHandle);
		World->GetTimerManager().ClearTimer(ProgressTimerHandle);
	}
	Super::NativeDestruct();
}

void UZonefallLoadingScreenWidget::StartOnlineTravelLoading(const FText& Subtitle)
{
	ConfigureOnlineTravelLoading(EZonefallOnlineTravelPhase::Joining, Subtitle);
}

void UZonefallLoadingScreenWidget::ConfigureOnlineTravelLoading(EZonefallOnlineTravelPhase Phase, const FText& StatusHint)
{
	bOnlineTravelMode = true;
	OnlineTravelPhase = Phase;
	OnlineProgressCap = 92.0f;
	ProgressAlpha = 0.02f;

	switch (Phase)
	{
	case EZonefallOnlineTravelPhase::Hosting:
		LoadingTitle = NSLOCTEXT("ZonefallUI", "OnlineHostTitle", "CREATING SESSION");
		LoadingStatusPhrases = {
			NSLOCTEXT("ZonefallUI", "OnlineHostP1", "Registering your lobby"),
			NSLOCTEXT("ZonefallUI", "OnlineHostP2", "Opening world slots for players"),
			NSLOCTEXT("ZonefallUI", "OnlineHostP3", "Streaming the open world"),
			NSLOCTEXT("ZonefallUI", "OnlineHostP4", "Session is going live")
		};
		break;
	case EZonefallOnlineTravelPhase::Syncing:
		LoadingTitle = NSLOCTEXT("ZonefallUI", "OnlineSyncTitle", "ENTERING WORLD");
		LoadingStatusPhrases = {
			NSLOCTEXT("ZonefallUI", "OnlineSyncP1", "Syncing world state with host"),
			NSLOCTEXT("ZonefallUI", "OnlineSyncP2", "Spawning players and vehicles"),
			NSLOCTEXT("ZonefallUI", "OnlineSyncP3", "Finalizing your character")
		};
		break;
	default:
		LoadingTitle = NSLOCTEXT("ZonefallUI", "OnlineJoinTitle", "JOINING SESSION");
		LoadingStatusPhrases = {
			NSLOCTEXT("ZonefallUI", "OnlineJoinP1", "Finding the host"),
			NSLOCTEXT("ZonefallUI", "OnlineJoinP2", "Connecting to the session"),
			NSLOCTEXT("ZonefallUI", "OnlineJoinP3", "Loading the shared world"),
			NSLOCTEXT("ZonefallUI", "OnlineJoinP4", "Spawning into the match")
		};
		break;
	}

	LoadingSubtitle = StatusHint.IsEmpty()
		? LoadingStatusPhrases[0]
		: StatusHint;
	BaseAnimatedTitleText = LoadingTitle;
	BaseAnimatedSubtitleText = LoadingSubtitle;
	StatusPhraseIndex = 0;

	bAnimateProgress = true;
	bAnimateStatusText = true;
	bAnimateLoadingText = true;
	bAutoRotateImages = false;
	bAutoStartProgress = true;
	ProgressDurationMinSeconds = 8.0f;
	ProgressDurationMaxSeconds = 28.0f;
	ProgressDurationSeconds = 18.0f;
	StatusTextAnimInterval = 2.0f;
	LoadingTextAnimInterval = 0.4f;

	UpdateTexts();
	StartLoading();
}

void UZonefallLoadingScreenWidget::SetOnlineTravelStatus(const FText& Status)
{
	if (!Status.IsEmpty())
	{
		LoadingSubtitle = Status;
		BaseAnimatedSubtitleText = Status;
		if (SubtitleText)
		{
			SubtitleText->SetText(Status);
		}
	}
}

void UZonefallLoadingScreenWidget::StartLoading()
{
	if (bAnimateProgress)
	{
		ProgressAlpha = 0.0f;
		ProgressDurationSeconds = EstimateHardwareProgressDuration();
		if (ProgressBar) { ProgressBar->SetVisibility(ESlateVisibility::Collapsed); }
		if (ProgressText)
		{
			ProgressText->SetText(FText::FromString(TEXT("Loading 1%")));
		}

		if (UWorld* World = GetWorld())
		{
			World->GetTimerManager().SetTimer(
				ProgressTimerHandle,
				this,
				&UZonefallLoadingScreenWidget::HandleProgressTick,
				FMath::Max(0.02f, ProgressTickInterval),
				true
			);
		}
	}

	if (!bAnimateStatusText || !SubtitleText)
	{
		return;
	}

	BaseAnimatedSubtitleText = LoadingSubtitle.IsEmpty() ? NSLOCTEXT("ZonefallUI", "LoadingStatusBase", "Preparing world and gameplay systems") : LoadingSubtitle;
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(
			StatusTextTimerHandle,
			this,
			&UZonefallLoadingScreenWidget::HandleStatusTextTick,
			FMath::Max(0.2f, StatusTextAnimInterval),
			true
		);
	}
}

void UZonefallLoadingScreenWidget::CompleteLoading()
{
	bOnlineTravelMode = false;
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(LoadingTextTimerHandle);
		World->GetTimerManager().ClearTimer(StatusTextTimerHandle);
		World->GetTimerManager().ClearTimer(ProgressTimerHandle);
	}
	if (ProgressBar) { ProgressBar->SetVisibility(ESlateVisibility::Collapsed); }
	if (ProgressText)
	{
		ProgressText->SetText(FText::FromString(TEXT("Loading complete")));
	}
	StopImageRotation();
}

void UZonefallLoadingScreenWidget::EnterFinalizingPhase(float InPercent)
{
	const float ClampedPercent = FMath::Clamp(InPercent, 75.0f, 99.0f);
	ProgressAlpha = ClampedPercent / 100.0f;

	if (ProgressBar)
	{
		ProgressBar->SetVisibility(ESlateVisibility::Collapsed);
	}
	if (ProgressText)
	{
		ProgressText->SetText(FText::FromString(FString::Printf(TEXT("| Finalizing... %.0f%%"), ClampedPercent)));
	}
	if (SubtitleText)
	{
		SubtitleText->SetText(FText::FromString(TEXT("Finishing level streaming and preparing gameplay...")));
	}
}

void UZonefallLoadingScreenWidget::StartImageRotation()
{
	if (LoadingImages.Num() <= 1)
	{
		return;
	}

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(
			ImageRotateTimerHandle,
			this,
			&UZonefallLoadingScreenWidget::HandleImageRotateTick,
			FMath::Max(0.2f, ImageRotateInterval),
			true
		);
	}
}

void UZonefallLoadingScreenWidget::StopImageRotation()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(ImageRotateTimerHandle);
	}
}

void UZonefallLoadingScreenWidget::ResolveNamedWidgets()
{
	if (!WidgetTree)
	{
		return;
	}

	if (!ImageBorder && !ImageBorderName.IsNone())
	{
		ImageBorder = Cast<UBorder>(WidgetTree->FindWidget(ImageBorderName));
	}
	if (!ImageBorder)
	{
		WidgetTree->ForEachWidget([this](UWidget* Widget)
		{
			if (!ImageBorder)
			{
				ImageBorder = Cast<UBorder>(Widget);
			}
		});
	}

	if (!RootBorder && !RootBorderName.IsNone())
	{
		RootBorder = Cast<UBorder>(WidgetTree->FindWidget(RootBorderName));
	}
	if (!RootBorder)
	{
		RootBorder = Cast<UBorder>(WidgetTree->FindWidget(TEXT("LoadingRoot")));
	}

	if (!RootBox && !RootBoxName.IsNone())
	{
		RootBox = Cast<UVerticalBox>(WidgetTree->FindWidget(RootBoxName));
	}
	if (!RootBox)
	{
		RootBox = Cast<UVerticalBox>(WidgetTree->FindWidget(TEXT("LoadingRootBox")));
	}
	if (!RootBox)
	{
		WidgetTree->ForEachWidget([this](UWidget* Widget)
		{
			if (!RootBox)
			{
				RootBox = Cast<UVerticalBox>(Widget);
			}
		});
	}

	if (!TitleText && !TitleTextName.IsNone())
	{
		TitleText = Cast<UTextBlock>(WidgetTree->FindWidget(TitleTextName));
	}
	if (!TitleText)
	{
		TitleText = Cast<UTextBlock>(WidgetTree->FindWidget(TEXT("LoadingTitleText")));
	}
	if (!TitleText)
	{
		TitleText = Cast<UTextBlock>(WidgetTree->FindWidget(TEXT("TitleText")));
	}

	if (!SubtitleText && !SubtitleTextName.IsNone())
	{
		SubtitleText = Cast<UTextBlock>(WidgetTree->FindWidget(SubtitleTextName));
	}
	if (!SubtitleText)
	{
		SubtitleText = Cast<UTextBlock>(WidgetTree->FindWidget(TEXT("LoadingSubtitleText")));
	}
	if (!SubtitleText)
	{
		SubtitleText = Cast<UTextBlock>(WidgetTree->FindWidget(TEXT("SubtitleText")));
	}
	if (!TitleText || !SubtitleText)
	{
		WidgetTree->ForEachWidget([this](UWidget* Widget)
		{
			if (UTextBlock* FoundText = Cast<UTextBlock>(Widget))
			{
				if (!TitleText)
				{
					TitleText = FoundText;
				}
				else if (!SubtitleText && FoundText != TitleText)
				{
					SubtitleText = FoundText;
				}
			}
		});
	}

	if (!ProgressBar && !ProgressBarName.IsNone())
	{
		ProgressBar = Cast<UProgressBar>(WidgetTree->FindWidget(ProgressBarName));
	}
	if (!ProgressBar)
	{
		ProgressBar = Cast<UProgressBar>(WidgetTree->FindWidget(TEXT("LoadingProgressBar")));
	}
	if (ProgressBar)
	{
		ProgressBar->SetVisibility(ESlateVisibility::Collapsed);
	}

	if (!ProgressText && !ProgressTextName.IsNone())
	{
		ProgressText = Cast<UTextBlock>(WidgetTree->FindWidget(ProgressTextName));
	}
	if (!ProgressText)
	{
		ProgressText = Cast<UTextBlock>(WidgetTree->FindWidget(TEXT("LoadingProgressText")));
	}
}

void UZonefallLoadingScreenWidget::BuildWidgetTree()
{
	if (!WidgetTree)
	{
		return;
	}

	// If a designer-authored layout already exists, don't spawn duplicate widgets.
	if (WidgetTree->RootWidget && !RootBorder && !RootBox)
	{
		return;
	}

	if (!RootBorder)
	{
		RootBorder = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("LoadingRoot"));
		RootBorder->SetBrush(FSlateRoundedBoxBrush(FLinearColor(0.01f, 0.03f, 0.06f, 0.98f), 0.0f));
		RootBorder->SetPadding(FMargin(40.0f));
		if (!WidgetTree->RootWidget)
		{
			WidgetTree->RootWidget = RootBorder;
		}
	}

	if (!RootBox)
	{
		RootBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("LoadingRootBox"));
		RootBorder->SetContent(RootBox);
	}

	if (!TitleText)
	{
		TitleText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("LoadingTitleText"));
		if (UVerticalBoxSlot* TitleSlot = RootBox->AddChildToVerticalBox(TitleText))
		{
			TitleSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 12.0f));
		}
	}

	if (!SubtitleText)
	{
		SubtitleText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("LoadingSubtitleText"));
		if (UVerticalBoxSlot* SubtitleSlot = RootBox->AddChildToVerticalBox(SubtitleText))
		{
			SubtitleSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 20.0f));
		}
	}

	if (!ImageBorder)
	{
		ImageBorder = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("LoadingImageBorder"));
		ImageBorder->SetPadding(FMargin(0.0f));
		if (UVerticalBoxSlot* ImageSlot = RootBox->AddChildToVerticalBox(ImageBorder))
		{
			ImageSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 22.0f));
		}
	}

	if (!ProgressText)
	{
		ProgressText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("LoadingProgressText"));
		ProgressText->SetText(FText::FromString(TEXT("Loading 1%")));
		ProgressText->SetColorAndOpacity(FSlateColor(FLinearColor(0.68f, 0.82f, 0.94f, 1.0f)));
		if (UVerticalBoxSlot* ProgressTextSlot = RootBox->AddChildToVerticalBox(ProgressText))
		{
			ProgressTextSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 8.0f));
		}
	}

}

void UZonefallLoadingScreenWidget::UpdateTexts()
{
	if (TitleText)
	{
		if (LoadingTitle.IsEmpty())
		{
			TitleText->SetVisibility(ESlateVisibility::Collapsed);
		}
		else
		{
			TitleText->SetVisibility(ESlateVisibility::Visible);
			TitleText->SetText(LoadingTitle);
			TitleText->SetColorAndOpacity(FSlateColor(FLinearColor(0.92f, 0.97f, 1.0f, 1.0f)));
		}
	}

	if (SubtitleText)
	{
		if (LoadingSubtitle.IsEmpty())
		{
			SubtitleText->SetVisibility(ESlateVisibility::Collapsed);
		}
		else
		{
			SubtitleText->SetVisibility(ESlateVisibility::Visible);
			SubtitleText->SetText(LoadingSubtitle);
			SubtitleText->SetColorAndOpacity(FSlateColor(FLinearColor(0.72f, 0.84f, 0.95f, 1.0f)));
		}
	}
}

void UZonefallLoadingScreenWidget::UpdateImageVisual()
{
	if (!ImageBorder)
	{
		return;
	}

	const int32 ImageCount = LoadingImages.Num();
	if (ImageCount <= 0)
	{
		if (DefaultLoadingImage)
		{
			FSlateBrush DefaultBrush;
			DefaultBrush.SetResourceObject(DefaultLoadingImage);
			DefaultBrush.ImageSize = FVector2D(1024.0f, 360.0f);
			DefaultBrush.DrawAs = ESlateBrushDrawType::Image;
			ImageBorder->SetBrush(DefaultBrush);
		}
		else
		{
			ImageBorder->SetBrush(FSlateRoundedBoxBrush(FLinearColor(0.05f, 0.08f, 0.12f, 0.95f), 10.0f));
		}
		ImageBorder->SetVisibility(ESlateVisibility::Visible);
		return;
	}

	CurrentImageIndex = FMath::Clamp(CurrentImageIndex, 0, ImageCount - 1);
	UTexture2D* Texture = LoadingImages[CurrentImageIndex];
	if (!Texture)
	{
		return;
	}

	FSlateBrush ImageBrush;
	ImageBrush.SetResourceObject(Texture);
	ImageBrush.ImageSize = FVector2D(1024.0f, 360.0f);
	ImageBrush.DrawAs = ESlateBrushDrawType::Image;
	ImageBorder->SetBrush(ImageBrush);
	ImageBorder->SetVisibility(ESlateVisibility::Visible);
}

void UZonefallLoadingScreenWidget::HandleImageRotateTick()
{
	if (LoadingImages.Num() <= 1)
	{
		return;
	}

	CurrentImageIndex = (CurrentImageIndex + 1) % LoadingImages.Num();
	UpdateImageVisual();
}

void UZonefallLoadingScreenWidget::HandleLoadingTextTick()
{
	if (!TitleText || !bAnimateLoadingText)
	{
		return;
	}

	LoadingTextDotCount = (LoadingTextDotCount + 1) % 4;
	FString Dots;
	for (int32 i = 0; i < LoadingTextDotCount; ++i)
	{
		Dots += TEXT(".");
	}

	const TCHAR SpinnerChars[] = { TEXT('|'), TEXT('/'), TEXT('-'), TEXT('\\') };
	FString SpinnerPrefix;
	if (bAnimateSpinner)
	{
		SpinnerPrefix = FString::Printf(TEXT("%c "), SpinnerChars[SpinnerFrameIndex % 4]);
		SpinnerFrameIndex = (SpinnerFrameIndex + 1) % 4;
	}

	TitleText->SetText(FText::FromString(SpinnerPrefix + BaseAnimatedTitleText.ToString() + Dots));
}

void UZonefallLoadingScreenWidget::HandleStatusTextTick()
{
	if (!SubtitleText || !bAnimateStatusText)
	{
		return;
	}

	if (LoadingStatusPhrases.Num() > 0)
	{
		StatusPhraseIndex = (StatusPhraseIndex + 1) % LoadingStatusPhrases.Num();
		SubtitleText->SetText(LoadingStatusPhrases[StatusPhraseIndex]);
	}
	else
	{
		SubtitleText->SetText(BaseAnimatedSubtitleText);
	}
}

void UZonefallLoadingScreenWidget::HandleProgressTick()
{
	if (!bAnimateProgress || !ProgressText)
	{
		return;
	}

	const float StepScale = bOnlineTravelMode ? 0.45f : 1.0f;
	const float Step = FMath::Clamp(
		(FMath::Max(ProgressTickInterval, 0.02f) / FMath::Max(ProgressDurationSeconds, 0.2f)) * StepScale,
		0.001f,
		0.06f);
	const float MaxAlpha = bOnlineTravelMode ? (OnlineProgressCap / 100.0f) : 1.0f;
	ProgressAlpha = FMath::Clamp(ProgressAlpha + Step, 0.0f, MaxAlpha);

	// Ease curve: quick feedback at start, slower near completion to look natural.
	const float Smoothed = FMath::InterpEaseOut(0.0f, 1.0f, ProgressAlpha / FMath::Max(MaxAlpha, 0.01f), 2.2f);
	const float Capped = bOnlineTravelMode
		? FMath::Min(Smoothed * MaxAlpha, MaxAlpha)
		: FMath::Min(Smoothed, 0.985f);

	const int32 Percent = bOnlineTravelMode
		? FMath::Clamp(FMath::RoundToInt(Capped * 100.0f), 1, FMath::RoundToInt(OnlineProgressCap))
		: FMath::Clamp(FMath::RoundToInt(Capped * 100.0f), 1, 99);
	const TCHAR SpinnerChars[] = { TEXT('|'), TEXT('/'), TEXT('-'), TEXT('\\') };
	const TCHAR SpinnerChar = SpinnerChars[SpinnerFrameIndex % 4];
	SpinnerFrameIndex = (SpinnerFrameIndex + 1) % 4;
	const FString ProgressLabel = bOnlineTravelMode
		? (OnlineTravelPhase == EZonefallOnlineTravelPhase::Hosting
			? FString::Printf(TEXT("%c Hosting %d%%"), SpinnerChar, Percent)
			: FString::Printf(TEXT("%c Connecting %d%%"), SpinnerChar, Percent))
		: FString::Printf(TEXT("%c Loading %d%%"), SpinnerChar, Percent);
	ProgressText->SetText(FText::FromString(ProgressLabel));
}

float UZonefallLoadingScreenWidget::EstimateHardwareProgressDuration() const
{
	const int32 Cores = FPlatformMisc::NumberOfCoresIncludingHyperthreads();
	const uint64 TotalPhysicalBytes = FPlatformMemory::GetConstants().TotalPhysical;
	const float TotalRamGB = static_cast<float>(TotalPhysicalBytes) / (1024.0f * 1024.0f * 1024.0f);

	float CpuFactor = 0.35f;
	if (Cores <= 4)
	{
		CpuFactor = 1.0f;
	}
	else if (Cores <= 8)
	{
		CpuFactor = 0.72f;
	}
	else if (Cores <= 12)
	{
		CpuFactor = 0.52f;
	}

	float RamFactor = 0.35f;
	if (TotalRamGB < 8.0f)
	{
		RamFactor = 1.0f;
	}
	else if (TotalRamGB < 16.0f)
	{
		RamFactor = 0.72f;
	}
	else if (TotalRamGB < 24.0f)
	{
		RamFactor = 0.52f;
	}

	const float HardwareFactor = FMath::Max(CpuFactor, RamFactor);
	const float Target = ProgressDurationMinSeconds + (HardwareFactor * (ProgressDurationMaxSeconds - ProgressDurationMinSeconds));
	return FMath::Clamp(Target, ProgressDurationMinSeconds, ProgressDurationMaxSeconds);
}


