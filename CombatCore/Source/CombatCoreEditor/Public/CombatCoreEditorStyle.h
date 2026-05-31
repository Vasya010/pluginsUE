#pragma once
#include "CoreMinimal.h"

class FCombatCoreEditorStyle
{
public:
    static void Initialize();
    static void Shutdown();

    static FName GetStyleSetName();

private:
    static TSharedPtr<class FSlateStyleSet> StyleSet;
};