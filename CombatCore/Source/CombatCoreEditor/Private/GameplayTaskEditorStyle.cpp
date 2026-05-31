#include "GameplayTaskEditorStyle.h"
#include "Styling/SlateStyleRegistry.h"
#include "Interfaces/IPluginManager.h"
#include "Styling/SlateTypes.h"
#include "Styling/SlateStyle.h"

TSharedPtr<FSlateStyleSet> FGameplayTaskEditorStyle::StyleSet = nullptr;

FName FGameplayTaskEditorStyle::GetStyleSetName()
{
    static FName Name(TEXT("GameplayTaskEditorStyle"));
    return Name;
}

void FGameplayTaskEditorStyle::Initialize()
{
    if (StyleSet.IsValid())
    {
        return;
    }

    StyleSet = MakeShared<FSlateStyleSet>(GetStyleSetName());

    const FString BaseDir = IPluginManager::Get().FindPlugin(TEXT("CombatCore"))->GetBaseDir();
    StyleSet->SetContentRoot(BaseDir / TEXT("Resources"));

    StyleSet->Set(TEXT("ClassIcon.GameplayTaskBlueprint"),
        new FSlateVectorImageBrush(StyleSet->RootToContentDir(TEXT("GameplayTaskBlueprint_16.svg")), FVector2D(16.f, 16.f)));

    StyleSet->Set(TEXT("ClassThumbnail.GameplayTaskBlueprint"),
        new FSlateVectorImageBrush(StyleSet->RootToContentDir(TEXT("GameplayTaskBlueprint_64.svg")), FVector2D(64.f, 64.f)));

    FSlateStyleRegistry::RegisterSlateStyle(*StyleSet.Get());

}

void FGameplayTaskEditorStyle::Shutdown()
{
    if (StyleSet.IsValid())
    {
        FSlateStyleRegistry::UnRegisterSlateStyle(*StyleSet.Get());
        StyleSet.Reset();
    }
}