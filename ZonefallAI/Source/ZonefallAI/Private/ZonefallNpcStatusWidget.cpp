#include "ZonefallNpcStatusWidget.h"

#include "ZonefallAICharacterComponent.h"
#include "ZonefallNpcVitalsComponent.h"

#include "Components/WidgetComponent.h"

#include "Blueprint/WidgetTree.h"
#include "Brushes/SlateRoundedBoxBrush.h"
#include "Components/Border.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/ProgressBar.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Engine/Font.h"
#include "Styling/SlateTypes.h"

UZonefallNpcStatusWidget::UZonefallNpcStatusWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

TSharedRef<SWidget> UZonefallNpcStatusWidget::RebuildWidget()
{
	if (!WidgetTree || !WidgetTree->RootWidget)
	{
		BuildLayout();
	}
	EnsureVitalsBound();
	return Super::RebuildWidget();
}

void UZonefallNpcStatusWidget::NativeConstruct()
{
	Super::NativeConstruct();
	EnsureVitalsBound();
}

void UZonefallNpcStatusWidget::EnsureVitalsBound()
{
	if (Vitals)
	{
		return;
	}

	// The WidgetComponent is our Outer; its owning actor carries the vitals component.
	AActor* OwnerActor = nullptr;
	if (const UWidgetComponent* WidgetComp = Cast<UWidgetComponent>(GetOuter()))
	{
		OwnerActor = WidgetComp->GetOwner();
	}
	if (!OwnerActor)
	{
		OwnerActor = GetOwningPlayerPawn();
	}
	if (!OwnerActor)
	{
		return;
	}

	Vitals = OwnerActor->FindComponentByClass<UZonefallNpcVitalsComponent>();
}

void UZonefallNpcStatusWidget::BindToVitals(UZonefallNpcVitalsComponent* InVitals)
{
	Vitals = InVitals;
}

void UZonefallNpcStatusWidget::BuildLayout()
{
	if (!WidgetTree)
	{
		return;
	}

	WidgetTree->RootWidget = nullptr;

	UVerticalBox* Root = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("NpcRoot"));
	WidgetTree->RootWidget = Root;

	auto MakeBar = [this](FName Name, const FLinearColor& Fill, float Height) -> UProgressBar*
	{
		UProgressBar* Bar = WidgetTree->ConstructWidget<UProgressBar>(UProgressBar::StaticClass(), Name);
		FProgressBarStyle Style = Bar->GetWidgetStyle();
		Style.BackgroundImage = FSlateRoundedBoxBrush(FLinearColor(0.0f, 0.0f, 0.0f, 0.7f), Height * 0.5f, FLinearColor(0, 0, 0, 0.85f), 1.0f);
		Style.FillImage = FSlateRoundedBoxBrush(Fill, Height * 0.5f);
		Bar->SetWidgetStyle(Style);
		Bar->SetPercent(1.0f);
		return Bar;
	};

	// --- Health row (hostiles only) ---
	{
		USizeBox* HpSize = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("NpcHpSize"));
		HpSize->SetHeightOverride(8.0f);
		HealthBar = MakeBar(TEXT("NpcHealthBar"), HealthColor, 8.0f);
		HpSize->AddChild(HealthBar);

		HealthRow = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("NpcHealthRow"));
		HealthRow->SetBrush(FSlateRoundedBoxBrush(FLinearColor(0, 0, 0, 0), 0.0f));
		HealthRow->SetPadding(FMargin(0));
		HealthRow->SetContent(HpSize);
		HealthRow->SetVisibility(ESlateVisibility::Collapsed);
		Root->AddChildToVerticalBox(HealthRow);
	}

	// --- Detection row (fills as the NPC spots the hero) ---
	{
		UOverlay* DetectOverlay = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass(), TEXT("NpcDetectOverlay"));

		USizeBox* DetSize = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("NpcDetSize"));
		DetSize->SetHeightOverride(6.0f);
		DetectBar = MakeBar(TEXT("NpcDetectBar"), DetectionColor, 6.0f);
		DetectBar->SetPercent(0.0f);
		DetSize->AddChild(DetectBar);
		if (UOverlaySlot* DS = DetectOverlay->AddChildToOverlay(DetSize))
		{
			DS->SetHorizontalAlignment(HAlign_Fill);
			DS->SetVerticalAlignment(VAlign_Fill);
		}

		AlertText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("NpcAlert"));
		AlertText->SetText(FText::FromString(TEXT("!")));
		{
			FSlateFontInfo Font;
			Font.Size = 16;
			Font.TypefaceFontName = FName(TEXT("Bold"));
			Font.FontObject = LoadObject<UObject>(nullptr, TEXT("/Engine/EngineFonts/Roboto.Roboto"));
			AlertText->SetFont(Font);
		}
		AlertText->SetColorAndOpacity(FSlateColor(AlertColor));
		AlertText->SetJustification(ETextJustify::Center);
		AlertText->SetVisibility(ESlateVisibility::Collapsed);
		if (UOverlaySlot* AS = DetectOverlay->AddChildToOverlay(AlertText))
		{
			AS->SetHorizontalAlignment(HAlign_Center);
			AS->SetVerticalAlignment(VAlign_Bottom);
		}

		DetectRow = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("NpcDetectRow"));
		DetectRow->SetBrush(FSlateRoundedBoxBrush(FLinearColor(0, 0, 0, 0), 0.0f));
		DetectRow->SetContent(DetectOverlay);
		DetectRow->SetVisibility(ESlateVisibility::Collapsed);
		if (UVerticalBoxSlot* DRSlot = Root->AddChildToVerticalBox(DetectRow))
		{
			DRSlot->SetPadding(FMargin(0, 3, 0, 0));
		}
	}
}

void UZonefallNpcStatusWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	EnsureVitalsBound();
	if (!Vitals)
	{
		return;
	}

	PulseTime += InDeltaTime;

	const bool bHostile = Vitals->IsHostileNpc();
	const bool bDead = Vitals->IsDead();

	// Health bar — hostiles only (civilians never show a floating health bar).
	if (HealthRow && HealthBar)
	{
		const bool bShowHealth = bHostile && !bDead;
		HealthRow->SetVisibility(bShowHealth ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
		if (bShowHealth)
		{
			const float Frac = Vitals->GetHealthFraction();
			HealthBar->SetPercent(Frac);
			const FLinearColor Lo(0.85f, 0.20f, 0.16f, 1.0f);
			const FLinearColor Hi(0.30f, 0.85f, 0.45f, 1.0f);
			HealthBar->SetFillColorAndOpacity(FMath::Lerp(Lo, Hi, Frac));
		}
	}

	// Detection meter — grows as the NPC spots the hero.
	if (DetectRow && DetectBar)
	{
		const float Awareness = Vitals->GetAwareness();
		const bool bShowDetect = !bDead && Awareness > 0.02f;
		DetectRow->SetVisibility(bShowDetect ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
		if (bShowDetect)
		{
			DetectBar->SetPercent(Awareness);
			DetectBar->SetFillColorAndOpacity(FMath::Lerp(DetectionColor, AlertColor, Awareness));
		}

		if (AlertText)
		{
			if (Vitals->IsAlerted())
			{
				AlertText->SetVisibility(ESlateVisibility::HitTestInvisible);
				const float Blink = 0.5f + 0.5f * FMath::Sin(PulseTime * 10.0f);
				AlertText->SetRenderOpacity(0.4f + 0.6f * Blink);
			}
			else
			{
				AlertText->SetVisibility(ESlateVisibility::Collapsed);
			}
		}
	}
}
