#include "UI/ZonefallDeadEyeWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Brushes/SlateRoundedBoxBrush.h"
#include "Components/Border.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/ProgressBar.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Engine/Font.h"
#include "Styling/SlateTypes.h"

#include "Character/ZonefallPlayerCharacter.h"

UZonefallDeadEyeWidget::UZonefallDeadEyeWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

TSharedRef<SWidget> UZonefallDeadEyeWidget::RebuildWidget()
{
	if (!WidgetTree || !WidgetTree->RootWidget)
	{
		BuildLayout();
	}
	return Super::RebuildWidget();
}

void UZonefallDeadEyeWidget::BindToCharacter(AZonefallPlayerCharacter* InCharacter)
{
	Character = InCharacter;
}

void UZonefallDeadEyeWidget::BuildLayout()
{
	if (!WidgetTree)
	{
		return;
	}

	WidgetTree->RootWidget = nullptr;

	UOverlay* Root = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass(), TEXT("DeadEyeRoot"));
	WidgetTree->RootWidget = Root;

	UVerticalBox* Col = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("DeadEyeCol"));
	if (UOverlaySlot* ColSlot = Root->AddChildToOverlay(Col))
	{
		ColSlot->SetHorizontalAlignment(HAlign_Center);
		ColSlot->SetVerticalAlignment(VAlign_Bottom);
	}

	Label = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("DeadEyeLabel"));
	Label->SetText(NSLOCTEXT("ZonefallDeadEye", "DeadEye", "DEAD EYE"));
	{
		FSlateFontInfo Font;
		Font.Size = 14;
		Font.FontObject = LoadObject<UObject>(nullptr, TEXT("/Engine/EngineFonts/Roboto.Roboto"));
		Label->SetFont(Font);
	}
	Label->SetColorAndOpacity(FSlateColor(FLinearColor(0.95f, 0.85f, 0.85f, 1.0f)));
	Label->SetJustification(ETextJustify::Center);
	if (UVerticalBoxSlot* LblSlot = Col->AddChildToVerticalBox(Label))
	{
		LblSlot->SetHorizontalAlignment(HAlign_Center);
		LblSlot->SetPadding(FMargin(0, 0, 0, 4));
	}

	// Bar inside a sized box so it has a fixed width.
	USizeBox* BarSize = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("DeadEyeBarSize"));
	BarSize->SetWidthOverride(280.0f);
	BarSize->SetHeightOverride(14.0f);

	Meter = WidgetTree->ConstructWidget<UProgressBar>(UProgressBar::StaticClass(), TEXT("DeadEyeMeter"));
	{
		FProgressBarStyle Style = Meter->GetWidgetStyle();
		Style.BackgroundImage = FSlateRoundedBoxBrush(FLinearColor(0.0f, 0.0f, 0.0f, 0.55f), 7.0f);
		Style.FillImage = FSlateRoundedBoxBrush(FillColor, 7.0f);
		Meter->SetWidgetStyle(Style);
	}
	Meter->SetPercent(1.0f);
	BarSize->AddChild(Meter);

	if (UVerticalBoxSlot* BarSlot = Col->AddChildToVerticalBox(BarSize))
	{
		BarSlot->SetHorizontalAlignment(HAlign_Center);
		BarSlot->SetPadding(FMargin(0, 0, 0, 60));
	}
}

void UZonefallDeadEyeWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	if (Character && Meter)
	{
		Meter->SetPercent(Character->GetDeadEyeFraction());
	}
}
