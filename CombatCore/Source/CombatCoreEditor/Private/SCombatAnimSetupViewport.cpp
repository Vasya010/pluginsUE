

#include "SCombatAnimSetupViewport.h"

#include "CombatAnimSetup.h"
#include "CombatAnimSetupViewportClient.h"

void SCombatAnimSetupViewport::Construct(const FArguments& InArgs)
{
    PreviewScene = MakeShared<FAdvancedPreviewScene>(FPreviewScene::ConstructionValues());
    SEditorViewport::Construct(SEditorViewport::FArguments());
}

TSharedRef<FEditorViewportClient> SCombatAnimSetupViewport::MakeEditorViewportClient()
{
    ViewportClient = MakeShared<FCombatAnimSetupViewportClient>(PreviewScene.ToSharedRef(), SharedThis(this));

    ViewportClient->SetViewLocation(FVector(-250.f, 0.f, 120.f));
    ViewportClient->SetViewRotation(FRotator(0.f, 0.f, 0.f));
    ViewportClient->SetRealtime(true);

    return ViewportClient.ToSharedRef();
}

void SCombatAnimSetupViewport::SetAsset(UCombatAnimSetup* InAsset)
{
    Asset = InAsset;
}

void SCombatAnimSetupViewport::RefreshFromAsset()
{
    if (ViewportClient.IsValid())
    {
        ViewportClient->ApplyAsset(Asset.Get());
    }
}
	

