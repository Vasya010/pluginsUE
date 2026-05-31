#include "ZonefallShaderLoadingWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Brushes/SlateRoundedBoxBrush.h"
#include "Components/Border.h"
#include "Components/CircularThrobber.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Image.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Engine/Texture2D.h"
#include "Engine/Font.h"
#include "ShaderCompiler.h"
#include "TimerManager.h"

DEFINE_LOG_CATEGORY_STATIC(LogZonefallShaderLoadingWidget, Log, All);

UZonefallShaderLoadingWidget::UZonefallShaderLoadingWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
	, StartupTitleText(NSLOCTEXT("ZonefallUI", "StartupTitle", "ZONEFALL PROTOCOL"))
	, StartupSubtitleText(NSLOCTEXT("ZonefallUI", "StartupSubtitle", "PREPARING THE ZONE"))
	, ShaderLoadingBaseText(NSLOCTEXT("ZonefallUI", "ShaderLoadingBase", "Compiling shaders"))
	, ShaderCheckingCacheText(NSLOCTEXT("ZonefallUI", "ShaderCheckingCache", "Checking shader cache"))
	, ShaderFinalizingText(NSLOCTEXT("ZonefallUI", "ShaderFinalizing", "Finalizing shader setup"))
	, PleaseWaitText(NSLOCTEXT("ZonefallUI", "ShaderPleaseWait", "Please wait..."))
	, ShaderCacheHintText(NSLOCTEXT("ZonefallUI", "ShaderCacheHint", "Preparing and caching shaders for faster future loading. After GPU driver updates, shaders may need to be recompiled."))
	, TipRotateSeconds(5.0f)
	, ShaderCompileProgressPercent(0.0f)
	, TextAnimInterval(0.12f)
	, bUseRealShaderCompilerProgress(true)
	, BackgroundImageTint(FLinearColor(1.0f, 1.0f, 1.0f, 0.22f))
	, BottomPanelTint(FLinearColor(0.03f, 0.07f, 0.10f, 0.88f))
	, AccentColor(FLinearColor(0.24f, 0.72f, 0.98f, 1.0f))
	, TitleFontSize(54)
	, BodyFontSize(18)
	, SmoothProgressSpeed(6.0f)
	, ProgressUnitsPerSecond(34.0f)
	, bEnableUiAnimations(true)
	, PanelPulseSpeed(1.2f)
	, BarGlowSpeed(2.6f)
	, DotCount(0)
	, InitialShaderJobCount(0)
	, LastKnownRemainingJobs(0)
	, ConsecutiveZeroJobTicks(0)
	, bShaderProgressInitialized(false)
	, bEnteredFinalizingState(false)
	, SmoothedProgressPercent(0.0f)
	, AnimationTimeSeconds(0.0f)
	, CurrentTipIndex(0)
	, TipElapsedSeconds(0.0f)
{
	RotatingTips =
	{
		NSLOCTEXT("ZonefallUI", "Tip0", "TIP: Shaders are cached on disk — the next launch will be much faster."),
		NSLOCTEXT("ZonefallUI", "Tip1", "TIP: Updating your GPU driver can trigger a one-time shader recompile."),
		NSLOCTEXT("ZonefallUI", "Tip2", "TIP: Precompiling shaders now prevents hitches and stutter in-game."),
		NSLOCTEXT("ZonefallUI", "Tip3", "TIP: Lower the graphics preset in Settings for higher, steadier frame rates."),
		NSLOCTEXT("ZonefallUI", "Tip4", "TIP: Enabling DLSS / FSR can boost performance with minimal visual cost.")
	};
}

TSharedRef<SWidget> UZonefallShaderLoadingWidget::RebuildWidget()
{
	BindExistingWidgetsFromTree();
	const bool bNeedsRuntimeFallback =
		!TitleText || !ShaderText || !ProgressText || !WaitText || !CacheHintText;
	UE_LOG(
		LogZonefallShaderLoadingWidget,
		Log,
		TEXT("RebuildWidget: Title=%d Shader=%d Progress=%d Wait=%d Hint=%d ProgressBar=%d NeedsFallback=%d"),
		TitleText != nullptr,
		ShaderText != nullptr,
		ProgressText != nullptr,
		WaitText != nullptr,
		CacheHintText != nullptr,
		CompileProgressBar != nullptr,
		bNeedsRuntimeFallback
	);
	BuildLayoutIfNeeded(bNeedsRuntimeFallback);
	return Super::RebuildWidget();
}

void UZonefallShaderLoadingWidget::NativeConstruct()
{
	Super::NativeConstruct();

	UE_LOG(LogZonefallShaderLoadingWidget, Log, TEXT("NativeConstruct: widget=%s"), *GetNameSafe(this));

	BindExistingWidgetsFromTree();
	UE_LOG(
		LogZonefallShaderLoadingWidget,
		Log,
		TEXT("NativeConstruct bindings: Title=%d Shader=%d Progress=%d Wait=%d Hint=%d ProgressBar=%d"),
		TitleText != nullptr,
		ShaderText != nullptr,
		ProgressText != nullptr,
		WaitText != nullptr,
		CacheHintText != nullptr,
		CompileProgressBar != nullptr
	);
	ApplyRuntimeFonts();
	UpdateTexts();
	SetVisibility(ESlateVisibility::Visible);
	SetRenderOpacity(1.0f);

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(
			TextAnimTimerHandle,
			this,
			&UZonefallShaderLoadingWidget::HandleTextAnimTick,
			FMath::Max(0.1f, TextAnimInterval),
			true
		);
		UE_LOG(LogZonefallShaderLoadingWidget, Log, TEXT("Text animation timer started: interval=%.2f"), FMath::Max(0.1f, TextAnimInterval));
	}
}

void UZonefallShaderLoadingWidget::NativeDestruct()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(TextAnimTimerHandle);
	}
	Super::NativeDestruct();
}

void UZonefallShaderLoadingWidget::BindExistingWidgetsFromTree()
{
	if (!WidgetTree)
	{
		return;
	}

	if (!RootBorder)
	{
		RootBorder = Cast<UBorder>(WidgetTree->FindWidget(TEXT("ShaderLoadingRoot")));
	}
	if (!BackgroundImageWidget)
	{
		BackgroundImageWidget = Cast<UImage>(WidgetTree->FindWidget(TEXT("ShaderBackgroundImage")));
	}
	if (!RootBox)
	{
		RootBox = Cast<UVerticalBox>(WidgetTree->FindWidget(TEXT("ShaderLoadingRootBox")));
	}
	if (!BottomPanel)
	{
		BottomPanel = Cast<UBorder>(WidgetTree->FindWidget(TEXT("ShaderLoadingBottomPanel")));
	}
	if (!AccentDivider)
	{
		AccentDivider = Cast<UImage>(WidgetTree->FindWidget(TEXT("ShaderLoadingAccentDivider")));
	}
	if (!TipText)
	{
		TipText = Cast<UTextBlock>(WidgetTree->FindWidget(TEXT("ShaderLoadingTipText")));
	}
	if (!TitleText)
	{
		TitleText = Cast<UTextBlock>(WidgetTree->FindWidget(TEXT("StartupTitleText")));
	}
	if (!SubtitleText)
	{
		SubtitleText = Cast<UTextBlock>(WidgetTree->FindWidget(TEXT("ShaderSubtitleText")));
	}
	if (!PercentText)
	{
		PercentText = Cast<UTextBlock>(WidgetTree->FindWidget(TEXT("ShaderPercentText")));
	}
	if (!Spinner)
	{
		Spinner = Cast<UCircularThrobber>(WidgetTree->FindWidget(TEXT("ShaderSpinner")));
	}
	if (!TopAccentLine)
	{
		TopAccentLine = Cast<UImage>(WidgetTree->FindWidget(TEXT("ShaderTopAccentLine")));
	}
	if (!ShaderText)
	{
		ShaderText = Cast<UTextBlock>(WidgetTree->FindWidget(TEXT("ShaderLoadingText")));
	}
	if (!ProgressText)
	{
		ProgressText = Cast<UTextBlock>(WidgetTree->FindWidget(TEXT("ShaderProgressText")));
	}
	if (!WaitText)
	{
		WaitText = Cast<UTextBlock>(WidgetTree->FindWidget(TEXT("ShaderWaitText")));
	}
	if (!CacheHintText)
	{
		CacheHintText = Cast<UTextBlock>(WidgetTree->FindWidget(TEXT("ShaderCacheHintText")));
	}
	if (!CompileProgressBar)
	{
		CompileProgressBar = Cast<UProgressBar>(WidgetTree->FindWidget(TEXT("ShaderCompileProgressBar")));
	}
}

void UZonefallShaderLoadingWidget::BuildLayoutIfNeeded(bool bForceRebuild)
{
	if (!WidgetTree)
	{
		UE_LOG(LogZonefallShaderLoadingWidget, Warning, TEXT("BuildLayoutIfNeeded: WidgetTree is null"));
		return;
	}

	if (!bForceRebuild && WidgetTree->RootWidget)
	{
		UE_LOG(LogZonefallShaderLoadingWidget, Log, TEXT("BuildLayoutIfNeeded: using existing root widget"));
		return;
	}

	if (bForceRebuild)
	{
		UE_LOG(LogZonefallShaderLoadingWidget, Warning, TEXT("BuildLayoutIfNeeded: forcing runtime fallback layout rebuild"));
		RootBorder = nullptr;
		BackgroundImageWidget = nullptr;
		RootBox = nullptr;
		BottomPanel = nullptr;
		AccentDivider = nullptr;
		TipText = nullptr;
		TitleText = nullptr;
		SubtitleText = nullptr;
		PercentText = nullptr;
		Spinner = nullptr;
		TopAccentLine = nullptr;
		ShaderText = nullptr;
		ProgressText = nullptr;
		WaitText = nullptr;
		CacheHintText = nullptr;
		CompileProgressBar = nullptr;
		WidgetTree->RootWidget = nullptr;
	}

	UObject* FontObj = LoadObject<UObject>(nullptr, TEXT("/Engine/EngineFonts/Roboto.Roboto"));
	auto MakeFont = [FontObj](int32 Size) -> FSlateFontInfo
	{
		FSlateFontInfo F;
		F.Size = FMath::Clamp(Size, 8, 120);
		F.FontObject = FontObj;
		return F;
	};

	RootBorder = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("ShaderLoadingRoot"));
	RootBorder->SetBrush(FSlateRoundedBoxBrush(FLinearColor(0.01f, 0.02f, 0.04f, 1.0f), 0.0f));
	RootBorder->SetPadding(FMargin(0.0f));
	WidgetTree->RootWidget = RootBorder;

	UOverlay* RootOverlay = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass(), TEXT("ShaderRootOverlay"));
	RootBorder->SetContent(RootOverlay);

	// --- Background image (fill) ---
	BackgroundImageWidget = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("ShaderBackgroundImage"));
	BackgroundImageWidget->SetColorAndOpacity(BackgroundImageTint);
	if (BackgroundImageTexture.IsValid() || !BackgroundImageTexture.ToSoftObjectPath().IsNull())
	{
		if (UTexture2D* LoadedTexture = BackgroundImageTexture.LoadSynchronous())
		{
			BackgroundImageWidget->SetBrushFromTexture(LoadedTexture, true);
		}
	}
	if (UOverlaySlot* BgSlot = RootOverlay->AddChildToOverlay(BackgroundImageWidget))
	{
		BgSlot->SetHorizontalAlignment(HAlign_Fill);
		BgSlot->SetVerticalAlignment(VAlign_Fill);
	}

	// --- Dark vignette for legibility ---
	UBorder* Vignette = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("ShaderVignette"));
	Vignette->SetBrush(FSlateRoundedBoxBrush(FLinearColor(0.0f, 0.0f, 0.0f, 0.55f), 0.0f));
	if (UOverlaySlot* VSlot = RootOverlay->AddChildToOverlay(Vignette))
	{
		VSlot->SetHorizontalAlignment(HAlign_Fill);
		VSlot->SetVerticalAlignment(VAlign_Fill);
	}

	// --- Top accent line ---
	TopAccentLine = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("ShaderTopAccentLine"));
	{
		FSlateRoundedBoxBrush LineBrush(FLinearColor::White, 0.0f);
		LineBrush.ImageSize = FVector2D(512.0f, 3.0f);
		TopAccentLine->SetBrush(LineBrush);
		TopAccentLine->SetColorAndOpacity(AccentColor);
	}
	if (UOverlaySlot* TLSlot = RootOverlay->AddChildToOverlay(TopAccentLine))
	{
		TLSlot->SetHorizontalAlignment(HAlign_Fill);
		TLSlot->SetVerticalAlignment(VAlign_Top);
		TLSlot->SetPadding(FMargin(0.0f, 64.0f, 0.0f, 0.0f));
	}

	// --- Centered game title block ---
	UVerticalBox* CenterBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("ShaderCenterBox"));

	TitleText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("StartupTitleText"));
	TitleText->SetFont(MakeFont(TitleFontSize));
	TitleText->SetJustification(ETextJustify::Center);
	TitleText->SetColorAndOpacity(FSlateColor(FLinearColor::White));
	TitleText->SetShadowOffset(FVector2D(2.0f, 2.0f));
	TitleText->SetShadowColorAndOpacity(FLinearColor(0.0f, 0.0f, 0.0f, 0.9f));
	if (UVerticalBoxSlot* TitleSlot = CenterBox->AddChildToVerticalBox(TitleText))
	{
		TitleSlot->SetHorizontalAlignment(HAlign_Center);
		TitleSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 12.0f));
	}

	AccentDivider = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("ShaderLoadingAccentDivider"));
	{
		FSlateRoundedBoxBrush DividerBrush(FLinearColor::White, 2.0f);
		DividerBrush.ImageSize = FVector2D(220.0f, 3.0f);
		AccentDivider->SetBrush(DividerBrush);
		AccentDivider->SetColorAndOpacity(AccentColor);
	}
	if (UVerticalBoxSlot* DividerSlot = CenterBox->AddChildToVerticalBox(AccentDivider))
	{
		DividerSlot->SetHorizontalAlignment(HAlign_Center);
		DividerSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 12.0f));
	}

	SubtitleText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("ShaderSubtitleText"));
	SubtitleText->SetFont(MakeFont(BodyFontSize + 2));
	SubtitleText->SetJustification(ETextJustify::Center);
	SubtitleText->SetColorAndOpacity(FSlateColor(AccentColor * FLinearColor(1, 1, 1, 0.9f)));
	if (UVerticalBoxSlot* SubSlot = CenterBox->AddChildToVerticalBox(SubtitleText))
	{
		SubSlot->SetHorizontalAlignment(HAlign_Center);
	}

	if (UOverlaySlot* CenterSlot = RootOverlay->AddChildToOverlay(CenterBox))
	{
		CenterSlot->SetHorizontalAlignment(HAlign_Center);
		CenterSlot->SetVerticalAlignment(VAlign_Center);
		CenterSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 80.0f));
	}

	// --- Bottom status panel ---
	BottomPanel = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("ShaderLoadingBottomPanel"));
	BottomPanel->SetBrush(FSlateRoundedBoxBrush(BottomPanelTint, 12.0f));
	BottomPanel->SetPadding(FMargin(28.0f, 22.0f));
	if (UOverlaySlot* PanelSlot = RootOverlay->AddChildToOverlay(BottomPanel))
	{
		PanelSlot->SetHorizontalAlignment(HAlign_Fill);
		PanelSlot->SetVerticalAlignment(VAlign_Bottom);
		PanelSlot->SetPadding(FMargin(40.0f, 0.0f, 40.0f, 36.0f));
	}

	RootBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("ShaderLoadingRootBox"));
	BottomPanel->SetContent(RootBox);

	// Row 1: status (left) + big percent (right).
	UHorizontalBox* StatusRow = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("ShaderStatusRow"));

	ShaderText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("ShaderLoadingText"));
	ShaderText->SetFont(MakeFont(BodyFontSize + 4));
	ShaderText->SetColorAndOpacity(FSlateColor(FLinearColor(0.85f, 0.93f, 0.99f, 1.0f)));
	if (UHorizontalBoxSlot* StS = StatusRow->AddChildToHorizontalBox(ShaderText))
	{
		StS->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
		StS->SetVerticalAlignment(VAlign_Center);
	}

	PercentText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("ShaderPercentText"));
	PercentText->SetFont(MakeFont(BodyFontSize + 22));
	PercentText->SetColorAndOpacity(FSlateColor(FLinearColor::White));
	if (UHorizontalBoxSlot* PtS = StatusRow->AddChildToHorizontalBox(PercentText))
	{
		PtS->SetVerticalAlignment(VAlign_Center);
	}

	if (UVerticalBoxSlot* RowSlot = RootBox->AddChildToVerticalBox(StatusRow))
	{
		RowSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 10.0f));
	}

	// Progress bar.
	CompileProgressBar = WidgetTree->ConstructWidget<UProgressBar>(UProgressBar::StaticClass(), TEXT("ShaderCompileProgressBar"));
	CompileProgressBar->SetPercent(0.0f);
	CompileProgressBar->SetFillColorAndOpacity(AccentColor);
	if (UVerticalBoxSlot* ProgressBarSlot = RootBox->AddChildToVerticalBox(CompileProgressBar))
	{
		ProgressBarSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 12.0f));
	}

	// Row 2: rotating tip (left) + jobs/detail (right).
	UHorizontalBox* DetailRow = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("ShaderDetailRow"));

	TipText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("ShaderLoadingTipText"));
	TipText->SetFont(MakeFont(BodyFontSize - 2));
	TipText->SetColorAndOpacity(FSlateColor(AccentColor * FLinearColor(1, 1, 1, 0.9f)));
	TipText->SetAutoWrapText(true);
	if (RotatingTips.Num() > 0)
	{
		TipText->SetText(RotatingTips[0]);
	}
	if (UHorizontalBoxSlot* TipS = DetailRow->AddChildToHorizontalBox(TipText))
	{
		TipS->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
		TipS->SetVerticalAlignment(VAlign_Center);
	}

	ProgressText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("ShaderProgressText"));
	ProgressText->SetFont(MakeFont(BodyFontSize - 1));
	ProgressText->SetColorAndOpacity(FSlateColor(FLinearColor(0.68f, 0.85f, 0.98f, 1.0f)));
	if (UHorizontalBoxSlot* PrS = DetailRow->AddChildToHorizontalBox(ProgressText))
	{
		PrS->SetVerticalAlignment(VAlign_Center);
		PrS->SetPadding(FMargin(16.0f, 0.0f, 0.0f, 0.0f));
	}

	if (UVerticalBoxSlot* DetailSlot = RootBox->AddChildToVerticalBox(DetailRow))
	{
		DetailSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 8.0f));
	}

	// Cache hint (small).
	CacheHintText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("ShaderCacheHintText"));
	CacheHintText->SetFont(MakeFont(BodyFontSize - 4));
	CacheHintText->SetColorAndOpacity(FSlateColor(FLinearColor(0.55f, 0.66f, 0.78f, 1.0f)));
	CacheHintText->SetAutoWrapText(true);
	RootBox->AddChildToVerticalBox(CacheHintText);

	// WaitText retained for compatibility (kept subtle / hidden by default).
	WaitText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("ShaderWaitText"));
	WaitText->SetFont(MakeFont(BodyFontSize - 4));
	WaitText->SetColorAndOpacity(FSlateColor(FLinearColor(0.55f, 0.66f, 0.78f, 1.0f)));
	WaitText->SetVisibility(ESlateVisibility::Collapsed);
	RootBox->AddChildToVerticalBox(WaitText);

	// --- Spinner (auto-animated) bottom-right ---
	Spinner = WidgetTree->ConstructWidget<UCircularThrobber>(UCircularThrobber::StaticClass(), TEXT("ShaderSpinner"));
	Spinner->SetNumberOfPieces(10);
	Spinner->SetRadius(26.0f);
	if (UOverlaySlot* SpinSlot = RootOverlay->AddChildToOverlay(Spinner))
	{
		SpinSlot->SetHorizontalAlignment(HAlign_Right);
		SpinSlot->SetVerticalAlignment(VAlign_Bottom);
		SpinSlot->SetPadding(FMargin(0.0f, 0.0f, 60.0f, 150.0f));
	}

	UE_LOG(LogZonefallShaderLoadingWidget, Log, TEXT("BuildLayoutIfNeeded: runtime fallback layout created successfully"));
}

void UZonefallShaderLoadingWidget::ApplyRuntimeFonts()
{
	UObject* TitleFontObject = CustomTitleFont.IsNull()
		? LoadObject<UObject>(nullptr, TEXT("/Engine/EngineFonts/Roboto.Roboto"))
		: Cast<UObject>(CustomTitleFont.LoadSynchronous());
	UObject* BodyFontObject = CustomBodyFont.IsNull()
		? LoadObject<UObject>(nullptr, TEXT("/Engine/EngineFonts/Roboto.Roboto"))
		: Cast<UObject>(CustomBodyFont.LoadSynchronous());

	if (TitleText)
	{
		FSlateFontInfo FontInfo = TitleText->GetFont();
		FontInfo.Size = FMath::Clamp(TitleFontSize, 10, 72);
		if (TitleFontObject)
		{
			FontInfo.FontObject = TitleFontObject;
		}
		TitleText->SetFont(FontInfo);
	}

	auto ApplyBody = [this, BodyFontObject](UTextBlock* Text, int32 Size)
	{
		if (!Text)
		{
			return;
		}
		FSlateFontInfo FontInfo = Text->GetFont();
		FontInfo.Size = FMath::Clamp(Size, 10, 48);
		if (BodyFontObject)
		{
			FontInfo.FontObject = BodyFontObject;
		}
		Text->SetFont(FontInfo);
	};

	ApplyBody(ShaderText, BodyFontSize + 4);
	ApplyBody(SubtitleText, BodyFontSize + 2);
	ApplyBody(PercentText, BodyFontSize + 22);
	ApplyBody(ProgressText, BodyFontSize - 1);
	ApplyBody(WaitText, BodyFontSize - 4);
	ApplyBody(CacheHintText, BodyFontSize - 4);
	ApplyBody(TipText, BodyFontSize - 2);
}

void UZonefallShaderLoadingWidget::UpdateTexts()
{
	if (TitleText)
	{
		TitleText->SetText(StartupTitleText);
	}
	if (SubtitleText)
	{
		SubtitleText->SetText(StartupSubtitleText);
	}
	if (ShaderText)
	{
		ShaderText->SetText(ShaderLoadingBaseText);
	}
	const float DisplayedPercent = FMath::Clamp(SmoothedProgressPercent, 0.0f, 100.0f);
	if (PercentText)
	{
		PercentText->SetText(FText::FromString(FString::Printf(TEXT("%.0f%%"), DisplayedPercent)));
	}
	if (ProgressText)
	{
		// Show both shader-map jobs AND render pipelines (PSOs) — PSOs are what cause hitches.
		FString Detail;
		if (LastKnownRemainingJobs > 0)
		{
			Detail = FString::Printf(TEXT("Shaders: %d"), LastKnownRemainingJobs);
		}
		if (PipelinesRemaining > 0)
		{
			if (!Detail.IsEmpty()) { Detail += TEXT("   •   "); }
			Detail += FString::Printf(TEXT("Pipelines: %d"), PipelinesRemaining);
		}
		ProgressText->SetText(Detail.IsEmpty() ? FText::GetEmpty() : FText::FromString(Detail));
	}
	if (CompileProgressBar)
	{
		CompileProgressBar->SetPercent(DisplayedPercent / 100.0f);
	}
	if (WaitText)
	{
		WaitText->SetText(PleaseWaitText);
	}
	if (CacheHintText)
	{
		CacheHintText->SetText(ShaderCacheHintText);
	}
	if (TipText && RotatingTips.IsValidIndex(CurrentTipIndex))
	{
		TipText->SetText(RotatingTips[CurrentTipIndex]);
	}
}

void UZonefallShaderLoadingWidget::HandleTextAnimTick()
{
	if (!ShaderText)
	{
		return;
	}

	DotCount = (DotCount + 1) % 4;
	FString Dots;
	for (int32 Index = 0; Index < DotCount; ++Index)
	{
		Dots += TEXT(".");
	}

	if (bUseRealShaderCompilerProgress && GShaderCompilingManager)
	{
		const int32 RemainingJobs = FMath::Max(0, GShaderCompilingManager->GetNumRemainingJobs());
		if (RemainingJobs <= 0)
		{
			++ConsecutiveZeroJobTicks;
		}
		else
		{
			ConsecutiveZeroJobTicks = 0;
			bEnteredFinalizingState = false;
		}

		const bool bStableZeroJobs = ConsecutiveZeroJobTicks >= 3;
		LastKnownRemainingJobs = bStableZeroJobs ? 0 : RemainingJobs;
		if (!bShaderProgressInitialized)
		{
			InitialShaderJobCount = FMath::Max(1, RemainingJobs);
			bShaderProgressInitialized = true;
		}
		else if (RemainingJobs > InitialShaderJobCount)
		{
			InitialShaderJobCount = RemainingJobs;
		}

		if (InitialShaderJobCount > 0)
		{
			const float ComputedPercent = ((InitialShaderJobCount - RemainingJobs) * 100.0f) / static_cast<float>(InitialShaderJobCount);
			ShaderCompileProgressPercent = FMath::Max(ShaderCompileProgressPercent, ComputedPercent);
		}
		if (bStableZeroJobs)
		{
			bEnteredFinalizingState = true;
			ShaderCompileProgressPercent = 100.0f;
		}
	}

	const float ClampedPercent = FMath::Clamp(ShaderCompileProgressPercent, 0.0f, 100.0f);
	const float TickDelta = FMath::Max(0.016f, TextAnimInterval);
	AnimationTimeSeconds += TickDelta;

	SmoothedProgressPercent = FMath::FInterpConstantTo(
		SmoothedProgressPercent,
		ClampedPercent,
		TickDelta,
		FMath::Max(5.0f, ProgressUnitsPerSecond)
	);
	if (LastKnownRemainingJobs > 0 && SmoothedProgressPercent < 1.0f)
	{
		SmoothedProgressPercent = 1.0f;
	}

	if (bEnteredFinalizingState || (LastKnownRemainingJobs <= 0 && SmoothedProgressPercent >= 99.5f))
	{
		ShaderText->SetText(ShaderFinalizingText);
	}
	else if (LastKnownRemainingJobs <= 0)
	{
		ShaderText->SetText(FText::FromString(ShaderCheckingCacheText.ToString() + Dots));
	}
	else
	{
		ShaderText->SetText(FText::FromString(ShaderLoadingBaseText.ToString() + Dots));
	}
	if (PercentText)
	{
		PercentText->SetText(FText::FromString(FString::Printf(TEXT("%.0f%%"), FMath::Clamp(SmoothedProgressPercent, 0.0f, 100.0f))));
	}
	if (ProgressText)
	{
		// Show both shader-map jobs AND render pipelines (PSOs) — PSOs are what cause hitches.
		FString Detail;
		if (LastKnownRemainingJobs > 0)
		{
			Detail = FString::Printf(TEXT("Shaders: %d"), LastKnownRemainingJobs);
		}
		if (PipelinesRemaining > 0)
		{
			if (!Detail.IsEmpty()) { Detail += TEXT("   •   "); }
			Detail += FString::Printf(TEXT("Pipelines: %d"), PipelinesRemaining);
		}
		ProgressText->SetText(Detail.IsEmpty() ? FText::GetEmpty() : FText::FromString(Detail));
	}
	if (CompileProgressBar)
	{
		CompileProgressBar->SetPercent(FMath::Clamp(SmoothedProgressPercent, 0.0f, 100.0f) / 100.0f);
		if (bEnableUiAnimations)
		{
			const float Glow = 0.80f + 0.20f * (0.5f + 0.5f * FMath::Sin(AnimationTimeSeconds * FMath::Max(0.1f, BarGlowSpeed)));
			CompileProgressBar->SetFillColorAndOpacity(AccentColor * Glow + FLinearColor(0.0f, 0.0f, 0.0f, 1.0f - AccentColor.A));
		}
	}
	if (BottomPanel && bEnableUiAnimations)
	{
		const float Pulse = 0.90f + 0.10f * (0.5f + 0.5f * FMath::Sin(AnimationTimeSeconds * FMath::Max(0.1f, PanelPulseSpeed)));
		const FLinearColor AnimatedPanelTint(BottomPanelTint.R * Pulse, BottomPanelTint.G * Pulse, BottomPanelTint.B * Pulse, BottomPanelTint.A);
		BottomPanel->SetBrushColor(AnimatedPanelTint);
	}

	if (bEnableUiAnimations)
	{
		// Accent divider sweeps brightness in sync with the bar glow.
		if (AccentDivider)
		{
			const float DividerGlow = 0.70f + 0.30f * (0.5f + 0.5f * FMath::Sin(AnimationTimeSeconds * FMath::Max(0.1f, BarGlowSpeed)));
			AccentDivider->SetColorAndOpacity(AccentColor * DividerGlow);
		}
		if (TopAccentLine)
		{
			const float LineGlow = 0.55f + 0.45f * (0.5f + 0.5f * FMath::Sin(AnimationTimeSeconds * FMath::Max(0.1f, BarGlowSpeed) + 1.0f));
			TopAccentLine->SetColorAndOpacity(AccentColor * FLinearColor(1, 1, 1, LineGlow));
		}

		// Title gently "breathes" so the logo never looks frozen.
		if (TitleText)
		{
			const float TitleBreath = 0.85f + 0.15f * (0.5f + 0.5f * FMath::Sin(AnimationTimeSeconds * FMath::Max(0.1f, PanelPulseSpeed) * 0.6f));
			TitleText->SetRenderOpacity(TitleBreath);
		}
	}

	// Rotate the loading tip on its own slow cadence.
	if (TipText && RotatingTips.Num() > 1)
	{
		TipElapsedSeconds += TickDelta;
		if (TipElapsedSeconds >= FMath::Max(1.0f, TipRotateSeconds))
		{
			TipElapsedSeconds = 0.0f;
			CurrentTipIndex = (CurrentTipIndex + 1) % RotatingTips.Num();
			TipText->SetText(RotatingTips[CurrentTipIndex]);
		}
	}
}

void UZonefallShaderLoadingWidget::SetShaderCompileProgress(float NewPercent)
{
	ShaderCompileProgressPercent = FMath::Clamp(NewPercent, 0.0f, 100.0f);
	SmoothedProgressPercent = ShaderCompileProgressPercent;
	UpdateTexts();
}

void UZonefallShaderLoadingWidget::SetInitialShaderJobCount(int32 InInitialJobCount)
{
	InitialShaderJobCount = FMath::Max(1, InInitialJobCount);
	LastKnownRemainingJobs = FMath::Max(0, InInitialJobCount);
	ShaderCompileProgressPercent = 0.0f;
	SmoothedProgressPercent = 0.0f;
	bShaderProgressInitialized = true;
}

void UZonefallShaderLoadingWidget::SetPipelinesRemaining(int32 InRemaining)
{
	PipelinesRemaining = FMath::Max(0, InRemaining);
}

bool UZonefallShaderLoadingWidget::IsShaderCompilationLikelyFinished() const
{
	if (!bUseRealShaderCompilerProgress || !GShaderCompilingManager)
	{
		return ShaderCompileProgressPercent >= 100.0f;
	}

	return GShaderCompilingManager->GetNumRemainingJobs() <= 0;
}

