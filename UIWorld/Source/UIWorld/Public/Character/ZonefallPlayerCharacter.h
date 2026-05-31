#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "InputCoreTypes.h"
#include "Character/ZonefallCharacterAppearance.h"
#include "ZonefallPlayerCharacter.generated.h"

class USkeletalMesh;

class USpringArmComponent;
class UCameraComponent;
class UStaticMeshComponent;
class UInputComponent;
class UInputAction;
class UInputMappingContext;
class UUserWidget;
class UAnimMontage;
class UZonefallInventoryComponent;
class UZonefallWeaponInventoryComponent;
class AZonefallWorldItem;
struct FInputActionValue;

UENUM(BlueprintType)
enum class EZonefallCameraView : uint8
{
	ThirdPerson UMETA(DisplayName = "Third Person"),
	FirstPerson UMETA(DisplayName = "First Person")
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FZonefallCameraViewChanged, EZonefallCameraView, NewView);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FZonefallHealthChanged, float, NewHealth, float, MaxHealth);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FZonefallCharacterDied);

/**
 * Fully code-driven main player character for Zonefall.
 *
 *  - Third-person (spring-arm) AND first-person cameras with a one-key toggle.
 *  - Modern Enhanced Input built entirely in C++ — no editor assets required.
 *  - Key bindings are exposed to Config/DefaultInput.ini so they can be re-bound
 *    without recompiling ([/Script/UIWorld.ZonefallPlayerCharacter]).
 *  - Walk / sprint / crouch / jump / interact, gamepad supported out of the box.
 *
 * Drop it in as your GameMode's DefaultPawnClass (or subclass it in Blueprint to
 * assign a skeletal mesh / anim blueprint).
 */
UCLASS(Config = Input, Blueprintable, BlueprintType)
class UIWORLD_API AZonefallPlayerCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	AZonefallPlayerCharacter();

	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;
	virtual void PawnClientRestart() override;
	virtual void PossessedBy(AController* NewController) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	// --- Components ---
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Zonefall|Camera")
	TObjectPtr<USpringArmComponent> CameraBoom;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Zonefall|Camera")
	TObjectPtr<UCameraComponent> ThirdPersonCamera;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Zonefall|Camera")
	TObjectPtr<UCameraComponent> FirstPersonCamera;

	// --- Camera tuning ---
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Zonefall|Camera")
	EZonefallCameraView CameraView = EZonefallCameraView::ThirdPerson;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zonefall|Camera")
	float ThirdPersonArmLength = 320.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zonefall|Camera")
	FVector ThirdPersonSocketOffset = FVector(0.0f, 55.0f, 70.0f);

	// Relative to the capsule; ~eye height so it works even before a mesh is assigned.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zonefall|Camera")
	FVector FirstPersonCameraOffset = FVector(12.0f, 0.0f, 64.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Config, Category = "Zonefall|Camera|Look", meta = (ClampMin = "0.05", ClampMax = "5.0"))
	float LookSensitivity = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Config, Category = "Zonefall|Camera|Look")
	bool bInvertLookY = false;

	// --- Movement tuning ---
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zonefall|Movement", meta = (ClampMin = "10.0"))
	float WalkSpeed = 450.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zonefall|Movement", meta = (ClampMin = "10.0"))
	float SprintSpeed = 820.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zonefall|Movement", meta = (ClampMin = "10.0"))
	float CrouchSpeed = 250.0f;

	// --- Interaction ---
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zonefall|Interaction", meta = (ClampMin = "10.0"))
	float InteractionRange = 280.0f;

	// --- Inventory (replicated for online play) ---
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Zonefall|Inventory")
	TObjectPtr<UZonefallInventoryComponent> Inventory;

	// Widget class used for the in-game inventory. Defaults to the C++ self-assembling one.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zonefall|Inventory|UI")
	TSubclassOf<UUserWidget> InventoryWidgetClass;

	UFUNCTION(BlueprintPure, Category = "Zonefall|Inventory")
	UZonefallInventoryComponent* GetInventory() const { return Inventory; }

	// Requests a drop of a slot (routes to the server). Safe to call from client UI.
	UFUNCTION(BlueprintCallable, Category = "Zonefall|Inventory")
	void RequestDropItem(int32 Index, int32 Count = 1);

	UFUNCTION(BlueprintCallable, Category = "Zonefall|Inventory")
	void ToggleInventoryUI();

	UFUNCTION(BlueprintCallable, Category = "Zonefall|Inventory")
	void CloseInventoryUI();

	// Routes a USE request for an inventory slot to the server.
	UFUNCTION(BlueprintCallable, Category = "Zonefall|Inventory")
	void RequestUseItem(int32 Index);

	// --- Weapons (held in hand, server-authoritative, replicated) ---
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Zonefall|Weapon")
	TObjectPtr<UStaticMeshComponent> WeaponMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Zonefall|Weapon")
	TObjectPtr<UZonefallWeaponInventoryComponent> Weapons;

	UFUNCTION(BlueprintPure, Category = "Zonefall|Weapon")
	UZonefallWeaponInventoryComponent* GetWeapons() const { return Weapons; }

	// Equips a weapon by index (routes to server). -1 = holster.
	UFUNCTION(BlueprintCallable, Category = "Zonefall|Weapon")
	void RequestEquipWeapon(int32 Index);

	// Optional widget class for the radial weapon wheel (defaults to the C++ self-assembling one).
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zonefall|Weapon|UI")
	TSubclassOf<UUserWidget> WeaponWheelWidgetClass;

	UFUNCTION(BlueprintCallable, Category = "Zonefall|Weapon")
	void OpenWeaponWheel();

	UFUNCTION(BlueprintCallable, Category = "Zonefall|Weapon")
	void CloseWeaponWheel(bool bEquipSelection = true);

	// --- Dead Eye (RDR2-style slow motion) ---
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Config, Category = "Zonefall|DeadEye", meta = (ClampMin = "0.05", ClampMax = "1.0"))
	float DeadEyeTimeDilation = 0.35f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zonefall|DeadEye", meta = (ClampMin = "0.05", ClampMax = "2.0"))
	float DeadEyeDrainPerSecond = 0.28f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zonefall|DeadEye", meta = (ClampMin = "0.0", ClampMax = "2.0"))
	float DeadEyeRefillPerSecond = 0.07f;

	// Optional widget class for the Dead Eye meter HUD (defaults to the C++ self-assembling one).
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zonefall|DeadEye|UI")
	TSubclassOf<UUserWidget> DeadEyeWidgetClass;

	UFUNCTION(BlueprintPure, Category = "Zonefall|DeadEye")
	float GetDeadEyeFraction() const { return DeadEyeMeter; }

	UFUNCTION(BlueprintPure, Category = "Zonefall|DeadEye")
	bool IsDeadEyeActive() const { return bDeadEyeActive; }

	UFUNCTION(BlueprintCallable, Category = "Zonefall|DeadEye")
	void StartDeadEye();

	UFUNCTION(BlueprintCallable, Category = "Zonefall|DeadEye")
	void StopDeadEye();

	// Fires the equipped weapon (hitscan). Routes to the server.
	UFUNCTION(BlueprintCallable, Category = "Zonefall|Weapon")
	void FireWeapon();

	// Reloads the equipped weapon from reserve (routes to the server).
	UFUNCTION(BlueprintCallable, Category = "Zonefall|Weapon")
	void ReloadWeapon();

	// Optional anim montage played when firing (assign in a Blueprint subclass).
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zonefall|Weapon|Anim")
	TObjectPtr<UAnimMontage> FireMontage;

	// --- Health / damage / death ---
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zonefall|Health", meta = (ClampMin = "1.0"))
	float MaxHealth = 100.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zonefall|Health", meta = (ClampMin = "0.0"))
	float HealthRegenPerSecond = 4.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zonefall|Health", meta = (ClampMin = "0.0"))
	float HealthRegenDelay = 4.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zonefall|Health", meta = (ClampMin = "0.0"))
	float RespawnDelay = 4.0f;

	UFUNCTION(BlueprintPure, Category = "Zonefall|Health")
	float GetHealth() const { return Health; }

	UFUNCTION(BlueprintPure, Category = "Zonefall|Health")
	float GetMaxHealth() const { return MaxHealth; }

	UFUNCTION(BlueprintPure, Category = "Zonefall|Health")
	float GetHealthFraction() const { return MaxHealth > 0.0f ? FMath::Clamp(Health / MaxHealth, 0.0f, 1.0f) : 0.0f; }

	UFUNCTION(BlueprintPure, Category = "Zonefall|Health")
	bool IsDead() const { return bIsDead; }

	UFUNCTION(BlueprintCallable, Category = "Zonefall|Health")
	void Heal(float Amount);

	virtual float TakeDamage(float DamageAmount, const FDamageEvent& DamageEvent, AController* EventInstigator, AActor* DamageCauser) override;

	UPROPERTY(BlueprintAssignable, Category = "Zonefall|Health")
	FZonefallHealthChanged OnHealthChanged;

	UPROPERTY(BlueprintAssignable, Category = "Zonefall|Health")
	FZonefallCharacterDied OnDied;

	// --- Character creation / appearance ---
	// Optional skeletal meshes selectable as "Body Type" in the creator (index 0 = first option).
	// Leave empty to keep the assigned mesh and only tint/scale it.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zonefall|Appearance")
	TArray<TSoftObjectPtr<USkeletalMesh>> BodyMeshOptions;

	// Optional widget class for the self-assembling character creator (defaults to the C++ one).
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zonefall|Appearance|UI")
	TSubclassOf<UUserWidget> CharacterCreatorWidgetClass;

	UFUNCTION(BlueprintPure, Category = "Zonefall|Appearance")
	FZonefallCharacterAppearance GetAppearance() const { return Appearance; }

	// Applies a look to this character (mesh / scale / material colours). Routes to the server
	// so the change replicates to everyone, and updates the visuals locally right away.
	UFUNCTION(BlueprintCallable, Category = "Zonefall|Appearance")
	void ApplyAppearance(const FZonefallCharacterAppearance& NewAppearance);

	// Opens / closes the self-assembling character-creator UI on this (local) character.
	UFUNCTION(BlueprintCallable, Category = "Zonefall|Appearance")
	void OpenCharacterCreatorUI();

	UFUNCTION(BlueprintCallable, Category = "Zonefall|Appearance")
	void CloseCharacterCreatorUI();

	UFUNCTION(BlueprintPure, Category = "Zonefall|Appearance")
	bool IsOnMenuMap() const;

	UFUNCTION(BlueprintCallable, Category = "Zonefall|Appearance")
	void ToggleCharacterCreatorUI();

	// Console command to open the creator on this possessed pawn (type "ZfCreator" in the ~ console).
	UFUNCTION(Exec)
	void ZfCreator();

	// --- Save / restore (full player state: transform, health, weapons + ammo, inventory) ---
	// Writes the live player state into a save object (call on the server / standalone host).
	UFUNCTION(BlueprintCallable, Category = "Zonefall|Save")
	void CaptureToSaveGame(class UUIWorldSaveGame* Save) const;

	// Restores transform/health/weapons/inventory from a save object (server / standalone only).
	UFUNCTION(BlueprintCallable, Category = "Zonefall|Save")
	void ApplyFromSaveGame(const class UUIWorldSaveGame* Save);

	// --- Aiming (ADS) ---
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zonefall|Aim", meta = (ClampMin = "20.0", ClampMax = "120.0"))
	float AimFOV = 55.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zonefall|Aim", meta = (ClampMin = "40.0", ClampMax = "120.0"))
	float DefaultFOV = 90.0f;

	// Replicated so anim blueprints / remote clients can pose the aim.
	UFUNCTION(BlueprintPure, Category = "Zonefall|Aim")
	bool IsAiming() const { return bIsAiming; }

	// --- HUD ---
	// Optional widget class for the self-assembling HUD (health/ammo/minimap).
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zonefall|HUD")
	TSubclassOf<UUserWidget> HUDWidgetClass;

	// --- Runtime key rebinding (used by the settings UI) ---
	// Stable identifiers for each rebindable action.
	UFUNCTION(BlueprintPure, Category = "Zonefall|Input")
	void GetBindableActions(TArray<FName>& OutActionIds, TArray<FText>& OutDisplayNames) const;

	UFUNCTION(BlueprintPure, Category = "Zonefall|Input")
	FKey GetActionKey(FName ActionId) const;

	// Rebinds an action's key, persists to config and rebuilds the live mappings.
	UFUNCTION(BlueprintCallable, Category = "Zonefall|Input")
	void SetActionKey(FName ActionId, FKey NewKey);

	UFUNCTION(BlueprintCallable, Category = "Zonefall|Input")
	void RebuildInputMappings();

	// --- Config-driven key bindings (editable in DefaultInput.ini, no recompile needed) ---
	UPROPERTY(EditAnywhere, Config, Category = "Zonefall|Input|Keys")
	FKey MoveForwardKey = EKeys::W;

	UPROPERTY(EditAnywhere, Config, Category = "Zonefall|Input|Keys")
	FKey MoveBackKey = EKeys::S;

	UPROPERTY(EditAnywhere, Config, Category = "Zonefall|Input|Keys")
	FKey MoveLeftKey = EKeys::A;

	UPROPERTY(EditAnywhere, Config, Category = "Zonefall|Input|Keys")
	FKey MoveRightKey = EKeys::D;

	UPROPERTY(EditAnywhere, Config, Category = "Zonefall|Input|Keys")
	FKey JumpKey = EKeys::SpaceBar;

	UPROPERTY(EditAnywhere, Config, Category = "Zonefall|Input|Keys")
	FKey SprintKey = EKeys::LeftShift;

	UPROPERTY(EditAnywhere, Config, Category = "Zonefall|Input|Keys")
	FKey CrouchKey = EKeys::LeftControl;

	UPROPERTY(EditAnywhere, Config, Category = "Zonefall|Input|Keys")
	FKey InteractKey = EKeys::E;

	UPROPERTY(EditAnywhere, Config, Category = "Zonefall|Input|Keys")
	FKey ToggleCameraKey = EKeys::V;

	UPROPERTY(EditAnywhere, Config, Category = "Zonefall|Input|Keys")
	FKey DropKey = EKeys::G;

	UPROPERTY(EditAnywhere, Config, Category = "Zonefall|Input|Keys")
	FKey InventoryKey = EKeys::Tab;

	// Opens the pause menu. Default Backspace because Esc ends Play-In-Editor sessions.
	UPROPERTY(EditAnywhere, Config, Category = "Zonefall|Input|Keys")
	FKey PauseKey = EKeys::BackSpace;

	// Hold to open the weapon wheel (slows time while open).
	UPROPERTY(EditAnywhere, Config, Category = "Zonefall|Input|Keys")
	FKey WeaponWheelKey = EKeys::Q;

	// Hold for Dead Eye slow motion.
	UPROPERTY(EditAnywhere, Config, Category = "Zonefall|Input|Keys")
	FKey DeadEyeKey = EKeys::CapsLock;

	// Fire the equipped weapon.
	UPROPERTY(EditAnywhere, Config, Category = "Zonefall|Input|Keys")
	FKey FireKey = EKeys::LeftMouseButton;

	// Aim down sights (hold).
	UPROPERTY(EditAnywhere, Config, Category = "Zonefall|Input|Keys")
	FKey AimKey = EKeys::RightMouseButton;

	// Reload the equipped weapon from reserve ammo.
	UPROPERTY(EditAnywhere, Config, Category = "Zonefall|Input|Keys")
	FKey ReloadKey = EKeys::R;

	// --- Blueprint hooks / events ---
	UPROPERTY(BlueprintAssignable, Category = "Zonefall|Camera")
	FZonefallCameraViewChanged OnCameraViewChanged;

	UFUNCTION(BlueprintImplementableEvent, Category = "Zonefall|Interaction")
	void BP_OnInteract(AActor* Target);

	UFUNCTION(BlueprintCallable, Category = "Zonefall|Camera")
	void SetCameraView(EZonefallCameraView NewView);

	UFUNCTION(BlueprintCallable, Category = "Zonefall|Camera")
	void ToggleCameraView();

	UFUNCTION(BlueprintPure, Category = "Zonefall|Camera")
	bool IsFirstPerson() const { return CameraView == EZonefallCameraView::FirstPerson; }

	UFUNCTION(BlueprintCallable, Category = "Zonefall|Interaction")
	AActor* TraceInteractable() const;

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	// Enhanced Input handlers.
	void Input_Move(const FInputActionValue& Value);
	void Input_Look(const FInputActionValue& Value);
	void Input_JumpStarted(const FInputActionValue& Value);
	void Input_JumpCompleted(const FInputActionValue& Value);
	void Input_SprintStarted(const FInputActionValue& Value);
	void Input_SprintCompleted(const FInputActionValue& Value);
	void Input_ToggleCrouch(const FInputActionValue& Value);
	void Input_Interact(const FInputActionValue& Value);
	void Input_ToggleCamera(const FInputActionValue& Value);
	void Input_Drop(const FInputActionValue& Value);
	void Input_ToggleInventory(const FInputActionValue& Value);
	void Input_OpenPause(const FInputActionValue& Value);
	void Input_WeaponWheelStarted(const FInputActionValue& Value);
	void Input_WeaponWheelCompleted(const FInputActionValue& Value);
	void Input_DeadEyeStarted(const FInputActionValue& Value);
	void Input_DeadEyeCompleted(const FInputActionValue& Value);
	void Input_Fire(const FInputActionValue& Value);
	void Input_Reload(const FInputActionValue& Value);
	void Input_AimStarted(const FInputActionValue& Value);
	void Input_AimCompleted(const FInputActionValue& Value);

	// Server RPCs (inventory + weapons are server-authoritative).
	UFUNCTION(Server, Reliable)
	void Server_PickupItem(AZonefallWorldItem* WorldItem);

	UFUNCTION(Server, Reliable)
	void Server_DropItem(int32 Index, int32 Count);

	UFUNCTION(Server, Reliable)
	void Server_UseItem(int32 Index);

	UFUNCTION(Server, Reliable)
	void Server_EquipWeapon(int32 Index);

	UFUNCTION(Server, Reliable)
	void Server_FireWeapon(FVector TraceStart, FVector TraceEnd);

	UFUNCTION(NetMulticast, Unreliable)
	void Multicast_PlayFireMontage();

	// Draws a visible tracer/impact for the shot on every client.
	UFUNCTION(NetMulticast, Unreliable)
	void Multicast_FireFX(FVector Start, FVector End, bool bHit);

	/** Click / dry-fire feedback when the magazine and reserve are empty. */
	UFUNCTION(NetMulticast, Unreliable)
	void Multicast_DryFireFX();

	UFUNCTION(Server, Reliable)
	void Server_SetAiming(bool bNewAiming);

	UFUNCTION(Server, Reliable)
	void Server_Reload();

	UFUNCTION(NetMulticast, Unreliable)
	void Multicast_ReloadFX();

	UFUNCTION()
	void HandleEquippedWeaponChanged(int32 EquippedIndex);

	void ApplyEquippedWeapon();

	// Appearance internals.
	UFUNCTION(Server, Reliable)
	void Server_SetAppearance(const FZonefallCharacterAppearance& NewAppearance);

	UFUNCTION()
	void OnRep_Appearance();

	// Pure-visual application (mesh swap / scale / material colours). Safe on any machine.
	void ApplyAppearanceVisuals(const FZonefallCharacterAppearance& In);

	// Health / death internals.
	void Die();
	void Respawn();
	void ApplyAimFOV(bool bAiming);

	UFUNCTION()
	void OnRep_Health();

	UFUNCTION()
	void OnRep_IsAiming();

private:
	void EnsureInputAssets();
	void RebuildKeyMappings();
	FKey* FindActionKeyByIdInternal(FName ActionId);
	void AddMappingContextForOwner();
	void ApplyCameraView();

	// On gameplay maps: closes any leftover menu UI and forces game input focus so the
	// keyboard reaches the pawn. On the menu map it deliberately does nothing.
	void EnsureGameplayInputFocus();
	void EnsureLocalHUD();
	void InitializeOnlineGameplay();
	bool BindOnlineGameInstanceDelegates();

	UFUNCTION()
	void HandleOnlineMatchReady(UWorld* World);

	UPROPERTY(Transient) TObjectPtr<UInputMappingContext> InputMapping;
	UPROPERTY(Transient) TObjectPtr<UInputAction> IA_Move;
	UPROPERTY(Transient) TObjectPtr<UInputAction> IA_Look;
	UPROPERTY(Transient) TObjectPtr<UInputAction> IA_Jump;
	UPROPERTY(Transient) TObjectPtr<UInputAction> IA_Sprint;
	UPROPERTY(Transient) TObjectPtr<UInputAction> IA_Crouch;
	UPROPERTY(Transient) TObjectPtr<UInputAction> IA_Interact;
	UPROPERTY(Transient) TObjectPtr<UInputAction> IA_ToggleCamera;
	UPROPERTY(Transient) TObjectPtr<UInputAction> IA_Drop;
	UPROPERTY(Transient) TObjectPtr<UInputAction> IA_Inventory;
	UPROPERTY(Transient) TObjectPtr<UInputAction> IA_Pause;
	UPROPERTY(Transient) TObjectPtr<UInputAction> IA_WeaponWheel;
	UPROPERTY(Transient) TObjectPtr<UInputAction> IA_DeadEye;
	UPROPERTY(Transient) TObjectPtr<UInputAction> IA_Fire;
	UPROPERTY(Transient) TObjectPtr<UInputAction> IA_Aim;
	UPROPERTY(Transient) TObjectPtr<UInputAction> IA_Reload;

	UPROPERTY(Transient) TObjectPtr<UUserWidget> InventoryWidgetInstance;
	UPROPERTY(Transient) TObjectPtr<UUserWidget> WeaponWheelWidgetInstance;
	UPROPERTY(Transient) TObjectPtr<UUserWidget> DeadEyeWidgetInstance;
	UPROPERTY(Transient) TObjectPtr<UUserWidget> HUDWidgetInstance;
	UPROPERTY(Transient) TObjectPtr<UUserWidget> CharacterCreatorWidgetInstance;

	// Replicated cosmetic look (so remote players see it). Applied via OnRep + on authority.
	UPROPERTY(ReplicatedUsing = OnRep_Appearance)
	FZonefallCharacterAppearance Appearance;

	bool bCharacterCreatorOpen = false;

	// --- Health / aim replicated state ---
	UPROPERTY(ReplicatedUsing = OnRep_Health)
	float Health = 100.0f;

	UPROPERTY(Replicated)
	bool bIsDead = false;

	UPROPERTY(ReplicatedUsing = OnRep_IsAiming)
	bool bIsAiming = false;

	FTimerHandle RespawnTimerHandle;
	float TimeSinceDamage = 1000.0f;

	bool bSprinting = false;
	bool bInventoryOpen = false;
	bool bWeaponWheelOpen = false;

	// Dead Eye runtime state.
	bool bDeadEyeActive = false;
	float DeadEyeMeter = 1.0f; // 0..1
};
