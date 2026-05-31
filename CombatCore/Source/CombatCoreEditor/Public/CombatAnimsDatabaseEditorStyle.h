#pragma once
#include "CoreMinimal.h"

class FCombatAnimsDatabaseEditorStyle
{
public:
    static void Initialize();
    static void Shutdown();

    static FName GetStyleSetName();

private:
    static TSharedPtr<class FSlateStyleSet> StyleSet;
};