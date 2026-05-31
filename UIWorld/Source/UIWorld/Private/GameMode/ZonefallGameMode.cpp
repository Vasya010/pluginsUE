#include "GameMode/ZonefallGameMode.h"

#include "Character/ZonefallPlayerCharacter.h"

AZonefallGameMode::AZonefallGameMode()
{
	DefaultPawnClass = AZonefallPlayerCharacter::StaticClass();

	// Seamless travel breaks Steam listen-server lobby connect URLs for joining clients.
	bUseSeamlessTravel = false;
}
