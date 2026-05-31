#include "SCombatAnimSetupViewportTopBar.h"

#include "CombatAnimSetupViewportClient.h"

#include "Framework/MultiBox/MultiBoxBuilder.h"
#include "Styling/AppStyle.h"
#include "Widgets/Input/SComboBox.h"
#include "Widgets/Input/SComboButton.h"
//#include "Widgets/Layout/SHorizontalBox.h"
#include "Widgets/Text/STextBlock.h"

static void ApplyViewMode(FCombatAnimSetupViewportClient& VC, const FString& Mode)
{
    if (Mode == TEXT("Lit"))
    {
        VC.SetViewMode(VMI_Lit);
    }
    else if (Mode == TEXT("Unlit"))
    {
        VC.SetViewMode(VMI_Unlit);
    }
    else if (Mode == TEXT("Wireframe"))
    {
        VC.SetViewMode(VMI_Wireframe);
    }

    VC.Invalidate();
}

static void ApplyProjection(FCombatAnimSetupViewportClient& VC, const FString& Mode)
{
    if (Mode == TEXT("Perspective"))
    {
        VC.SetViewportType(LVT_Perspective);
    }
    else if (Mode == TEXT("Ortho"))
    {
        VC.SetViewportType(LVT_OrthoXY);
    }

    VC.Invalidate();
}

void SCombatAnimSetupViewportTopBar::Construct(const FArguments& InArgs)
{
    ClientWeak = InArgs._ViewportClient;

    ViewModes = {
        MakeShared<FString>(TEXT("Lit")),
        MakeShared<FString>(TEXT("Unlit")),
        MakeShared<FString>(TEXT("Wireframe"))
    };
    SelectedViewMode = ViewModes[0];

    Projections = {
        MakeShared<FString>(TEXT("Perspective")),
        MakeShared<FString>(TEXT("Ortho"))
    };
    SelectedProjection = Projections[0];

    ChildSlot
        [
            SNew(SHorizontalBox)

                // Left: Projection
                + SHorizontalBox::Slot()
                .AutoWidth()
                .Padding(2.f, 2.f)
                [
                    SNew(SComboBox<TSharedPtr<FString>>)
                        .OptionsSource(&Projections)
                        .InitiallySelectedItem(SelectedProjection)
                        .OnSelectionChanged(this, &SCombatAnimSetupViewportTopBar::OnProjectionChanged)
                        .OnGenerateWidget(this, &SCombatAnimSetupViewportTopBar::MakeProjectionWidget)
                        [
                            SNew(STextBlock).Text(this, &SCombatAnimSetupViewportTopBar::GetProjectionText)
                        ]
                ]

            // Left: ViewMode
            + SHorizontalBox::Slot()
                .AutoWidth()
                .Padding(2.f, 2.f)
                [
                    SNew(SComboBox<TSharedPtr<FString>>)
                        .OptionsSource(&ViewModes)
                        .InitiallySelectedItem(SelectedViewMode)
                        .OnSelectionChanged(this, &SCombatAnimSetupViewportTopBar::OnViewModeChanged)
                        .OnGenerateWidget(this, &SCombatAnimSetupViewportTopBar::MakeViewModeWidget)
                        [
                            SNew(STextBlock).Text(this, &SCombatAnimSetupViewportTopBar::GetViewModeText)
                        ]
                ]

            // Spacer
            + SHorizontalBox::Slot()
                .FillWidth(1.f)

                // Right: Eye menu (Twoje toggles)
                + SHorizontalBox::Slot()
                .AutoWidth()
                .Padding(2.f, 2.f)
                [
                    SNew(SComboButton)
                        .HasDownArrow(false)
                        .ButtonStyle(FAppStyle::Get(), "SimpleButton")
                        .ContentPadding(FMargin(8.f, 2.f))
                        .ButtonContent()
                        [
                            SNew(STextBlock).Text(FText::FromString(TEXT("👁")))
                        ]
                        .MenuContent()
                        [
                            BuildEyeMenu()
                        ]
                ]
        ];
}

TSharedRef<SWidget> SCombatAnimSetupViewportTopBar::BuildEyeMenu() const
{
    TSharedPtr<FCombatAnimSetupViewportClient> Client = ClientWeak.Pin();

    FMenuBuilder MenuBuilder(true, nullptr);
    MenuBuilder.BeginSection(NAME_None, FText::FromString(TEXT("Debug Draw")));
    {
        if (Client.IsValid())
        {
            auto AddToggle = [&](const TCHAR* Label, const TCHAR* Tooltip, bool& Flag)
                {
                    MenuBuilder.AddMenuEntry(
                        FText::FromString(Label),
                        FText::FromString(Tooltip),
                        FSlateIcon(),
                        FUIAction(
                            FExecuteAction::CreateLambda([Client, &Flag]()
                                {
                                    Flag = !Flag;
                                    Client->Invalidate();
                                }),
                            FCanExecuteAction(),
                            FIsActionChecked::CreateLambda([&Flag]() { return Flag; })
                        ),
                        NAME_None,
                        EUserInterfaceActionType::ToggleButton
                    );
                };

            AddToggle(TEXT("Show Root Motion Preview"), TEXT(""), Client->bShowRootMotionPreview);
            AddToggle(TEXT("Show Sampled Trajectories"), TEXT(""), Client->bShowSampledTrajectories);
            AddToggle(TEXT("Show Bones"), TEXT(""), Client->bShowBones);
            AddToggle(TEXT("Show Collision Shapes"), TEXT(""), Client->bShowCollisionShapes);
        }
        else
        {
            MenuBuilder.AddMenuEntry(
                FText::FromString(TEXT("No viewport client")),
                FText::GetEmpty(),
                FSlateIcon(),
                FUIAction()
            );
        }
    }
    MenuBuilder.EndSection();

    return MenuBuilder.MakeWidget();
}

void SCombatAnimSetupViewportTopBar::OnViewModeChanged(TSharedPtr<FString> NewSelection, ESelectInfo::Type)
{
    SelectedViewMode = NewSelection;

    if (TSharedPtr<FCombatAnimSetupViewportClient> Client = ClientWeak.Pin())
    {
        ApplyViewMode(*Client, *SelectedViewMode);
    }
}

TSharedRef<SWidget> SCombatAnimSetupViewportTopBar::MakeViewModeWidget(TSharedPtr<FString> Item) const
{
    return SNew(STextBlock).Text(FText::FromString(Item.IsValid() ? *Item : TEXT("")));
}

FText SCombatAnimSetupViewportTopBar::GetViewModeText() const
{
    return FText::FromString(SelectedViewMode.IsValid() ? *SelectedViewMode : TEXT("Lit"));
}

void SCombatAnimSetupViewportTopBar::OnProjectionChanged(TSharedPtr<FString> NewSelection, ESelectInfo::Type)
{
    SelectedProjection = NewSelection;

    if (TSharedPtr<FCombatAnimSetupViewportClient> Client = ClientWeak.Pin())
    {
        ApplyProjection(*Client, *SelectedProjection);
    }
}

TSharedRef<SWidget> SCombatAnimSetupViewportTopBar::MakeProjectionWidget(TSharedPtr<FString> Item) const
{
    return SNew(STextBlock).Text(FText::FromString(Item.IsValid() ? *Item : TEXT("")));
}

FText SCombatAnimSetupViewportTopBar::GetProjectionText() const
{
    return FText::FromString(SelectedProjection.IsValid() ? *SelectedProjection : TEXT("Perspective"));
}