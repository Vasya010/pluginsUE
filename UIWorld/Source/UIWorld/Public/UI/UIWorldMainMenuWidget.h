#pragma once

#include "CoreMinimal.h"
#include "UI/ZonefallFancyMenuWidget.h"
#include "UIWorldMainMenuWidget.generated.h"

// Dedicated main menu widget class for UIWorld plugin.
// Inherits all existing C++ functionality from ZonefallFancyMenuWidget.
UCLASS(BlueprintType, Blueprintable)
class UIWORLD_API UUIWorldMainMenuWidget : public UZonefallFancyMenuWidget
{
	GENERATED_BODY()
};
