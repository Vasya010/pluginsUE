#include "UI/ZonefallWeaponWheelWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Brushes/SlateRoundedBoxBrush.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/ButtonSlot.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Engine/Font.h"

#include "Character/ZonefallPlayerCharacter.h"
#include "Weapons/ZonefallWeaponInventoryComponent.h"
#include "Weapons/ZonefallWeaponTypes.h"

namespace
{
	FSlateFontInfo MakeWheelFont(int32 Size, bool bBold = false)
	{
		FSlateFontInfo Font;
		Font.Size = FMath::Clamp(Size, 8, 64);
		Font.TypefaceFontName = bBold ? FName(TEXT("Bold")) : FName(TEXT("Regular"));
		Font.FontObject = LoadObject<UObject>(nullptr, TEXT("/Engine/EngineFonts/Roboto.Roboto"));
		return Font;
	}

	// RDR2-ish colour code per weapon class so the wheel reads at a glance.
	FLinearColor SlotColor(EZonefallWeaponSlot Slot)
	{
		switch (Slot)
		{
		case EZonefallWeaponSlot::Sidearm: return FLinearColor(0.95f, 0.78f, 0.35f, 1.0f); // gold
		case EZonefallWeaponSlot::Longarm: return FLinearColor(0.55f, 0.78f, 0.98f, 1.0f); // steel blue
		case EZonefallWeaponSlot::Thrown:  return FLinearColor(0.85f, 0.45f, 0.30f, 1.0f); // ember
		case EZonefallWeaponSlot::Melee:   return FLinearColor(0.78f, 0.80f, 0.84f, 1.0f); // silver
		default:                           return FLinearColor(0.70f, 0.74f, 0.80f, 1.0f);
		}
	}

	FText SlotName(EZonefallWeaponSlot Slot)
	{
		switch (Slot)
		{
		case EZonefallWeaponSlot::Sidearm: return NSLOCTEXT("ZonefallWheel", "Sidearm", "SIDEARM");
		case EZonefallWeaponSlot::Longarm: return NSLOCTEXT("ZonefallWheel", "Longarm", "LONG ARM");
		case EZonefallWeaponSlot::Thrown:  return NSLOCTEXT("ZonefallWheel", "Thrown", "THROWN");
		case EZonefallWeaponSlot::Melee:   return NSLOCTEXT("ZonefallWheel", "Melee", "MELEE");
		default:                           return NSLOCTEXT("ZonefallWheel", "Unarmed", "UNARMED");
		}
	}

	FText AmmoLine(const FZonefallWeaponItem& W)
	{
		if (W.Slot == EZonefallWeaponSlot::Melee)
		{
			return FText::GetEmpty();
		}
		return FText::FromString(FString::Printf(TEXT("%d / %d"), W.AmmoInClip, W.AmmoReserve));
	}
}

UZonefallWeaponWheelButton::UZonefallWeaponWheelButton()
{
	OnClicked.AddDynamic(this, &UZonefallWeaponWheelButton::HandleInternalClicked);
}

void UZonefallWeaponWheelButton::HandleInternalClicked()
{
	OnWeaponSlotClicked.Broadcast(WeaponIndex);
}

UZonefallWeaponWheelWidget::UZonefallWeaponWheelWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

TSharedRef<SWidget> UZonefallWeaponWheelWidget::RebuildWidget()
{
	if (!WidgetTree || !WidgetTree->RootWidget)
	{
		BuildLayout();
	}
	return Super::RebuildWidget();
}

void UZonefallWeaponWheelWidget::SetupForCharacter(AZonefallPlayerCharacter* InCharacter)
{
	Character = InCharacter;
	WeaponInv = InCharacter ? InCharacter->GetWeapons() : nullptr;
	RefreshFromWeapons();
}

void UZonefallWeaponWheelWidget::BuildLayout()
{
	if (!WidgetTree)
	{
		return;
	}

	WidgetTree->RootWidget = nullptr;

	UOverlay* Root = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass(), TEXT("WheelRoot"));
	WidgetTree->RootWidget = Root;

	// Dim backdrop (full screen).
	UBorder* Dim = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("WheelDim"));
	Dim->SetBrush(FSlateRoundedBoxBrush(FLinearColor(0.0f, 0.0f, 0.0f, 0.55f), 0.0f));
	if (UOverlaySlot* DimSlot = Root->AddChildToOverlay(Dim))
	{
		DimSlot->SetHorizontalAlignment(HAlign_Fill);
		DimSlot->SetVerticalAlignment(VAlign_Fill);
	}

	// Bounded, centred square so the wheel is always a perfect circle and never overflows.
	const float BoxSize = FMath::Max(WheelRadius * 2.0f + 150.0f, 420.0f);

	USizeBox* WheelBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("WheelBox"));
	WheelBox->SetWidthOverride(BoxSize);
	WheelBox->SetHeightOverride(BoxSize);
	if (UOverlaySlot* BoxSlot = Root->AddChildToOverlay(WheelBox))
	{
		BoxSlot->SetHorizontalAlignment(HAlign_Center);
		BoxSlot->SetVerticalAlignment(VAlign_Center);
	}

	UOverlay* Inner = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass(), TEXT("WheelInner"));
	WheelBox->AddChild(Inner);

	// Circular ring backdrop (radius = half the box => a true circle).
	UBorder* Ring = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("WheelRing"));
	Ring->SetBrush(FSlateRoundedBoxBrush(FLinearColor(0.04f, 0.05f, 0.06f, 0.82f), BoxSize * 0.5f, AccentColor * FLinearColor(1, 1, 1, 0.6f), 2.5f));
	if (UOverlaySlot* RingSlot = Inner->AddChildToOverlay(Ring))
	{
		RingSlot->SetHorizontalAlignment(HAlign_Fill);
		RingSlot->SetVerticalAlignment(VAlign_Fill);
	}

	// Center hub (a smaller circle that holds the selected-weapon details).
	UBorder* Hub = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("WheelHub"));
	Hub->SetBrush(FSlateRoundedBoxBrush(FLinearColor(0.02f, 0.02f, 0.03f, 0.96f), 82.0f, AccentColor * FLinearColor(1, 1, 1, 0.5f), 2.0f));
	Hub->SetPadding(FMargin(16.0f));
	USizeBox* HubSize = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("WheelHubSize"));
	HubSize->SetWidthOverride(164.0f);
	HubSize->SetHeightOverride(164.0f);
	HubSize->AddChild(Hub);
	if (UOverlaySlot* HubSlot = Inner->AddChildToOverlay(HubSize))
	{
		HubSlot->SetHorizontalAlignment(HAlign_Center);
		HubSlot->SetVerticalAlignment(VAlign_Center);
	}

	UVerticalBox* HubCol = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("WheelHubCol"));
	Hub->SetContent(HubCol);

	CenterSlotLabel = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("WheelCenterSlot"));
	CenterSlotLabel->SetText(NSLOCTEXT("ZonefallWheel", "Weapons", "WEAPONS"));
	CenterSlotLabel->SetFont(MakeWheelFont(10, true));
	CenterSlotLabel->SetColorAndOpacity(FSlateColor(AccentColor));
	CenterSlotLabel->SetJustification(ETextJustify::Center);
	HubCol->AddChildToVerticalBox(CenterSlotLabel);

	CenterLabel = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("WheelCenterLabel"));
	CenterLabel->SetText(NSLOCTEXT("ZonefallWheel", "SelectAWeapon", "Select a weapon"));
	CenterLabel->SetFont(MakeWheelFont(16, true));
	CenterLabel->SetColorAndOpacity(FSlateColor(FLinearColor::White));
	CenterLabel->SetJustification(ETextJustify::Center);
	CenterLabel->SetAutoWrapText(true);
	if (UVerticalBoxSlot* NameSlot = HubCol->AddChildToVerticalBox(CenterLabel))
	{
		NameSlot->SetPadding(FMargin(0.0f, 4.0f, 0.0f, 0.0f));
		NameSlot->SetHorizontalAlignment(HAlign_Center);
	}

	CenterAmmoLabel = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("WheelCenterAmmo"));
	CenterAmmoLabel->SetText(FText::GetEmpty());
	CenterAmmoLabel->SetFont(MakeWheelFont(13, true));
	CenterAmmoLabel->SetColorAndOpacity(FSlateColor(FLinearColor(0.78f, 0.84f, 0.92f, 1.0f)));
	CenterAmmoLabel->SetJustification(ETextJustify::Center);
	if (UVerticalBoxSlot* AmmoSlot = HubCol->AddChildToVerticalBox(CenterAmmoLabel))
	{
		AmmoSlot->SetPadding(FMargin(0.0f, 6.0f, 0.0f, 0.0f));
		AmmoSlot->SetHorizontalAlignment(HAlign_Center);
	}

	// Slots are positioned on this canvas (fills the box; centre = box centre).
	WheelCanvas = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("WheelCanvas"));
	if (UOverlaySlot* CanvasSlot = Inner->AddChildToOverlay(WheelCanvas))
	{
		CanvasSlot->SetHorizontalAlignment(HAlign_Fill);
		CanvasSlot->SetVerticalAlignment(VAlign_Fill);
	}
}

void UZonefallWeaponWheelWidget::RefreshFromWeapons()
{
	if (!WheelCanvas || !WidgetTree)
	{
		return;
	}

	// Clear old slots.
	for (UZonefallWeaponWheelButton* B : SlotButtons)
	{
		if (B) { B->RemoveFromParent(); }
	}
	SlotButtons.Reset();
	SlotBorders.Reset();

	if (!WeaponInv)
	{
		return;
	}

	const TArray<FZonefallWeaponItem>& List = WeaponInv->GetWeapons();
	const int32 Count = List.Num();
	if (Count == 0)
	{
		return;
	}

	SelectedIndex = WeaponInv->GetEquippedIndex();
	if (!List.IsValidIndex(SelectedIndex))
	{
		SelectedIndex = 0;
	}

	for (int32 i = 0; i < Count; ++i)
	{
		const FZonefallWeaponItem& Item = List[i];
		const FLinearColor Color = SlotColor(Item.Slot);

		const float Angle = (2.0f * PI * (float)i / (float)Count) - (PI * 0.5f); // start at top
		const FVector2D Pos(FMath::Cos(Angle) * WheelRadius, FMath::Sin(Angle) * WheelRadius);

		UBorder* SlotBorder = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), FName(*FString::Printf(TEXT("WheelSlotBg_%d"), i)));
		SlotBorder->SetBrush(FSlateRoundedBoxBrush(FLinearColor(0.05f, 0.06f, 0.08f, 0.92f), 14.0f, Color * FLinearColor(1, 1, 1, 0.45f), 1.5f));
		SlotBorder->SetPadding(FMargin(2.0f));

		UZonefallWeaponWheelButton* SlotBtn = WidgetTree->ConstructWidget<UZonefallWeaponWheelButton>(
			UZonefallWeaponWheelButton::StaticClass(), FName(*FString::Printf(TEXT("WheelSlot_%d"), i)));
		SlotBtn->WeaponIndex = i;
		{
			FButtonStyle Style = SlotBtn->GetStyle();
			Style.Normal = FSlateRoundedBoxBrush(FLinearColor(1, 1, 1, 0.03f), 12.0f);
			Style.Hovered = FSlateRoundedBoxBrush(Color * FLinearColor(1, 1, 1, 0.30f), 12.0f);
			Style.Pressed = FSlateRoundedBoxBrush(Color * FLinearColor(1, 1, 1, 0.45f), 12.0f);
			SlotBtn->SetStyle(Style);
		}

		// Chip content: weapon name + ammo line stacked.
		UVerticalBox* Chip = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), FName(*FString::Printf(TEXT("WheelChip_%d"), i)));

		UTextBlock* Label = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), FName(*FString::Printf(TEXT("WheelLabel_%d"), i)));
		const FText Name = Item.DisplayName.IsEmpty() ? FText::FromName(Item.WeaponId) : Item.DisplayName;
		Label->SetText(Name);
		Label->SetFont(MakeWheelFont(13, true));
		Label->SetColorAndOpacity(FSlateColor(FLinearColor::White));
		Label->SetJustification(ETextJustify::Center);
		Chip->AddChildToVerticalBox(Label);

		const FText Ammo = AmmoLine(Item);
		if (!Ammo.IsEmpty())
		{
			UTextBlock* AmmoLabel = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), FName(*FString::Printf(TEXT("WheelAmmo_%d"), i)));
			AmmoLabel->SetText(Ammo);
			AmmoLabel->SetFont(MakeWheelFont(10));
			AmmoLabel->SetColorAndOpacity(FSlateColor(Color));
			AmmoLabel->SetJustification(ETextJustify::Center);
			if (UVerticalBoxSlot* AmmoChipSlot = Chip->AddChildToVerticalBox(AmmoLabel))
			{
				AmmoChipSlot->SetHorizontalAlignment(HAlign_Center);
				AmmoChipSlot->SetPadding(FMargin(0.0f, 2.0f, 0.0f, 0.0f));
			}
		}

		SlotBtn->AddChild(Chip);
		if (UButtonSlot* ChipSlot = Cast<UButtonSlot>(Chip->Slot))
		{
			ChipSlot->SetHorizontalAlignment(HAlign_Center);
			ChipSlot->SetVerticalAlignment(VAlign_Center);
			ChipSlot->SetPadding(FMargin(16.0f, 12.0f));
		}

		SlotBorder->SetContent(SlotBtn);
		SlotBtn->OnWeaponSlotClicked.AddDynamic(this, &UZonefallWeaponWheelWidget::HandleWeaponSlotClicked);

		if (UCanvasPanelSlot* CanvasSlot = WheelCanvas->AddChildToCanvas(SlotBorder))
		{
			CanvasSlot->SetAnchors(FAnchors(0.5f, 0.5f));
			CanvasSlot->SetAlignment(FVector2D(0.5f, 0.5f));
			CanvasSlot->SetAutoSize(true);
			CanvasSlot->SetPosition(Pos);
		}

		SlotButtons.Add(SlotBtn);
		SlotBorders.Add(SlotBorder);
	}

	UpdateSelectionVisuals();
}

void UZonefallWeaponWheelWidget::HandleWeaponSlotClicked(int32 WeaponIndex)
{
	SelectedIndex = WeaponIndex;
	UpdateSelectionVisuals();

	// Equip immediately so the choice feels responsive (also equipped again on close).
	if (Character)
	{
		Character->RequestEquipWeapon(WeaponIndex);
	}
}

void UZonefallWeaponWheelWidget::UpdateSelectionVisuals()
{
	const TArray<FZonefallWeaponItem>* List = WeaponInv ? &WeaponInv->GetWeapons() : nullptr;

	for (int32 i = 0; i < SlotBorders.Num(); ++i)
	{
		if (!SlotBorders[i])
		{
			continue;
		}
		const bool bSel = (i == SelectedIndex);
		const FLinearColor Color = (List && List->IsValidIndex(i)) ? SlotColor((*List)[i].Slot) : AccentColor;
		SlotBorders[i]->SetBrush(FSlateRoundedBoxBrush(
			bSel ? FLinearColor(0.12f, 0.13f, 0.16f, 0.97f) : FLinearColor(0.05f, 0.06f, 0.08f, 0.92f),
			14.0f,
			bSel ? Color : Color * FLinearColor(1, 1, 1, 0.40f),
			bSel ? 3.0f : 1.5f));
	}

	if (List && List->IsValidIndex(SelectedIndex))
	{
		const FZonefallWeaponItem& Sel = (*List)[SelectedIndex];
		const FText Name = Sel.DisplayName.IsEmpty() ? FText::FromName(Sel.WeaponId) : Sel.DisplayName;
		if (CenterLabel) { CenterLabel->SetText(Name); }
		if (CenterSlotLabel)
		{
			CenterSlotLabel->SetText(SlotName(Sel.Slot));
			CenterSlotLabel->SetColorAndOpacity(FSlateColor(SlotColor(Sel.Slot)));
		}
		if (CenterAmmoLabel)
		{
			const FText Ammo = AmmoLine(Sel);
			CenterAmmoLabel->SetText(Ammo.IsEmpty() ? NSLOCTEXT("ZonefallWheel", "MeleeDash", "—") : Ammo);
		}
	}
}
