// Copyright Epic Games, Inc. All Rights Reserved.

#include "CombatCoreEditor.h"

#include "AssetToolsModule.h"

#include "CombatAnimSetupAssetActions.h"
#include "CombatCoreEditorStyle.h"
#include "CombatAnimsDatabaseEditorStyle.h"
#include "CombatHitReactionsEditorStyle.h"

#include "DyingSequencesDataEditorStyle.h"

#include "GameplayTaskAssetActions.h"
#include "GameplayTaskEditorStyle.h"
#include "GameplayTaskCompAssetActions.h"
#include "GameplayTaskComponentEditorStyle.h"

#include "Modules/ModuleManager.h"
#include "IAssetTools.h"
#include "AssetTypeCategories.h"


#define LOCTEXT_NAMESPACE "FCombatCoreEditorModule"

void FCombatCoreEditorModule::StartupModule()
{
    RegisterAssetTools();

    //Register CombatAnimSetup
    IAssetTools& AssetTools = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools").Get();
    CombatCategory =
        AssetTools.RegisterAdvancedAssetCategory(
            FName("Combat"),
            FText::FromString("Animation") // nazwa widoczna w UI (mo¿e byæ "Gameplay" albo "Combat")
        );

    //Register GameplayTask
    GameplayCategory =
        AssetTools.RegisterAdvancedAssetCategory(
            FName("GameplayTask"),
            FText::FromString("Gameplay") // nazwa widoczna w UI (mo¿e byæ "Gameplay" albo "Combat")
        );

    FCombatCoreEditorStyle::Initialize();
    FCombatAnimsDatabaseEditorStyle::Initialize();
    FCombatHitReactionsDatabaseEditorStyle::Initialize();
    FGameplayTaskEditorStyle::Initialize();
    FGameplayTasksComponentEditorStyle::Initialize();
    FDyingSequencesDatabaseEditorStyle::Initialize();
}

void FCombatCoreEditorModule::ShutdownModule()
{
    UnregisterAssetTools();
    FCombatCoreEditorStyle::Shutdown();
    FCombatAnimsDatabaseEditorStyle::Shutdown();
    FCombatHitReactionsDatabaseEditorStyle::Shutdown();
    FGameplayTaskEditorStyle::Shutdown();
    FGameplayTasksComponentEditorStyle::Shutdown();
    FDyingSequencesDatabaseEditorStyle::Shutdown();
}

void FCombatCoreEditorModule::RegisterAssetTools()
{
    IAssetTools& AssetTools = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools").Get();

    //Register CombatAnimSetup
    const TSharedPtr<IAssetTypeActions> Action = MakeShared<FCombatAnimSetupAssetActions>();
    AssetTools.RegisterAssetTypeActions(Action.ToSharedRef());
    RegisteredAssetActions.Add(Action);

    //Register GameplayTask
    const TSharedPtr<IAssetTypeActions> ActionGT = MakeShared<FGameplayTaskAssetActions>(GameplayCategory);
    AssetTools.RegisterAssetTypeActions(ActionGT.ToSharedRef());
    RegisteredAssetActions.Add(ActionGT);

    //Register GameplayTaskComponent
    const TSharedPtr<IAssetTypeActions> ActionGTC = MakeShared<FGameplayTaskCompAssetActions>(GameplayCategory);
    AssetTools.RegisterAssetTypeActions(ActionGTC.ToSharedRef());
    RegisteredAssetActions.Add(ActionGTC);
}

void FCombatCoreEditorModule::UnregisterAssetTools()
{
    if (FModuleManager::Get().IsModuleLoaded("AssetTools"))
    {
        IAssetTools& AssetTools = FModuleManager::GetModuleChecked<FAssetToolsModule>("AssetTools").Get();
        for (const TSharedPtr<IAssetTypeActions>& Action : RegisteredAssetActions)
        {
            if (Action.IsValid())
            {
                AssetTools.UnregisterAssetTypeActions(Action.ToSharedRef());
            }
        }
    }
    RegisteredAssetActions.Empty();
}

IMPLEMENT_MODULE(FCombatCoreEditorModule, CombatCoreEditor)

#undef LOCTEXT_NAMESPACE
	