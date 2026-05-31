

#include "CombatAnimSetupEditorToolkit.h"

#include "CombatAnimSetup.h"
#include "SCombatAnimSetupViewport.h"
#include "SCombatAnimSetupViewportTopBar.h"
//#include "SCombatAnimSetupViewportToolbar.h"

#include "PropertyEditorModule.h"
#include "Modules/ModuleManager.h"
#include "Widgets/Docking/SDockTab.h"
#include "Framework/Docking/TabManager.h"
#include "Styling/AppStyle.h"
#include "Styling/SlateStyleRegistry.h"

const FName FCombatAnimSetupEditorToolkit::DetailsTabId(TEXT("CombatAnimSetup_Details"));
const FName FCombatAnimSetupEditorToolkit::ViewportTabId(TEXT("CombatAnimSetup_Viewport"));

void FCombatAnimSetupEditorToolkit::InitCombatAnimSetupEditor(
    const EToolkitMode::Type Mode,
    const TSharedPtr<IToolkitHost>& InitToolkitHost,
    UCombatAnimSetup* InAsset)
{
    Asset = InAsset;

    // DetailsView
    FPropertyEditorModule& PropModule = FModuleManager::LoadModuleChecked<FPropertyEditorModule>("PropertyEditor");
    FDetailsViewArgs Args;
    Args.bHideSelectionTip = true;
    Args.bAllowSearch = true;

    DetailsView = PropModule.CreateDetailView(Args);
    DetailsView->SetObject(Asset.Get());
    DetailsView->OnFinishedChangingProperties().AddLambda([this](const FPropertyChangedEvent&)
        {
            OnAssetPropertyChanged();
        });

    // Layout
    const TSharedRef<FTabManager::FLayout> Layout = FTabManager::NewLayout("CombatAnimSetupEditor_Layout_v1")
        ->AddArea
        (
            FTabManager::NewPrimaryArea()
            ->SetOrientation(Orient_Horizontal)
            ->Split
            (
                FTabManager::NewStack()
                ->SetSizeCoefficient(0.35f)
                ->AddTab(DetailsTabId, ETabState::OpenedTab)
                ->SetHideTabWell(true)
            )
            ->Split
            (
                FTabManager::NewStack()
                ->SetSizeCoefficient(0.65f)
                ->AddTab(ViewportTabId, ETabState::OpenedTab)
                ->SetHideTabWell(true)
            )
        );

    InitAssetEditor(Mode, InitToolkitHost, GetToolkitFName(), Layout, true, true, Asset.Get());
    RegenerateMenusAndToolbars();

    OnAssetPropertyChanged();
}

FName FCombatAnimSetupEditorToolkit::GetToolkitFName() const
{
    return FName("CombatAnimSetupEditor");
}

FText FCombatAnimSetupEditorToolkit::GetBaseToolkitName() const
{
    return FText::FromString(TEXT("Combat Anim Setup"));
}

FString FCombatAnimSetupEditorToolkit::GetWorldCentricTabPrefix() const
{
    return TEXT("CombatAnimSetup");
}

FLinearColor FCombatAnimSetupEditorToolkit::GetWorldCentricTabColorScale() const
{
    return FLinearColor(0.2f, 0.5f, 1.0f, 1.0f);
}

void FCombatAnimSetupEditorToolkit::RegisterTabSpawners(const TSharedRef<FTabManager>& InTabManager)
{
    FAssetEditorToolkit::RegisterTabSpawners(InTabManager);

    InTabManager->RegisterTabSpawner(DetailsTabId, FOnSpawnTab::CreateRaw(this, &FCombatAnimSetupEditorToolkit::SpawnDetailsTab));
    InTabManager->RegisterTabSpawner(ViewportTabId, FOnSpawnTab::CreateRaw(this, &FCombatAnimSetupEditorToolkit::SpawnViewportTab));
}

void FCombatAnimSetupEditorToolkit::UnregisterTabSpawners(const TSharedRef<FTabManager>& InTabManager)
{
    InTabManager->UnregisterTabSpawner(DetailsTabId);
    InTabManager->UnregisterTabSpawner(ViewportTabId);

    FAssetEditorToolkit::UnregisterTabSpawners(InTabManager);
}

TSharedRef<SDockTab> FCombatAnimSetupEditorToolkit::SpawnDetailsTab(const FSpawnTabArgs& Args)
{
    return SNew(SDockTab)
        .Label(FText::FromString(TEXT("Details")))
        [
            DetailsView.ToSharedRef()
        ];
}

TSharedRef<SDockTab> FCombatAnimSetupEditorToolkit::SpawnViewportTab(const FSpawnTabArgs& Args)
{
    ViewportWidget = SNew(SCombatAnimSetupViewport);
    ViewportWidget->SetAsset(Asset.Get());

    return SNew(SDockTab)
        .Label(FText::FromString(TEXT("Viewport")))
        [
            SNew(SVerticalBox)

                // ✅ TOP BAR NA GÓRZE
                + SVerticalBox::Slot()
                .AutoHeight()
                .Padding(4.f)
                [
                    SNew(SCombatAnimSetupViewportTopBar)
                        .ViewportClient(ViewportWidget->GetViewportClient())
                ]

                // VIEWPORT
                + SVerticalBox::Slot()
                .FillHeight(1.f)
                [
                    ViewportWidget.ToSharedRef()
                ]

                // TIMELINE / TRANSPORT NA DOLE
                + SVerticalBox::Slot()
                .AutoHeight()
                .Padding(6.f)
                [
                    SNew(SCombatAnimSetupTransport)
                        .ViewportClient(ViewportWidget->GetViewportClient())
                ]
        ];
}

void FCombatAnimSetupEditorToolkit::OnAssetPropertyChanged()
{
    if (ViewportWidget.IsValid())
    {
        ViewportWidget->RefreshFromAsset();
    }
}

void FCombatAnimSetupEditorToolkit::AddReferencedObjects(FReferenceCollector& Collector)
{
    Collector.AddReferencedObject(Asset);
}

FText FCombatAnimSetupEditorToolkit::GetToolkitName() const
{
    const UObject* EditingObj = GetEditingObject();
    const FString Name = EditingObj ? EditingObj->GetName() : TEXT("CombatAnimSetup");
    return FText::FromString(FString::Printf(TEXT("Combat Anim Setup: %s"), *Name));
}

const FSlateBrush* FCombatAnimSetupEditorToolkit::GetDefaultTabIcon() const
{
    // Upewnij się, że to jest dokładna nazwa Twojego StyleSet (ta z FSlateStyleSet ctor)
    const ISlateStyle* Style = FSlateStyleRegistry::FindSlateStyle(TEXT("CombatCoreEditorStyle"));
    if (Style)
    {
        // Klucz taki jak rejestrowałeś: "ClassIcon.CombatAnimSetup"
        const FSlateBrush* Brush = Style->GetBrush(TEXT("ClassIcon.CombatAnimSetup"));
        if (Brush)
        {
            return Brush;
        }
    }

    // Fallback – żeby nigdy nie było pusto
    return FAppStyle::Get().GetBrush("ClassIcon.DataAsset");
}
