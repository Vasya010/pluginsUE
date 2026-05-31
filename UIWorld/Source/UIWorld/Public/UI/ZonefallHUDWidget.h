#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "ZonefallHUDWidget.generated.h"

class UBorder;
class UImage;
class UProgressBar;
class UTextBlock;
class AZonefallPlayerCharacter;
class AZonefallMinimapCapture;
class AZonefallCharacterPortraitCapture;

/**
 * Fully self-assembling in-game HUD (no Blueprint required), RDR2/GTA-flavoured:
 *  - Rich VITALS CARD on the bottom-RIGHT: live character portrait, health meter, stamina/
 *    Dead-Eye meter, and the equipped weapon + ammo, all in one glass card.
 *  - Polished round minimap on the bottom-LEFT with compass ticks and a rotating player blip.
 */
UCLASS(BlueprintType, Blueprintable)
class UIWORLD_API UZonefallHUDWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UZonefallHUDWidget(const FObjectInitializer& ObjectInitializer);

	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	UFUNCTION(BlueprintCallable, Category = "Zonefall|HUD")
	void BindToCharacter(AZonefallPlayerCharacter* InCharacter);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zonefall|HUD|Style")
	FLinearColor AccentColor = FLinearColor(0.86f, 0.80f, 0.66f, 1.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zonefall|HUD|Style")
	float MinimapSize = 220.0f;

	// Square size of the character portrait inside the vitals card.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zonefall|HUD|Style")
	float PortraitSize = 96.0f;

	// Width of the vitals card body (health/stamina/weapon column).
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zonefall|HUD|Style")
	float VitalsWidth = 260.0f;

	// Player name shown on the vitals card.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zonefall|HUD|Style")
	FText PlayerDisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zonefall|HUD|Style")
	TObjectPtr<class UMaterialInterface> MinimapMaterial;

private:
	void BuildLayout();
	void RefreshMinimapBrush();
	void RefreshPortraitBrush();

	UPROPERTY(Transient) TObjectPtr<AZonefallPlayerCharacter> Character;
	UPROPERTY(Transient) TObjectPtr<AZonefallMinimapCapture> MinimapCapture;
	UPROPERTY(Transient) TObjectPtr<AZonefallCharacterPortraitCapture> PortraitCapture;

	// Vitals card.
	UPROPERTY(Transient) TObjectPtr<UBorder> VitalsPanel;
	UPROPERTY(Transient) TObjectPtr<UImage> PortraitImage;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> PlayerNameText;
	UPROPERTY(Transient) TObjectPtr<UProgressBar> HealthBar;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> HealthText;
	UPROPERTY(Transient) TObjectPtr<UProgressBar> StaminaBar;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> WeaponNameText;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> AmmoText;

	// Minimap.
	UPROPERTY(Transient) TObjectPtr<UImage> MinimapImage;
	UPROPERTY(Transient) TObjectPtr<UImage> MinimapBlip;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> CompassNorthText;

	UPROPERTY(Transient) TObjectPtr<class UMaterialInstanceDynamic> MinimapMID;

	bool bMinimapBrushSet = false;
	bool bPortraitBrushSet = false;
	float HudPulseTime = 0.0f;
};
