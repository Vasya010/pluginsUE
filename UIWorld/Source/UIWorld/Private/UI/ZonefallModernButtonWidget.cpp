#include "ZonefallModernButtonWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/ButtonSlot.h"
#include "Components/Overlay.h"
#include "Components/TextBlock.h"
#include "Brushes/SlateRoundedBoxBrush.h"
#include "Kismet/GameplayStatics.h"
#include "Math/UnrealMathUtility.h"
#include "Sound/SoundBase.h"
#include "Styling/SlateTypes.h"

UZonefallModernButtonWidget::UZonefallModernButtonWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
	, Label(NSLOCTEXT("ZonefallUI", "ModernButtonDefaultLabel", "CONTINUE"))
	, FallbackLabel(NSLOCTEXT("ZonefallUI", "ModernButtonFallbackLabel", "PLAY"))
	, LabelFontSize(30)
	, LabelColorNormal(FLinearColor(0.92f, 0.95f, 1.0f, 1.0f))
	, LabelColorHover(FLinearColor(0.70f, 0.92f, 1.0f, 1.0f))
	, LabelColorPressed(FLinearColor(0.45f, 0.82f, 1.0f, 1.0f))
	, BackgroundNormal(FLinearColor(0.04f, 0.07f, 0.13f, 0.94f))
	, BackgroundHover(FLinearColor(0.09f, 0.15f, 0.24f, 1.0f))
	, BackgroundPressed(FLinearColor(0.03f, 0.05f, 0.10f, 1.0f))
	, ThemePreset(EZonefallModernButtonTheme::Dark)
	, bUseGlassGradientStyle(false)
	, bEmitStartGameOnClick(false)
	, HoverSound(nullptr)
	, PressSound(nullptr)
	, ClickSound(nullptr)
	, TargetButtonWidgetName(TEXT("ClickButton"))
	, TargetTextWidgetName(TEXT("LabelText"))
{
}

void UZonefallModernButtonWidget::NativePreConstruct()
{
	Super::NativePreConstruct();

	ValidateAndBindWidgets();
	ApplyVisualStyle();
}

void UZonefallModernButtonWidget::NativeConstruct()
{
	Super::NativeConstruct();

	ValidateAndBindWidgets();
	ApplyVisualStyle();
}

void UZonefallModernButtonWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	if (!ClickButton || !LabelText)
	{
		return;
	}

	if (bHoverPulseActive)
	{
		HoverPulseTime += InDeltaTime;

		const float Pulse = 1.0f + (FMath::Sin(HoverPulseTime * 9.0f) * 0.03f);
		ClickButton->SetRenderScale(FVector2D(Pulse, Pulse));

		const float GlowPulse = 0.80f + (FMath::Sin(HoverPulseTime * 10.0f) * 0.18f);
		const FLinearColor NeonGlow = LabelColorHover * GlowPulse;
		LabelText->SetShadowColorAndOpacity(FLinearColor(NeonGlow.R, NeonGlow.G, NeonGlow.B, 0.95f));
	}
	else
	{
		// Smoothly return to idle state after mouse leaves button.
		const FVector2D CurrentScale = ClickButton->GetRenderTransform().Scale;
		const float NextX = FMath::FInterpTo(CurrentScale.X, 1.0f, InDeltaTime, 12.0f);
		const float NextY = FMath::FInterpTo(CurrentScale.Y, 1.0f, InDeltaTime, 12.0f);
		ClickButton->SetRenderScale(FVector2D(NextX, NextY));

		const FLinearColor CurrentGlow = LabelText->GetShadowColorAndOpacity();
		const FLinearColor TargetGlow(0.0f, 0.0f, 0.0f, 0.7f);
		const float NextR = FMath::FInterpTo(CurrentGlow.R, TargetGlow.R, InDeltaTime, 10.0f);
		const float NextG = FMath::FInterpTo(CurrentGlow.G, TargetGlow.G, InDeltaTime, 10.0f);
		const float NextB = FMath::FInterpTo(CurrentGlow.B, TargetGlow.B, InDeltaTime, 10.0f);
		const float NextA = FMath::FInterpTo(CurrentGlow.A, TargetGlow.A, InDeltaTime, 10.0f);
		LabelText->SetShadowColorAndOpacity(FLinearColor(NextR, NextG, NextB, NextA));
	}
}

void UZonefallModernButtonWidget::SynchronizeProperties()
{
	Super::SynchronizeProperties();

	ValidateAndBindWidgets();
	ApplyVisualStyle();
}

void UZonefallModernButtonWidget::SetLabel(const FText& InLabel)
{
	Label = InLabel;
	if (LabelText)
	{
		LabelText->SetText(GetEffectiveLabel());
	}
}

void UZonefallModernButtonWidget::ApplyVisualStyle()
{
	SyncThemeColors();
	ApplyButtonCoreStyle();
	UpdateVisualState(BackgroundNormal, LabelColorNormal, FVector2D(1.0f, 2.0f));
}

void UZonefallModernButtonWidget::ApplyThemePreset(EZonefallModernButtonTheme NewTheme)
{
	ThemePreset = NewTheme;
	ApplyVisualStyle();
}

void UZonefallModernButtonWidget::RequestStartGame()
{
	OnStartGameRequested.Broadcast();
	BP_OnStartGameRequested();
}

bool UZonefallModernButtonWidget::ValidateAndBindWidgets()
{
	TryBindFromWidgetTree();
	BuildWidgetTree();

	const bool bIsValid = (ClickButton != nullptr) && (LabelText != nullptr);
	if (!bIsValid)
	{
		UE_LOG(LogTemp, Warning, TEXT("ZonefallModernButtonWidget: button/text binding failed in widget '%s'."), *GetName());
	}
	return bIsValid;
}

void UZonefallModernButtonWidget::TryBindFromWidgetTree()
{
	if (!WidgetTree)
	{
		return;
	}

	auto FindButtonByName = [this](const FName NameToFind) -> UButton*
	{
		if (NameToFind.IsNone())
		{
			return nullptr;
		}
		return Cast<UButton>(WidgetTree->FindWidget(NameToFind));
	};

	auto FindTextByName = [this](const FName NameToFind) -> UTextBlock*
	{
		if (NameToFind.IsNone())
		{
			return nullptr;
		}
		return Cast<UTextBlock>(WidgetTree->FindWidget(NameToFind));
	};

	if (!ClickButton)
	{
		ClickButton = FindButtonByName(TargetButtonWidgetName);
		if (!ClickButton)
		{
			ClickButton = FindButtonByName(TEXT("ClickButton"));
		}
		if (!ClickButton)
		{
			ClickButton = FindButtonByName(TEXT("Button"));
		}
	}

	if (!LabelText)
	{
		LabelText = FindTextByName(TargetTextWidgetName);
		if (!LabelText)
		{
			LabelText = FindTextByName(TEXT("LabelText"));
		}
		if (!LabelText)
		{
			LabelText = FindTextByName(TEXT("Text"));
		}
		if (!LabelText)
		{
			LabelText = FindTextByName(TEXT("TextBlock"));
		}
	}
}

void UZonefallModernButtonWidget::BuildWidgetTree()
{
	if (!WidgetTree)
	{
		return;
	}

	if (!RootBorder)
	{
		RootBorder = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("ModernButtonRoot"));
		RootBorder->SetPadding(FMargin(2.0f));
		RootBorder->SetBrushColor(FLinearColor(0.20f, 0.55f, 0.9f, 0.65f));
		if (!WidgetTree->RootWidget)
		{
			WidgetTree->RootWidget = RootBorder;
		}
	}

	if (!ClickButton)
	{
		ClickButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("ModernButtonCore"));
		ClickButton->SetToolTipText(NSLOCTEXT("ZonefallUI", "ModernButtonTooltip", "Modern action button"));

		if (RootBorder)
		{
			RootBorder->SetContent(ClickButton);
		}
	}

	if (!LabelText)
	{
		LabelText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("ModernButtonLabel"));
		LabelText->SetJustification(ETextJustify::Center);
		LabelText->SetText(GetEffectiveLabel());
		if (ClickButton)
		{
			ClickButton->AddChild(LabelText);
		}
	}

	if (!ClickButton)
	{
		return;
	}

	ClickButton->OnClicked.RemoveDynamic(this, &UZonefallModernButtonWidget::HandleClicked);
	ClickButton->OnHovered.RemoveDynamic(this, &UZonefallModernButtonWidget::HandleHovered);
	ClickButton->OnUnhovered.RemoveDynamic(this, &UZonefallModernButtonWidget::HandleUnhovered);
	ClickButton->OnPressed.RemoveDynamic(this, &UZonefallModernButtonWidget::HandlePressed);
	ClickButton->OnReleased.RemoveDynamic(this, &UZonefallModernButtonWidget::HandleReleased);

	ClickButton->OnClicked.AddDynamic(this, &UZonefallModernButtonWidget::HandleClicked);
	ClickButton->OnHovered.AddDynamic(this, &UZonefallModernButtonWidget::HandleHovered);
	ClickButton->OnUnhovered.AddDynamic(this, &UZonefallModernButtonWidget::HandleUnhovered);
	ClickButton->OnPressed.AddDynamic(this, &UZonefallModernButtonWidget::HandlePressed);
	ClickButton->OnReleased.AddDynamic(this, &UZonefallModernButtonWidget::HandleReleased);
}

void UZonefallModernButtonWidget::SyncThemeColors()
{
	if (ThemePreset == EZonefallModernButtonTheme::Neon)
	{
		LabelColorNormal = FLinearColor(0.86f, 0.98f, 1.0f, 1.0f);
		LabelColorHover = FLinearColor(0.15f, 1.00f, 0.88f, 1.0f);
		LabelColorPressed = FLinearColor(0.12f, 0.92f, 0.82f, 1.0f);
		BackgroundNormal = FLinearColor(0.02f, 0.12f, 0.18f, 0.92f);
		BackgroundHover = FLinearColor(0.01f, 0.01f, 0.01f, 0.98f);
		BackgroundPressed = FLinearColor(0.00f, 0.00f, 0.00f, 1.0f);
	}
	else
	{
		// Dark mode: nearly invisible button body + neon typography.
		LabelColorNormal = FLinearColor(0.10f, 1.00f, 0.80f, 1.0f);
		LabelColorHover = FLinearColor(0.12f, 1.00f, 0.84f, 1.0f);
		LabelColorPressed = FLinearColor(0.10f, 0.88f, 0.74f, 1.0f);
		BackgroundNormal = FLinearColor(0.0f, 0.0f, 0.0f, 0.00f);
		BackgroundHover = FLinearColor(0.0f, 0.0f, 0.0f, 0.06f);
		BackgroundPressed = FLinearColor(0.0f, 0.0f, 0.0f, 0.12f);
	}
}

void UZonefallModernButtonWidget::ApplyButtonCoreStyle()
{
	if (!ClickButton || !RootBorder)
	{
		return;
	}

	FButtonStyle Style = ClickButton->GetStyle();
	Style.NormalPadding = FMargin(24.0f, 14.0f);
	Style.PressedPadding = FMargin(24.0f, 16.0f, 24.0f, 12.0f);

	if (bUseGlassGradientStyle)
	{
		Style.SetNormal(FSlateRoundedBoxBrush(FLinearColor(1.0f, 1.0f, 1.0f, 0.62f), 12.0f));
		Style.SetHovered(FSlateRoundedBoxBrush(FLinearColor(1.0f, 1.0f, 1.0f, 0.78f), 12.0f));
		Style.SetPressed(FSlateRoundedBoxBrush(FLinearColor(1.0f, 1.0f, 1.0f, 0.50f), 12.0f));
		RootBorder->SetBrushColor(FLinearColor(0.35f, 0.75f, 1.0f, 0.45f));
	}
	else
	{
		Style.SetNormal(FSlateRoundedBoxBrush(BackgroundNormal, 12.0f));
		Style.SetHovered(FSlateRoundedBoxBrush(BackgroundHover, 12.0f));
		Style.SetPressed(FSlateRoundedBoxBrush(BackgroundPressed, 12.0f));
		if (ThemePreset == EZonefallModernButtonTheme::Dark)
		{
			RootBorder->SetBrushColor(FLinearColor(0.0f, 0.0f, 0.0f, 0.0f));
		}
		else
		{
			RootBorder->SetBrushColor(FLinearColor(0.20f, 0.55f, 0.9f, 0.65f));
		}
	}

	ClickButton->SetStyle(Style);
}

void UZonefallModernButtonWidget::UpdateVisualState(const FLinearColor& InBackground, const FLinearColor& InTextColor, const FVector2D& ShadowOffset)
{
	if (RootBorder)
	{
		RootBorder->SetBrushColor(InBackground);
	}

	if (LabelText)
	{
		// Keep designer-authored font unless a concrete override font is provided.
		FSlateFontInfo EffectiveFont = LabelText->GetFont();
		if (LabelFont.FontObject != nullptr || !LabelFont.TypefaceFontName.IsNone())
		{
			EffectiveFont = LabelFont;
		}
		EffectiveFont.Size = LabelFontSize;
		LabelText->SetText(GetEffectiveLabel());
		LabelText->SetFont(EffectiveFont);
		LabelText->SetColorAndOpacity(FSlateColor(InTextColor));
		LabelText->SetShadowOffset(ShadowOffset);
		LabelText->SetShadowColorAndOpacity(FLinearColor(0.0f, 0.0f, 0.0f, 0.7f));
	}
}

FText UZonefallModernButtonWidget::GetEffectiveLabel() const
{
	if (!Label.IsEmpty())
	{
		return Label;
	}

	if (!FallbackLabel.IsEmpty())
	{
		return FallbackLabel;
	}

	return NSLOCTEXT("ZonefallUI", "ModernButtonEmergencyLabel", "BUTTON");
}

void UZonefallModernButtonWidget::HandleClicked()
{
	if (ClickSound)
	{
		UGameplayStatics::PlaySound2D(this, ClickSound);
	}

	OnClicked.Broadcast();
	BP_OnButtonClicked();

	if (bEmitStartGameOnClick)
	{
		RequestStartGame();
	}
}

void UZonefallModernButtonWidget::HandleHovered()
{
	if (HoverSound)
	{
		UGameplayStatics::PlaySound2D(this, HoverSound);
	}

	bHoverPulseActive = true;
	HoverPulseTime = 0.0f;
	BP_OnButtonHovered();
	UpdateVisualState(BackgroundHover, LabelColorHover, FVector2D(0.0f, 0.0f));
}

void UZonefallModernButtonWidget::HandleUnhovered()
{
	bHoverPulseActive = false;
	UpdateVisualState(BackgroundNormal, LabelColorNormal, FVector2D(1.0f, 2.0f));
}

void UZonefallModernButtonWidget::HandlePressed()
{
	if (PressSound)
	{
		UGameplayStatics::PlaySound2D(this, PressSound);
	}

	BP_OnButtonPressed();
	UpdateVisualState(BackgroundPressed, LabelColorPressed, FVector2D(0.0f, 0.0f));
}

void UZonefallModernButtonWidget::HandleReleased()
{
	UpdateVisualState(BackgroundHover, LabelColorHover, FVector2D(0.0f, 0.0f));
}

