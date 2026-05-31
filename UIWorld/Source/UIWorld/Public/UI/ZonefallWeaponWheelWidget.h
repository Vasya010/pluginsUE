#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/Button.h"
#include "ZonefallWeaponWheelWidget.generated.h"

class UBorder;
class UButton;
class UCanvasPanel;
class UTextBlock;
class AZonefallPlayerCharacter;
class UZonefallWeaponInventoryComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnZonefallWeaponSlotClicked, int32, WeaponIndex);

/** A UButton that remembers its weapon index and re-broadcasts clicks with it. */
UCLASS()
class UIWORLD_API UZonefallWeaponWheelButton : public UButton
{
	GENERATED_BODY()

public:
	UZonefallWeaponWheelButton();

	UPROPERTY(Transient)
	int32 WeaponIndex = INDEX_NONE;

	UPROPERTY(BlueprintAssignable)
	FOnZonefallWeaponSlotClicked OnWeaponSlotClicked;

private:
	UFUNCTION()
	void HandleInternalClicked();
};

/**
 * Self-assembling RDR2-style radial weapon wheel. Reads the character's weapon store and
 * lays the weapons out around a circle. Click a slot to select; the character equips the
 * selection when the wheel closes. No Blueprint required.
 */
UCLASS(BlueprintType, Blueprintable)
class UIWORLD_API UZonefallWeaponWheelWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UZonefallWeaponWheelWidget(const FObjectInitializer& ObjectInitializer);

	virtual TSharedRef<SWidget> RebuildWidget() override;

	UFUNCTION(BlueprintCallable, Category = "Zonefall|Weapon")
	void SetupForCharacter(AZonefallPlayerCharacter* InCharacter);

	UFUNCTION(BlueprintCallable, Category = "Zonefall|Weapon")
	void RefreshFromWeapons();

	UFUNCTION(BlueprintPure, Category = "Zonefall|Weapon")
	int32 GetSelectedWeaponIndex() const { return SelectedIndex; }

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zonefall|Weapon|Style")
	FLinearColor AccentColor = FLinearColor(0.95f, 0.82f, 0.35f, 1.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zonefall|Weapon|Style")
	float WheelRadius = 210.0f;

private:
	void BuildLayout();
	void UpdateSelectionVisuals();

	UFUNCTION()
	void HandleWeaponSlotClicked(int32 WeaponIndex);

	UPROPERTY(Transient) TObjectPtr<AZonefallPlayerCharacter> Character;
	UPROPERTY(Transient) TObjectPtr<UZonefallWeaponInventoryComponent> WeaponInv;
	UPROPERTY(Transient) TObjectPtr<UCanvasPanel> WheelCanvas;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> CenterLabel;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> CenterSlotLabel;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> CenterAmmoLabel;
	UPROPERTY(Transient) TArray<TObjectPtr<UZonefallWeaponWheelButton>> SlotButtons;
	UPROPERTY(Transient) TArray<TObjectPtr<UBorder>> SlotBorders;

	int32 SelectedIndex = INDEX_NONE;
};
