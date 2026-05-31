#include "Weapons/ZonefallWeaponInventoryComponent.h"

#include "GameFramework/Actor.h"
#include "Net/UnrealNetwork.h"

UZonefallWeaponInventoryComponent::UZonefallWeaponInventoryComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
}

void UZonefallWeaponInventoryComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	// Replicate to everyone — remote players need to see which weapon is in hand.
	DOREPLIFETIME(UZonefallWeaponInventoryComponent, Weapons);
	DOREPLIFETIME(UZonefallWeaponInventoryComponent, EquippedIndex);
}

bool UZonefallWeaponInventoryComponent::OwnerHasAuthority() const
{
	const AActor* Owner = GetOwner();
	return Owner && Owner->HasAuthority();
}

void UZonefallWeaponInventoryComponent::BeginPlay()
{
	Super::BeginPlay();

	if (OwnerHasAuthority() && bSeedDefaultWeapons && Weapons.Num() == 0)
	{
		// Prefer the designer-assigned loadout (with REAL meshes). Only fall back to the
		// engine-basic-shape placeholders when no loadout was configured.
		if (DefaultLoadout.Num() > 0)
		{
			for (const FZonefallWeaponItem& W : DefaultLoadout)
			{
				if (W.IsValid())
				{
					Weapons.Add(W);
				}
			}
			if (Weapons.Num() > 0)
			{
				EquippedIndex = 0;
				OnWeaponsChanged.Broadcast();
				OnEquippedChanged.Broadcast(EquippedIndex);
				return;
			}
		}

		// Placeholder loadout so the wheel has content out of the box. Meshes default to engine
		// basic shapes; assign real weapon meshes via DefaultLoadout in a Blueprint subclass.
		auto MakeWeapon = [](FName Id, const FText& Name, EZonefallWeaponSlot Slot, int32 Clip, int32 Reserve, float Damage, const TCHAR* MeshPath)
		{
			FZonefallWeaponItem W;
			W.WeaponId = Id;
			W.DisplayName = Name;
			W.Slot = Slot;
			W.ClipSize = Clip;
			W.AmmoInClip = Clip;
			W.AmmoReserve = Reserve;
			W.Damage = Damage;
			W.WeaponMesh = TSoftObjectPtr<UStaticMesh>(FSoftObjectPath(MeshPath));
			W.RelativeScale = FVector(0.18f, 0.18f, 0.55f);
			W.RelativeRotation = FRotator(0.0f, 0.0f, 0.0f);
			return W;
		};

		Weapons.Add(MakeWeapon(TEXT("Revolver"), FText::FromString(TEXT("Cattleman Revolver")), EZonefallWeaponSlot::Sidearm, 6, 60, 35.0f, TEXT("/Engine/BasicShapes/Cylinder.Cylinder")));
		Weapons.Add(MakeWeapon(TEXT("Repeater"), FText::FromString(TEXT("Lever-Action Repeater")), EZonefallWeaponSlot::Longarm, 12, 120, 45.0f, TEXT("/Engine/BasicShapes/Cylinder.Cylinder")));
		Weapons.Add(MakeWeapon(TEXT("Shotgun"), FText::FromString(TEXT("Pump Shotgun")), EZonefallWeaponSlot::Longarm, 5, 40, 70.0f, TEXT("/Engine/BasicShapes/Cylinder.Cylinder")));
		Weapons.Add(MakeWeapon(TEXT("Knife"), FText::FromString(TEXT("Hunting Knife")), EZonefallWeaponSlot::Melee, 0, 0, 30.0f, TEXT("/Engine/BasicShapes/Cube.Cube")));

		// Start equipped with the sidearm.
		EquippedIndex = 0;
		OnWeaponsChanged.Broadcast();
		OnEquippedChanged.Broadcast(EquippedIndex);
	}
}

int32 UZonefallWeaponInventoryComponent::AddWeapon(const FZonefallWeaponItem& Weapon)
{
	if (!OwnerHasAuthority() || !Weapon.IsValid())
	{
		return INDEX_NONE;
	}

	// Stack ammo if we already own this weapon.
	for (int32 i = 0; i < Weapons.Num(); ++i)
	{
		if (Weapons[i].WeaponId == Weapon.WeaponId)
		{
			Weapons[i].AmmoReserve += Weapon.AmmoReserve + Weapon.AmmoInClip;
			OnWeaponsChanged.Broadcast();
			return i;
		}
	}

	const int32 NewIndex = Weapons.Add(Weapon);
	OnWeaponsChanged.Broadcast();
	return NewIndex;
}

void UZonefallWeaponInventoryComponent::EquipIndex(int32 Index)
{
	if (!OwnerHasAuthority())
	{
		return;
	}

	const int32 Clamped = Weapons.IsValidIndex(Index) ? Index : INDEX_NONE;
	if (Clamped == EquippedIndex)
	{
		return;
	}
	EquippedIndex = Clamped;
	OnEquippedChanged.Broadcast(EquippedIndex);
}

void UZonefallWeaponInventoryComponent::EquipNext()
{
	if (Weapons.Num() == 0)
	{
		return;
	}
	const int32 Start = (EquippedIndex == INDEX_NONE) ? -1 : EquippedIndex;
	EquipIndex((Start + 1) % Weapons.Num());
}

void UZonefallWeaponInventoryComponent::EquipPrevious()
{
	if (Weapons.Num() == 0)
	{
		return;
	}
	const int32 Start = (EquippedIndex == INDEX_NONE) ? Weapons.Num() : EquippedIndex;
	EquipIndex((Start - 1 + Weapons.Num()) % Weapons.Num());
}

FZonefallWeaponItem UZonefallWeaponInventoryComponent::GetEquippedWeapon() const
{
	if (Weapons.IsValidIndex(EquippedIndex))
	{
		return Weapons[EquippedIndex];
	}
	return FZonefallWeaponItem();
}

bool UZonefallWeaponInventoryComponent::HasAmmoForShot() const
{
	if (!Weapons.IsValidIndex(EquippedIndex))
	{
		return false;
	}

	const FZonefallWeaponItem& W = Weapons[EquippedIndex];
	if (W.Slot == EZonefallWeaponSlot::Melee)
	{
		return true;
	}

	if (W.AmmoInClip > 0)
	{
		return true;
	}

	return W.AmmoReserve > 0;
}

bool UZonefallWeaponInventoryComponent::ConsumeAmmoForShot()
{
	if (!OwnerHasAuthority() || !Weapons.IsValidIndex(EquippedIndex))
	{
		return false;
	}

	FZonefallWeaponItem& W = Weapons[EquippedIndex];
	if (W.Slot == EZonefallWeaponSlot::Melee)
	{
		return true;
	}

	const int32 ClipCap = FMath::Max(1, W.ClipSize);

	if (W.AmmoInClip <= 0)
	{
		const int32 ToLoad = FMath::Min(ClipCap, W.AmmoReserve);
		if (ToLoad <= 0)
		{
			return false;
		}
		W.AmmoInClip = ToLoad;
		W.AmmoReserve -= ToLoad;
	}

	--W.AmmoInClip;
	OnWeaponsChanged.Broadcast();
	return true;
}

bool UZonefallWeaponInventoryComponent::Reload()
{
	if (!OwnerHasAuthority() || !Weapons.IsValidIndex(EquippedIndex))
	{
		return false;
	}

	FZonefallWeaponItem& W = Weapons[EquippedIndex];
	if (W.Slot == EZonefallWeaponSlot::Melee)
	{
		return false;
	}

	const int32 ClipCap = FMath::Max(1, W.ClipSize);
	const int32 Need = ClipCap - W.AmmoInClip;
	if (Need <= 0 || W.AmmoReserve <= 0)
	{
		return false;
	}

	const int32 ToLoad = FMath::Min(Need, W.AmmoReserve);
	W.AmmoInClip += ToLoad;
	W.AmmoReserve -= ToLoad;
	OnWeaponsChanged.Broadcast();
	return true;
}

void UZonefallWeaponInventoryComponent::RestoreWeapons(const TArray<FZonefallWeaponItem>& InWeapons, int32 InEquippedIndex)
{
	if (!OwnerHasAuthority())
	{
		return;
	}

	Weapons = InWeapons;
	EquippedIndex = Weapons.IsValidIndex(InEquippedIndex) ? InEquippedIndex : (Weapons.Num() > 0 ? 0 : INDEX_NONE);

	// Replicated arrays don't auto-fire OnRep on the authority — broadcast so the local
	// character re-attaches the held mesh and the HUD refreshes immediately.
	OnWeaponsChanged.Broadcast();
	OnEquippedChanged.Broadcast(EquippedIndex);
}

void UZonefallWeaponInventoryComponent::OnRep_Weapons()
{
	OnWeaponsChanged.Broadcast();
}

void UZonefallWeaponInventoryComponent::OnRep_Equipped()
{
	OnEquippedChanged.Broadcast(EquippedIndex);
}
