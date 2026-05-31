// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Modules/ModuleManager.h"
#include "AssetTypeCategories.h"

class FCombatCoreEditorModule : public IModuleInterface
{
public:
    virtual void StartupModule() override;
    virtual void ShutdownModule() override;

    EAssetTypeCategories::Type CombatCategory;
    EAssetTypeCategories::Type GameplayCategory;

private:
    void RegisterAssetTools();
    void UnregisterAssetTools();

private:
    TArray<TSharedPtr<class IAssetTypeActions>> RegisteredAssetActions;

};
