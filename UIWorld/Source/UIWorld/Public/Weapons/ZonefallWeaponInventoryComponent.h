#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Weapons/ZonefallWeaponTypes.h"
#include "ZonefallWeaponInventoryComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FZonefallWeaponsChanged);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FZonefallEquippedWeaponChanged, int32, EquippedIndex);

/**
 * Server-authoritative, replicated store of the weapons a character owns, plus which one is
 * currently equipped. Replicates to everyone (so remote players see the weapon in hand).
 * The character listens to OnEquippedChanged to attach the right mesh.
 */
UCLASS(ClassGroup = (Zonefall), BlueprintType, meta = (BlueprintSpawnableComponent))
class UIWORLD_API UZonefallWeaponInventoryComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UZonefallWeaponInventoryComponent();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void BeginPlay() override;

	UPROPERTY(BlueprintAssignable, Category = "Zonefall|Weapon")
	FZonefallWeaponsChanged OnWeaponsChanged;

	UPROPERTY(BlueprintAssignable, Category = "Zonefall|Weapon")
	FZonefallEquippedWeaponChanged OnEquippedChanged;

	// If true, seeds a default loadout on the server when empty (so the wheel has content).
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zonefall|Weapon")
	bool bSeedDefaultWeapons = true;

	/**
	 * Designer-editable starting loadout. Assign REAL weapon meshes (WeaponMesh) + per-weapon
	 * hand offsets here in a Blueprint subclass or the details panel — no code change needed.
	 * If left empty, a placeholder loadout (engine basic shapes) is seeded instead.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zonefall|Weapon")
	TArray<FZonefallWeaponItem> DefaultLoadout;

	/** Adds a weapon (server). Returns the index, or INDEX_NONE on failure. */
	UFUNCTION(BlueprintCallable, Category = "Zonefall|Weapon")
	int32 AddWeapon(const FZonefallWeaponItem& Weapon);

	/** Equips a weapon by index (server). -1 = holster / unarmed. */
	UFUNCTION(BlueprintCallable, Category = "Zonefall|Weapon")
	void EquipIndex(int32 Index);

	UFUNCTION(BlueprintCallable, Category = "Zonefall|Weapon")
	void EquipNext();

	UFUNCTION(BlueprintCallable, Category = "Zonefall|Weapon")
	void EquipPrevious();

	UFUNCTION(BlueprintPure, Category = "Zonefall|Weapon")
	const TArray<FZonefallWeaponItem>& GetWeapons() const { return Weapons; }

	UFUNCTION(BlueprintPure, Category = "Zonefall|Weapon")
	int32 GetEquippedIndex() const { return EquippedIndex; }

	UFUNCTION(BlueprintPure, Category = "Zonefall|Weapon")
	bool HasEquippedWeapon() const { return Weapons.IsValidIndex(EquippedIndex); }

	/** Returns the equipped weapon (or an invalid default if unarmed). */
	UFUNCTION(BlueprintPure, Category = "Zonefall|Weapon")
	FZonefallWeaponItem GetEquippedWeapon() const;

	/** True if the equipped weapon can fire (clip, reserve reload, or melee). */
	UFUNCTION(BlueprintPure, Category = "Zonefall|Weapon")
	bool HasAmmoForShot() const;

	/** Spends one round from the equipped weapon's clip (server). Returns true if a shot was available. */
	UFUNCTION(BlueprintCallable, Category = "Zonefall|Weapon")
	bool ConsumeAmmoForShot();

	/** Refills the equipped weapon's clip from its reserve (server). Returns true if anything loaded. */
	UFUNCTION(BlueprintCallable, Category = "Zonefall|Weapon")
	bool Reload();

	/**
	 * Replaces the whole loadout from a saved snapshot (server/standalone only).
	 * Used by the save system to restore the player's weapons + equipped slot on Continue.
	 */
	UFUNCTION(BlueprintCallable, Category = "Zonefall|Weapon")
	void RestoreWeapons(const TArray<FZonefallWeaponItem>& InWeapons, int32 InEquippedIndex);

protected:
	UFUNCTION()
	void OnRep_Weapons();

	UFUNCTION()
	void OnRep_Equipped();

	bool OwnerHasAuthority() const;

	UPROPERTY(ReplicatedUsing = OnRep_Weapons)
	TArray<FZonefallWeaponItem> Weapons;

	UPROPERTY(ReplicatedUsing = OnRep_Equipped)
	int32 EquippedIndex = INDEX_NONE;
};
