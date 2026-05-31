#include "UI/ZonefallInventoryWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Brushes/SlateRoundedBoxBrush.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/ButtonSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/ScrollBox.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/UniformGridPanel.h"
#include "Components/UniformGridSlot.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Components/WrapBox.h"
#include "Components/WrapBoxSlot.h"
#include "Engine/Font.h"

#include "Character/ZonefallPlayerCharacter.h"
#include "Inventory/ZonefallInventoryComponent.h"

namespace
{
	FSlateFontInfo MakeInvFont(int32 Size)
	{
		FSlateFontInfo Font;
		Font.Size = FMath::Clamp(Size, 8, 96);
		Font.FontObject = LoadObject<UObject>(nullptr, TEXT("/Engine/EngineFonts/Roboto.Roboto"));
		return Font;
	}

	UTextBlock* MakeInvText(UWidgetTree* Tree, FName Name, const FText& InText, int32 Size, FLinearColor Color)
	{
		UTextBlock* T = Tree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), Name);
		T->SetText(InText);
		T->SetFont(MakeInvFont(Size));
		T->SetColorAndOpacity(FSlateColor(Color));
		return T;
	}

	void StyleInvButton(UButton* B, FLinearColor Base, FLinearColor Hover, FLinearColor Pressed)
	{
		if (!B)
		{
			return;
		}
		FButtonStyle Style = B->GetStyle();
		Style.Normal = FSlateRoundedBoxBrush(Base, 5.0f);
		Style.Hovered = FSlateRoundedBoxBrush(Hover, 5.0f);
		Style.Pressed = FSlateRoundedBoxBrush(Pressed, 5.0f);
		Style.Disabled = FSlateRoundedBoxBrush(FLinearColor(0.12f, 0.12f, 0.12f, 0.6f), 5.0f);
		B->SetStyle(Style);
	}

	// Tab/category labels in EZonefallItemCategory order.
	const TCHAR* CategoryLabel(int32 Cat)
	{
		switch (Cat)
		{
		case (int32)EZonefallItemCategory::Provisions: return TEXT("PROVISIONS");
		case (int32)EZonefallItemCategory::Materials:  return TEXT("MATERIALS");
		case (int32)EZonefallItemCategory::Valuables:  return TEXT("VALUABLES");
		case (int32)EZonefallItemCategory::Documents:  return TEXT("DOCUMENTS");
		case (int32)EZonefallItemCategory::Kit:        return TEXT("KIT");
		case (int32)EZonefallItemCategory::Other:      return TEXT("OTHER");
		default:                                       return TEXT("ALL");
		}
	}
}

UZonefallInventorySlotButton::UZonefallInventorySlotButton()
{
	OnClicked.AddDynamic(this, &UZonefallInventorySlotButton::HandleInternalClicked);
}

void UZonefallInventorySlotButton::HandleInternalClicked()
{
	OnSlotClicked.Broadcast(SlotIndex);
}

UZonefallInventoryWidget::UZonefallInventoryWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

TSharedRef<SWidget> UZonefallInventoryWidget::RebuildWidget()
{
	if (!WidgetTree || !WidgetTree->RootWidget)
	{
		BuildLayout();
	}
	return Super::RebuildWidget();
}

void UZonefallInventoryWidget::NativeConstruct()
{
	Super::NativeConstruct();
	SetIsFocusable(true);
	RebuildCategoryTabs();
	RebuildSlots();
}

void UZonefallInventoryWidget::NativeDestruct()
{
	if (BoundInventory)
	{
		BoundInventory->OnInventoryChanged.RemoveDynamic(this, &UZonefallInventoryWidget::HandleInventoryChanged);
	}
	Super::NativeDestruct();
}

FReply UZonefallInventoryWidget::NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
	const FKey Key = InKeyEvent.GetKey();
	if (Key == EKeys::Escape || Key == EKeys::BackSpace || Key == EKeys::Tab || Key == EKeys::Gamepad_FaceButton_Right)
	{
		HandleBackClicked();
		return FReply::Handled();
	}
	return Super::NativeOnKeyDown(InGeometry, InKeyEvent);
}

void UZonefallInventoryWidget::SetInventory(UZonefallInventoryComponent* InInventory)
{
	if (BoundInventory == InInventory)
	{
		return;
	}

	if (BoundInventory)
	{
		BoundInventory->OnInventoryChanged.RemoveDynamic(this, &UZonefallInventoryWidget::HandleInventoryChanged);
	}

	BoundInventory = InInventory;

	if (BoundInventory)
	{
		BoundInventory->OnInventoryChanged.AddDynamic(this, &UZonefallInventoryWidget::HandleInventoryChanged);
	}

	RebuildSlots();
}

void UZonefallInventoryWidget::BuildLayout()
{
	if (!WidgetTree)
	{
		return;
	}

	WidgetTree->RootWidget = nullptr;

	// Full-screen overlay so the satchel panel can sit centred over a dim backdrop.
	UOverlay* Root = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass(), TEXT("InvRoot"));
	WidgetTree->RootWidget = Root;

	UBorder* Dim = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("InvDim"));
	Dim->SetBrush(FSlateRoundedBoxBrush(FLinearColor(0.0f, 0.0f, 0.0f, 0.55f), 0.0f));
	if (UOverlaySlot* DimSlot = Root->AddChildToOverlay(Dim))
	{
		DimSlot->SetHorizontalAlignment(HAlign_Fill);
		DimSlot->SetVerticalAlignment(VAlign_Fill);
	}

	USizeBox* PanelSize = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("InvPanelSize"));
	PanelSize->SetWidthOverride(560.0f);
	if (UOverlaySlot* PanelSlot = Root->AddChildToOverlay(PanelSize))
	{
		PanelSlot->SetHorizontalAlignment(HAlign_Center);
		PanelSlot->SetVerticalAlignment(VAlign_Center);
	}

	RootBorder = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("InventoryRoot"));
	RootBorder->SetBrush(FSlateRoundedBoxBrush(BackgroundTint, 10.0f, FLinearColor(1, 1, 1, 0.10f), 1.0f));
	RootBorder->SetPadding(FMargin(26.0f, 20.0f));
	PanelSize->AddChild(RootBorder);

	RootBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("InventoryRootBox"));
	RootBorder->SetContent(RootBox);

	// --- Header: title + capacity count ---
	UHorizontalBox* HeaderRow = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("InventoryHeader"));

	TitleText = MakeInvText(WidgetTree, TEXT("InventoryTitle"), NSLOCTEXT("ZonefallInventory", "Title", "SATCHEL"), TitleFontSize, FLinearColor::White);
	if (UHorizontalBoxSlot* TitleSlot = HeaderRow->AddChildToHorizontalBox(TitleText))
	{
		TitleSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
		TitleSlot->SetVerticalAlignment(VAlign_Center);
	}

	CountText = MakeInvText(WidgetTree, TEXT("InventoryCount"), FText::GetEmpty(), BodyFontSize, AccentColor);
	if (UHorizontalBoxSlot* CountSlot = HeaderRow->AddChildToHorizontalBox(CountText))
	{
		CountSlot->SetVerticalAlignment(VAlign_Center);
	}

	if (UVerticalBoxSlot* HeaderSlot = RootBox->AddChildToVerticalBox(HeaderRow))
	{
		HeaderSlot->SetPadding(FMargin(0, 0, 0, 6));
	}

	// Divider.
	UBorder* Rule = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("InvRule"));
	Rule->SetBrush(FSlateRoundedBoxBrush(FLinearColor(1, 1, 1, 0.12f), 0.0f));
	if (USizeBox* RuleSize = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("InvRuleSize")))
	{
		RuleSize->SetHeightOverride(1.0f);
		RuleSize->AddChild(Rule);
		if (UVerticalBoxSlot* RuleSlot = RootBox->AddChildToVerticalBox(RuleSize))
		{
			RuleSlot->SetPadding(FMargin(0, 0, 0, 10));
		}
	}

	// --- Category tabs row (wrap box so the tabs never overflow the panel width) ---
	CategoryBar = WidgetTree->ConstructWidget<UWrapBox>(UWrapBox::StaticClass(), TEXT("InventoryCategoryBar"));
	CategoryBar->SetInnerSlotPadding(FVector2D(4.0f, 4.0f));
	if (UVerticalBoxSlot* CatSlot = RootBox->AddChildToVerticalBox(CategoryBar))
	{
		CatSlot->SetPadding(FMargin(0, 0, 0, 12));
		CatSlot->SetHorizontalAlignment(HAlign_Center);
	}

	// --- Item grid (inside a height-capped scroll box so the panel never overflows the screen) ---
	UScrollBox* GridScroll = WidgetTree->ConstructWidget<UScrollBox>(UScrollBox::StaticClass(), TEXT("InventoryGridScroll"));

	Grid = WidgetTree->ConstructWidget<UUniformGridPanel>(UUniformGridPanel::StaticClass(), TEXT("InventoryGrid"));
	Grid->SetSlotPadding(FMargin(6.0f));
	GridScroll->AddChild(Grid);

	USizeBox* GridArea = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("InventoryGridArea"));
	GridArea->SetHeightOverride(404.0f); // ~4 rows; extra rows scroll inside this area
	GridArea->AddChild(GridScroll);
	if (UVerticalBoxSlot* GridSlot = RootBox->AddChildToVerticalBox(GridArea))
	{
		GridSlot->SetPadding(FMargin(0, 0, 0, 12));
	}

	// --- Detail panel ---
	UBorder* DetailPanel = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("InvDetailPanel"));
	DetailPanel->SetBrush(FSlateRoundedBoxBrush(FLinearColor(1, 1, 1, 0.03f), 8.0f));
	DetailPanel->SetPadding(FMargin(16.0f, 12.0f));
	UVerticalBox* DetailCol = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("InvDetailCol"));
	DetailPanel->SetContent(DetailCol);

	DetailNameText = MakeInvText(WidgetTree, TEXT("InvDetailName"), FText::GetEmpty(), BodyFontSize + 6, FLinearColor::White);
	DetailCol->AddChildToVerticalBox(DetailNameText);

	DetailDescText = MakeInvText(WidgetTree, TEXT("InvDetailDesc"), FText::GetEmpty(), BodyFontSize, FLinearColor(0.78f, 0.78f, 0.78f, 1.0f));
	DetailDescText->SetAutoWrapText(true);
	if (UVerticalBoxSlot* DescSlot = DetailCol->AddChildToVerticalBox(DetailDescText))
	{
		DescSlot->SetPadding(FMargin(0, 4, 0, 4));
	}

	DetailCountText = MakeInvText(WidgetTree, TEXT("InvDetailCount"), FText::GetEmpty(), BodyFontSize, AccentColor);
	DetailCol->AddChildToVerticalBox(DetailCountText);

	if (UVerticalBoxSlot* DetailSlot = RootBox->AddChildToVerticalBox(DetailPanel))
	{
		DetailSlot->SetPadding(FMargin(0, 0, 0, 12));
	}

	// --- Action row: DROP / USE / BACK ---
	UHorizontalBox* ActionRow = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("InvActionRow"));

	auto MakeActionButton = [&](FName Name, const FText& Label, FLinearColor Base) -> UButton*
	{
		UButton* B = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), Name);
		StyleInvButton(B, Base, Base * 1.4f + FLinearColor(0.05f, 0.05f, 0.05f, 0), Base * 0.7f);
		UTextBlock* L = MakeInvText(WidgetTree, FName(*(Name.ToString() + TEXT("_Lbl"))), Label, BodyFontSize, FLinearColor::White);
		B->AddChild(L);
		if (UButtonSlot* LS = Cast<UButtonSlot>(L->Slot))
		{
			LS->SetHorizontalAlignment(HAlign_Center);
			LS->SetVerticalAlignment(VAlign_Center);
			LS->SetPadding(FMargin(18.0f, 8.0f));
		}
		return B;
	};

	DropButton = MakeActionButton(TEXT("InvDrop"), NSLOCTEXT("ZonefallInventory", "Drop", "DROP"), FLinearColor(0.32f, 0.10f, 0.10f, 0.95f));
	DropButton->OnClicked.AddDynamic(this, &UZonefallInventoryWidget::HandleDropClicked);
	if (UHorizontalBoxSlot* DS = ActionRow->AddChildToHorizontalBox(DropButton))
	{
		DS->SetPadding(FMargin(0, 0, 8, 0));
	}

	UseButton = MakeActionButton(TEXT("InvUse"), NSLOCTEXT("ZonefallInventory", "Use", "USE"), FLinearColor(0.12f, 0.20f, 0.14f, 0.95f));
	UseButton->OnClicked.AddDynamic(this, &UZonefallInventoryWidget::HandleUseClicked);
	if (UHorizontalBoxSlot* US = ActionRow->AddChildToHorizontalBox(UseButton))
	{
		US->SetPadding(FMargin(0, 0, 8, 0));
	}

	BackButton = MakeActionButton(TEXT("InvBack"), NSLOCTEXT("ZonefallInventory", "Back", "BACK"), FLinearColor(0.14f, 0.14f, 0.16f, 0.95f));
	BackButton->OnClicked.AddDynamic(this, &UZonefallInventoryWidget::HandleBackClicked);
	if (UHorizontalBoxSlot* BS = ActionRow->AddChildToHorizontalBox(BackButton))
	{
		BS->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
		BS->SetHorizontalAlignment(HAlign_Right);
	}

	RootBox->AddChildToVerticalBox(ActionRow);
}

void UZonefallInventoryWidget::RebuildCategoryTabs()
{
	if (!CategoryBar || !WidgetTree)
	{
		return;
	}

	CategoryBar->ClearChildren();
	CategoryButtons.Reset();

	// All + each category.
	TArray<int32> Cats = { -1,
		(int32)EZonefallItemCategory::Provisions,
		(int32)EZonefallItemCategory::Materials,
		(int32)EZonefallItemCategory::Valuables,
		(int32)EZonefallItemCategory::Documents,
		(int32)EZonefallItemCategory::Kit,
		(int32)EZonefallItemCategory::Other };

	for (int32 Cat : Cats)
	{
		UZonefallInventorySlotButton* Tab = WidgetTree->ConstructWidget<UZonefallInventorySlotButton>(
			UZonefallInventorySlotButton::StaticClass(), FName(*FString::Printf(TEXT("InvCat_%d"), Cat)));
		Tab->SlotIndex = Cat;
		const bool bActive = (Cat == CurrentCategoryFilter);
		StyleInvButton(Tab,
			bActive ? FLinearColor(0.85f, 0.18f, 0.16f, 0.85f) : FLinearColor(1, 1, 1, 0.04f),
			FLinearColor(1, 1, 1, 0.12f),
			SelectionColor * 0.7f);

		UTextBlock* L = MakeInvText(WidgetTree, FName(*FString::Printf(TEXT("InvCatLbl_%d"), Cat)),
			FText::FromString(CategoryLabel(Cat)), BodyFontSize - 5, bActive ? FLinearColor::White : FLinearColor(0.7f, 0.7f, 0.7f, 1.0f));
		Tab->AddChild(L);
		if (UButtonSlot* LS = Cast<UButtonSlot>(L->Slot))
		{
			LS->SetHorizontalAlignment(HAlign_Center);
			LS->SetVerticalAlignment(VAlign_Center);
			LS->SetPadding(FMargin(7.0f, 4.0f));
		}

		Tab->OnSlotClicked.AddDynamic(this, &UZonefallInventoryWidget::HandleCategoryClicked);
		CategoryBar->AddChildToWrapBox(Tab);
		CategoryButtons.Add(Tab);
	}
}

void UZonefallInventoryWidget::RebuildSlots()
{
	if (!Grid || !WidgetTree)
	{
		return;
	}

	Grid->ClearChildren();
	SlotButtons.Reset();
	SlotBorders.Reset();
	VisibleItemIndices.Reset();

	const TArray<FZonefallInventoryItem> Empty;
	const TArray<FZonefallInventoryItem>& Items = BoundInventory ? BoundInventory->GetItems() : Empty;

	if (CountText)
	{
		const int32 Cap = BoundInventory ? BoundInventory->Capacity : 0;
		CountText->SetText(FText::FromString(FString::Printf(TEXT("%d / %d"), Items.Num(), Cap)));
	}

	// Build the filtered list of visible item indices.
	for (int32 i = 0; i < Items.Num(); ++i)
	{
		if (CurrentCategoryFilter < 0 || (int32)Items[i].Category == CurrentCategoryFilter)
		{
			VisibleItemIndices.Add(i);
		}
	}

	// Always render a fixed grid of cells so the panel keeps its shape (RDR2-style empty slots).
	const int32 Columns = FMath::Clamp(GridColumns, 2, 8);
	const int32 MinCells = Columns * 4; // at least 4 rows
	const int32 CellCount = FMath::Max(MinCells, ((VisibleItemIndices.Num() + Columns - 1) / Columns) * Columns);

	for (int32 Cell = 0; Cell < CellCount; ++Cell)
	{
		const int32 Row = Cell / Columns;
		const int32 Col = Cell % Columns;
		const bool bHasItem = VisibleItemIndices.IsValidIndex(Cell);
		const int32 RealIndex = bHasItem ? VisibleItemIndices[Cell] : INDEX_NONE;

		UBorder* SlotBorder = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), FName(*FString::Printf(TEXT("InvSlotBg_%d"), Cell)));
		SlotBorder->SetBrush(FSlateRoundedBoxBrush(SlotTint, 6.0f, FLinearColor(1, 1, 1, 0.08f), 1.0f));
		SlotBorder->SetPadding(FMargin(2.0f));

		USizeBox* CellSize = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), FName(*FString::Printf(TEXT("InvSlotSize_%d"), Cell)));
		CellSize->SetWidthOverride(112.0f);
		CellSize->SetHeightOverride(96.0f);
		CellSize->AddChild(SlotBorder);

		UZonefallInventorySlotButton* SlotBtn = WidgetTree->ConstructWidget<UZonefallInventorySlotButton>(
			UZonefallInventorySlotButton::StaticClass(), FName(*FString::Printf(TEXT("InvSlot_%d"), Cell)));
		SlotBtn->SlotIndex = RealIndex;
		StyleInvButton(SlotBtn, FLinearColor(1, 1, 1, 0.02f), FLinearColor(1, 1, 1, 0.10f), SelectionColor * 0.5f);
		SlotBtn->SetIsEnabled(bHasItem);
		SlotBorder->SetContent(SlotBtn);

		UVerticalBox* CellCol = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), FName(*FString::Printf(TEXT("InvSlotCol_%d"), Cell)));
		SlotBtn->AddChild(CellCol);

		if (bHasItem)
		{
			const FZonefallInventoryItem& It = Items[RealIndex];
			const FText NameText = It.DisplayName.IsEmpty() ? FText::FromName(It.ItemId) : It.DisplayName;

			UTextBlock* NameLabel = MakeInvText(WidgetTree, FName(*FString::Printf(TEXT("InvSlotName_%d"), Cell)),
				NameText, BodyFontSize - 4, FLinearColor::White);
			NameLabel->SetAutoWrapText(true);
			NameLabel->SetJustification(ETextJustify::Center);
			NameLabel->SetClipping(EWidgetClipping::ClipToBounds);
			NameLabel->SetTextOverflowPolicy(ETextOverflowPolicy::Ellipsis);
			CellCol->AddChildToVerticalBox(NameLabel);

			UTextBlock* QtyLabel = MakeInvText(WidgetTree, FName(*FString::Printf(TEXT("InvSlotQty_%d"), Cell)),
				FText::FromString(FString::Printf(TEXT("x%d"), It.Quantity)), BodyFontSize - 1, AccentColor);
			QtyLabel->SetJustification(ETextJustify::Center);
			if (UVerticalBoxSlot* QS = CellCol->AddChildToVerticalBox(QtyLabel))
			{
				QS->SetPadding(FMargin(0, 2, 0, 0));
			}

			SlotBtn->OnSlotClicked.AddDynamic(this, &UZonefallInventoryWidget::HandleSlotClicked);
		}

		if (UUniformGridSlot* GS = Grid->AddChildToUniformGrid(CellSize, Row, Col))
		{
			GS->SetHorizontalAlignment(HAlign_Fill);
			GS->SetVerticalAlignment(VAlign_Fill);
		}

		SlotButtons.Add(SlotBtn);
		SlotBorders.Add(SlotBorder);
	}

	// Keep / refresh the selection.
	if (!Items.IsValidIndex(SelectedItemIndex) || !VisibleItemIndices.Contains(SelectedItemIndex))
	{
		SelectedItemIndex = VisibleItemIndices.Num() > 0 ? VisibleItemIndices[0] : INDEX_NONE;
	}
	SelectSlot(SelectedItemIndex);
}

void UZonefallInventoryWidget::SelectSlot(int32 ItemIndex)
{
	SelectedItemIndex = ItemIndex;

	// Highlight the matching cell border.
	for (int32 Cell = 0; Cell < SlotBorders.Num(); ++Cell)
	{
		if (!SlotBorders[Cell])
		{
			continue;
		}
		const int32 RealIndex = VisibleItemIndices.IsValidIndex(Cell) ? VisibleItemIndices[Cell] : INDEX_NONE;
		const bool bSel = (RealIndex != INDEX_NONE && RealIndex == SelectedItemIndex);
		SlotBorders[Cell]->SetBrush(FSlateRoundedBoxBrush(
			SlotTint, 6.0f,
			bSel ? SelectionColor : FLinearColor(1, 1, 1, 0.08f),
			bSel ? 3.0f : 1.0f));
	}

	UpdateDetailPanel();
}

void UZonefallInventoryWidget::UpdateDetailPanel()
{
	const TArray<FZonefallInventoryItem> Empty;
	const TArray<FZonefallInventoryItem>& Items = BoundInventory ? BoundInventory->GetItems() : Empty;

	const bool bValid = Items.IsValidIndex(SelectedItemIndex);

	if (DetailNameText)
	{
		DetailNameText->SetText(bValid
			? (Items[SelectedItemIndex].DisplayName.IsEmpty() ? FText::FromName(Items[SelectedItemIndex].ItemId) : Items[SelectedItemIndex].DisplayName)
			: NSLOCTEXT("ZonefallInventory", "NoSelection", "—"));
	}
	if (DetailDescText)
	{
		DetailDescText->SetText(bValid ? Items[SelectedItemIndex].Description : FText::GetEmpty());
	}
	if (DetailCountText)
	{
		if (bValid)
		{
			const FZonefallInventoryItem& It = Items[SelectedItemIndex];
			DetailCountText->SetText(FText::FromString(FString::Printf(
				TEXT("%s   •   At hand: %d of %d"),
				CategoryLabel((int32)It.Category),
				It.Quantity, FMath::Max(It.Quantity, It.MaxStack))));
		}
		else
		{
			DetailCountText->SetText(FText::GetEmpty());
		}
	}

	if (DropButton) { DropButton->SetIsEnabled(bValid); }
	if (UseButton)  { UseButton->SetIsEnabled(bValid); }
}

void UZonefallInventoryWidget::HandleInventoryChanged()
{
	RebuildSlots();
}

void UZonefallInventoryWidget::HandleSlotClicked(int32 SlotIndex)
{
	if (SlotIndex != INDEX_NONE)
	{
		SelectSlot(SlotIndex);
	}
}

void UZonefallInventoryWidget::HandleCategoryClicked(int32 CategoryId)
{
	CurrentCategoryFilter = CategoryId;
	RebuildCategoryTabs();
	RebuildSlots();
}

void UZonefallInventoryWidget::HandleDropClicked()
{
	if (!BoundInventory || SelectedItemIndex == INDEX_NONE)
	{
		return;
	}
	if (AZonefallPlayerCharacter* Character = Cast<AZonefallPlayerCharacter>(BoundInventory->GetOwner()))
	{
		Character->RequestDropItem(SelectedItemIndex, 1);
	}
}

void UZonefallInventoryWidget::HandleUseClicked()
{
	if (!BoundInventory || SelectedItemIndex == INDEX_NONE)
	{
		return;
	}
	if (AZonefallPlayerCharacter* Character = Cast<AZonefallPlayerCharacter>(BoundInventory->GetOwner()))
	{
		Character->RequestUseItem(SelectedItemIndex);
	}
}

void UZonefallInventoryWidget::HandleBackClicked()
{
	OnCloseRequested.Broadcast();

	// Fallback: close directly through the owning character if nobody handled the delegate.
	if (BoundInventory)
	{
		if (AZonefallPlayerCharacter* Character = Cast<AZonefallPlayerCharacter>(BoundInventory->GetOwner()))
		{
			Character->CloseInventoryUI();
		}
	}
}
