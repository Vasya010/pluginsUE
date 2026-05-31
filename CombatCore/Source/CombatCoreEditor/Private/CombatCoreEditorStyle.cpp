#include "CombatCoreEditorStyle.h"
#include "Styling/SlateStyleRegistry.h"
#include "Interfaces/IPluginManager.h"
#include "Styling/SlateTypes.h"
#include "Styling/SlateStyle.h"

TSharedPtr<FSlateStyleSet> FCombatCoreEditorStyle::StyleSet = nullptr;

FName FCombatCoreEditorStyle::GetStyleSetName()
{
    static FName Name(TEXT("CombatCoreEditorStyle"));
    return Name;
}

void FCombatCoreEditorStyle::Initialize()
{
    if (StyleSet.IsValid())
    {
        return;
    }

    StyleSet = MakeShared<FSlateStyleSet>(GetStyleSetName());

    const FString BaseDir = IPluginManager::Get().FindPlugin(TEXT("CombatCore"))->GetBaseDir();
    StyleSet->SetContentRoot(BaseDir / TEXT("Resources"));

    StyleSet->Set(TEXT("ClassIcon.CombatAnimSetup"),
        new FSlateVectorImageBrush(StyleSet->RootToContentDir(TEXT("CombatAnimSetup_16.svg")), FVector2D(16.f, 16.f)));

    StyleSet->Set(TEXT("ClassThumbnail.CombatAnimSetup"),
        new FSlateVectorImageBrush(StyleSet->RootToContentDir(TEXT("CombatAnimSetup_64.svg")), FVector2D(64.f, 64.f)));

    FSlateStyleRegistry::RegisterSlateStyle(*StyleSet.Get());

}

void FCombatCoreEditorStyle::Shutdown()
{
    if (StyleSet.IsValid())
    {
        FSlateStyleRegistry::UnRegisterSlateStyle(*StyleSet.Get());
        StyleSet.Reset();
    }
}