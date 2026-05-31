#pragma once

#include "CoreMinimal.h"
#include "ZonefallCharacterAppearance.generated.h"

/**
 * Full cosmetic description of a created character. Drives the skeletal mesh choice (from the
 * character's BodyMeshOptions), a gentle height/build scale, and dynamic-material colour params
 * (SkinTone / HairColor / PrimaryColor / SecondaryColor / scalar FaceType / HairStyle).
 *
 * Every field carries SaveGame so the struct can be embedded directly in the save file, and is
 * replicated as a whole on the character so remote players see the look.
 */
USTRUCT(BlueprintType)
struct FZonefallCharacterAppearance
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category = "Zonefall|Appearance")
	FString CharacterName = TEXT("Drifter");

	// Index into AZonefallPlayerCharacter::BodyMeshOptions (0 = keep the default mesh).
	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category = "Zonefall|Appearance")
	int32 BodyType = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category = "Zonefall|Appearance")
	int32 FaceType = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category = "Zonefall|Appearance")
	int32 HairStyle = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category = "Zonefall|Appearance")
	FLinearColor SkinTone = FLinearColor(0.80f, 0.62f, 0.49f, 1.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category = "Zonefall|Appearance")
	FLinearColor HairColor = FLinearColor(0.12f, 0.08f, 0.05f, 1.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category = "Zonefall|Appearance")
	FLinearColor PrimaryColor = FLinearColor(0.18f, 0.22f, 0.30f, 1.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category = "Zonefall|Appearance")
	FLinearColor SecondaryColor = FLinearColor(0.45f, 0.32f, 0.18f, 1.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category = "Zonefall|Appearance", meta = (ClampMin = "0.88", ClampMax = "1.12"))
	float Height = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category = "Zonefall|Appearance", meta = (ClampMin = "0.85", ClampMax = "1.20"))
	float Build = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category = "Zonefall|Appearance")
	uint8 VoiceType = 0;

	// True once the player has actually committed a character in the creator.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category = "Zonefall|Appearance")
	bool bCreated = false;
};
