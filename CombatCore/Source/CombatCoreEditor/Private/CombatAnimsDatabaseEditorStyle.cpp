

#include "CombatAnimsDatabaseEditorStyle.h"
#include "Styling/SlateStyleRegistry.h"
#include "Interfaces/IPluginManager.h"
#include "Styling/SlateTypes.h"
#include "Styling/SlateStyle.h"

TSharedPtr<FSlateStyleSet> FCombatAnimsDatabaseEditorStyle::StyleSet = nullptr;

FName FCombatAnimsDatabaseEditorStyle::GetStyleSetName()
{
    static FName Name(TEXT("CombatAnimsDatabaseEditorStyle"));
    return Name;
}

void FCombatAnimsDatabaseEditorStyle::Initialize()
{
    if (StyleSet.IsValid())
    {
        return;
    }

    StyleSet = MakeShared<FSlateStyleSet>(GetStyleSetName());

    const FString BaseDir = IPluginManager::Get().FindPlugin(TEXT("CombatCore"))->GetBaseDir();
    StyleSet->SetContentRoot(BaseDir / TEXT("Resources"));

    StyleSet->Set(TEXT("ClassIcon.CombatAnimsDatabase"),
        new FSlateVectorImageBrush(StyleSet->RootToContentDir(TEXT("CombatAnimsDatabase_16.svg")), FVector2D(16.f, 16.f)));

    StyleSet->Set(TEXT("ClassThumbnail.CombatAnimsDatabase"),
        new FSlateVectorImageBrush(StyleSet->RootToContentDir(TEXT("CombatAnimsDatabase_64.svg")), FVector2D(64.f, 64.f)));

    FSlateStyleRegistry::RegisterSlateStyle(*StyleSet.Get());

}

void FCombatAnimsDatabaseEditorStyle::Shutdown()
{
    if (StyleSet.IsValid())
    {
        FSlateStyleRegistry::UnRegisterSlateStyle(*StyleSet.Get());
        StyleSet.Reset();
    }
}