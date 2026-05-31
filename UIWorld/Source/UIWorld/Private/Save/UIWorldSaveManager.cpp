#include "Save/UIWorldSaveManager.h"

#include "Kismet/GameplayStatics.h"
#include "Save/UIWorldSaveGame.h"

UUIWorldSaveManager::UUIWorldSaveManager()
	: SaveSlotName(TEXT("UIWorld_Main_Save"))
	, UserIndex(0)
{
}

bool UUIWorldSaveManager::HasSave() const
{
	if (SaveSlotName.IsEmpty())
	{
		return false;
	}

	return UGameplayStatics::DoesSaveGameExist(SaveSlotName, UserIndex);
}

bool UUIWorldSaveManager::SaveLevelProgress(const FString& LevelName)
{
	if (SaveSlotName.IsEmpty() || LevelName.IsEmpty())
	{
		return false;
	}

	// Preserve any existing player snapshot — a level-only save should not wipe weapons/items.
	UUIWorldSaveGame* SaveObject = LoadSave();
	if (!SaveObject)
	{
		SaveObject = CreateEmptySave();
	}
	if (!SaveObject)
	{
		return false;
	}

	SaveObject->SavedLevelName = LevelName;
	SaveObject->SavedAtUtc = FDateTime::UtcNow();
	return WriteSave(SaveObject);
}

bool UUIWorldSaveManager::LoadSavedLevelName(FString& OutLevelName) const
{
	OutLevelName.Reset();

	const UUIWorldSaveGame* SaveObject = LoadSave();
	if (!SaveObject || SaveObject->SavedLevelName.IsEmpty())
	{
		return false;
	}

	OutLevelName = SaveObject->SavedLevelName;
	return true;
}

UUIWorldSaveGame* UUIWorldSaveManager::LoadSave() const
{
	if (!HasSave())
	{
		return nullptr;
	}

	USaveGame* LoadedGame = UGameplayStatics::LoadGameFromSlot(SaveSlotName, UserIndex);
	return Cast<UUIWorldSaveGame>(LoadedGame);
}

UUIWorldSaveGame* UUIWorldSaveManager::CreateEmptySave() const
{
	return Cast<UUIWorldSaveGame>(UGameplayStatics::CreateSaveGameObject(UUIWorldSaveGame::StaticClass()));
}

bool UUIWorldSaveManager::WriteSave(UUIWorldSaveGame* Save)
{
	if (SaveSlotName.IsEmpty() || !Save)
	{
		return false;
	}
	return UGameplayStatics::SaveGameToSlot(Save, SaveSlotName, UserIndex);
}

bool UUIWorldSaveManager::DeleteSave()
{
	if (SaveSlotName.IsEmpty())
	{
		return false;
	}
	return UGameplayStatics::DeleteGameInSlot(SaveSlotName, UserIndex);
}
