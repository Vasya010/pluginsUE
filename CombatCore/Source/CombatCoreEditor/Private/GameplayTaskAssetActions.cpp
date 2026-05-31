

#include "GameplayTaskAssetActions.h"

#include "GameplayTaskBlueprint.h"

#define LOCTEXT_NAMESPACE "GameplayTaskAssetActions"

FText FGameplayTaskAssetActions::GetName() const
{
    return LOCTEXT("GameplayTaskName", "Gameplay Task");
}

FColor FGameplayTaskAssetActions::GetTypeColor() const
{
    return FColor(191, 68, 35);
}

UClass* FGameplayTaskAssetActions::GetSupportedClass() const
{
    return UGameplayTaskBlueprint::StaticClass();
}

uint32 FGameplayTaskAssetActions::GetCategories()
{
    return MyCategory;
}



#undef LOCTEXT_NAMESPACE
	