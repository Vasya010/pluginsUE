#include "ZonefallStartupIntroWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Brushes/SlateRoundedBoxBrush.h"
#include "Components/Border.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "MediaPlayer.h"
#include "MediaSource.h"
#include "MediaTexture.h"
#include "TimerManager.h"

UZonefallStartupIntroWidget::UZonefallStartupIntroWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
	, IntroMediaPlayer(nullptr)
	, IntroMediaSource(nullptr)
	, IntroMediaTexture(nullptr)
	, IntroVideoUIMaterial(nullptr)
	, IntroVideoTextureParameterName(TEXT("VideoTexture"))
	, FallbackIntroDuration(4.0f)
	, bAutoPlayOnConstruct(true)
	, IntroFallbackText(NSLOCTEXT("ZonefallUI", "StartupIntroFallback", "PRESENTED BY ZONEFALL STUDIO"))
	, IntroVideoMaterialMID(nullptr)
	, PlaybackRetryCount(0)
	, bFinished(false)
{
}

void UZonefallStartupIntroWidget::NativeConstruct()
{
	Super::NativeConstruct();
	UE_LOG(
		LogTemp,
		Log,
		TEXT("[StartupIntro] NativeConstruct: Widget=%s Player=%s Source=%s Texture=%s AutoPlay=%d Fallback=%.2f"),
		*GetNameSafe(this),
		*GetNameSafe(IntroMediaPlayer),
		*GetNameSafe(IntroMediaSource),
		*GetNameSafe(IntroMediaTexture),
		bAutoPlayOnConstruct ? 1 : 0,
		FallbackIntroDuration
	);

	// Force a deterministic fullscreen root so intro rendering does not depend on BP layout.
	RootBorder = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("StartupIntroRoot"));
	RootBorder->SetBrush(FSlateRoundedBoxBrush(FLinearColor::Black, 0.0f));
	WidgetTree->RootWidget = RootBorder;

	if (!VideoImage)
	{
		VideoImage = Cast<UImage>(WidgetTree->FindWidget(TEXT("StartupIntroVideoImage")));
	}
	if (!VideoImage)
	{
		VideoImage = Cast<UImage>(WidgetTree->FindWidget(TEXT("VideoImage")));
	}
	if (!VideoImage)
	{
		VideoImage = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("StartupIntroVideoImage"));
	}
	if (VideoImage)
	{
		VideoImage->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
		VideoImage->SetRenderOpacity(1.0f);
	}

	if (RootBorder && VideoImage)
	{
		FSlateBrush VideoBrush;
		bool bHasVideoTexture = false;
		if (IntroMediaTexture)
		{
			IntroMediaTexture->SetMediaPlayer(IntroMediaPlayer);
			IntroMediaTexture->UpdateResource();

			// Prefer UI material path for more reliable media rendering in UMG.
			if (IntroVideoUIMaterial)
			{
				IntroVideoMaterialMID = UMaterialInstanceDynamic::Create(IntroVideoUIMaterial, this);
				if (IntroVideoMaterialMID)
				{
					IntroVideoMaterialMID->SetTextureParameterValue(IntroVideoTextureParameterName, IntroMediaTexture);
					VideoImage->SetBrushFromMaterial(IntroVideoMaterialMID);
				}
			}

			if (!IntroVideoMaterialMID)
			{
				VideoBrush.SetResourceObject(IntroMediaTexture);
				VideoBrush.ImageSize = FVector2D(1920.0f, 1080.0f);
				VideoBrush.DrawAs = ESlateBrushDrawType::Image;
				VideoImage->SetBrush(VideoBrush);
			}
			bHasVideoTexture = true;
		}

		if (bHasVideoTexture)
		{
			RootBorder->SetContent(VideoImage);
			UE_LOG(LogTemp, Log, TEXT("[StartupIntro] Video image assigned to RootBorder."));
		}
		else
		{
			ShowFallbackIntroText();
			UE_LOG(LogTemp, Warning, TEXT("[StartupIntro] IntroMediaTexture is null. Showing fallback text."));
		}
	}

	if (bAutoPlayOnConstruct)
	{
		StartIntro();
	}
}

void UZonefallStartupIntroWidget::NativeDestruct()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(FallbackFinishTimerHandle);
		World->GetTimerManager().ClearTimer(PlaybackRetryTimerHandle);
	}

	if (IntroMediaPlayer)
	{
		IntroMediaPlayer->OnMediaOpened.RemoveDynamic(this, &UZonefallStartupIntroWidget::HandleMediaOpened);
		IntroMediaPlayer->OnMediaOpenFailed.RemoveDynamic(this, &UZonefallStartupIntroWidget::HandleMediaOpenFailed);
		IntroMediaPlayer->OnMediaOpened.RemoveDynamic(this, &UZonefallStartupIntroWidget::HandleMediaOpened);
		IntroMediaPlayer->OnEndReached.RemoveDynamic(this, &UZonefallStartupIntroWidget::HandleMediaEndReached);
		IntroMediaPlayer->Close();
	}

	Super::NativeDestruct();
}

void UZonefallStartupIntroWidget::StartIntro()
{
	bFinished = false;
	PlaybackRetryCount = 0;
	UE_LOG(
		LogTemp,
		Log,
		TEXT("[StartupIntro] StartIntro: Player=%s Source=%s Texture=%s"),
		*GetNameSafe(IntroMediaPlayer),
		*GetNameSafe(IntroMediaSource),
		*GetNameSafe(IntroMediaTexture)
	);

	// Hard safety: never let startup intro hang forever.
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(FallbackFinishTimerHandle);
		World->GetTimerManager().SetTimer(
			FallbackFinishTimerHandle,
			FTimerDelegate::CreateUObject(this, &UZonefallStartupIntroWidget::FinishIntro),
			FMath::Max(0.5f, FallbackIntroDuration),
			false
		);
	}

	if (IntroMediaPlayer)
	{
		IntroMediaPlayer->OnMediaOpened.RemoveDynamic(this, &UZonefallStartupIntroWidget::HandleMediaOpened);
		IntroMediaPlayer->OnMediaOpened.AddDynamic(this, &UZonefallStartupIntroWidget::HandleMediaOpened);
		IntroMediaPlayer->OnMediaOpenFailed.RemoveDynamic(this, &UZonefallStartupIntroWidget::HandleMediaOpenFailed);
		IntroMediaPlayer->OnMediaOpenFailed.AddDynamic(this, &UZonefallStartupIntroWidget::HandleMediaOpenFailed);
		IntroMediaPlayer->OnEndReached.RemoveDynamic(this, &UZonefallStartupIntroWidget::HandleMediaEndReached);
		IntroMediaPlayer->OnEndReached.AddDynamic(this, &UZonefallStartupIntroWidget::HandleMediaEndReached);

		const bool bOpened = IntroMediaSource ? IntroMediaPlayer->OpenSource(IntroMediaSource) : false;
		UE_LOG(
			LogTemp,
			Log,
			TEXT("[StartupIntro] OpenSource result=%d HasSource=%d"),
			bOpened ? 1 : 0,
			IntroMediaSource ? 1 : 0
		);
		if (bOpened)
		{
			UE_LOG(LogTemp, Log, TEXT("[StartupIntro] OpenSource accepted. Waiting OnMediaOpened to start playback."));
			if (!IntroMediaTexture)
			{
				ShowFallbackIntroText();
				UE_LOG(LogTemp, Warning, TEXT("[StartupIntro] Opened media but IntroMediaTexture is null. Showing fallback text."));
			}
			return;
		}
	}

	// Fallback if media failed to open.
	UE_LOG(LogTemp, Warning, TEXT("[StartupIntro] Failed to open media source. Showing fallback text."));
	ShowFallbackIntroText();
}

void UZonefallStartupIntroWidget::FinishIntro()
{
	if (bFinished)
	{
		UE_LOG(LogTemp, Verbose, TEXT("[StartupIntro] FinishIntro ignored (already finished)."));
		return;
	}
	bFinished = true;
	UE_LOG(LogTemp, Log, TEXT("[StartupIntro] FinishIntro fired. Broadcasting OnIntroFinished."));

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(FallbackFinishTimerHandle);
		World->GetTimerManager().ClearTimer(PlaybackRetryTimerHandle);
	}

	if (IntroMediaPlayer)
	{
		IntroMediaPlayer->OnMediaOpened.RemoveDynamic(this, &UZonefallStartupIntroWidget::HandleMediaOpened);
		IntroMediaPlayer->OnMediaOpenFailed.RemoveDynamic(this, &UZonefallStartupIntroWidget::HandleMediaOpenFailed);
		IntroMediaPlayer->OnEndReached.RemoveDynamic(this, &UZonefallStartupIntroWidget::HandleMediaEndReached);
		IntroMediaPlayer->Close();
	}

	OnIntroFinished.Broadcast();
}

void UZonefallStartupIntroWidget::HandleMediaEndReached()
{
	UE_LOG(LogTemp, Log, TEXT("[StartupIntro] Media end reached event."));
	FinishIntro();
}

void UZonefallStartupIntroWidget::HandleMediaOpened(FString OpenedUrl)
{
	UE_LOG(LogTemp, Log, TEXT("[StartupIntro] OnMediaOpened: Url=%s"), *OpenedUrl);
	TryStartPlayback(TEXT("OnMediaOpened"));

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(PlaybackRetryTimerHandle);
		World->GetTimerManager().SetTimer(
			PlaybackRetryTimerHandle,
			FTimerDelegate::CreateUObject(this, &UZonefallStartupIntroWidget::HandlePlaybackRetryTick),
			0.15f,
			true
		);
	}
}

void UZonefallStartupIntroWidget::HandleMediaOpenFailed(FString FailedUrl)
{
	UE_LOG(LogTemp, Error, TEXT("[StartupIntro] OnMediaOpenFailed: Url=%s"), *FailedUrl);
	ShowFallbackIntroText();
}

void UZonefallStartupIntroWidget::ShowFallbackIntroText()
{
	if (!WidgetTree)
	{
		UE_LOG(LogTemp, Warning, TEXT("[StartupIntro] WidgetTree is null, cannot show fallback text."));
		return;
	}

	if (!FallbackText)
	{
		FallbackText = Cast<UTextBlock>(WidgetTree->FindWidget(TEXT("StartupIntroFallbackText")));
	}
	if (!FallbackText)
	{
		FallbackText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("StartupIntroFallbackText"));
	}
	if (!FallbackText)
	{
		UE_LOG(LogTemp, Warning, TEXT("[StartupIntro] FallbackText is null, cannot show fallback text."));
		return;
	}

	FallbackText->SetText(IntroFallbackText);
	FallbackText->SetColorAndOpacity(FSlateColor(FLinearColor(0.90f, 0.95f, 1.0f, 1.0f)));
	FSlateFontInfo FontInfo;
	FontInfo.Size = 34;
	FallbackText->SetFont(FontInfo);
	FallbackText->SetJustification(ETextJustify::Center);

	if (!RootBorder)
	{
		RootBorder = Cast<UBorder>(WidgetTree->FindWidget(TEXT("RootBorder")));
	}
	if (!RootBorder && WidgetTree->RootWidget)
	{
		RootBorder = Cast<UBorder>(WidgetTree->RootWidget);
	}
	if (RootBorder)
	{
		RootBorder->SetContent(FallbackText);
		UE_LOG(LogTemp, Log, TEXT("[StartupIntro] Fallback text attached to RootBorder."));
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("[StartupIntro] RootBorder is null, fallback text could not be attached."));
	}
}

void UZonefallStartupIntroWidget::TryStartPlayback(const TCHAR* ReasonTag)
{
	if (!IntroMediaPlayer)
	{
		UE_LOG(LogTemp, Warning, TEXT("[StartupIntro] TryStartPlayback(%s): player is null."), ReasonTag);
		return;
	}

	const bool bPlayStarted = IntroMediaPlayer->Play();
	UE_LOG(
		LogTemp,
		Log,
		TEXT("[StartupIntro] TryStartPlayback(%s): PlayResult=%d IsPlaying=%d"),
		ReasonTag,
		bPlayStarted ? 1 : 0,
		IntroMediaPlayer->IsPlaying() ? 1 : 0
	);
}

void UZonefallStartupIntroWidget::HandlePlaybackRetryTick()
{
	if (!IntroMediaPlayer)
	{
		return;
	}

	if (IntroMediaPlayer->IsPlaying())
	{
		if (UWorld* World = GetWorld())
		{
			World->GetTimerManager().ClearTimer(PlaybackRetryTimerHandle);
		}
		UE_LOG(LogTemp, Log, TEXT("[StartupIntro] Playback confirmed as active."));
		return;
	}

	++PlaybackRetryCount;
	TryStartPlayback(TEXT("Retry"));
	if (PlaybackRetryCount >= 10)
	{
		if (UWorld* World = GetWorld())
		{
			World->GetTimerManager().ClearTimer(PlaybackRetryTimerHandle);
		}
		UE_LOG(LogTemp, Warning, TEXT("[StartupIntro] Playback did not start after retries, keeping fallback timer."));
	}
}

