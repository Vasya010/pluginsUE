#include "GameplayTaskComponentEditorStyle.h"
#include "Styling/SlateStyleRegistry.h"
#include "Interfaces/IPluginManager.h"
#include "Styling/SlateTypes.h"
#include "Styling/SlateStyle.h"

TSharedPtr<FSlateStyleSet> FGameplayTasksComponentEditorStyle::StyleSet = nullptr;

FName FGameplayTasksComponentEditorStyle::GetStyleSetName()
{
    static FName Name(TEXT("GameplayTaskComponentEditorStyle"));
    return Name;
}

void FGameplayTasksComponentEditorStyle::Initialize()
{
    if (StyleSet.IsValid())
    {
        return;
    }

    StyleSet = MakeShared<FSlateStyleSet>(GetStyleSetName());

    const FString BaseDir = IPluginManager::Get().FindPlugin(TEXT("CombatCore"))->GetBaseDir();
    StyleSet->SetContentRoot(BaseDir / TEXT("Resources"));

    StyleSet->Set(TEXT("ClassIcon.GameplayTasksExtendedComponent"),
        new FSlateVectorImageBrush(StyleSet->RootToContentDir(TEXT("GameplayTasksComponent_16.svg")), FVector2D(16.f, 16.f)));

    StyleSet->Set(TEXT("ClassThumbnail.GameplayTasksExtendedComponent"),
        new FSlateVectorImageBrush(StyleSet->RootToContentDir(TEXT("GameplayTasksComponent_64.svg")), FVector2D(64.f, 64.f)));

    FSlateStyleRegistry::RegisterSlateStyle(*StyleSet.Get());

}

void FGameplayTasksComponentEditorStyle::Shutdown()
{
    if (StyleSet.IsValid())
    {
        FSlateStyleRegistry::UnRegisterSlateStyle(*StyleSet.Get());
        StyleSet.Reset();
    }
}