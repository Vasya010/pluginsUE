
#pragma once

#include "CoreMinimal.h"
#include "SCommonEditorViewportToolbarBase.h"

class SCombatAnimSetupViewport;
class FCombatAnimSetupViewportClient;

class SCombatAnimSetupViewportToolbar : public SCommonEditorViewportToolbarBase
{
public:
    SLATE_BEGIN_ARGS(SCombatAnimSetupViewportToolbar) {}
        SLATE_ARGUMENT(TSharedPtr<ICommonEditorViewportToolbarInfoProvider>, InfoProvider)
        SLATE_ARGUMENT(TSharedPtr<FCombatAnimSetupViewportClient>, ViewportClient)
    SLATE_END_ARGS()

    void Construct(const FArguments& InArgs);

protected:
    // ✅ pozwala dołożyć własne entry do “Show” (ikonka oka)
    virtual TSharedRef<SWidget> GenerateShowMenu() const override;

private:
    TWeakPtr<FCombatAnimSetupViewportClient> ClientWeak;
};