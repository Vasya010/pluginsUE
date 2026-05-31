

#include "DyingSequencesDataEditorStyle.h"
#include "Styling/SlateStyleRegistry.h"
#include "Interfaces/IPluginManager.h"
#include "Styling/SlateTypes.h"
#include "Styling/SlateStyle.h"

TSharedPtr<FSlateStyleSet> FDyingSequencesDatabaseEditorStyle::StyleSet = nullptr;

FName FDyingSequencesDatabaseEditorStyle::GetStyleSetName()
{
    static FName Name(TEXT("DyingSequencesDatabaseEditorStyle"));
    return Name;
}

void FDyingSequencesDatabaseEditorStyle::Initialize()
{
    if (StyleSet.IsValid())
    {
        return;
    }

    StyleSet = MakeShared<FSlateStyleSet>(GetStyleSetName());

    const FString BaseDir = IPluginManager::Get().FindPlugin(TEXT("CombatCore"))->GetBaseDir();
    StyleSet->SetContentRoot(BaseDir / TEXT("Resources"));

    StyleSet->Set(TEXT("ClassIcon.DyingSequencesDatabase"),
        new FSlateVectorImageBrush(StyleSet->RootToContentDir(TEXT("DyingSequencesDatabase_16.svg")), FVector2D(16.f, 16.f)));

    StyleSet->Set(TEXT("ClassThumbnail.DyingSequencesDatabase"),
        new FSlateVectorImageBrush(StyleSet->RootToContentDir(TEXT("DyingSequencesDatabase_64.svg")), FVector2D(64.f, 64.f)));

    FSlateStyleRegistry::RegisterSlateStyle(*StyleSet.Get());

}

void FDyingSequencesDatabaseEditorStyle::Shutdown()
{
    if (StyleSet.IsValid())
    {
        FSlateStyleRegistry::UnRegisterSlateStyle(*StyleSet.Get());
        StyleSet.Reset();
    }
}