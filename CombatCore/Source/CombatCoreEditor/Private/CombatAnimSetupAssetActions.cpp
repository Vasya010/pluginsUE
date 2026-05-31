

#include "CombatAnimSetupAssetActions.h"

#include "CombatAnimSetup.h"
#include "CombatAnimSetupEditorToolkit.h"

#define LOCTEXT_NAMESPACE "CombatAnimSetupAssetActions"

FText FCombatAnimSetupAssetActions::GetName() const
{
    return LOCTEXT("CombatAnimSetupName", "Combat Anim Setup");
}

FColor FCombatAnimSetupAssetActions::GetTypeColor() const
{
    //return FColor(90, 160, 255);
    return FColor(191, 68, 35);
}

UClass* FCombatAnimSetupAssetActions::GetSupportedClass() const
{
    return UCombatAnimSetup::StaticClass();
}

uint32 FCombatAnimSetupAssetActions::GetCategories()
{
    // Najproœciej: Misc
    return EAssetTypeCategories::Animation;
}

void FCombatAnimSetupAssetActions::OpenAssetEditor(
    const TArray<UObject*>& InObjects,
    TSharedPtr<IToolkitHost> EditWithinLevelEditor)
{
    for (UObject* Obj : InObjects)
    {
        UCombatAnimSetup* Asset = Cast<UCombatAnimSetup>(Obj);
        if (!Asset) continue;

        TSharedRef<FCombatAnimSetupEditorToolkit> Editor = MakeShared<FCombatAnimSetupEditorToolkit>();
        Editor->InitCombatAnimSetupEditor(EToolkitMode::Standalone, EditWithinLevelEditor, Asset);
    }
}

#undef LOCTEXT_NAMESPACE
	