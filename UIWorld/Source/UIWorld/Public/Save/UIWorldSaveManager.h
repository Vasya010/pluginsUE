#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "UIWorldSaveManager.generated.h"

class UUIWorldSaveGame;

UCLASS(BlueprintType, Blueprintable)
class UIWORLD_API UUIWorldSaveManager : public UObject
{
	GENERATED_BODY()

public:
	UUIWorldSaveManager();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UIWorld|Save")
	FString SaveSlotName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UIWorld|Save")
	int32 UserIndex;

	UFUNCTION(BlueprintCallable, Category = "UIWorld|Save")
	bool HasSave() const;

	/** Level-only quick save (kept for compatibility). Prefer WriteSave for full state. */
	UFUNCTION(BlueprintCallable, Category = "UIWorld|Save")
	bool SaveLevelProgress(const FString& LevelName);

	UFUNCTION(BlueprintCallable, Category = "UIWorld|Save")
	bool LoadSavedLevelName(FString& OutLevelName) const;

	/** Loads the full save object from the slot, or null if none / wrong type. */
	UFUNCTION(BlueprintCallable, Category = "UIWorld|Save")
	UUIWorldSaveGame* LoadSave() const;

	/** Creates an empty save object of the right type (caller fills it, then WriteSave). */
	UFUNCTION(BlueprintCallable, Category = "UIWorld|Save")
	UUIWorldSaveGame* CreateEmptySave() const;

	/** Writes a fully-populated save object to the slot. */
	UFUNCTION(BlueprintCallable, Category = "UIWorld|Save")
	bool WriteSave(UUIWorldSaveGame* Save);

	UFUNCTION(BlueprintCallable, Category = "UIWorld|Save")
	bool DeleteSave();
};
