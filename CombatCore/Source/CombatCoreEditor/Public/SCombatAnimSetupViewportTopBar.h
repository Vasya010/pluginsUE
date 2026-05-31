
#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "SCommonEditorViewportToolbarBase.h"

class FCombatAnimSetupViewportClient;

class SCombatAnimSetupViewportTopBar : public SCompoundWidget
{
public:
    SLATE_BEGIN_ARGS(SCombatAnimSetupViewportTopBar) {}
        SLATE_ARGUMENT(TWeakPtr<FCombatAnimSetupViewportClient>, ViewportClient)
    SLATE_END_ARGS()

    void Construct(const FArguments& InArgs);

private:
    TSharedRef<SWidget> BuildEyeMenu() const;

    // View mode handlers
    void OnViewModeChanged(TSharedPtr<FString> NewSelection, ESelectInfo::Type);
    TSharedRef<SWidget> MakeViewModeWidget(TSharedPtr<FString> Item) const;
    FText GetViewModeText() const;

    // Perspective handlers (opcjonalne)
    void OnProjectionChanged(TSharedPtr<FString> NewSelection, ESelectInfo::Type);
    TSharedRef<SWidget> MakeProjectionWidget(TSharedPtr<FString> Item) const;
    FText GetProjectionText() const;

private:
    TWeakPtr<FCombatAnimSetupViewportClient> ClientWeak;

    TArray<TSharedPtr<FString>> ViewModes;
    TSharedPtr<FString> SelectedViewMode;

    TArray<TSharedPtr<FString>> Projections;
    TSharedPtr<FString> SelectedProjection;
};