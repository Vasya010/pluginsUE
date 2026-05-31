#include "SCombatAnimSetupViewportToolbar.h"

#include "SCombatAnimSetupViewport.h"
#include "CombatAnimSetupViewportClient.h"

#include "Framework/MultiBox/MultiBoxBuilder.h"

void SCombatAnimSetupViewportToolbar::Construct(const FArguments& InArgs)
{
    ClientWeak = InArgs._ViewportClient;

    if (!InArgs._InfoProvider.IsValid())
    {
        ChildSlot
            [
                SNew(STextBlock).Text(FText::FromString(TEXT("Toolbar: missing InfoProvider")))
            ];
        return;
    }

    // W UE5.7 Construct może mieć overload na TSharedPtr
    SCommonEditorViewportToolbarBase::Construct(
        SCommonEditorViewportToolbarBase::FArguments(),
        InArgs._InfoProvider
    );
}

TSharedRef<SWidget> SCombatAnimSetupViewportToolbar::GenerateShowMenu() const
{
    // Najpierw weź domyślne show menu z bazy (ma standardowe opcje)
    // a potem dodaj własną sekcję.
    FMenuBuilder MenuBuilder(/*bInShouldCloseWindowAfterMenuSelection*/ true, /*CommandList*/ nullptr);

    // ⚠️ SCommonEditorViewportToolbarBase nie zawsze “samo” wypełnia menu,
    // więc robimy własne sekcje. Jeśli chcesz totalnie standardowe, też się da,
    // ale zależy od wersji. Na start: własne "Debug Draw" jak w PoseSearch.

    MenuBuilder.BeginSection(NAME_None, FText::FromString(TEXT("Debug Draw")));
    {
        TSharedPtr<FCombatAnimSetupViewportClient> Client = ClientWeak.Pin();
        if (Client.IsValid())
        {
            auto AddToggle = [&](const TCHAR* Label, bool& Flag)
                {
                    MenuBuilder.AddMenuEntry(
                        FText::FromString(Label),
                        FText::GetEmpty(),
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

            AddToggle(TEXT("Show Root Motion Preview"), Client->bShowRootMotionPreview);
            AddToggle(TEXT("Show Sampled Trajectories"), Client->bShowSampledTrajectories);
            AddToggle(TEXT("Show Bones"), Client->bShowBones);
            AddToggle(TEXT("Show Collision Shapes"), Client->bShowCollisionShapes);
        }
    }
    MenuBuilder.EndSection();

    return MenuBuilder.MakeWidget();
}