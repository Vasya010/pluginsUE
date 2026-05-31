

#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"

class FCombatAnimSetupViewportClient;

class SCombatAnimSetupTransport : public SCompoundWidget
{
public:
    SLATE_BEGIN_ARGS(SCombatAnimSetupTransport) {}
        SLATE_ARGUMENT(TWeakPtr<FCombatAnimSetupViewportClient>, ViewportClient)
    SLATE_END_ARGS()

    void Construct(const FArguments& InArgs);

private:
    // UI callbacks
    FReply OnPlayPauseClicked();
    FReply OnResetClicked();
    void OnScrubValueChanged(float InValue);
    void OnScrubMouseCaptureBegin();
    void OnScrubMouseCaptureEnd();
    void OnLoopChanged(ECheckBoxState NewState);

    // UI getters
    FText GetPlayPauseText() const;
    float GetScrubValue() const;
    FText GetTimeText() const;
    ECheckBoxState GetLoopState() const;

    float GetLength() const;
    float GetTime() const;

private:
    TWeakPtr<FCombatAnimSetupViewportClient> ClientWeak;

    bool bIsScrubbing = false;
};

