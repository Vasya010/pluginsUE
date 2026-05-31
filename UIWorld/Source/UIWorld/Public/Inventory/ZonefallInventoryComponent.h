#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ZonefallInventoryComponent.generated.h"

class AZonefallWorldItem;
class UTexture2D;

/** RDR2-satchel-style categories used by the inventory tabs. */
UENUM(BlueprintType)
enum class EZonefallItemCategory : uint8
{
	Provisions UMETA(DisplayName = "Provisions"),   // food / consumables
	Materials  UMETA(DisplayName = "Materials"),    // crafting components
	Valuables  UMETA(DisplayName = "Valuables"),
	Documents  UMETA(DisplayName = "Documents"),
	Kit        UMETA(DisplayName = "Kit"),
	Other      UMETA(DisplayName = "Other")
};

/** A single inventory entry. Designed to replicate cleanly to the owning client. */
USTRUCT(BlueprintType)
struct FZonefallInventoryItem
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zonefall|Inventory")
	FName ItemId = NAME_None;

	// Which satchel tab this item lives under.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zonefall|Inventory")
	EZonefallItemCategory Category = EZonefallItemCategory::Other;

	// Longer flavour/usage text shown in the detail panel.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zonefall|Inventory")
	FText Description;

	// If true, USE consumes one of this item (food etc.).
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zonefall|Inventory")
	bool bConsumable = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zonefall|Inventory")
	FText DisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zonefall|Inventory", meta = (ClampMin = "1"))
	int32 Quantity = 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zonefall|Inventory", meta = (ClampMin = "1"))
	int32 MaxStack = 99;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zonefall|Inventory")
	TSoftObjectPtr<UTexture2D> Icon;

	// The world actor class that represents this item when dropped / picked up.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zonefall|Inventory")
	TSoftClassPtr<AZonefallWorldItem> PickupClass;

	bool IsValid() const { return ItemId != NAME_None && Quantity > 0; }

	bool CanStackWith(const FZonefallInventoryItem& Other) const
	{
		return ItemId != NAME_None && ItemId == Other.ItemId;
	}
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FZonefallInventoryChanged);

/**
 * Server-authoritative, replicated inventory. The item list replicates to the
 * owning client (COND_OwnerOnly) and fires OnInventoryChanged on both server and
 * client so a self-assembling widget can rebuild itself automatically.
 */
UCLASS(ClassGroup = (Zonefall), BlueprintType, meta = (BlueprintSpawnableComponent))
class UIWORLD_API UZonefallInventoryComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UZonefallInventoryComponent();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UPROPERTY(BlueprintAssignable, Category = "Zonefall|Inventory")
	FZonefallInventoryChanged OnInventoryChanged;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Zonefall|Inventory", meta = (ClampMin = "1", ClampMax = "200"))
	int32 Capacity = 24;

	/** Adds an item (server only). Stacks into existing slots first. Returns true if anything was stored. */
	UFUNCTION(BlueprintCallable, Category = "Zonefall|Inventory")
	bool AddItem(const FZonefallInventoryItem& Item);

	/** Removes up to Count from a slot (server only). Returns the amount actually removed. */
	UFUNCTION(BlueprintCallable, Category = "Zonefall|Inventory")
	int32 RemoveItemAt(int32 Index, int32 Count = 1);

	/** Uses a slot (server only). Consumable items lose one; returns true if something happened. */
	UFUNCTION(BlueprintCallable, Category = "Zonefall|Inventory")
	bool UseItemAt(int32 Index);

	/** Drops Count from a slot into the world as a pickup actor (server only). */
	UFUNCTION(BlueprintCallable, Category = "Zonefall|Inventory")
	void DropItemAt(int32 Index, int32 Count = 1);

	UFUNCTION(BlueprintPure, Category = "Zonefall|Inventory")
	const TArray<FZonefallInventoryItem>& GetItems() const { return Items; }

	/** Replaces the whole inventory from a saved snapshot (server/standalone only). */
	UFUNCTION(BlueprintCallable, Category = "Zonefall|Inventory")
	void RestoreItems(const TArray<FZonefallInventoryItem>& InItems);

	UFUNCTION(BlueprintPure, Category = "Zonefall|Inventory")
	int32 GetNumItems() const { return Items.Num(); }

	UFUNCTION(BlueprintPure, Category = "Zonefall|Inventory")
	bool IsFull() const { return Items.Num() >= Capacity; }

	// The actor class used when an item has no explicit PickupClass set.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zonefall|Inventory")
	TSoftClassPtr<AZonefallWorldItem> DefaultPickupClass;

protected:
	UFUNCTION()
	void OnRep_Items();

	UPROPERTY(ReplicatedUsing = OnRep_Items)
	TArray<FZonefallInventoryItem> Items;

	bool OwnerHasAuthority() const;
	void NotifyChanged();
};
