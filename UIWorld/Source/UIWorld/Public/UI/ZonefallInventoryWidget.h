#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/Button.h"
#include "ZonefallInventoryWidget.generated.h"

class UBorder;
class UButton;
class UTextBlock;
class UVerticalBox;
class UHorizontalBox;
class UWrapBox;
class UUniformGridPanel;
class UZonefallInventoryComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnZonefallInventorySlotClicked, int32, SlotIndex);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnZonefallInventoryClosed);

/** A UButton that remembers an integer id (slot index or category) and re-broadcasts clicks with it. */
UCLASS()
class UIWORLD_API UZonefallInventorySlotButton : public UButton
{
	GENERATED_BODY()

public:
	UZonefallInventorySlotButton();

	UPROPERTY(Transient)
	int32 SlotIndex = INDEX_NONE;

	UPROPERTY(BlueprintAssignable)
	FOnZonefallInventorySlotClicked OnSlotClicked;

private:
	UFUNCTION()
	void HandleInternalClicked();
};

/**
 * Fully self-assembled RDR2-satchel-style inventory UI (no Blueprint required).
 *
 *  - Category tabs across the top (ALL / PROVISIONS / MATERIALS / ...).
 *  - A 4-column grid of item slots with a quantity badge and a selection highlight.
 *  - A detail panel showing the selected item's name, description and "At hand: X of Y".
 *  - DROP / USE / BACK actions that route through the owning character's server RPCs.
 *
 * Rebuilds itself automatically whenever the bound inventory changes.
 */
UCLASS(BlueprintType, Blueprintable)
class UIWORLD_API UZonefallInventoryWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UZonefallInventoryWidget(const FObjectInitializer& ObjectInitializer);

	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	virtual FReply NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;

	/** Binds this widget to an inventory and does an initial build. */
	UFUNCTION(BlueprintCallable, Category = "Zonefall|Inventory")
	void SetInventory(UZonefallInventoryComponent* InInventory);

	/** Fired when the player presses BACK (the character closes the satchel). */
	UPROPERTY(BlueprintAssignable, Category = "Zonefall|Inventory")
	FOnZonefallInventoryClosed OnCloseRequested;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zonefall|Inventory|Style")
	FLinearColor BackgroundTint = FLinearColor(0.02f, 0.02f, 0.02f, 0.97f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zonefall|Inventory|Style")
	FLinearColor SlotTint = FLinearColor(0.09f, 0.09f, 0.10f, 0.96f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zonefall|Inventory|Style")
	FLinearColor SelectionColor = FLinearColor(0.85f, 0.18f, 0.16f, 1.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zonefall|Inventory|Style")
	FLinearColor AccentColor = FLinearColor(0.86f, 0.80f, 0.66f, 1.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zonefall|Inventory|Style")
	int32 TitleFontSize = 28;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zonefall|Inventory|Style")
	int32 BodyFontSize = 15;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zonefall|Inventory|Style", meta = (ClampMin = "2", ClampMax = "8"))
	int32 GridColumns = 4;

private:
	void BuildLayout();
	void RebuildCategoryTabs();
	void RebuildSlots();
	void UpdateDetailPanel();
	void SelectSlot(int32 ItemIndex);

	UFUNCTION() void HandleInventoryChanged();
	UFUNCTION() void HandleSlotClicked(int32 SlotIndex);
	UFUNCTION() void HandleCategoryClicked(int32 CategoryId);
	UFUNCTION() void HandleDropClicked();
	UFUNCTION() void HandleUseClicked();
	UFUNCTION() void HandleBackClicked();

	UPROPERTY(Transient) TObjectPtr<UZonefallInventoryComponent> BoundInventory;
	UPROPERTY(Transient) TObjectPtr<UBorder> RootBorder;
	UPROPERTY(Transient) TObjectPtr<UVerticalBox> RootBox;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> TitleText;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> CountText;
	UPROPERTY(Transient) TObjectPtr<UWrapBox> CategoryBar;
	UPROPERTY(Transient) TObjectPtr<UUniformGridPanel> Grid;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> DetailNameText;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> DetailDescText;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> DetailCountText;
	UPROPERTY(Transient) TObjectPtr<UButton> DropButton;
	UPROPERTY(Transient) TObjectPtr<UButton> UseButton;
	UPROPERTY(Transient) TObjectPtr<UButton> BackButton;

	UPROPERTY(Transient) TArray<TObjectPtr<UZonefallInventorySlotButton>> SlotButtons;
	UPROPERTY(Transient) TArray<TObjectPtr<UBorder>> SlotBorders;
	UPROPERTY(Transient) TArray<TObjectPtr<UZonefallInventorySlotButton>> CategoryButtons;

	// Maps a grid cell -> real item index in the bound inventory (after category filtering).
	UPROPERTY(Transient) TArray<int32> VisibleItemIndices;

	// -1 = All, otherwise an EZonefallItemCategory value.
	UPROPERTY(Transient) int32 CurrentCategoryFilter = -1;

	UPROPERTY(Transient) int32 SelectedItemIndex = INDEX_NONE;
};
