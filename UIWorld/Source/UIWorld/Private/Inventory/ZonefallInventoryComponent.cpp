#include "Inventory/ZonefallInventoryComponent.h"

#include "Inventory/ZonefallWorldItem.h"
#include "GameFramework/Actor.h"
#include "Net/UnrealNetwork.h"

DEFINE_LOG_CATEGORY_STATIC(LogZonefallInventory, Log, All);

UZonefallInventoryComponent::UZonefallInventoryComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
}

void UZonefallInventoryComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	// Only the owning client needs to see its own inventory.
	DOREPLIFETIME_CONDITION(UZonefallInventoryComponent, Items, COND_OwnerOnly);
}

bool UZonefallInventoryComponent::OwnerHasAuthority() const
{
	const AActor* Owner = GetOwner();
	return Owner && Owner->HasAuthority();
}

void UZonefallInventoryComponent::NotifyChanged()
{
	OnInventoryChanged.Broadcast();
}

void UZonefallInventoryComponent::OnRep_Items()
{
	NotifyChanged();
}

void UZonefallInventoryComponent::RestoreItems(const TArray<FZonefallInventoryItem>& InItems)
{
	if (!OwnerHasAuthority())
	{
		return;
	}

	// Respect capacity so a tampered/old save can't overflow the satchel.
	Items = InItems;
	if (Items.Num() > Capacity)
	{
		Items.SetNum(Capacity);
	}
	NotifyChanged();
}

bool UZonefallInventoryComponent::AddItem(const FZonefallInventoryItem& Item)
{
	if (!OwnerHasAuthority() || !Item.IsValid())
	{
		return false;
	}

	int32 Remaining = Item.Quantity;

	// 1) Fill existing stacks.
	for (FZonefallInventoryItem& Slot : Items)
	{
		if (Remaining <= 0)
		{
			break;
		}
		if (Slot.CanStackWith(Item) && Slot.Quantity < Slot.MaxStack)
		{
			const int32 Space = Slot.MaxStack - Slot.Quantity;
			const int32 ToAdd = FMath::Min(Space, Remaining);
			Slot.Quantity += ToAdd;
			Remaining -= ToAdd;
		}
	}

	// 2) Create new stacks while there is capacity.
	while (Remaining > 0 && Items.Num() < Capacity)
	{
		FZonefallInventoryItem NewSlot = Item;
		NewSlot.Quantity = FMath::Min(Remaining, FMath::Max(1, Item.MaxStack));
		Items.Add(NewSlot);
		Remaining -= NewSlot.Quantity;
	}

	const bool bStoredAnything = Remaining < Item.Quantity;
	if (bStoredAnything)
	{
		NotifyChanged(); // server/host immediate; clients via OnRep.
	}
	return bStoredAnything;
}

int32 UZonefallInventoryComponent::RemoveItemAt(int32 Index, int32 Count)
{
	if (!OwnerHasAuthority() || !Items.IsValidIndex(Index) || Count <= 0)
	{
		return 0;
	}

	FZonefallInventoryItem& Slot = Items[Index];
	const int32 Removed = FMath::Min(Count, Slot.Quantity);
	Slot.Quantity -= Removed;
	if (Slot.Quantity <= 0)
	{
		Items.RemoveAt(Index);
	}

	if (Removed > 0)
	{
		NotifyChanged();
	}
	return Removed;
}

bool UZonefallInventoryComponent::UseItemAt(int32 Index)
{
	if (!OwnerHasAuthority() || !Items.IsValidIndex(Index))
	{
		return false;
	}

	// Consumables are spent on use; non-consumables are "equipped/inspected" (no-op here,
	// but still fires a change so UIs can react / Blueprints can hook it).
	if (Items[Index].bConsumable)
	{
		return RemoveItemAt(Index, 1) > 0;
	}

	NotifyChanged();
	return true;
}

void UZonefallInventoryComponent::DropItemAt(int32 Index, int32 Count)
{
	if (!OwnerHasAuthority() || !Items.IsValidIndex(Index) || Count <= 0)
	{
		return;
	}

	AActor* Owner = GetOwner();
	UWorld* World = Owner ? Owner->GetWorld() : nullptr;
	if (!World)
	{
		return;
	}

	// Snapshot the item before we mutate the list.
	FZonefallInventoryItem Dropped = Items[Index];
	const int32 RemovedCount = RemoveItemAt(Index, Count);
	if (RemovedCount <= 0)
	{
		return;
	}
	Dropped.Quantity = RemovedCount;

	// Resolve the world-actor class for this item (item-specific, then component default).
	UClass* PickupClass = Dropped.PickupClass.IsNull() ? nullptr : Dropped.PickupClass.LoadSynchronous();
	if (!PickupClass)
	{
		PickupClass = DefaultPickupClass.IsNull() ? AZonefallWorldItem::StaticClass() : DefaultPickupClass.LoadSynchronous();
	}
	if (!PickupClass)
	{
		PickupClass = AZonefallWorldItem::StaticClass();
	}

	// Spawn slightly in front of and above the owner so it doesn't intersect the floor/owner.
	const FVector Forward = Owner->GetActorForwardVector();
	const FVector SpawnLocation = Owner->GetActorLocation() + Forward * 90.0f + FVector(0.0f, 0.0f, 30.0f);
	const FRotator SpawnRotation = Owner->GetActorRotation();

	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
	SpawnParams.Owner = Owner;

	if (AZonefallWorldItem* WorldItem = World->SpawnActor<AZonefallWorldItem>(PickupClass, SpawnLocation, SpawnRotation, SpawnParams))
	{
		WorldItem->InitializeFromItem(Dropped);

		// Give it a little toss so dropping feels physical.
		if (UPrimitiveComponent* Prim = Cast<UPrimitiveComponent>(WorldItem->GetRootComponent()))
		{
			if (Prim->IsSimulatingPhysics())
			{
				Prim->AddImpulse((Forward + FVector(0, 0, 0.6f)) * 25000.0f);
			}
		}
	}
}
