#include "ZonefallAICharacter.h"

#include "ZonefallAIController.h"
#include "ZonefallAIPatrolComponent.h"
#include "ZonefallNpcVitalsComponent.h"
#include "ZonefallPatrolPath.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Engine/SkeletalMesh.h"
#include "Animation/AnimInstance.h"
#include "NavigationSystem.h"
#include "UObject/ConstructorHelpers.h"

AZonefallAICharacter::AZonefallAICharacter()
{
	PrimaryActorTick.bCanEverTick = false;

	// Identity + vitals are embedded so the pawn works the instant it's placed.
	AICharacter = CreateDefaultSubobject<UZonefallAICharacterComponent>(TEXT("ZonefallAICharacter"));
	Vitals = CreateDefaultSubobject<UZonefallNpcVitalsComponent>(TEXT("ZonefallNpcVitals"));

	// Possess with the Zonefall AI controller (perception + coded behavior tree).
	AIControllerClass = AZonefallAIController::StaticClass();
	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;

	bReplicates = true;
	SetReplicateMovement(true);

	// --- Default body so the NPC is visible the moment it's placed (no manual setup) ---
	if (USkeletalMeshComponent* MeshComp = GetMesh())
	{
		// Standard mannequin placement: feet on the capsule bottom, facing +X.
		MeshComp->SetRelativeLocationAndRotation(FVector(0.0f, 0.0f, -90.0f), FRotator(0.0f, -90.0f, 0.0f));

		static ConstructorHelpers::FObjectFinder<USkeletalMesh> MeshFinder(
			TEXT("/Game/Characters/Mannequins/Meshes/SKM_Quinn_Simple.SKM_Quinn_Simple"));
		if (MeshFinder.Succeeded())
		{
			MeshComp->SetSkeletalMesh(MeshFinder.Object);
		}
		else
		{
			static ConstructorHelpers::FObjectFinder<USkeletalMesh> MeshFallback(
				TEXT("/Game/Characters/Mannequins/Meshes/SK_Mannequin.SK_Mannequin"));
			if (MeshFallback.Succeeded())
			{
				MeshComp->SetSkeletalMesh(MeshFallback.Object);
			}
		}

		static ConstructorHelpers::FClassFinder<UAnimInstance> AnimFinder(
			TEXT("/Game/Characters/Mannequins/Anims/Unarmed/ABP_Unarmed"));
		if (AnimFinder.Succeeded())
		{
			MeshComp->SetAnimInstanceClass(AnimFinder.Class);
		}
		MeshComp->SetAnimationMode(EAnimationMode::AnimationBlueprint);
	}

	if (UCharacterMovementComponent* Move = GetCharacterMovement())
	{
		Move->bUseRVOAvoidance = true;
		Move->RotationRate = FRotator(0.0f, 360.0f, 0.0f);
		Move->bOrientRotationToMovement = true;
		Move->MaxWalkSpeed = 400.0f;
	}
	bUseControllerRotationYaw = false;
}

void AZonefallAICharacter::BeginPlay()
{
	Super::BeginPlay();

	if (AICharacter)
	{
		AICharacter->CharacterProfile = Profile;
		AICharacter->bAutoApplyProfileOnBeginPlay = true;
		AICharacter->ApplyCharacterProfile();
		// Make sure vitals + status widget are created/refreshed for this profile.
		AICharacter->EnsureAutoZonefallComponents();
	}

	if (Vitals)
	{
		Vitals->MaxHealth = StartingHealth;
		Vitals->Heal(StartingHealth); // clamp to max on the server
		Vitals->SetupStatusWidget();
	}

	if (UCharacterMovementComponent* Move = GetCharacterMovement())
	{
		Move->MaxWalkSpeed = PatrolWalkSpeed;
	}

	SetupPatrol();
}

void AZonefallAICharacter::SetupPatrol()
{
	if (!AICharacter)
	{
		return;
	}

	UZonefallAIPatrolComponent* Patrol = AICharacter->EnsurePatrolComponent();
	if (!Patrol)
	{
		return;
	}

	// 1) A hand-placed spline path always wins — copy world points onto this pawn's patrol spline.
	if (PatrolPathActor)
	{
		if (const USplineComponent* SourceSpline = PatrolPathActor->GetSpline())
		{
			Patrol->ClearSplinePoints(false);
			const int32 NumPoints = SourceSpline->GetNumberOfSplinePoints();
			for (int32 PointIndex = 0; PointIndex < NumPoints; ++PointIndex)
			{
				const FVector WorldLocation = SourceSpline->GetLocationAtSplinePoint(PointIndex, ESplineCoordinateSpace::World);
				Patrol->AddSplinePoint(WorldLocation, ESplineCoordinateSpace::World, false);
			}
			Patrol->UpdateSpline();
			Patrol->PatrolPoints.Reset();
			Patrol->bUseSplineRoute = true;
			Patrol->PatrolMode = EZonefallPatrolMode::Loop;
			Patrol->ResetPatrol();
		}
		return;
	}

	// 2) Otherwise auto-build a wander ring on the NavMesh around the spawn point so the NPC
	//    actually walks (and therefore animates) instead of standing still.
	if (!bAutoGeneratePatrolRoute)
	{
		return;
	}

	UNavigationSystemV1* Nav = UNavigationSystemV1::GetCurrent(GetWorld());
	const FVector Origin = GetActorLocation();
	const int32 Count = FMath::Clamp(PatrolRoutePointCount, 2, 12);

	Patrol->ClearSplinePoints(false);
	Patrol->UpdateSpline();
	Patrol->bUseSplineRoute = false;
	Patrol->PatrolPoints.Reset();
	Patrol->SetPatrolAnchor(Origin);

	for (int32 i = 0; i < Count; ++i)
	{
		const float Angle = (2.0f * PI * i) / Count;
		FVector Candidate = Origin + FVector(FMath::Cos(Angle), FMath::Sin(Angle), 0.0f) * PatrolRouteRadius;

		// Snap each ring point onto the navmesh so MoveTo can actually reach it.
		if (Nav)
		{
			FVector Projected = Candidate;
			if (UNavigationSystemV1::K2_ProjectPointToNavigation(this, Candidate, Projected, nullptr, nullptr, FVector(600.0f, 600.0f, 600.0f)))
			{
				Candidate = Projected;
			}
		}

		FZonefallPatrolPoint Point;
		Point.LocalOffset = Candidate - Origin;
		Patrol->PatrolPoints.Add(Point);
	}

	Patrol->PatrolMode = EZonefallPatrolMode::Loop;
	Patrol->ResetPatrol();
}

#if WITH_EDITOR
void AZonefallAICharacter::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	if (AICharacter)
	{
		AICharacter->CharacterProfile = Profile;
	}
}
#endif
