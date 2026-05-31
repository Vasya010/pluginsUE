#include "UI/ZonefallConfirmDialogWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Brushes/SlateRoundedBoxBrush.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/ButtonSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"

namespace
{
	FSlateFontInfo MakeDlgFont(int32 Size)
	{
		FSlateFontInfo F;
		F.Size = FMath::Clamp(Size, 8, 64);
		F.FontObject = LoadObject<UObject>(nullptr, TEXT("/Engine/EngineFonts/Roboto.Roboto"));
		return F;
	}
}

UZonefallConfirmDialogWidget::UZonefallConfirmDialogWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

TSharedRef<SWidget> UZonefallConfirmDialogWidget::RebuildWidget()
{
	if (!WidgetTree || !WidgetTree->RootWidget)
	{
		BuildLayout();
	}
	return Super::RebuildWidget();
}

void UZonefallConfirmDialogWidget::NativeConstruct()
{
	Super::NativeConstruct();
	SetVisibility(ESlateVisibility::Visible);
}

void UZonefallConfirmDialogWidget::Setup(const FText& Title, const FText& Message, const FText& ConfirmLabel, const FText& CancelLabel)
{
	if (TitleText) { TitleText->SetText(Title); }
	if (MessageText) { MessageText->SetText(Message); }
	if (ConfirmLabelText && !ConfirmLabel.IsEmpty()) { ConfirmLabelText->SetText(ConfirmLabel); }
	if (CancelLabelText && !CancelLabel.IsEmpty()) { CancelLabelText->SetText(CancelLabel); }
}

void UZonefallConfirmDialogWidget::BuildLayout()
{
	if (!WidgetTree)
	{
		return;
	}

	WidgetTree->RootWidget = nullptr;

	// Dim backdrop covers everything.
	UBorder* Backdrop = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("DlgBackdrop"));
	Backdrop->SetBrush(FSlateRoundedBoxBrush(FLinearColor(0.0f, 0.0f, 0.0f, 0.72f), 0.0f));
	Backdrop->SetHorizontalAlignment(HAlign_Fill);
	Backdrop->SetVerticalAlignment(VAlign_Fill);
	WidgetTree->RootWidget = Backdrop;

	UOverlay* Root = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass(), TEXT("DlgOverlay"));
	Backdrop->SetContent(Root);

	// Centered panel (fixed width).
	USizeBox* PanelSize = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("DlgPanelSize"));
	PanelSize->SetWidthOverride(520.0f);
	if (UOverlaySlot* PSlot = Root->AddChildToOverlay(PanelSize))
	{
		PSlot->SetHorizontalAlignment(HAlign_Center);
		PSlot->SetVerticalAlignment(VAlign_Center);
	}

	UBorder* Panel = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("DlgPanel"));
	Panel->SetBrush(FSlateRoundedBoxBrush(PanelTint, 12.0f));
	Panel->SetPadding(FMargin(28.0f, 24.0f));
	PanelSize->AddChild(Panel);

	UVerticalBox* Box = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("DlgBox"));
	Panel->SetContent(Box);

	TitleText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("DlgTitle"));
	TitleText->SetText(NSLOCTEXT("ZonefallDialog", "Title", "CONFIRM"));
	TitleText->SetFont(MakeDlgFont(24));
	TitleText->SetColorAndOpacity(FSlateColor(AccentColor));
	if (UVerticalBoxSlot* TS = Box->AddChildToVerticalBox(TitleText))
	{
		TS->SetPadding(FMargin(0, 0, 0, 12));
	}

	MessageText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("DlgMessage"));
	MessageText->SetText(FText::GetEmpty());
	MessageText->SetFont(MakeDlgFont(16));
	MessageText->SetColorAndOpacity(FSlateColor(FLinearColor(0.85f, 0.91f, 0.97f, 1.0f)));
	MessageText->SetAutoWrapText(true);
	if (UVerticalBoxSlot* MS = Box->AddChildToVerticalBox(MessageText))
	{
		MS->SetPadding(FMargin(0, 0, 0, 22));
	}

	UHorizontalBox* ButtonRow = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("DlgButtonRow"));

	auto MakeBtn = [this](FName Name, const FText& Label, FLinearColor Base, FLinearColor Hover, TObjectPtr<UTextBlock>& OutLabel) -> UButton*
	{
		UButton* B = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), Name);
		FButtonStyle Style = B->GetStyle();
		Style.Normal = FSlateRoundedBoxBrush(Base, 6.0f);
		Style.Hovered = FSlateRoundedBoxBrush(Hover, 6.0f);
		Style.Pressed = FSlateRoundedBoxBrush(Hover, 6.0f);
		B->SetStyle(Style);
		UTextBlock* L = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), FName(*(Name.ToString() + TEXT("_Lbl"))));
		L->SetText(Label);
		L->SetFont(MakeDlgFont(16));
		L->SetColorAndOpacity(FSlateColor(FLinearColor::White));
		B->AddChild(L);
		if (UButtonSlot* Slot = Cast<UButtonSlot>(L->Slot))
		{
			Slot->SetHorizontalAlignment(HAlign_Center);
			Slot->SetVerticalAlignment(VAlign_Center);
			Slot->SetPadding(FMargin(16.0f, 9.0f));
		}
		OutLabel = L;
		return B;
	};

	CancelButton = MakeBtn(TEXT("DlgCancel"), NSLOCTEXT("ZonefallDialog", "Cancel", "CANCEL"),
		FLinearColor(0.12f, 0.15f, 0.20f, 1.0f), FLinearColor(0.18f, 0.22f, 0.28f, 1.0f), CancelLabelText);
	CancelButton->OnClicked.AddDynamic(this, &UZonefallConfirmDialogWidget::HandleCancel);
	if (UHorizontalBoxSlot* CS = ButtonRow->AddChildToHorizontalBox(CancelButton))
	{
		CS->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
		CS->SetPadding(FMargin(0, 0, 8, 0));
	}

	ConfirmButton = MakeBtn(TEXT("DlgConfirm"), NSLOCTEXT("ZonefallDialog", "Confirm", "CONFIRM"),
		AccentColor * 0.5f, AccentColor * 0.7f, ConfirmLabelText);
	ConfirmButton->OnClicked.AddDynamic(this, &UZonefallConfirmDialogWidget::HandleConfirm);
	if (UHorizontalBoxSlot* CFS = ButtonRow->AddChildToHorizontalBox(ConfirmButton))
	{
		CFS->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	}

	Box->AddChildToVerticalBox(ButtonRow);
}

void UZonefallConfirmDialogWidget::HandleConfirm()
{
	OnConfirmed.Broadcast();
	RemoveFromParent();
}

void UZonefallConfirmDialogWidget::HandleCancel()
{
	OnCancelled.Broadcast();
	RemoveFromParent();
}
