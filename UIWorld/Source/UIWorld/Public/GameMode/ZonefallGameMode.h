#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "ZonefallGameMode.generated.h"

/**
 * Default gameplay GameMode for Zonefall.
 *
 * Spawns AZonefallPlayerCharacter (third/first-person, Enhanced Input, replicated
 * inventory) as the default pawn so the hero works out of the box. Seamless travel
 * is enabled for clean online map transitions.
 *
 * Wired via Config/DefaultEngine.ini -> GlobalDefaultGameMode = /Script/UIWorld.ZonefallGameMode.
 */
UCLASS()
class UIWORLD_API AZonefallGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	AZonefallGameMode();
};
