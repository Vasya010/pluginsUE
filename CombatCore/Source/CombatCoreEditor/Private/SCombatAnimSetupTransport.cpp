#include "SCombatAnimSetupTransport.h"

#include "CombatAnimSetupViewportClient.h"

#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SSlider.h"
#include "Widgets/Input/SCheckBox.h"
#include "Widgets/Layout/SBox.h"
//#include "Widgets/Layout/SHorizontalBox.h"
#include "Widgets/Text/STextBlock.h"

static FText FormatTime(float Seconds)
{
    Seconds = FMath::Max(0.f, Seconds);
    const int32 TotalMs = FMath::RoundToInt(Seconds * 1000.f);
    const int32 TotalSec = TotalMs / 1000;
    const int32 Ms = TotalMs % 1000;
    const int32 Min = TotalSec / 60;
    const int32 Sec = TotalSec % 60;

    return FText::FromString(FString::Printf(TEXT("%02d:%02d.%03d"), Min, Sec, Ms));
}

void SCombatAnimSetupTransport::Construct(const FArguments& InArgs)
{
    ClientWeak = InArgs._ViewportClient;

    ChildSlot
        [
            SNew(SHorizontalBox)

                // Play/Pause
                + SHorizontalBox::Slot()
                .AutoWidth()
                .Padding(2.f)
                [
                    SNew(SButton)
                        .OnClicked(this, &SCombatAnimSetupTransport::OnPlayPauseClicked)
                        [
                            SNew(STextBlock)
                                .Text(this, &SCombatAnimSetupTransport::GetPlayPauseText)
                        ]
                ]

            // Reset transform (przy RM bardzo przydatne)
            + SHorizontalBox::Slot()
                .AutoWidth()
                .Padding(2.f)
                [
                    SNew(SButton)
                        .OnClicked(this, &SCombatAnimSetupTransport::OnResetClicked)
                        [
                            SNew(STextBlock)
                                .Text(FText::FromString(TEXT("Reset")))
                        ]
                ]

            // Loop checkbox
            + SHorizontalBox::Slot()
                .AutoWidth()
                .Padding(8.f, 2.f)
                .VAlign(VAlign_Center)
                [
                    SNew(SCheckBox)
                        .IsChecked(this, &SCombatAnimSetupTransport::GetLoopState)
                        .OnCheckStateChanged(this, &SCombatAnimSetupTransport::OnLoopChanged)
                        [
                            SNew(STextBlock).Text(FText::FromString(TEXT("Loop")))
                        ]
                ]

            // Slider
            + SHorizontalBox::Slot()
                .FillWidth(1.f)
                .Padding(8.f, 2.f)
                .VAlign(VAlign_Center)
                [
                    SNew(SSlider)
                        .Value(this, &SCombatAnimSetupTransport::GetScrubValue)
                        .OnValueChanged(this, &SCombatAnimSetupTransport::OnScrubValueChanged)
                        .OnMouseCaptureBegin(this, &SCombatAnimSetupTransport::OnScrubMouseCaptureBegin)
                        .OnMouseCaptureEnd(this, &SCombatAnimSetupTransport::OnScrubMouseCaptureEnd)
                ]

            // Time text
            + SHorizontalBox::Slot()
                .AutoWidth()
                .Padding(6.f, 2.f)
                .VAlign(VAlign_Center)
                [
                    SNew(STextBlock)
                        .Text(this, &SCombatAnimSetupTransport::GetTimeText)
                ]
        ];
}

FReply SCombatAnimSetupTransport::OnPlayPauseClicked()
{
    if (TSharedPtr<FCombatAnimSetupViewportClient> Client = ClientWeak.Pin())
    {
        Client->SetPlaying(!Client->IsPlaying());
    }
    return FReply::Handled();
}

FReply SCombatAnimSetupTransport::OnResetClicked()
{
    if (TSharedPtr<FCombatAnimSetupViewportClient> Client = ClientWeak.Pin())
    {
        Client->ResetPreviewTransform();
        // opcjonalnie: wróć na 0
        Client->SetCurrentTime(0.f, false);
    }
    return FReply::Handled();
}

void SCombatAnimSetupTransport::OnLoopChanged(ECheckBoxState NewState)
{
    if (TSharedPtr<FCombatAnimSetupViewportClient> Client = ClientWeak.Pin())
    {
        Client->SetLooping(NewState == ECheckBoxState::Checked);
    }
}

void SCombatAnimSetupTransport::OnScrubMouseCaptureBegin()
{
    bIsScrubbing = true;
    if (TSharedPtr<FCombatAnimSetupViewportClient> Client = ClientWeak.Pin())
    {
        Client->SetRealtime(true);
        Client->SetPlaying(false);
    }
}

void SCombatAnimSetupTransport::OnScrubMouseCaptureEnd()
{
    bIsScrubbing = false;
}

void SCombatAnimSetupTransport::OnScrubValueChanged(float InValue)
{
    if (TSharedPtr<FCombatAnimSetupViewportClient> Client = ClientWeak.Pin())
    {
        const float Len = Client->GetSequenceLength();
        if (Len > 0.f)
        {
            const float NewTime = FMath::Clamp(InValue, 0.f, 1.f) * Len;
            Client->SetCurrentTime(NewTime, false);

            // ✅ natychmiastowy redraw w trakcie drag
            Client->Invalidate();
        }
    }
}

FText SCombatAnimSetupTransport::GetPlayPauseText() const
{
    if (TSharedPtr<FCombatAnimSetupViewportClient> Client = ClientWeak.Pin())
    {
        return Client->IsPlaying() ? FText::FromString(TEXT("Pause")) : FText::FromString(TEXT("Play"));
    }
    return FText::FromString(TEXT("Play"));
}

ECheckBoxState SCombatAnimSetupTransport::GetLoopState() const
{
    if (TSharedPtr<FCombatAnimSetupViewportClient> Client = ClientWeak.Pin())
    {
        return Client->IsLooping() ? ECheckBoxState::Checked : ECheckBoxState::Unchecked;
        return ECheckBoxState::Unchecked;
    }
    return ECheckBoxState::Checked;
}

float SCombatAnimSetupTransport::GetScrubValue() const
{
    if (TSharedPtr<FCombatAnimSetupViewportClient> Client = ClientWeak.Pin())
    {
        const float Len = Client->GetSequenceLength();
        if (Len > 0.f)
        {
            return FMath::Clamp(Client->GetCurrentTime() / Len, 0.f, 1.f);
        }
    }
    return 0.f;
}

FText SCombatAnimSetupTransport::GetTimeText() const
{
    if (TSharedPtr<FCombatAnimSetupViewportClient> Client = ClientWeak.Pin())
    {
        const float Len = Client->GetSequenceLength();
        const float T = Client->GetCurrentTime();
        return FText::Format(
            FText::FromString(TEXT("{0} / {1}")),
            FormatTime(T),
            FormatTime(Len)
        );
    }
    return FText::FromString(TEXT("--:--.--- / --:--.---"));
}