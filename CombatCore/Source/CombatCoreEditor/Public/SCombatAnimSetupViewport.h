
#pragma once

#include "CoreMinimal.h"
#include "SEditorViewport.h"
#include "AdvancedPreviewScene.h"
#include "SCommonEditorViewportToolbarBase.h"

class UCombatAnimSetup;
class FCombatAnimSetupViewportClient;

class SCombatAnimSetupViewport : public SEditorViewport, public ICommonEditorViewportToolbarInfoProvider
{
public:
    SLATE_BEGIN_ARGS(SCombatAnimSetupViewport) {}
    SLATE_END_ARGS()

    void Construct(const FArguments& InArgs);

    void SetAsset(UCombatAnimSetup* InAsset);
    void RefreshFromAsset();

    TSharedPtr<FCombatAnimSetupViewportClient> GetViewportClient() const { return ViewportClient; }

protected:
    // SEditorViewport
    virtual TSharedRef<FEditorViewportClient> MakeEditorViewportClient() override;
    

private:
    TObjectPtr<UCombatAnimSetup> Asset = nullptr;

    TSharedPtr<FCombatAnimSetupViewportClient> ViewportClient;
    TSharedPtr<FAdvancedPreviewScene> PreviewScene;

//TOOLBAR
public:
    // ICommonEditorViewportToolbarInfoProvider
    virtual TSharedRef<SEditorViewport> GetViewportWidget() override { return SharedThis(this); }
    virtual TSharedPtr<FExtender> GetExtenders() const override { return nullptr; }
    virtual void OnFloatingButtonClicked() override {}

    TSharedPtr<FCombatAnimSetupViewportClient> GetCombatViewportClient() const { return ViewportClient; }

};