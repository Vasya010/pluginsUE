

#pragma once

#include "CoreMinimal.h"
#include "Toolkits/AssetEditorToolkit.h"
#include "SCombatAnimSetupTransport.h"
#include "UObject/GCObject.h"
//#include "SCommonEditorViewportToolbarBase.h"
//#include "SCombatAnimSetupViewport.h"

//TOOLBAR
//class FCombatAnimSetupViewportToolbarInfoProvider : public ICommonEditorViewportToolbarInfoProvider
//{
//public:
//    virtual ~FCombatAnimSetupViewportToolbarInfoProvider() = default; // ✅
//
//    explicit FCombatAnimSetupViewportToolbarInfoProvider(const TSharedPtr<SCombatAnimSetupViewport>& InViewportWidget)
//        : ViewportWidget(InViewportWidget)
//    {
//    }
//
//    virtual TSharedRef<SEditorViewport> GetViewportWidget() override
//    {
//        return ViewportWidget.ToSharedRef();
//    }
//
//    virtual TSharedPtr<FExtender> GetExtenders() const override { return nullptr; }
//    virtual void OnFloatingButtonClicked() override {}
//
//private:
//    TSharedPtr<SCombatAnimSetupViewport> ViewportWidget;
//};


class UCombatAnimSetup;
class IDetailsView;
class SCombatAnimSetupViewport;

class FCombatAnimSetupEditorToolkit : public FAssetEditorToolkit, public FGCObject
{
    public:
        void InitCombatAnimSetupEditor(
            const EToolkitMode::Type Mode,
            const TSharedPtr<class IToolkitHost>& InitToolkitHost,
            UCombatAnimSetup* InAsset);

        // FAssetEditorToolkit
        virtual FName GetToolkitFName() const override;
        virtual FText GetBaseToolkitName() const override;
        virtual FString GetWorldCentricTabPrefix() const override;
        virtual FLinearColor GetWorldCentricTabColorScale() const override;

        // Tabs
        virtual void RegisterTabSpawners(const TSharedRef<class FTabManager>& InTabManager) override;
        virtual void UnregisterTabSpawners(const TSharedRef<class FTabManager>& InTabManager) override;

        // FGCObject
        virtual void AddReferencedObjects(FReferenceCollector& Collector) override;
        virtual FString GetReferencerName() const override { return TEXT("FCombatAnimSetupEditorToolkit"); }

        // FAssetEditorToolkit
        virtual FText GetToolkitName() const override;

        virtual const FSlateBrush* GetDefaultTabIcon() const override;


private:
    TSharedRef<class SDockTab> SpawnDetailsTab(const class FSpawnTabArgs& Args);
    TSharedRef<class SDockTab> SpawnViewportTab(const class FSpawnTabArgs& Args);

    void OnAssetPropertyChanged();

private:
    TObjectPtr<UCombatAnimSetup> Asset = nullptr;

    TSharedPtr<IDetailsView> DetailsView;
    TSharedPtr<SCombatAnimSetupViewport> ViewportWidget;
    //TSharedPtr<ICommonEditorViewportToolbarInfoProvider> ToolbarInfoProvider;

    static const FName DetailsTabId;
    static const FName ViewportTabId;


};
