

#include "CombatHitReactionsEditorStyle.h"
#include "Styling/SlateStyleRegistry.h"
#include "Interfaces/IPluginManager.h"
#include "Styling/SlateTypes.h"
#include "Styling/SlateStyle.h"

TSharedPtr<FSlateStyleSet> FCombatHitReactionsDatabaseEditorStyle::StyleSet = nullptr;

FName FCombatHitReactionsDatabaseEditorStyle::GetStyleSetName()
{
    static FName Name(TEXT("CombatHitReactionsDatabaseEditorStyle"));
    return Name;
}

void FCombatHitReactionsDatabaseEditorStyle::Initialize()
{
    if (StyleSet.IsValid())
    {
        return;
    }

    StyleSet = MakeShared<FSlateStyleSet>(GetStyleSetName());

    const FString BaseDir = IPluginManager::Get().FindPlugin(TEXT("CombatCore"))->GetBaseDir();
    StyleSet->SetContentRoot(BaseDir / TEXT("Resources"));

    StyleSet->Set(TEXT("ClassIcon.CombatHitReactionsDatabase"),
        new FSlateVectorImageBrush(StyleSet->RootToContentDir(TEXT("CombatHitReactionsDatabase_16.svg")), FVector2D(16.f, 16.f)));

    StyleSet->Set(TEXT("ClassThumbnail.CombatHitReactionsDatabase"),
        new FSlateVectorImageBrush(StyleSet->RootToContentDir(TEXT("CombatHitReactionsDatabase_64.svg")), FVector2D(64.f, 64.f)));

    FSlateStyleRegistry::RegisterSlateStyle(*StyleSet.Get());

}

void FCombatHitReactionsDatabaseEditorStyle::Shutdown()
{
    if (StyleSet.IsValid())
    {
        FSlateStyleRegistry::UnRegisterSlateStyle(*StyleSet.Get());
        StyleSet.Reset();
    }
}