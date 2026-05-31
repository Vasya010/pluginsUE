#include "Character/ZonefallPlayerCharacter.h"

#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Engine/OverlapResult.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerStart.h"
#include "GameFramework/SpringArmComponent.h"

#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputAction.h"
#include "InputActionValue.h"
#include "InputMappingContext.h"
#include "InputModifiers.h"

#include "Blueprint/UserWidget.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/OverlapResult.h"
#include "GameFramework/DamageType.h"
#include "Animation/AnimMontage.h"
#include "Animation/AnimInstance.h"
#include "DrawDebugHelpers.h"
#include "Net/UnrealNetwork.h"
#include "TimerManager.h"
#include "Framework/Application/SlateApplication.h"
#include "Kismet/GameplayStatics.h"
#include "Inventory/ZonefallInventoryComponent.h"
#include "Inventory/ZonefallWorldItem.h"
#include "Weapons/ZonefallWeaponInventoryComponent.h"
#include "UI/ZonefallInventoryWidget.h"
#include "UI/ZonefallWeaponWheelWidget.h"
#include "UI/ZonefallDeadEyeWidget.h"
#include "UI/ZonefallHUDWidget.h"
#include "UI/ZonefallCharacterCreatorWidget.h"
#include "World/ZonefallMinimapCapture.h"
#include "World/ZonefallCharacterPortraitCapture.h"
#include "World/ZonefallCharacterPreviewCapture.h"
#include "Engine/SkeletalMesh.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "UIWorldMenuGameInstance.h"
#include "Save/UIWorldSaveGame.h"
#include "Save/UIWorldSaveManager.h"

DEFINE_LOG_CATEGORY_STATIC(LogZonefallPlayerCharacter, Log, All);

AZonefallPlayerCharacter::AZonefallPlayerCharacter()
{
	// Ticks for the Dead Eye meter drain/refill.
	PrimaryActorTick.bCanEverTick = true;

	// The character is steered by the camera/control rotation, not raw controller yaw,
	// so it can strafe in first person and orient-to-movement in third person.
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	// --- Third-person boom + camera ---
	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(GetCapsuleComponent());
	CameraBoom->TargetArmLength = ThirdPersonArmLength;
	CameraBoom->SocketOffset = ThirdPersonSocketOffset;
	CameraBoom->bUsePawnControlRotation = true;
	CameraBoom->bEnableCameraLag = true;
	CameraBoom->CameraLagSpeed = 12.0f;
	CameraBoom->bEnableCameraRotationLag = true;
	CameraBoom->CameraRotationLagSpeed = 18.0f;

	ThirdPersonCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("ThirdPersonCamera"));
	ThirdPersonCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	ThirdPersonCamera->bUsePawnControlRotation = false;

	// --- First-person camera (attached to capsule at eye height) ---
	FirstPersonCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FirstPersonCamera"));
	FirstPersonCamera->SetupAttachment(GetCapsuleComponent());
	FirstPersonCamera->SetRelativeLocation(FirstPersonCameraOffset);
	FirstPersonCamera->bUsePawnControlRotation = true;
	FirstPersonCamera->SetActive(false);

	// --- Movement defaults ---
	if (UCharacterMovementComponent* Move = GetCharacterMovement())
	{
		Move->bOrientRotationToMovement = true;
		Move->RotationRate = FRotator(0.0f, 560.0f, 0.0f);
		Move->JumpZVelocity = 520.0f;
		Move->AirControl = 0.35f;
		Move->MaxWalkSpeed = WalkSpeed;
		Move->MaxWalkSpeedCrouched = CrouchSpeed;
		Move->MinAnalogWalkSpeed = 20.0f;
		Move->BrakingDecelerationWalking = 2000.0f;
		Move->NavAgentProps.bCanCrouch = true;
	}

	if (UCapsuleComponent* Capsule = GetCapsuleComponent())
	{
		Capsule->InitCapsuleSize(38.0f, 90.0f);
	}

	// Replicated, server-authoritative inventory.
	Inventory = CreateDefaultSubobject<UZonefallInventoryComponent>(TEXT("Inventory"));

	// Replicated weapon store (what the character owns + which is equipped).
	Weapons = CreateDefaultSubobject<UZonefallWeaponInventoryComponent>(TEXT("Weapons"));

	// Weapon held in hand — attached to the mesh hand socket, mesh set on equip.
	WeaponMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("WeaponMesh"));
	WeaponMesh->SetupAttachment(GetMesh(), TEXT("hand_r"));
	WeaponMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	WeaponMesh->SetCastShadow(true);
	WeaponMesh->SetVisibility(false);

	// Character replicates by default; make sure movement does too for online play.
	bReplicates = true;
	SetReplicateMovement(true);

	Health = MaxHealth;
}

void AZonefallPlayerCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AZonefallPlayerCharacter, Health);
	DOREPLIFETIME(AZonefallPlayerCharacter, bIsDead);
	DOREPLIFETIME(AZonefallPlayerCharacter, bIsAiming);
	DOREPLIFETIME(AZonefallPlayerCharacter, Appearance);
}

void AZonefallPlayerCharacter::BeginPlay()
{
	Super::BeginPlay();

	if (UCharacterMovementComponent* Move = GetCharacterMovement())
	{
		Move->MaxWalkSpeed = WalkSpeed;
		Move->MaxWalkSpeedCrouched = CrouchSpeed;
	}

	ApplyCameraView();

	// Attach the held-weapon mesh to the actual mesh once it exists, and react to equip changes.
	if (Weapons)
	{
		Weapons->OnEquippedChanged.AddDynamic(this, &AZonefallPlayerCharacter::HandleEquippedWeaponChanged);
	}
	ApplyEquippedWeapon();

	DeadEyeMeter = 1.0f;

	if (HasAuthority())
	{
		Health = MaxHealth;

		// If the player chose "Continue" in the main menu, the game instance flagged a restore.
		// We loaded into the saved level; now re-apply the saved player snapshot. Weapons were
		// already seeded in component BeginPlay above (via Super) — RestoreWeapons overwrites them.
		if (UUIWorldMenuGameInstance* GI = GetGameInstance<UUIWorldMenuGameInstance>())
		{
			if (GI->ConsumeSaveRestoreRequest())
			{
				UUIWorldSaveManager* SaveManager = NewObject<UUIWorldSaveManager>(this);
				if (UUIWorldSaveGame* Save = SaveManager->LoadSave())
				{
					ApplyFromSaveGame(Save); // includes the saved appearance
				}
			}
			else if (GI->HasCurrentAppearance())
			{
				// Carry the character created/edited in the creator into gameplay.
				ApplyAppearance(GI->GetCurrentAppearance());
			}
		}
	}

	BindOnlineGameInstanceDelegates();

	if (IsLocallyControlled())
	{
		InitializeOnlineGameplay();
	}
}

bool AZonefallPlayerCharacter::BindOnlineGameInstanceDelegates()
{
	if (UUIWorldMenuGameInstance* GI = GetGameInstance<UUIWorldMenuGameInstance>())
	{
		GI->OnOnlineMatchReady.AddUniqueDynamic(this, &AZonefallPlayerCharacter::HandleOnlineMatchReady);
		return true;
	}
	return false;
}

void AZonefallPlayerCharacter::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);
	if (IsLocallyControlled())
	{
		InitializeOnlineGameplay();
	}
}

void AZonefallPlayerCharacter::HandleOnlineMatchReady(UWorld* World)
{
	if (!World || World != GetWorld() || !IsLocallyControlled())
	{
		return;
	}
	InitializeOnlineGameplay();
}

void AZonefallPlayerCharacter::CaptureToSaveGame(UUIWorldSaveGame* Save) const
{
	if (!Save)
	{
		return;
	}

	Save->bHasPlayerState = true;
	Save->PlayerLocation = GetActorLocation();
	Save->PlayerRotation = GetActorRotation();
	Save->Health = Health;
	Save->MaxHealth = MaxHealth;

	Save->Weapons.Reset();
	Save->EquippedWeaponIndex = INDEX_NONE;
	if (Weapons)
	{
		Save->EquippedWeaponIndex = Weapons->GetEquippedIndex();
		for (const FZonefallWeaponItem& W : Weapons->GetWeapons())
		{
			FUIWorldSavedWeapon S;
			S.WeaponId = W.WeaponId;
			S.DisplayName = W.DisplayName.ToString();
			S.Slot = static_cast<uint8>(W.Slot);
			S.WeaponMeshPath = W.WeaponMesh.ToSoftObjectPath().ToString();
			S.AttachSocket = W.AttachSocket;
			S.RelativeLocation = W.RelativeLocation;
			S.RelativeRotation = W.RelativeRotation;
			S.RelativeScale = W.RelativeScale;
			S.AmmoInClip = W.AmmoInClip;
			S.AmmoReserve = W.AmmoReserve;
			S.ClipSize = W.ClipSize;
			S.Damage = W.Damage;
			S.Range = W.Range;
			Save->Weapons.Add(S);
		}
	}

	Save->Items.Reset();
	if (Inventory)
	{
		for (const FZonefallInventoryItem& It : Inventory->GetItems())
		{
			FUIWorldSavedItem S;
			S.ItemId = It.ItemId;
			S.Category = static_cast<uint8>(It.Category);
			S.Description = It.Description.ToString();
			S.bConsumable = It.bConsumable;
			S.DisplayName = It.DisplayName.ToString();
			S.Quantity = It.Quantity;
			S.MaxStack = It.MaxStack;
			S.IconPath = It.Icon.ToSoftObjectPath().ToString();
			S.PickupClassPath = It.PickupClass.ToSoftObjectPath().ToString();
			Save->Items.Add(S);
		}
	}

	Save->bHasAppearance = Appearance.bCreated;
	Save->Appearance = Appearance;
}

void AZonefallPlayerCharacter::ApplyFromSaveGame(const UUIWorldSaveGame* Save)
{
	if (!Save || !Save->bHasPlayerState || !HasAuthority())
	{
		return;
	}

	// Transform (teleport so physics/movement don't fight the placement).
	SetActorLocationAndRotation(Save->PlayerLocation, Save->PlayerRotation, false, nullptr, ETeleportType::TeleportPhysics);
	if (AController* C = GetController())
	{
		C->SetControlRotation(Save->PlayerRotation);
	}

	// Health.
	MaxHealth = Save->MaxHealth > 0.0f ? Save->MaxHealth : MaxHealth;
	Health = FMath::Clamp(Save->Health, 0.0f, MaxHealth);
	bIsDead = Health <= 0.0f;
	OnHealthChanged.Broadcast(Health, MaxHealth);

	// Weapons (overwrites the seeded default loadout).
	if (Weapons)
	{
		TArray<FZonefallWeaponItem> Restored;
		Restored.Reserve(Save->Weapons.Num());
		for (const FUIWorldSavedWeapon& S : Save->Weapons)
		{
			FZonefallWeaponItem W;
			W.WeaponId = S.WeaponId;
			W.DisplayName = FText::FromString(S.DisplayName);
			W.Slot = static_cast<EZonefallWeaponSlot>(S.Slot);
			if (!S.WeaponMeshPath.IsEmpty())
			{
				W.WeaponMesh = TSoftObjectPtr<UStaticMesh>(FSoftObjectPath(S.WeaponMeshPath));
			}
			W.AttachSocket = S.AttachSocket;
			W.RelativeLocation = S.RelativeLocation;
			W.RelativeRotation = S.RelativeRotation;
			W.RelativeScale = S.RelativeScale;
			W.AmmoInClip = S.AmmoInClip;
			W.AmmoReserve = S.AmmoReserve;
			W.ClipSize = S.ClipSize;
			W.Damage = S.Damage;
			W.Range = S.Range;
			Restored.Add(W);
		}
		Weapons->RestoreWeapons(Restored, Save->EquippedWeaponIndex);
	}

	// Inventory (the items we picked up).
	if (Inventory)
	{
		TArray<FZonefallInventoryItem> Restored;
		Restored.Reserve(Save->Items.Num());
		for (const FUIWorldSavedItem& S : Save->Items)
		{
			FZonefallInventoryItem It;
			It.ItemId = S.ItemId;
			It.Category = static_cast<EZonefallItemCategory>(S.Category);
			It.Description = FText::FromString(S.Description);
			It.bConsumable = S.bConsumable;
			It.DisplayName = FText::FromString(S.DisplayName);
			It.Quantity = S.Quantity;
			It.MaxStack = S.MaxStack;
			if (!S.IconPath.IsEmpty())
			{
				It.Icon = TSoftObjectPtr<UTexture2D>(FSoftObjectPath(S.IconPath));
			}
			if (!S.PickupClassPath.IsEmpty())
			{
				It.PickupClass = TSoftClassPtr<AZonefallWorldItem>(FSoftObjectPath(S.PickupClassPath));
			}
			Restored.Add(It);
		}
		Inventory->RestoreItems(Restored);
	}

	// Appearance.
	if (Save->bHasAppearance)
	{
		ApplyAppearance(Save->Appearance);
	}
}

void AZonefallPlayerCharacter::ApplyAppearance(const FZonefallCharacterAppearance& NewAppearance)
{
	// Apply the visuals locally right away for responsiveness, then make it authoritative so
	// it replicates to everyone (the local pawn on a host/standalone already has authority).
	ApplyAppearanceVisuals(NewAppearance);

	if (HasAuthority())
	{
		Appearance = NewAppearance;
	}
	else
	{
		Server_SetAppearance(NewAppearance);
	}
}

void AZonefallPlayerCharacter::Server_SetAppearance_Implementation(const FZonefallCharacterAppearance& NewAppearance)
{
	Appearance = NewAppearance;
	ApplyAppearanceVisuals(NewAppearance);
}

void AZonefallPlayerCharacter::OnRep_Appearance()
{
	ApplyAppearanceVisuals(Appearance);
}

void AZonefallPlayerCharacter::ApplyAppearanceVisuals(const FZonefallCharacterAppearance& In)
{
	USkeletalMeshComponent* MeshComp = GetMesh();
	if (!MeshComp)
	{
		return;
	}

	// Body type -> skeletal mesh (only if options were configured).
	if (BodyMeshOptions.IsValidIndex(In.BodyType))
	{
		if (USkeletalMesh* SK = BodyMeshOptions[In.BodyType].LoadSynchronous())
		{
			if (MeshComp->GetSkeletalMeshAsset() != SK)
			{
				MeshComp->SetSkeletalMeshAsset(SK);
			}
		}
	}

	// Gentle height / build scale.
	const float H = FMath::Clamp(In.Height, 0.88f, 1.12f);
	const float B = FMath::Clamp(In.Build, 0.85f, 1.20f);
	MeshComp->SetRelativeScale3D(FVector(B, B, H));

	// Colour parameters on every material slot. Setting a parameter that the material doesn't
	// expose is a harmless no-op, so this works with any character material that opts in.
	const int32 NumMats = MeshComp->GetNumMaterials();
	for (int32 i = 0; i < NumMats; ++i)
	{
		UMaterialInstanceDynamic* MID = MeshComp->CreateAndSetMaterialInstanceDynamic(i);
		if (!MID)
		{
			continue;
		}
		MID->SetVectorParameterValue(TEXT("SkinTone"), In.SkinTone);
		MID->SetVectorParameterValue(TEXT("HairColor"), In.HairColor);
		MID->SetVectorParameterValue(TEXT("PrimaryColor"), In.PrimaryColor);
		MID->SetVectorParameterValue(TEXT("SecondaryColor"), In.SecondaryColor);
		MID->SetVectorParameterValue(TEXT("Tint"), In.PrimaryColor);
		MID->SetScalarParameterValue(TEXT("FaceType"), static_cast<float>(In.FaceType));
		MID->SetScalarParameterValue(TEXT("HairStyle"), static_cast<float>(In.HairStyle));
	}
}

void AZonefallPlayerCharacter::OpenCharacterCreatorUI()
{
	APlayerController* PC = Cast<APlayerController>(GetController());
	if (!PC || !PC->IsLocalController())
	{
		return;
	}

	// Spawn the full-body preview capture so the creator card shows a live model.
	AZonefallCharacterPreviewCapture* PreviewCapture = nullptr;
	if (UWorld* World = GetWorld())
	{
		PreviewCapture = AZonefallCharacterPreviewCapture::Get(World);
		if (!PreviewCapture)
		{
			FActorSpawnParameters Params;
			Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
			PreviewCapture = World->SpawnActor<AZonefallCharacterPreviewCapture>(
				AZonefallCharacterPreviewCapture::StaticClass(), GetActorTransform(), Params);
		}
		if (PreviewCapture)
		{
			PreviewCapture->SetTrackedActor(this);
		}
	}

	if (USkeletalMeshComponent* MeshComp = GetMesh())
	{
		MeshComp->SetVisibility(true, true);
		MeshComp->SetHiddenInGame(false);
	}

	if (!CharacterCreatorWidgetInstance)
	{
		TSubclassOf<UUserWidget> WidgetClass = CharacterCreatorWidgetClass;
		if (!WidgetClass)
		{
			if (UUIWorldMenuGameInstance* GI = GetGameInstance<UUIWorldMenuGameInstance>())
			{
				WidgetClass = GI->CharacterCreatorWidgetClass;
			}
		}
		if (!WidgetClass)
		{
			WidgetClass = TSubclassOf<UUserWidget>(UZonefallCharacterCreatorWidget::StaticClass());
		}
		CharacterCreatorWidgetInstance = CreateWidget<UUserWidget>(PC, WidgetClass);
		if (UZonefallCharacterCreatorWidget* Creator = Cast<UZonefallCharacterCreatorWidget>(CharacterCreatorWidgetInstance))
		{
			Creator->SetupForCharacter(this);
		}
	}
	if (CharacterCreatorWidgetInstance && !CharacterCreatorWidgetInstance->IsInViewport())
	{
		CharacterCreatorWidgetInstance->AddToViewport(50);
	}

	bCharacterCreatorOpen = true;

	FInputModeUIOnly Mode;
	if (CharacterCreatorWidgetInstance)
	{
		Mode.SetWidgetToFocus(CharacterCreatorWidgetInstance->TakeWidget());
	}
	Mode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	PC->SetInputMode(Mode);
	PC->SetShowMouseCursor(true);
}

void AZonefallPlayerCharacter::CloseCharacterCreatorUI()
{
	if (CharacterCreatorWidgetInstance)
	{
		CharacterCreatorWidgetInstance->RemoveFromParent();
		CharacterCreatorWidgetInstance = nullptr;
	}
	bCharacterCreatorOpen = false;

	if (APlayerController* PC = Cast<APlayerController>(GetController()))
	{
		PC->SetInputMode(FInputModeGameOnly());
		PC->SetShowMouseCursor(false);
	}
}

void AZonefallPlayerCharacter::ToggleCharacterCreatorUI()
{
	if (bCharacterCreatorOpen)
	{
		CloseCharacterCreatorUI();
	}
	else
	{
		OpenCharacterCreatorUI();
	}
}

void AZonefallPlayerCharacter::ZfCreator()
{
	ToggleCharacterCreatorUI();
}

void AZonefallPlayerCharacter::EnsureLocalHUD()
{
	APlayerController* PC = Cast<APlayerController>(GetController());
	if (!PC || !PC->IsLocalController() || IsOnMenuMap())
	{
		return;
	}

	if (!HUDWidgetInstance)
	{
		const TSubclassOf<UUserWidget> WidgetClass = HUDWidgetClass
			? HUDWidgetClass
			: TSubclassOf<UUserWidget>(UZonefallHUDWidget::StaticClass());
		HUDWidgetInstance = CreateWidget<UUserWidget>(PC, WidgetClass);
		if (UZonefallHUDWidget* HUD = Cast<UZonefallHUDWidget>(HUDWidgetInstance))
		{
			HUD->BindToCharacter(this);
		}
	}
	if (HUDWidgetInstance && !HUDWidgetInstance->IsInViewport())
	{
		HUDWidgetInstance->AddToViewport(5);
	}
}

void AZonefallPlayerCharacter::InitializeOnlineGameplay()
{
	EnsureGameplayInputFocus();

	// "Create Character" flow: show the creator on this pawn instead of the gameplay HUD.
	bool bShowCreator = false;
	if (UUIWorldMenuGameInstance* GI = GetGameInstance<UUIWorldMenuGameInstance>())
	{
		bShowCreator = GI->ConsumeShowCharacterCreatorRequest();
	}

	if (!bShowCreator)
	{
		EnsureLocalHUD();
	}

	if (UWorld* World = GetWorld())
	{
		FActorSpawnParameters Params;
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

		if (!AZonefallMinimapCapture::Get(World))
		{
			World->SpawnActor<AZonefallMinimapCapture>(AZonefallMinimapCapture::StaticClass(), GetActorTransform(), Params);
		}

		if (!AZonefallCharacterPortraitCapture::Get(World))
		{
			World->SpawnActor<AZonefallCharacterPortraitCapture>(AZonefallCharacterPortraitCapture::StaticClass(), GetActorTransform(), Params);
		}
	}

	if (bShowCreator)
	{
		OpenCharacterCreatorUI();
	}
}

void AZonefallPlayerCharacter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	// While Dead Eye is active the pawn's CustomTimeDilation cancels the global slow-mo, so
	// DeltaSeconds here is ~real time — fine for draining/refilling the meter at a real rate.
	if (bDeadEyeActive)
	{
		DeadEyeMeter = FMath::Max(0.0f, DeadEyeMeter - DeadEyeDrainPerSecond * DeltaSeconds);
		if (DeadEyeMeter <= 0.0f)
		{
			StopDeadEye();
		}
	}
	else if (DeadEyeMeter < 1.0f)
	{
		DeadEyeMeter = FMath::Min(1.0f, DeadEyeMeter + DeadEyeRefillPerSecond * DeltaSeconds);
	}

	// Server-side health regeneration after a short delay without taking damage.
	if (HasAuthority() && !bIsDead && HealthRegenPerSecond > 0.0f && Health < MaxHealth)
	{
		TimeSinceDamage += DeltaSeconds;
		if (TimeSinceDamage >= HealthRegenDelay)
		{
			const float Before = Health;
			Health = FMath::Min(MaxHealth, Health + HealthRegenPerSecond * DeltaSeconds);
			if (!FMath::IsNearlyEqual(Before, Health))
			{
				OnHealthChanged.Broadcast(Health, MaxHealth);
			}
		}
	}
}

void AZonefallPlayerCharacter::EnsureInputAssets()
{
	if (InputMapping)
	{
		return;
	}

	auto MakeAction = [this](const TCHAR* Name, EInputActionValueType ValueType)
	{
		UInputAction* Action = NewObject<UInputAction>(this, Name);
		Action->ValueType = ValueType;
		return Action;
	};

	IA_Move = MakeAction(TEXT("IA_Move"), EInputActionValueType::Axis2D);
	IA_Look = MakeAction(TEXT("IA_Look"), EInputActionValueType::Axis2D);
	IA_Jump = MakeAction(TEXT("IA_Jump"), EInputActionValueType::Boolean);
	IA_Sprint = MakeAction(TEXT("IA_Sprint"), EInputActionValueType::Boolean);
	IA_Crouch = MakeAction(TEXT("IA_Crouch"), EInputActionValueType::Boolean);
	IA_Interact = MakeAction(TEXT("IA_Interact"), EInputActionValueType::Boolean);
	IA_ToggleCamera = MakeAction(TEXT("IA_ToggleCamera"), EInputActionValueType::Boolean);
	IA_Drop = MakeAction(TEXT("IA_Drop"), EInputActionValueType::Boolean);
	IA_Inventory = MakeAction(TEXT("IA_Inventory"), EInputActionValueType::Boolean);
	IA_Pause = MakeAction(TEXT("IA_Pause"), EInputActionValueType::Boolean);
	IA_WeaponWheel = MakeAction(TEXT("IA_WeaponWheel"), EInputActionValueType::Boolean);
	IA_DeadEye = MakeAction(TEXT("IA_DeadEye"), EInputActionValueType::Boolean);
	IA_Fire = MakeAction(TEXT("IA_Fire"), EInputActionValueType::Boolean);
	IA_Aim = MakeAction(TEXT("IA_Aim"), EInputActionValueType::Boolean);
	IA_Reload = MakeAction(TEXT("IA_Reload"), EInputActionValueType::Boolean);

	InputMapping = NewObject<UInputMappingContext>(this, TEXT("ZonefallPlayerMappingContext"));

	RebuildKeyMappings();
}

void AZonefallPlayerCharacter::RebuildKeyMappings()
{
	if (!InputMapping)
	{
		return;
	}

	// Start from a clean slate so rebinding doesn't accumulate stale mappings.
	InputMapping->UnmapAll();

	// Helper: map a 1D/2D key to a 2D action, optionally swizzling its value onto Y
	// (for forward/back & vertical stick) and/or negating it (for back/left/down).
	auto MapAxis = [this](UInputAction* Action, const FKey& Key, bool bSwizzleToY, bool bNegate)
	{
		if (!Action || !Key.IsValid())
		{
			return;
		}
		FEnhancedActionKeyMapping& Mapping = InputMapping->MapKey(Action, Key);
		if (bSwizzleToY)
		{
			UInputModifierSwizzleAxis* Swizzle = NewObject<UInputModifierSwizzleAxis>(InputMapping);
			Swizzle->Order = EInputAxisSwizzle::YXZ;
			Mapping.Modifiers.Add(Swizzle);
		}
		if (bNegate)
		{
			Mapping.Modifiers.Add(NewObject<UInputModifierNegate>(InputMapping));
		}
	};

	// If a configured key is empty (e.g. a Blueprint subclass left it unset), fall back
	// to a sane default so controls are NEVER dead.
	auto Resolve = [](const FKey& Configured, const FKey& Fallback) { return Configured.IsValid() ? Configured : Fallback; };

	// Move — keyboard (WASD) maps onto a 2D axis.
	MapAxis(IA_Move, Resolve(MoveForwardKey, EKeys::W), /*swizzle*/ true, /*negate*/ false);   // +Y
	MapAxis(IA_Move, Resolve(MoveBackKey, EKeys::S), true, true);                              // -Y
	MapAxis(IA_Move, Resolve(MoveRightKey, EKeys::D), false, false);                          // +X
	MapAxis(IA_Move, Resolve(MoveLeftKey, EKeys::A), false, true);                            // -X
	// Move — gamepad left stick.
	MapAxis(IA_Move, EKeys::Gamepad_LeftX, false, false);                  // +X
	MapAxis(IA_Move, EKeys::Gamepad_LeftY, true, false);                   // +Y

	// Look — mouse delta is already a 2D axis; gamepad right stick mapped per-axis.
	MapAxis(IA_Look, EKeys::Mouse2D, false, false);
	MapAxis(IA_Look, EKeys::Gamepad_RightX, false, false);
	MapAxis(IA_Look, EKeys::Gamepad_RightY, true, false);

	// Boolean actions — keyboard + gamepad.
	auto MapButton = [this](UInputAction* Action, const FKey& Key)
	{
		if (Action && Key.IsValid())
		{
			InputMapping->MapKey(Action, Key);
		}
	};

	MapButton(IA_Jump, Resolve(JumpKey, EKeys::SpaceBar));
	MapButton(IA_Jump, EKeys::Gamepad_FaceButton_Bottom);
	MapButton(IA_Sprint, Resolve(SprintKey, EKeys::LeftShift));
	MapButton(IA_Sprint, EKeys::Gamepad_LeftShoulder);
	MapButton(IA_Crouch, Resolve(CrouchKey, EKeys::LeftControl));
	MapButton(IA_Crouch, EKeys::Gamepad_FaceButton_Right);
	MapButton(IA_Interact, Resolve(InteractKey, EKeys::E));
	MapButton(IA_Interact, EKeys::Gamepad_FaceButton_Left);
	MapButton(IA_ToggleCamera, Resolve(ToggleCameraKey, EKeys::V));
	MapButton(IA_ToggleCamera, EKeys::Gamepad_FaceButton_Top);
	MapButton(IA_Drop, Resolve(DropKey, EKeys::G));
	MapButton(IA_Drop, EKeys::Gamepad_DPad_Down);
	MapButton(IA_Inventory, Resolve(InventoryKey, EKeys::Tab));
	MapButton(IA_Inventory, EKeys::I); // Reliable fallback (Tab can be intercepted in PIE).
	MapButton(IA_Inventory, EKeys::Gamepad_Special_Right);
	MapButton(IA_Pause, Resolve(PauseKey, EKeys::BackSpace));
	MapButton(IA_Pause, EKeys::Gamepad_Special_Left);

	// Weapon wheel — hold (keyboard + gamepad left bumper as a common "wheel" button).
	MapButton(IA_WeaponWheel, Resolve(WeaponWheelKey, EKeys::Q));
	MapButton(IA_WeaponWheel, EKeys::Gamepad_DPad_Up);

	// Dead Eye slow-mo — hold (right thumbstick click is the RDR2 default).
	MapButton(IA_DeadEye, Resolve(DeadEyeKey, EKeys::CapsLock));
	MapButton(IA_DeadEye, EKeys::Gamepad_RightThumbstick);

	// Fire — LMB + right trigger.
	MapButton(IA_Fire, Resolve(FireKey, EKeys::LeftMouseButton));
	MapButton(IA_Fire, EKeys::Gamepad_RightTrigger);

	// Aim down sights — RMB + left trigger (hold).
	MapButton(IA_Aim, Resolve(AimKey, EKeys::RightMouseButton));
	MapButton(IA_Aim, EKeys::Gamepad_LeftTrigger);

	// Reload — R + gamepad West (X) button.
	MapButton(IA_Reload, Resolve(ReloadKey, EKeys::R));
	MapButton(IA_Reload, EKeys::Gamepad_FaceButton_Left);
}

FKey* AZonefallPlayerCharacter::FindActionKeyByIdInternal(FName ActionId)
{
	if (ActionId == TEXT("MoveForward")) return &MoveForwardKey;
	if (ActionId == TEXT("MoveBack"))    return &MoveBackKey;
	if (ActionId == TEXT("MoveLeft"))    return &MoveLeftKey;
	if (ActionId == TEXT("MoveRight"))   return &MoveRightKey;
	if (ActionId == TEXT("Jump"))        return &JumpKey;
	if (ActionId == TEXT("Sprint"))      return &SprintKey;
	if (ActionId == TEXT("Crouch"))      return &CrouchKey;
	if (ActionId == TEXT("Interact"))    return &InteractKey;
	if (ActionId == TEXT("ToggleCamera"))return &ToggleCameraKey;
	if (ActionId == TEXT("Drop"))        return &DropKey;
	if (ActionId == TEXT("Inventory"))   return &InventoryKey;
	if (ActionId == TEXT("Fire"))        return &FireKey;
	if (ActionId == TEXT("Aim"))         return &AimKey;
	if (ActionId == TEXT("Reload"))      return &ReloadKey;
	if (ActionId == TEXT("WeaponWheel")) return &WeaponWheelKey;
	if (ActionId == TEXT("DeadEye"))     return &DeadEyeKey;
	if (ActionId == TEXT("Pause"))       return &PauseKey;
	return nullptr;
}

void AZonefallPlayerCharacter::GetBindableActions(TArray<FName>& OutActionIds, TArray<FText>& OutDisplayNames) const
{
	OutActionIds.Reset();
	OutDisplayNames.Reset();

	auto Add = [&](const TCHAR* Id, const FText& Name)
	{
		OutActionIds.Add(FName(Id));
		OutDisplayNames.Add(Name);
	};

	Add(TEXT("MoveForward"),  NSLOCTEXT("ZonefallControls", "MoveForward", "Move Forward"));
	Add(TEXT("MoveBack"),     NSLOCTEXT("ZonefallControls", "MoveBack", "Move Backward"));
	Add(TEXT("MoveLeft"),     NSLOCTEXT("ZonefallControls", "MoveLeft", "Move Left"));
	Add(TEXT("MoveRight"),    NSLOCTEXT("ZonefallControls", "MoveRight", "Move Right"));
	Add(TEXT("Jump"),         NSLOCTEXT("ZonefallControls", "Jump", "Jump"));
	Add(TEXT("Sprint"),       NSLOCTEXT("ZonefallControls", "Sprint", "Sprint"));
	Add(TEXT("Crouch"),       NSLOCTEXT("ZonefallControls", "Crouch", "Crouch"));
	Add(TEXT("Interact"),     NSLOCTEXT("ZonefallControls", "Interact", "Interact / Pick Up"));
	Add(TEXT("ToggleCamera"), NSLOCTEXT("ZonefallControls", "ToggleCamera", "Toggle Camera"));
	Add(TEXT("Fire"),         NSLOCTEXT("ZonefallControls", "Fire", "Fire"));
	Add(TEXT("Aim"),          NSLOCTEXT("ZonefallControls", "Aim", "Aim (ADS)"));
	Add(TEXT("Reload"),       NSLOCTEXT("ZonefallControls", "Reload", "Reload"));
	Add(TEXT("WeaponWheel"),  NSLOCTEXT("ZonefallControls", "WeaponWheel", "Weapon Wheel"));
	Add(TEXT("DeadEye"),      NSLOCTEXT("ZonefallControls", "DeadEye", "Dead Eye"));
	Add(TEXT("Drop"),         NSLOCTEXT("ZonefallControls", "Drop", "Drop Item"));
	Add(TEXT("Inventory"),    NSLOCTEXT("ZonefallControls", "Inventory", "Inventory"));
	Add(TEXT("Pause"),        NSLOCTEXT("ZonefallControls", "Pause", "Pause Menu"));
}

FKey AZonefallPlayerCharacter::GetActionKey(FName ActionId) const
{
	const FKey* Found = const_cast<AZonefallPlayerCharacter*>(this)->FindActionKeyByIdInternal(ActionId);
	return Found ? *Found : FKey();
}

void AZonefallPlayerCharacter::SetActionKey(FName ActionId, FKey NewKey)
{
	FKey* Slot = FindActionKeyByIdInternal(ActionId);
	if (!Slot)
	{
		return;
	}

	*Slot = NewKey;
	SaveConfig(); // Persists to DefaultInput.ini section for this class.
	RebuildInputMappings();
}

void AZonefallPlayerCharacter::RebuildInputMappings()
{
	EnsureInputAssets();
	RebuildKeyMappings();

	// Push the change live so it takes effect immediately.
	if (const APlayerController* PC = Cast<APlayerController>(GetController()))
	{
		if (ULocalPlayer* LocalPlayer = PC->GetLocalPlayer())
		{
			if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(LocalPlayer))
			{
				Subsystem->RequestRebuildControlMappings();
			}
		}
	}
}

void AZonefallPlayerCharacter::AddMappingContextForOwner()
{
	EnsureInputAssets();

	const APlayerController* PC = Cast<APlayerController>(GetController());
	if (!PC)
	{
		return;
	}

	if (ULocalPlayer* LocalPlayer = PC->GetLocalPlayer())
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(LocalPlayer))
		{
			Subsystem->AddMappingContext(InputMapping, /*Priority*/ 0);
		}
	}
}

void AZonefallPlayerCharacter::PawnClientRestart()
{
	Super::PawnClientRestart();
	AddMappingContextForOwner();
	InitializeOnlineGameplay();
}

bool AZonefallPlayerCharacter::IsOnMenuMap() const
{
	const UWorld* World = GetWorld();
	if (!World)
	{
		return false;
	}

	FString MenuMap;
	if (const UUIWorldMenuGameInstance* GI = GetGameInstance<UUIWorldMenuGameInstance>())
	{
		MenuMap = GI->MainMenuLevelName.ToString();
	}
	if (MenuMap.IsEmpty())
	{
		MenuMap = TEXT("Menu");
	}

	// GetMapName returns e.g. "UEDPIE_0_Menu" in PIE — substring match is robust here.
	return World->GetMapName().Contains(MenuMap);
}

void AZonefallPlayerCharacter::EnsureGameplayInputFocus()
{
	// On the menu map, leave input/menu alone so the main menu stays usable.
	if (IsOnMenuMap())
	{
		return;
	}

	// Close any menu widget left over from the menu map (it survives non-seamless travel
	// and would otherwise keep keyboard focus — camera turns but WASD is swallowed).
	if (UUIWorldMenuGameInstance* GI = GetGameInstance<UUIWorldMenuGameInstance>())
	{
		GI->CloseMenuUI(true);
	}

	if (APlayerController* PC = Cast<APlayerController>(GetController()))
	{
		if (PC->IsLocalController())
		{
			PC->SetInputMode(FInputModeGameOnly());
			PC->SetShowMouseCursor(false);
			if (FSlateApplication::IsInitialized())
			{
				FSlateApplication::Get().SetAllUserFocusToGameViewport();
			}
		}
	}
}

void AZonefallPlayerCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	EnsureInputAssets();

	if (UEnhancedInputComponent* EIC = Cast<UEnhancedInputComponent>(PlayerInputComponent))
	{
		EIC->BindAction(IA_Move, ETriggerEvent::Triggered, this, &AZonefallPlayerCharacter::Input_Move);
		EIC->BindAction(IA_Look, ETriggerEvent::Triggered, this, &AZonefallPlayerCharacter::Input_Look);
		EIC->BindAction(IA_Jump, ETriggerEvent::Started, this, &AZonefallPlayerCharacter::Input_JumpStarted);
		EIC->BindAction(IA_Jump, ETriggerEvent::Completed, this, &AZonefallPlayerCharacter::Input_JumpCompleted);
		EIC->BindAction(IA_Sprint, ETriggerEvent::Started, this, &AZonefallPlayerCharacter::Input_SprintStarted);
		EIC->BindAction(IA_Sprint, ETriggerEvent::Completed, this, &AZonefallPlayerCharacter::Input_SprintCompleted);
		EIC->BindAction(IA_Crouch, ETriggerEvent::Started, this, &AZonefallPlayerCharacter::Input_ToggleCrouch);
		EIC->BindAction(IA_Interact, ETriggerEvent::Started, this, &AZonefallPlayerCharacter::Input_Interact);
		EIC->BindAction(IA_ToggleCamera, ETriggerEvent::Started, this, &AZonefallPlayerCharacter::Input_ToggleCamera);
		EIC->BindAction(IA_Drop, ETriggerEvent::Started, this, &AZonefallPlayerCharacter::Input_Drop);
		EIC->BindAction(IA_Inventory, ETriggerEvent::Started, this, &AZonefallPlayerCharacter::Input_ToggleInventory);
		EIC->BindAction(IA_Pause, ETriggerEvent::Started, this, &AZonefallPlayerCharacter::Input_OpenPause);
		EIC->BindAction(IA_WeaponWheel, ETriggerEvent::Started, this, &AZonefallPlayerCharacter::Input_WeaponWheelStarted);
		EIC->BindAction(IA_WeaponWheel, ETriggerEvent::Completed, this, &AZonefallPlayerCharacter::Input_WeaponWheelCompleted);
		EIC->BindAction(IA_DeadEye, ETriggerEvent::Started, this, &AZonefallPlayerCharacter::Input_DeadEyeStarted);
		EIC->BindAction(IA_DeadEye, ETriggerEvent::Completed, this, &AZonefallPlayerCharacter::Input_DeadEyeCompleted);
		EIC->BindAction(IA_Fire, ETriggerEvent::Started, this, &AZonefallPlayerCharacter::Input_Fire);
		EIC->BindAction(IA_Reload, ETriggerEvent::Started, this, &AZonefallPlayerCharacter::Input_Reload);
		EIC->BindAction(IA_Aim, ETriggerEvent::Started, this, &AZonefallPlayerCharacter::Input_AimStarted);
		EIC->BindAction(IA_Aim, ETriggerEvent::Completed, this, &AZonefallPlayerCharacter::Input_AimCompleted);
	}
	else
	{
		UE_LOG(LogZonefallPlayerCharacter, Warning,
			TEXT("SetupPlayerInputComponent: InputComponent is not a UEnhancedInputComponent. Ensure DefaultInputComponentClass = EnhancedInputComponent."));
	}

	// Covers standalone/listen-server where SetupPlayerInputComponent may run before PawnClientRestart.
	AddMappingContextForOwner();
}

void AZonefallPlayerCharacter::Input_Move(const FInputActionValue& Value)
{
	const FVector2D Axis = Value.Get<FVector2D>();
	if (!Controller || Axis.IsNearlyZero())
	{
		return;
	}

	const FRotator YawRotation(0.0f, Controller->GetControlRotation().Yaw, 0.0f);
	const FVector Forward = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
	const FVector Right = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

	AddMovementInput(Forward, Axis.Y);
	AddMovementInput(Right, Axis.X);
}

void AZonefallPlayerCharacter::Input_Look(const FInputActionValue& Value)
{
	const FVector2D Axis = Value.Get<FVector2D>();
	const float PitchSign = bInvertLookY ? 1.0f : -1.0f;

	AddControllerYawInput(Axis.X * LookSensitivity);
	AddControllerPitchInput(Axis.Y * LookSensitivity * PitchSign);
}

void AZonefallPlayerCharacter::Input_JumpStarted(const FInputActionValue& /*Value*/)
{
	Jump();
}

void AZonefallPlayerCharacter::Input_JumpCompleted(const FInputActionValue& /*Value*/)
{
	StopJumping();
}

void AZonefallPlayerCharacter::Input_SprintStarted(const FInputActionValue& /*Value*/)
{
	bSprinting = true;
	if (UCharacterMovementComponent* Move = GetCharacterMovement())
	{
		Move->MaxWalkSpeed = SprintSpeed;
	}
}

void AZonefallPlayerCharacter::Input_SprintCompleted(const FInputActionValue& /*Value*/)
{
	bSprinting = false;
	if (UCharacterMovementComponent* Move = GetCharacterMovement())
	{
		Move->MaxWalkSpeed = WalkSpeed;
	}
}

void AZonefallPlayerCharacter::Input_ToggleCrouch(const FInputActionValue& /*Value*/)
{
	if (bIsCrouched)
	{
		UnCrouch();
	}
	else
	{
		Crouch();
	}
}

void AZonefallPlayerCharacter::Input_Interact(const FInputActionValue& /*Value*/)
{
	AActor* Target = TraceInteractable();
	if (!Target)
	{
		return;
	}

	// World items are picked up server-side via RPC; everything else is a Blueprint hook.
	if (AZonefallWorldItem* WorldItem = Cast<AZonefallWorldItem>(Target))
	{
		Server_PickupItem(WorldItem);
		return;
	}

	BP_OnInteract(Target);
}

void AZonefallPlayerCharacter::Input_Drop(const FInputActionValue& /*Value*/)
{
	// Quick-drop the first inventory slot; the widget offers per-slot dropping.
	if (Inventory && Inventory->GetNumItems() > 0)
	{
		RequestDropItem(0, 1);
	}
}

void AZonefallPlayerCharacter::Input_ToggleInventory(const FInputActionValue& /*Value*/)
{
	ToggleInventoryUI();
}

void AZonefallPlayerCharacter::Input_OpenPause(const FInputActionValue& /*Value*/)
{
	APlayerController* PC = Cast<APlayerController>(GetController());
	if (!PC || !PC->IsLocalController())
	{
		return;
	}

	if (UUIWorldMenuGameInstance* GI = GetGameInstance<UUIWorldMenuGameInstance>())
	{
		// Pause and show the self-assembling pause menu (RESUME / SAVE / SETTINGS / MAIN MENU / QUIT).
		UGameplayStatics::SetGamePaused(this, true);
		GI->ShowMenuFromList(EUIWorldMenuScreen::PauseMenu, false);
	}
}

void AZonefallPlayerCharacter::RequestDropItem(int32 Index, int32 Count)
{
	// Executes locally when called on the server, otherwise sends to the server.
	Server_DropItem(Index, Count);
}

void AZonefallPlayerCharacter::Server_PickupItem_Implementation(AZonefallWorldItem* WorldItem)
{
	if (WorldItem && Inventory)
	{
		WorldItem->TryGiveTo(Inventory);
	}
}

void AZonefallPlayerCharacter::Server_DropItem_Implementation(int32 Index, int32 Count)
{
	if (Inventory)
	{
		Inventory->DropItemAt(Index, Count);
	}
}

void AZonefallPlayerCharacter::RequestUseItem(int32 Index)
{
	Server_UseItem(Index);
}

void AZonefallPlayerCharacter::Server_UseItem_Implementation(int32 Index)
{
	if (Inventory)
	{
		Inventory->UseItemAt(Index);
	}
}

void AZonefallPlayerCharacter::CloseInventoryUI()
{
	if (bInventoryOpen)
	{
		ToggleInventoryUI(); // toggles to closed (restores game input).
	}
}

// ---------------------------------------------------------------------------
// Weapons — equip / hold in hand
// ---------------------------------------------------------------------------
void AZonefallPlayerCharacter::RequestEquipWeapon(int32 Index)
{
	Server_EquipWeapon(Index);
}

void AZonefallPlayerCharacter::Server_EquipWeapon_Implementation(int32 Index)
{
	if (Weapons)
	{
		Weapons->EquipIndex(Index);
	}
}

void AZonefallPlayerCharacter::HandleEquippedWeaponChanged(int32 /*EquippedIndex*/)
{
	ApplyEquippedWeapon();
}

void AZonefallPlayerCharacter::ApplyEquippedWeapon()
{
	if (!WeaponMesh || !Weapons)
	{
		return;
	}

	if (!Weapons->HasEquippedWeapon())
	{
		WeaponMesh->SetVisibility(false);
		WeaponMesh->SetStaticMesh(nullptr);
		return;
	}

	const FZonefallWeaponItem W = Weapons->GetEquippedWeapon();

	UStaticMesh* MeshAsset = W.WeaponMesh.IsNull() ? nullptr : W.WeaponMesh.LoadSynchronous();
	WeaponMesh->SetStaticMesh(MeshAsset);

	// Attach to the requested hand socket (falls back to the mesh root if the socket is missing).
	USkeletalMeshComponent* OwnerMesh = GetMesh();
	const FName Socket = W.AttachSocket.IsNone() ? FName(TEXT("hand_r")) : W.AttachSocket;
	if (OwnerMesh)
	{
		WeaponMesh->AttachToComponent(OwnerMesh, FAttachmentTransformRules::SnapToTargetNotIncludingScale, Socket);
	}
	WeaponMesh->SetRelativeLocation(W.RelativeLocation);
	WeaponMesh->SetRelativeRotation(W.RelativeRotation);
	WeaponMesh->SetRelativeScale3D(W.RelativeScale.IsNearlyZero() ? FVector(1.0f) : W.RelativeScale);
	WeaponMesh->SetVisibility(MeshAsset != nullptr);
}

// ---------------------------------------------------------------------------
// Weapon wheel
// ---------------------------------------------------------------------------
void AZonefallPlayerCharacter::Input_WeaponWheelStarted(const FInputActionValue& /*Value*/)
{
	OpenWeaponWheel();
}

void AZonefallPlayerCharacter::Input_WeaponWheelCompleted(const FInputActionValue& /*Value*/)
{
	CloseWeaponWheel(/*bEquipSelection*/ true);
}

void AZonefallPlayerCharacter::OpenWeaponWheel()
{
	APlayerController* PC = Cast<APlayerController>(GetController());
	if (!PC || !PC->IsLocalController() || bWeaponWheelOpen)
	{
		return;
	}
	bWeaponWheelOpen = true;

	if (!WeaponWheelWidgetInstance)
	{
		const TSubclassOf<UUserWidget> WidgetClass = WeaponWheelWidgetClass
			? WeaponWheelWidgetClass
			: TSubclassOf<UUserWidget>(UZonefallWeaponWheelWidget::StaticClass());
		WeaponWheelWidgetInstance = CreateWidget<UUserWidget>(PC, WidgetClass);
		if (UZonefallWeaponWheelWidget* Wheel = Cast<UZonefallWeaponWheelWidget>(WeaponWheelWidgetInstance))
		{
			Wheel->SetupForCharacter(this);
		}
	}

	if (WeaponWheelWidgetInstance && !WeaponWheelWidgetInstance->IsInViewport())
	{
		WeaponWheelWidgetInstance->AddToViewport(60);
	}
	if (UZonefallWeaponWheelWidget* Wheel = Cast<UZonefallWeaponWheelWidget>(WeaponWheelWidgetInstance))
	{
		Wheel->RefreshFromWeapons();
		Wheel->SetVisibility(ESlateVisibility::Visible);
	}

	// RDR2-style: time slows while the wheel is open.
	UGameplayStatics::SetGlobalTimeDilation(this, 0.25f);
	PC->SetShowMouseCursor(true);
	FInputModeGameAndUI Mode;
	Mode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	Mode.SetHideCursorDuringCapture(false);
	PC->SetInputMode(Mode);
}

void AZonefallPlayerCharacter::CloseWeaponWheel(bool bEquipSelection)
{
	if (!bWeaponWheelOpen)
	{
		return;
	}
	bWeaponWheelOpen = false;

	if (UZonefallWeaponWheelWidget* Wheel = Cast<UZonefallWeaponWheelWidget>(WeaponWheelWidgetInstance))
	{
		if (bEquipSelection)
		{
			const int32 Sel = Wheel->GetSelectedWeaponIndex();
			if (Sel != INDEX_NONE)
			{
				RequestEquipWeapon(Sel);
			}
		}
		Wheel->SetVisibility(ESlateVisibility::Collapsed);
	}

	UGameplayStatics::SetGlobalTimeDilation(this, 1.0f);

	if (APlayerController* PC = Cast<APlayerController>(GetController()))
	{
		PC->SetShowMouseCursor(false);
		PC->SetInputMode(FInputModeGameOnly());
		if (FSlateApplication::IsInitialized())
		{
			FSlateApplication::Get().SetAllUserFocusToGameViewport();
		}
	}
}

// ---------------------------------------------------------------------------
// Dead Eye (slow motion)
// ---------------------------------------------------------------------------
void AZonefallPlayerCharacter::Input_DeadEyeStarted(const FInputActionValue& /*Value*/)
{
	StartDeadEye();
}

void AZonefallPlayerCharacter::Input_DeadEyeCompleted(const FInputActionValue& /*Value*/)
{
	StopDeadEye();
}

void AZonefallPlayerCharacter::StartDeadEye()
{
	if (bDeadEyeActive || DeadEyeMeter <= 0.02f)
	{
		return;
	}
	bDeadEyeActive = true;

	// Slow the whole world, but keep the player near real-time so aiming stays crisp.
	const float Dilation = FMath::Clamp(DeadEyeTimeDilation, 0.05f, 1.0f);
	UGameplayStatics::SetGlobalTimeDilation(this, Dilation);
	CustomTimeDilation = 1.0f / Dilation;

	APlayerController* PC = Cast<APlayerController>(GetController());
	if (PC && PC->IsLocalController())
	{
		if (!DeadEyeWidgetInstance)
		{
			const TSubclassOf<UUserWidget> WidgetClass = DeadEyeWidgetClass
				? DeadEyeWidgetClass
				: TSubclassOf<UUserWidget>(UZonefallDeadEyeWidget::StaticClass());
			DeadEyeWidgetInstance = CreateWidget<UUserWidget>(PC, WidgetClass);
			if (UZonefallDeadEyeWidget* DE = Cast<UZonefallDeadEyeWidget>(DeadEyeWidgetInstance))
			{
				DE->BindToCharacter(this);
			}
		}
		if (DeadEyeWidgetInstance && !DeadEyeWidgetInstance->IsInViewport())
		{
			DeadEyeWidgetInstance->AddToViewport(40);
		}
		if (DeadEyeWidgetInstance)
		{
			DeadEyeWidgetInstance->SetVisibility(ESlateVisibility::HitTestInvisible);
		}
	}
}

void AZonefallPlayerCharacter::StopDeadEye()
{
	if (!bDeadEyeActive)
	{
		return;
	}
	bDeadEyeActive = false;

	UGameplayStatics::SetGlobalTimeDilation(this, 1.0f);
	CustomTimeDilation = 1.0f;

	if (DeadEyeWidgetInstance)
	{
		DeadEyeWidgetInstance->SetVisibility(ESlateVisibility::Collapsed);
	}
}

// ---------------------------------------------------------------------------
// Firing (hitscan)
// ---------------------------------------------------------------------------
void AZonefallPlayerCharacter::Input_Fire(const FInputActionValue& /*Value*/)
{
	FireWeapon();
}

void AZonefallPlayerCharacter::FireWeapon()
{
	const UCameraComponent* ActiveCamera = IsFirstPerson() ? FirstPersonCamera : ThirdPersonCamera;
	if (!ActiveCamera || !Weapons || !Weapons->HasEquippedWeapon())
	{
		return;
	}

	const FZonefallWeaponItem& Equipped = Weapons->GetEquippedWeapon();
	float Range = Equipped.Range;

	const FVector Start = ActiveCamera->GetComponentLocation();
	const FVector End = Start + ActiveCamera->GetForwardVector() * Range;

	if (!Weapons->HasAmmoForShot())
	{
		Multicast_DryFireFX();
		return;
	}

	if (HasAuthority())
	{
		Server_FireWeapon_Implementation(Start, End);
	}
	else
	{
		Server_FireWeapon(Start, End);
	}
}

void AZonefallPlayerCharacter::Server_FireWeapon_Implementation(FVector TraceStart, FVector TraceEnd)
{
	if (!Weapons || !Weapons->ConsumeAmmoForShot())
	{
		return; // out of ammo / unarmed
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	const float Damage = Weapons->HasEquippedWeapon() ? Weapons->GetEquippedWeapon().Damage : 20.0f;

	FHitResult Hit;
	FCollisionQueryParams Params(SCENE_QUERY_STAT(ZonefallFire), /*bTraceComplex*/ true);
	Params.AddIgnoredActor(this);

	const bool bHit = World->LineTraceSingleByChannel(Hit, TraceStart, TraceEnd, ECC_Visibility, Params);
	const FVector ShotEnd = bHit ? Hit.ImpactPoint : TraceEnd;

	if (bHit)
	{
		if (AActor* HitActor = Hit.GetActor())
		{
			const FVector ShotDir = (TraceEnd - TraceStart).GetSafeNormal();
			UGameplayStatics::ApplyPointDamage(HitActor, Damage, ShotDir, Hit, GetController(), this, UDamageType::StaticClass());
		}
	}

	// Visible tracer + fire animation on everyone.
	Multicast_FireFX(TraceStart, ShotEnd, bHit);
	Multicast_PlayFireMontage();
}

void AZonefallPlayerCharacter::Multicast_FireFX_Implementation(FVector Start, FVector End, bool bHit)
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	// Start the tracer at the weapon muzzle if we have a held mesh, else from the camera point.
	FVector TracerStart = Start;
	if (WeaponMesh && WeaponMesh->IsVisible())
	{
		TracerStart = WeaponMesh->GetComponentLocation();
	}

	DrawDebugLine(World, TracerStart, End, FColor(255, 220, 90), false, 0.35f, 0, 1.5f);
	if (bHit)
	{
		DrawDebugSphere(World, End, 9.0f, 8, FColor(255, 120, 60), false, 0.4f, 0, 1.0f);
	}
}

void AZonefallPlayerCharacter::Multicast_PlayFireMontage_Implementation()
{
	if (FireMontage && GetMesh())
	{
		if (UAnimInstance* Anim = GetMesh()->GetAnimInstance())
		{
			Anim->Montage_Play(FireMontage);
		}
	}
}

void AZonefallPlayerCharacter::Multicast_DryFireFX_Implementation()
{
	if (FireMontage && GetMesh())
	{
		if (UAnimInstance* Anim = GetMesh()->GetAnimInstance())
		{
			Anim->Montage_Play(FireMontage, 1.0f, EMontagePlayReturnType::MontageLength, 0.0f, true);
		}
	}

	if (UWorld* World = GetWorld())
	{
		const FVector Loc = WeaponMesh && WeaponMesh->IsVisible()
			? WeaponMesh->GetComponentLocation()
			: GetActorLocation();
		DrawDebugString(World, Loc + FVector(0, 0, 40.0f), TEXT("CLICK"), nullptr, FColor(180, 180, 180), 0.35f, true);
	}
}

void AZonefallPlayerCharacter::Input_Reload(const FInputActionValue& /*Value*/)
{
	ReloadWeapon();
}

void AZonefallPlayerCharacter::ReloadWeapon()
{
	if (HasAuthority())
	{
		Server_Reload_Implementation();
	}
	else
	{
		Server_Reload();
	}
}

void AZonefallPlayerCharacter::Server_Reload_Implementation()
{
	if (Weapons && Weapons->Reload())
	{
		Multicast_ReloadFX();
	}
}

void AZonefallPlayerCharacter::Multicast_ReloadFX_Implementation()
{
	if (UWorld* World = GetWorld())
	{
		const FVector Loc = WeaponMesh && WeaponMesh->IsVisible()
			? WeaponMesh->GetComponentLocation()
			: GetActorLocation();
		DrawDebugString(World, Loc + FVector(0, 0, 40.0f), TEXT("RELOAD"), nullptr, FColor(120, 200, 255), 0.6f, true);
	}
}

// ---------------------------------------------------------------------------
// Aiming (ADS)
// ---------------------------------------------------------------------------
void AZonefallPlayerCharacter::Input_AimStarted(const FInputActionValue& /*Value*/)
{
	bIsAiming = true;
	ApplyAimFOV(true);
	Server_SetAiming(true);
}

void AZonefallPlayerCharacter::Input_AimCompleted(const FInputActionValue& /*Value*/)
{
	bIsAiming = false;
	ApplyAimFOV(false);
	Server_SetAiming(false);
}

void AZonefallPlayerCharacter::Server_SetAiming_Implementation(bool bNewAiming)
{
	bIsAiming = bNewAiming; // replicated to other clients for aim poses
}

void AZonefallPlayerCharacter::OnRep_IsAiming()
{
	// Remote proxies: reflect the aim FOV locally too (harmless for non-owning views).
	ApplyAimFOV(bIsAiming);
}

void AZonefallPlayerCharacter::ApplyAimFOV(bool bAiming)
{
	const float TargetFOV = bAiming ? AimFOV : DefaultFOV;
	if (ThirdPersonCamera)
	{
		ThirdPersonCamera->SetFieldOfView(TargetFOV);
	}
	if (FirstPersonCamera)
	{
		FirstPersonCamera->SetFieldOfView(TargetFOV);
	}
}

// ---------------------------------------------------------------------------
// Health / damage / death
// ---------------------------------------------------------------------------
float AZonefallPlayerCharacter::TakeDamage(float DamageAmount, const FDamageEvent& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
	const float Applied = Super::TakeDamage(DamageAmount, DamageEvent, EventInstigator, DamageCauser);

	if (!HasAuthority() || bIsDead || DamageAmount <= 0.0f)
	{
		return Applied;
	}

	Health = FMath::Clamp(Health - DamageAmount, 0.0f, MaxHealth);
	TimeSinceDamage = 0.0f;
	OnHealthChanged.Broadcast(Health, MaxHealth);

	if (Health <= 0.0f)
	{
		Die();
	}
	return Applied;
}

void AZonefallPlayerCharacter::Heal(float Amount)
{
	if (!HasAuthority() || bIsDead || Amount <= 0.0f)
	{
		return;
	}
	Health = FMath::Clamp(Health + Amount, 0.0f, MaxHealth);
	OnHealthChanged.Broadcast(Health, MaxHealth);
}

void AZonefallPlayerCharacter::OnRep_Health()
{
	OnHealthChanged.Broadcast(Health, MaxHealth);
}

void AZonefallPlayerCharacter::Die()
{
	if (bIsDead)
	{
		return;
	}
	bIsDead = true;
	OnDied.Broadcast();

	// Ragdoll the mesh.
	if (USkeletalMeshComponent* MeshComp = GetMesh())
	{
		MeshComp->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		MeshComp->SetCollisionProfileName(TEXT("Ragdoll"));
		MeshComp->SetSimulatePhysics(true);
	}
	if (UCapsuleComponent* Capsule = GetCapsuleComponent())
	{
		Capsule->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}
	if (UCharacterMovementComponent* Move = GetCharacterMovement())
	{
		Move->DisableMovement();
	}

	if (APlayerController* PC = Cast<APlayerController>(GetController()))
	{
		PC->SetIgnoreMoveInput(true);
		PC->SetIgnoreLookInput(true);
	}

	// Respawn after a delay (authority drives it).
	if (HasAuthority() && RespawnDelay > 0.0f)
	{
		GetWorldTimerManager().SetTimer(RespawnTimerHandle, this, &AZonefallPlayerCharacter::Respawn, RespawnDelay, false);
	}
}

void AZonefallPlayerCharacter::Respawn()
{
	if (!HasAuthority())
	{
		return;
	}

	bIsDead = false;
	Health = MaxHealth;
	TimeSinceDamage = 1000.0f;
	OnHealthChanged.Broadcast(Health, MaxHealth);

	// Stand the mesh back up.
	if (USkeletalMeshComponent* MeshComp = GetMesh())
	{
		MeshComp->SetSimulatePhysics(false);
		MeshComp->SetCollisionProfileName(TEXT("CharacterMesh"));
		MeshComp->AttachToComponent(GetCapsuleComponent(), FAttachmentTransformRules::SnapToTargetNotIncludingScale);
		MeshComp->SetRelativeLocationAndRotation(FVector(0.0f, 0.0f, -90.0f), FRotator(0.0f, -90.0f, 0.0f));
	}
	if (UCapsuleComponent* Capsule = GetCapsuleComponent())
	{
		Capsule->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	}
	if (UCharacterMovementComponent* Move = GetCharacterMovement())
	{
		Move->SetMovementMode(MOVE_Walking);
	}

	if (APlayerController* PC = Cast<APlayerController>(GetController()))
	{
		PC->SetIgnoreMoveInput(false);
		PC->SetIgnoreLookInput(false);

		// Teleport to a player start if one exists.
		if (AActor* Start = UGameplayStatics::GetActorOfClass(GetWorld(), APlayerStart::StaticClass()))
		{
			SetActorLocationAndRotation(Start->GetActorLocation() + FVector(0, 0, 100.0f), Start->GetActorRotation());
		}
	}
}

void AZonefallPlayerCharacter::ToggleInventoryUI()
{
	APlayerController* PC = Cast<APlayerController>(GetController());
	if (!PC || !PC->IsLocalController())
	{
		return;
	}

	bInventoryOpen = !bInventoryOpen;

	if (bInventoryOpen)
	{
		if (!InventoryWidgetInstance)
		{
			const TSubclassOf<UUserWidget> WidgetClass = InventoryWidgetClass
				? InventoryWidgetClass
				: TSubclassOf<UUserWidget>(UZonefallInventoryWidget::StaticClass());

			InventoryWidgetInstance = CreateWidget<UUserWidget>(PC, WidgetClass);
			if (UZonefallInventoryWidget* InvWidget = Cast<UZonefallInventoryWidget>(InventoryWidgetInstance))
			{
				InvWidget->SetInventory(Inventory);
			}
		}

		if (InventoryWidgetInstance)
		{
			if (!InventoryWidgetInstance->IsInViewport())
			{
				InventoryWidgetInstance->AddToViewport(50);
			}
			InventoryWidgetInstance->SetVisibility(ESlateVisibility::Visible);
		}

		PC->SetShowMouseCursor(true);
		FInputModeGameAndUI InputMode;
		InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
		InputMode.SetHideCursorDuringCapture(false);
		PC->SetInputMode(InputMode);
	}
	else
	{
		if (InventoryWidgetInstance)
		{
			InventoryWidgetInstance->SetVisibility(ESlateVisibility::Collapsed);
		}
		PC->SetShowMouseCursor(false);
		PC->SetInputMode(FInputModeGameOnly());
		if (FSlateApplication::IsInitialized())
		{
			// Hand keyboard focus back to the game so WASD works again after closing.
			FSlateApplication::Get().SetAllUserFocusToGameViewport();
		}
	}
}

void AZonefallPlayerCharacter::Input_ToggleCamera(const FInputActionValue& /*Value*/)
{
	ToggleCameraView();
}

void AZonefallPlayerCharacter::SetCameraView(EZonefallCameraView NewView)
{
	if (CameraView == NewView)
	{
		return;
	}
	CameraView = NewView;
	ApplyCameraView();
}

void AZonefallPlayerCharacter::ToggleCameraView()
{
	CameraView = IsFirstPerson() ? EZonefallCameraView::ThirdPerson : EZonefallCameraView::FirstPerson;
	ApplyCameraView();
}

void AZonefallPlayerCharacter::ApplyCameraView()
{
	const bool bFirstPerson = IsFirstPerson();

	if (FirstPersonCamera)
	{
		FirstPersonCamera->SetActive(bFirstPerson);
	}
	if (ThirdPersonCamera)
	{
		ThirdPersonCamera->SetActive(!bFirstPerson);
	}

	// Hide our own mesh from the first-person camera to avoid head/body clipping.
	if (USkeletalMeshComponent* MeshComp = GetMesh())
	{
		MeshComp->SetOwnerNoSee(bFirstPerson);
	}

	// First person turns with the camera; third person orients toward movement.
	bUseControllerRotationYaw = bFirstPerson;
	if (UCharacterMovementComponent* Move = GetCharacterMovement())
	{
		Move->bOrientRotationToMovement = !bFirstPerson;
	}

	OnCameraViewChanged.Broadcast(CameraView);
}

AActor* AZonefallPlayerCharacter::TraceInteractable() const
{
	const UCameraComponent* ActiveCamera = IsFirstPerson() ? FirstPersonCamera : ThirdPersonCamera;
	UWorld* World = GetWorld();
	if (!World || !ActiveCamera)
	{
		return nullptr;
	}

	const FVector Start = ActiveCamera->GetComponentLocation();
	const FVector End = Start + ActiveCamera->GetForwardVector() * InteractionRange;

	FCollisionQueryParams Params(SCENE_QUERY_STAT(ZonefallInteract), /*bTraceComplex*/ false);
	Params.AddIgnoredActor(this);

	// 1) Forgiving sphere sweep down the camera ray (easier to aim than a thin line).
	FHitResult Hit;
	if (World->SweepSingleByChannel(
			Hit, Start, End, FQuat::Identity, ECC_Visibility, FCollisionShape::MakeSphere(22.0f), Params))
	{
		if (Hit.GetActor())
		{
			return Hit.GetActor();
		}
	}

	// 2) Straight line trace as a backup.
	if (World->LineTraceSingleByChannel(Hit, Start, End, ECC_Visibility, Params))
	{
		if (Hit.GetActor())
		{
			return Hit.GetActor();
		}
	}

	// 3) Last resort: nearest world-item within reach of the character (so loot on the
	//    ground / at the feet is still grabbable even if the camera isn't aimed at it).
	const FVector PawnLocation = GetActorLocation();
	TArray<FOverlapResult> Overlaps;
	const FCollisionShape Sphere = FCollisionShape::MakeSphere(InteractionRange);
	if (World->OverlapMultiByObjectType(
			Overlaps, PawnLocation, FQuat::Identity,
			FCollisionObjectQueryParams(FCollisionObjectQueryParams::AllObjects), Sphere, Params))
	{
		AActor* NearestItem = nullptr;
		float NearestDistSq = TNumericLimits<float>::Max();
		for (const FOverlapResult& Overlap : Overlaps)
		{
			AActor* OverlapActor = Overlap.GetActor();
			if (OverlapActor && OverlapActor->IsA(AZonefallWorldItem::StaticClass()))
			{
				const float DistSq = FVector::DistSquared(PawnLocation, OverlapActor->GetActorLocation());
				if (DistSq < NearestDistSq)
				{
					NearestDistSq = DistSq;
					NearestItem = OverlapActor;
				}
			}
		}
		if (NearestItem)
		{
			return NearestItem;
		}
	}

	return nullptr;
}
