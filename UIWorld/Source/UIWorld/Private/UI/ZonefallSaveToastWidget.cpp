#include "UI/ZonefallSaveToastWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Brushes/SlateRoundedBoxBrush.h"
#include "Components/Border.h"
#include "Components/CircularThrobber.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"

namespace
{
	FSlateFontInfo MakeToastFont(int32 Size)
	{
		FSlateFontInfo F;
		F.Size = FMath::Clamp(Size, 8, 64);
		F.FontObject = LoadObject<UObject>(nullptr, TEXT("/Engine/EngineFonts/Roboto.Roboto"));
		return F;
	}

	constexpr float SlideInTime = 0.4f;
	constexpr float SlideOutTime = 0.45f;
	constexpr float OffsetX = 420.0f;
}

UZonefallSaveToastWidget::UZonefallSaveToastWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

TSharedRef<SWidget> UZonefallSaveToastWidget::RebuildWidget()
{
	if (!WidgetTree || !WidgetTree->RootWidget)
	{
		BuildLayout();
	}
	return Super::RebuildWidget();
}

void UZonefallSaveToastWidget::NativeConstruct()
{
	Super::NativeConstruct();
	AnimTime = 0.0f;
	bRemoved = false;
	if (Panel)
	{
		Panel->SetRenderTranslation(FVector2D(OffsetX, 0.0f)); // start off-screen right
	}
}

void UZonefallSaveToastWidget::ShowToast(const FText& Title, const FText& Message)
{
	if (TitleText)
	{
		TitleText->SetText(Title.IsEmpty() ? DefaultTitle : Title);
	}
	if (MessageText)
	{
		MessageText->SetText(Message);
	}
	AnimTime = 0.0f;
	bRemoved = false;
}

void UZonefallSaveToastWidget::BuildLayout()
{
	if (!WidgetTree)
	{
		return;
	}

	WidgetTree->RootWidget = nullptr;

	UOverlay* Root = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass(), TEXT("ToastRoot"));
	WidgetTree->RootWidget = Root;

	Panel = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("ToastPanel"));
	Panel->SetBrush(FSlateRoundedBoxBrush(PanelTint, 8.0f));
	Panel->SetPadding(FMargin(18.0f, 12.0f));
	Panel->SetRenderTranslation(FVector2D(OffsetX, 0.0f));
	if (UOverlaySlot* PanelSlot = Root->AddChildToOverlay(Panel))
	{
		PanelSlot->SetHorizontalAlignment(HAlign_Right);
		PanelSlot->SetVerticalAlignment(VAlign_Top);
		PanelSlot->SetPadding(FMargin(0.0f, 48.0f, 48.0f, 0.0f));
	}

	UHorizontalBox* Row = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("ToastRow"));
	Panel->SetContent(Row);

	Spinner = WidgetTree->ConstructWidget<UCircularThrobber>(UCircularThrobber::StaticClass(), TEXT("ToastSpinner"));
	Spinner->SetNumberOfPieces(8);
	Spinner->SetRadius(14.0f);
	if (UHorizontalBoxSlot* SpinSlot = Row->AddChildToHorizontalBox(Spinner))
	{
		SpinSlot->SetVerticalAlignment(VAlign_Center);
		SpinSlot->SetPadding(FMargin(0.0f, 0.0f, 14.0f, 0.0f));
	}

	UVerticalBox* TextCol = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("ToastTextCol"));
	if (UHorizontalBoxSlot* ColSlot = Row->AddChildToHorizontalBox(TextCol))
	{
		ColSlot->SetVerticalAlignment(VAlign_Center);
	}

	TitleText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("ToastTitle"));
	TitleText->SetText(DefaultTitle);
	TitleText->SetFont(MakeToastFont(16));
	TitleText->SetColorAndOpacity(FSlateColor(AccentColor));
	TextCol->AddChildToVerticalBox(TitleText);

	MessageText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("ToastMessage"));
	MessageText->SetText(FText::GetEmpty());
	MessageText->SetFont(MakeToastFont(12));
	MessageText->SetColorAndOpacity(FSlateColor(FLinearColor(0.8f, 0.86f, 0.92f, 1.0f)));
	TextCol->AddChildToVerticalBox(MessageText);
}

void UZonefallSaveToastWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	if (bRemoved || !Panel)
	{
		return;
	}

	AnimTime += InDeltaTime;

	const float HoldEnd = SlideInTime + FMath::Max(0.5f, DisplayDuration);
	const float Total = HoldEnd + SlideOutTime;

	float OffsetT = 0.0f; // 0 = fully visible, 1 = off-screen right
	if (AnimTime <= SlideInTime)
	{
		const float In = FMath::Clamp(AnimTime / SlideInTime, 0.0f, 1.0f);
		OffsetT = 1.0f - (1.0f - FMath::Pow(1.0f - In, 3.0f)); // ease-out
	}
	else if (AnimTime <= HoldEnd)
	{
		OffsetT = 0.0f;
	}
	else
	{
		const float Out = FMath::Clamp((AnimTime - HoldEnd) / SlideOutTime, 0.0f, 1.0f);
		OffsetT = Out * Out; // ease-in
	}

	Panel->SetRenderTranslation(FVector2D(OffsetX * OffsetT, 0.0f));

	if (AnimTime >= Total)
	{
		bRemoved = true;
		RemoveFromParent();
	}
}
