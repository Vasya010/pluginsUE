#pragma once

#include "CoreMinimal.h"
#include "ZonefallWeaponTypes.generated.h"

class UStaticMesh;

/** Broad weapon class — used to colour-code / group the weapon wheel. */
UENUM(BlueprintType)
enum class EZonefallWeaponSlot : uint8
{
	Unarmed  UMETA(DisplayName = "Unarmed"),
	Sidearm  UMETA(DisplayName = "Sidearm"),
	Longarm  UMETA(DisplayName = "Long Arm"),
	Thrown   UMETA(DisplayName = "Thrown"),
	Melee    UMETA(DisplayName = "Melee")
};

/**
 * A weapon the character can own and hold. Stored in UZonefallWeaponInventoryComponent.
 * The mesh is held in the character's hand via AttachSocket + the relative transform.
 */
USTRUCT(BlueprintType)
struct FZonefallWeaponItem
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zonefall|Weapon")
	FName WeaponId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zonefall|Weapon")
	FText DisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zonefall|Weapon")
	EZonefallWeaponSlot Slot = EZonefallWeaponSlot::Sidearm;

	// Mesh held in hand. Defaults to an engine basic shape if left null so the wheel still works.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zonefall|Weapon")
	TSoftObjectPtr<UStaticMesh> WeaponMesh;

	// Hand bone/socket on the character mesh to attach to.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zonefall|Weapon")
	FName AttachSocket = TEXT("hand_r");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zonefall|Weapon")
	FVector RelativeLocation = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zonefall|Weapon")
	FRotator RelativeRotation = FRotator::ZeroRotator;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zonefall|Weapon")
	FVector RelativeScale = FVector(1.0f, 1.0f, 1.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zonefall|Weapon")
	int32 AmmoInClip = 6;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zonefall|Weapon")
	int32 AmmoReserve = 60;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zonefall|Weapon")
	int32 ClipSize = 6;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zonefall|Weapon")
	float Damage = 28.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zonefall|Weapon")
	float Range = 12000.0f;

	bool IsValid() const { return WeaponId != NAME_None; }
};
