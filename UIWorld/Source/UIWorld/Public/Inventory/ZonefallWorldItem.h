#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Inventory/ZonefallInventoryComponent.h"
#include "ZonefallWorldItem.generated.h"

class UStaticMeshComponent;
class USphereComponent;
class UZonefallInventoryComponent;

/**
 * A pickup that lives in the world. Replicated so all clients see it; the actual
 * "give to inventory" only happens on the server. Can be picked up via interaction
 * trace (default) or automatically on overlap.
 */
UCLASS(BlueprintType, Blueprintable)
class UIWORLD_API AZonefallWorldItem : public AActor
{
	GENERATED_BODY()

public:
	AZonefallWorldItem();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Zonefall|Item")
	TObjectPtr<USphereComponent> CollisionSphere;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Zonefall|Item")
	TObjectPtr<UStaticMeshComponent> MeshComponent;

	// The item this pickup grants. Replicated so the icon/name can be shown on clients.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, ReplicatedUsing = OnRep_Item, Category = "Zonefall|Item")
	FZonefallInventoryItem Item;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zonefall|Item")
	bool bAutoPickupOnOverlap = false;

	// --- Designer-friendly defaults used when Item is left blank on a placed pickup ---
	// What this pickup is called in the satchel (e.g. "Venison", "Cattleman Revolver Ammo").
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zonefall|Item|Pickup")
	FText PickupDisplayName = NSLOCTEXT("ZonefallItem", "DefaultPickup", "Supplies");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zonefall|Item|Pickup")
	FText PickupDescription = NSLOCTEXT("ZonefallItem", "DefaultPickupDesc", "An item you can carry in your satchel.");

	// Which satchel tab it lands under — so you can tell food from materials from kit.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zonefall|Item|Pickup")
	EZonefallItemCategory PickupCategory = EZonefallItemCategory::Provisions;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zonefall|Item|Pickup")
	bool bPickupConsumable = true;

	// Spins the mesh for a bit of visual life.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zonefall|Item")
	float SpinDegreesPerSecond = 60.0f;

	/** Server-side: copy item data and refresh visuals. */
	UFUNCTION(BlueprintCallable, Category = "Zonefall|Item")
	void InitializeFromItem(const FZonefallInventoryItem& InItem);

	/** Server-side: try to give this pickup to an inventory; destroys self on success. Returns true if consumed. */
	UFUNCTION(BlueprintCallable, Category = "Zonefall|Item")
	bool TryGiveTo(UZonefallInventoryComponent* Inventory);

	UFUNCTION(BlueprintImplementableEvent, Category = "Zonefall|Item")
	void BP_OnPickedUp(AActor* Picker);

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	UFUNCTION()
	void OnRep_Item();

	UFUNCTION()
	void HandleBeginOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	void RefreshVisual();
};
