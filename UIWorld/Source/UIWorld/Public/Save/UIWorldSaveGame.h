#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "Character/ZonefallCharacterAppearance.h"
#include "UIWorldSaveGame.generated.h"

/**
 * Plain serializable snapshot of one owned weapon. We mirror FZonefallWeaponItem into a
 * POD record (soft paths -> strings, FText -> string, enum -> uint8) so the save format is
 * decoupled from the gameplay struct and every field is flagged for SaveGame serialization.
 */
USTRUCT(BlueprintType)
struct FUIWorldSavedWeapon
{
	GENERATED_BODY()

	UPROPERTY(SaveGame, BlueprintReadWrite, Category = "UIWorld|Save") FName WeaponId = NAME_None;
	UPROPERTY(SaveGame, BlueprintReadWrite, Category = "UIWorld|Save") FString DisplayName;
	UPROPERTY(SaveGame, BlueprintReadWrite, Category = "UIWorld|Save") uint8 Slot = 0;
	UPROPERTY(SaveGame, BlueprintReadWrite, Category = "UIWorld|Save") FString WeaponMeshPath;
	UPROPERTY(SaveGame, BlueprintReadWrite, Category = "UIWorld|Save") FName AttachSocket = TEXT("hand_r");
	UPROPERTY(SaveGame, BlueprintReadWrite, Category = "UIWorld|Save") FVector RelativeLocation = FVector::ZeroVector;
	UPROPERTY(SaveGame, BlueprintReadWrite, Category = "UIWorld|Save") FRotator RelativeRotation = FRotator::ZeroRotator;
	UPROPERTY(SaveGame, BlueprintReadWrite, Category = "UIWorld|Save") FVector RelativeScale = FVector::OneVector;
	UPROPERTY(SaveGame, BlueprintReadWrite, Category = "UIWorld|Save") int32 AmmoInClip = 0;
	UPROPERTY(SaveGame, BlueprintReadWrite, Category = "UIWorld|Save") int32 AmmoReserve = 0;
	UPROPERTY(SaveGame, BlueprintReadWrite, Category = "UIWorld|Save") int32 ClipSize = 0;
	UPROPERTY(SaveGame, BlueprintReadWrite, Category = "UIWorld|Save") float Damage = 0.0f;
	UPROPERTY(SaveGame, BlueprintReadWrite, Category = "UIWorld|Save") float Range = 0.0f;
};

/** Plain serializable snapshot of one inventory slot (see FUIWorldSavedWeapon for the rationale). */
USTRUCT(BlueprintType)
struct FUIWorldSavedItem
{
	GENERATED_BODY()

	UPROPERTY(SaveGame, BlueprintReadWrite, Category = "UIWorld|Save") FName ItemId = NAME_None;
	UPROPERTY(SaveGame, BlueprintReadWrite, Category = "UIWorld|Save") uint8 Category = 0;
	UPROPERTY(SaveGame, BlueprintReadWrite, Category = "UIWorld|Save") FString Description;
	UPROPERTY(SaveGame, BlueprintReadWrite, Category = "UIWorld|Save") bool bConsumable = false;
	UPROPERTY(SaveGame, BlueprintReadWrite, Category = "UIWorld|Save") FString DisplayName;
	UPROPERTY(SaveGame, BlueprintReadWrite, Category = "UIWorld|Save") int32 Quantity = 1;
	UPROPERTY(SaveGame, BlueprintReadWrite, Category = "UIWorld|Save") int32 MaxStack = 99;
	UPROPERTY(SaveGame, BlueprintReadWrite, Category = "UIWorld|Save") FString IconPath;
	UPROPERTY(SaveGame, BlueprintReadWrite, Category = "UIWorld|Save") FString PickupClassPath;
};

/**
 * Full saved game. Every persisted field carries the SaveGame specifier — without it the
 * SaveGame proxy archive (ArIsSaveGame == true) skips the property and nothing is written.
 */
UCLASS(BlueprintType)
class UIWORLD_API UUIWorldSaveGame : public USaveGame
{
	GENERATED_BODY()

public:
	UPROPERTY(SaveGame, EditAnywhere, BlueprintReadWrite, Category = "UIWorld|Save")
	FString SavedLevelName;

	UPROPERTY(SaveGame, EditAnywhere, BlueprintReadWrite, Category = "UIWorld|Save")
	FDateTime SavedAtUtc;

	UPROPERTY(SaveGame, EditAnywhere, BlueprintReadWrite, Category = "UIWorld|Save")
	int32 SaveVersion = 2;

	// --- Player snapshot (only valid when bHasPlayerState is true) ---

	UPROPERTY(SaveGame, EditAnywhere, BlueprintReadWrite, Category = "UIWorld|Save")
	bool bHasPlayerState = false;

	UPROPERTY(SaveGame, EditAnywhere, BlueprintReadWrite, Category = "UIWorld|Save")
	FVector PlayerLocation = FVector::ZeroVector;

	UPROPERTY(SaveGame, EditAnywhere, BlueprintReadWrite, Category = "UIWorld|Save")
	FRotator PlayerRotation = FRotator::ZeroRotator;

	UPROPERTY(SaveGame, EditAnywhere, BlueprintReadWrite, Category = "UIWorld|Save")
	float Health = 0.0f;

	UPROPERTY(SaveGame, EditAnywhere, BlueprintReadWrite, Category = "UIWorld|Save")
	float MaxHealth = 0.0f;

	UPROPERTY(SaveGame, EditAnywhere, BlueprintReadWrite, Category = "UIWorld|Save")
	TArray<FUIWorldSavedWeapon> Weapons;

	UPROPERTY(SaveGame, EditAnywhere, BlueprintReadWrite, Category = "UIWorld|Save")
	int32 EquippedWeaponIndex = INDEX_NONE;

	UPROPERTY(SaveGame, EditAnywhere, BlueprintReadWrite, Category = "UIWorld|Save")
	TArray<FUIWorldSavedItem> Items;

	// --- Created character look ---

	UPROPERTY(SaveGame, EditAnywhere, BlueprintReadWrite, Category = "UIWorld|Save")
	bool bHasAppearance = false;

	UPROPERTY(SaveGame, EditAnywhere, BlueprintReadWrite, Category = "UIWorld|Save")
	FZonefallCharacterAppearance Appearance;
};
