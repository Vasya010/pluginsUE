

#include "GameplayTaskCompAssetActions.h"
#include "GameplayTasksComponent.h"
#include "GameplayTasksExtendedComponent.h"

#define LOCTEXT_NAMESPACE "GameplayTaskCompAssetActions"

FText FGameplayTaskCompAssetActions::GetName() const
{
    return LOCTEXT("GameplayTaskComponentName", "Gameplay Task Component");
}

FColor FGameplayTaskCompAssetActions::GetTypeColor() const
{
    return FColor(191, 68, 35);
}

UClass* FGameplayTaskCompAssetActions::GetSupportedClass() const
{
    return UGameplayTasksExtendedComponent::StaticClass();
}

uint32 FGameplayTaskCompAssetActions::GetCategories()
{
    return MyCategory;
}



#undef LOCTEXT_NAMESPACE
	