#include "Inventory/ZonefallWorldItem.h"

#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "GameFramework/Pawn.h"
#include "Net/UnrealNetwork.h"
#include "UObject/ConstructorHelpers.h"

AZonefallWorldItem::AZonefallWorldItem()
{
	PrimaryActorTick.bCanEverTick = true;

	bReplicates = true;
	SetReplicateMovement(true);

	CollisionSphere = CreateDefaultSubobject<USphereComponent>(TEXT("CollisionSphere"));
	CollisionSphere->InitSphereRadius(60.0f);
	CollisionSphere->SetCollisionProfileName(TEXT("OverlapAllDynamic"));
	SetRootComponent(CollisionSphere);

	MeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MeshComponent"));
	MeshComponent->SetupAttachment(CollisionSphere);
	// Visible and line-trace blocking (so interaction traces hit it), but no physics blocking by default.
	MeshComponent->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	MeshComponent->SetCollisionResponseToAllChannels(ECR_Ignore);
	MeshComponent->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);

	// A simple default cube so a dropped item is visible even without art assigned.
	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(TEXT("/Engine/BasicShapes/Cube.Cube"));
	if (CubeMesh.Succeeded())
	{
		MeshComponent->SetStaticMesh(CubeMesh.Object);
		MeshComponent->SetRelativeScale3D(FVector(0.35f));
	}
}

void AZonefallWorldItem::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AZonefallWorldItem, Item);
}

void AZonefallWorldItem::BeginPlay()
{
	Super::BeginPlay();

	// Items dropped straight into the level via the editor usually have empty Item data,
	// which makes Item.IsValid() false and silently blocks pickup. Give every placed item a
	// usable id on the server (a clean readable name, NOT the ugly actor id), so the satchel
	// shows something sensible and the category tabs work.
	if (HasAuthority())
	{
		if (Item.ItemId.IsNone())
		{
			// Stable, hidden id derived from the category so stacking still works sensibly.
			Item.ItemId = FName(*FString::Printf(TEXT("Item_%s"), *PickupDisplayName.ToString().Replace(TEXT(" "), TEXT(""))));
			if (Item.ItemId.IsNone() || PickupDisplayName.IsEmpty())
			{
				Item.ItemId = FName(TEXT("Item_Unknown"));
			}
		}
		if (Item.DisplayName.IsEmpty())
		{
			Item.DisplayName = PickupDisplayName.IsEmpty()
				? NSLOCTEXT("ZonefallItem", "Unknown", "Unknown Item")
				: PickupDisplayName;
		}
		if (Item.Description.IsEmpty())
		{
			Item.Description = PickupDescription;
		}
		Item.Category = PickupCategory;
		Item.bConsumable = bPickupConsumable;
		Item.Quantity = FMath::Max(1, Item.Quantity);
		ForceNetUpdate();
	}

	if (CollisionSphere)
	{
		CollisionSphere->OnComponentBeginOverlap.AddDynamic(this, &AZonefallWorldItem::HandleBeginOverlap);
	}
	RefreshVisual();
}

void AZonefallWorldItem::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (MeshComponent && !FMath::IsNearlyZero(SpinDegreesPerSecond))
	{
		MeshComponent->AddLocalRotation(FRotator(0.0f, SpinDegreesPerSecond * DeltaSeconds, 0.0f));
	}
}

void AZonefallWorldItem::OnRep_Item()
{
	RefreshVisual();
}

void AZonefallWorldItem::RefreshVisual()
{
	// Hook point for setting a material/icon billboard based on Item. Kept minimal here.
}

void AZonefallWorldItem::InitializeFromItem(const FZonefallInventoryItem& InItem)
{
	Item = InItem;
	RefreshVisual();
	ForceNetUpdate();
}

bool AZonefallWorldItem::TryGiveTo(UZonefallInventoryComponent* Inventory)
{
	if (!HasAuthority() || !Inventory || !Item.IsValid())
	{
		return false;
	}

	if (Inventory->AddItem(Item))
	{
		BP_OnPickedUp(Inventory->GetOwner());
		Destroy();
		return true;
	}
	return false;
}

void AZonefallWorldItem::HandleBeginOverlap(UPrimitiveComponent* /*OverlappedComp*/, AActor* OtherActor,
	UPrimitiveComponent* /*OtherComp*/, int32 /*OtherBodyIndex*/, bool /*bFromSweep*/, const FHitResult& /*SweepResult*/)
{
	if (!bAutoPickupOnOverlap || !HasAuthority() || !OtherActor)
	{
		return;
	}

	if (UZonefallInventoryComponent* Inventory = OtherActor->FindComponentByClass<UZonefallInventoryComponent>())
	{
		TryGiveTo(Inventory);
	}
}
