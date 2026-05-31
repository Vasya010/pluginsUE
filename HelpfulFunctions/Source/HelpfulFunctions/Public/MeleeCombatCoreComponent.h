// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/StreamableManager.h"
#include "Components/ActorComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "ALS_StructuresAndEnumsCpp.h"
#include "AGLS_AI_CharacterInterface.h"
#include "Components/Widget.h"
#include "CombatAnimSetup.h"
#include "CombatAnimsDatabase.h"
#include "CombatHitReactionsDatabase.h"
#include "CharacterFocusingComponent.h"
#include "Curves/CurveFloat.h"
#include "MeleeCombatCoreComponent.generated.h"

UENUM(BlueprintType)
enum class CMaleeCombat_PreCombatState : uint8
{
	Default,
	InMeleeCombat,
	ForgottenEnemy,
	ReachDroppedItems
};

UENUM(BlueprintType)
enum class CMaleeCombat_CurrentAction : uint8
{
	None,
	MeleeAction,
	StealthFinisher,
	HoldHostage,
	Dodge,
	Other
};

UENUM(BlueprintType)
enum class CE_MontageDelegateActions : uint8
{
	OnStarted,
	OnComplete,
	OnBlendOut,
	OnInterrupted,
	OnNotifyBegin
};

UENUM(BlueprintType)
enum class CMaleeCombat_CustomWarpingActorsSet : uint8
{
	LocationAndRotation,
	LocationOnly,
	NoPositionUpdate
};

UENUM(BlueprintType)
enum class CMaleeCombat_WarpingMotionType : uint8
{
	OnlyDesiredPosition UMETA(DisplayName = "Init Desired Position"),
	TransformsAndRootMotion UMETA(DisplayName = "Desired Position + RootMotion"),
	AllAboveAndOriginOffset UMETA(DisplayName = "Position + RootMotion + OriginOffset")
};

UENUM(BlueprintType)
enum class CMaleeCombat_WarpingPositionSolver : uint8
{
	SubtractInitPosition UMETA(DisplayName = "Current - InitPosition"),
	AccumulatedMotion UMETA(DisplayName = "Accumulated + MotionDelta")
};


USTRUCT(BlueprintType)
struct FMeleeCombat_TargetingResult : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Targeting Result")
	ACharacter* TargetedCharacter = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Targeting Result")
	bool bChoosedActorStillAvailable = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Targeting Result")
	bool bAvaliableForFinisher = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Targeting Result")
	CMaleeCombat_PreCombatState TargetedCharCombatState = CMaleeCombat_PreCombatState::Default;

};


DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnMontageLatendAction, CE_MontageDelegateActions, Type, FName, NotifyName);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FMeleeCombatInputTrigger_Attack, bool, Value);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FMeleeCombatInputTrigger_Dodge, bool, Value);

/*
A Core component designed for the Melee Combat system in the AGLS project. It contains many key functions and variable declarations.
It is connected with the CombatCore plugin, where the component uses objects such as:
UCombatAnimSetup*
UCombatAnimsDatabase*
UCombatHitReactionsDatabase*
*/
UCLASS(Blueprintable, ClassGroup = (Gameplay), meta = (BlueprintSpawnableComponent))
class HELPFULFUNCTIONS_API UMeleeCombatCoreComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	//░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░ 𝐕𝐀𝐑𝐈𝐀𝐁𝐋𝐄𝐒 𝐒𝐄𝐂𝐓𝐈𝐎𝐍
	UMeleeCombatCoreComponent();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = "True", Category = "Melee Combat Core|Main"))
	ACharacter* RefChar = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = "True", Category = "Melee Combat Core|Main"))
	UCharacterFocusingComponent* FocusingComponent = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Melee Combat Core|Config", meta = (AllowPrivateAccess = "True"))
	TSoftClassPtr<UAnimInstance> MeleeCombatAnimLayer;

	//Duration To Wait Untill Unlink Combat Anim Layer (When not anymore usefull)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = "True", Category = "Melee Combat Core|Config"))
	float DurationToWaitUntillRemoveAnim = 20;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = "True", Category = "Melee Combat Core|Config"))
	float InputsAsActivatedWaitTime = 0.2;


/*
▶ 𝘊𝘜𝘚𝘛𝘖𝘔 𝙒𝘼𝙍𝙋𝙄𝙉𝙂 ◀ ᴍᴇʟᴇᴇ ᴄᴏᴍʙᴀᴛ
*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = "True", Category = "Melee Combat Core|Config|Custom Warping"))
	bool PerformMoveToWhenAttacksStart = true;

/*
▶ 𝘊𝘜𝘚𝘛𝘖𝘔 𝙒𝘼𝙍𝙋𝙄𝙉𝙂 ◀ ᴍᴇʟᴇᴇ ᴄᴏᴍʙᴀᴛ
When 0.0 as center use 100% of ComponentOwner. When 1.0 as center use 100% of GetChoosedEnemyCher().
*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = "True", Category = "Melee Combat Core|Config|Custom Warping", ClampMin = "0.0", ClampMax = "1.0", EditCondition = "PerformMoveToWhenAttacksStart"))
	float CustomWarpingCenterAlpha = 1.0;

/*
▶ 𝘊𝘜𝘚𝘛𝘖𝘔 𝙒𝘼𝙍𝙋𝙄𝙉𝙂 ◀ ᴍᴇʟᴇᴇ ᴄᴏᴍʙᴀᴛ
Use Simple Lerp to Desired Positions or use Interpolating mode like using VInterp or RInterp functions
*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = "True", Category = "Melee Combat Core|Config|Custom Warping"))
	bool bUseInterpingPerTickWarping = true;

/*
▶ 𝘊𝘜𝘚𝘛𝘖𝘔 𝙒𝘼𝙍𝙋𝙄𝙉𝙂 ◀ ᴍᴇʟᴇᴇ ᴄᴏᴍʙᴀᴛ

ENG:
When set to 'true', some transformations will be stored locally and resolved to the global space when used. 
This can help CustomWarping work on moving platforms or non-inertial reference frames.

PL:
W przypadku kiedy 'true' niektóre transformacje będą przechowywane w postaci lokalnej a w momencie ich użycia 
rozwiązywane do przestrzeni globalnej. Może to pomóc na działanie CustomWarping w przypadku ruchomych platform 
lub nieinercjalnych układów odniesienia
*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = "True", Category = "Melee Combat Core|Config|Custom Warping"))
	bool bTryResolveToWorldSpace = false;

/*
▶ 𝘊𝘜𝘚𝘛𝘖𝘔 𝙒𝘼𝙍𝙋𝙄𝙉𝙂 ◀ ᴍᴇʟᴇᴇ ᴄᴏᴍʙᴀᴛ
*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = "True", Category = "Melee Combat Core|Config|Custom Warping"))
	CMaleeCombat_CustomWarpingActorsSet WarpingTypeOwner = CMaleeCombat_CustomWarpingActorsSet::LocationAndRotation;
/*
▶ 𝘊𝘜𝘚𝘛𝘖𝘔 𝙒𝘼𝙍𝙋𝙄𝙉𝙂 ◀ ᴍᴇʟᴇᴇ ᴄᴏᴍʙᴀᴛ
*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = "True", Category = "Melee Combat Core|Config|Custom Warping"))
	CMaleeCombat_CustomWarpingActorsSet WarpingTypeOpponent = CMaleeCombat_CustomWarpingActorsSet::LocationOnly;
	
/*
▶ 𝘊𝘜𝘚𝘛𝘖𝘔 𝙒𝘼𝙍𝙋𝙄𝙉𝙂 ◀ ᴍᴇʟᴇᴇ ᴄᴏᴍʙᴀᴛ

ENG:
One of the more important settings for CustomWarping. It specifies which movements/displacements should be taken into account during CustomWarping.
CustomWarping processes three main movement components separately:
➊ Reaching the position defined by DesiredPositionOwner using interpolation.
➋ Preserving the displacement resulting from RootMotion and adding it to DesiredLocation.
➌ Determining the displacement of the center, which is usually GetChoosedEnemyChar()->GetActorTransform(), and then calculating the displacement
   difference or accumulating it.
▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂
PL:
Jedna z ważniejszych ustawień dla CustomWarping. Wskazuje ona na jakie ruchy/przemieszczenia mają być brane pod uwagę podczas CustomWarping.
CustomWarping oddzielnie przetwarza 3 główne ruchy gdzie:
➊ Osiągnięcie pozycji związanej z wartością DesiredPositionOwner używając interpolacji
➋ Zachowanie przemieszczenia związanego z RootMotion i dodanie go do DesiredLocation
➌ Wyznaczenie przemieszczenia środka, gdzie zazwyczaj jest to GetChoosedEnemyChar()->GetActorTransform(), a następnie obliczenie różnicy
przemieszczenia lub akumulowanie go.

Zazwyczaj powinno to być ustawione na AllAboveAndOriginOffset.
▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂
*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = "True", Category = "Melee Combat Core|Config|Custom Warping"))
	CMaleeCombat_WarpingMotionType SolvingPositionType = CMaleeCombat_WarpingMotionType::AllAboveAndOriginOffset;

/*
▶ 𝘊𝘜𝘚𝘛𝘖𝘔 𝙒𝘼𝙍𝙋𝙄𝙉𝙂 ◀ ᴍᴇʟᴇᴇ ᴄᴏᴍʙᴀᴛ

ENG:
The method used to calculate displacement resulting from a change in the position of GetChoosedEnemyChar()->GetActorLocation().

If SubtractInitPosition is used, the displacement difference can be calculated as:
GetChoosedEnemyChar()->GetActorLocation() - SolvedInitPosition;
where SolvedInitPosition is set during the initialization of CustomWarping StartCustomWarpingPositions().

In the case of AccumulatedSolver, position solving is based on calculating the displacement delta and accumulating the value
into the appropriate variable: AccumulatedEnemyOffset += EnemyPositionDelta;
▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂
PL:
Sposób przeliczania przemieszczenia wynikającego ze zmiany pozycji GetChoosedEnemyChar()->GetActorLocation().
Jeżeli SubtractInitPosition to w takiej konfiguracji różnicę przemieszczenia można wyznaczyć jako:
GetChoosedEnemyChar()->GetActorLocation() - SolvedInitPosition; gdzie SolvedInitPosition jest ustawiany podczas
inicjacji CustomWarping StartCustomWarpingPositions()

W przypadku AccumulatedSOLVER rozwiązywanie pozycji polega na obliczaniu delty przemieszczenia i akumulowanie
wartości do odpowiedniej zmiennej - AccumulatedEnemyOffset += EnemyPositionDelta;
▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂
*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = "True", Category = "Melee Combat Core|Config|Custom Warping"))
	CMaleeCombat_WarpingPositionSolver OpponentMotionSolverType = CMaleeCombat_WarpingPositionSolver::SubtractInitPosition;

/*
▶ 𝘊𝘜𝘚𝘛𝘖𝘔 𝙒𝘼𝙍𝙋𝙄𝙉𝙂 ◀ ᴍᴇʟᴇᴇ ᴄᴏᴍʙᴀᴛ

	This value is important when:
OpponentMotionSolverType == CMeleeCombat_WarpingPositionSolver::AccumulatedMotion &&
SolvingPositionType = CMeleeCombat_WarpingMotionType::AllAboveAndOriginOffset &&
(WarpingTypeOwner = CMeleeCombat_CustomWarpingActorsSet::LocationAndRotation ||
WarpingTypeOwner = CMeleeCombat_CustomWarpingActorsSet::LocationOnly)

The variable determines the acceptable distance between the difference of the previous position (as GetChoosedEnemyChar()->GetActorLocation())
and the current one GetChoosedEnemyChar()->GetActorLocation(). If ResolveToWorldSpace is used, the transform of the previous position is
converted to world space before calculating the difference.

Code fragment:
if (OpponentMotionSolverType == AccumulatedSOLVER)
{
	EnemyPositionDelta = (GetChoosedEnemyChar()->GetActorLocation() - SolvedInitPosition) * OpponentMotionFollowScale;
	AccumulatedEnemyOffset += EnemyPositionDelta;
	if (ResolveToWorldSpace)
	{
		OnWarpingStartEnemyPosition.Transform = GetChoosedEnemyChar()->GetActorTransform();
		OnWarpingStartEnemyPosition = HFL::ConvertWorldToLocalFastMatrix(OnWarpingStartEnemyPosition);
	}
	else
	{
		OnWarpingStartEnemyPosition.Transform = GetChoosedEnemyChar()->GetActorTransform();
	}
}
▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂
PL:
Wartość jest istotna w przypadku kiedy:
OpponentMotionSolverType == CMaleeCombat_WarpingPositionSolver::AccumulatedMotion &&
SolvingPositionType = CMaleeCombat_WarpingMotionType::AllAboveAndOriginOffset &&
(WarpingTypeOwner = CMaleeCombat_CustomWarpingActorsSet::LocationAndRotation ||
WarpingTypeOwner = CMaleeCombat_CustomWarpingActorsSet::LocationOnly)

Zmienna określa jaka jest akceptowalna odległość pomiędzy różnicą poprzedniej pozycji (jako GetChoosedEnemyChar()->GetActorLocation()) oraz
obecnej GetChoosedEnemyChar()->GetActorLocation(). Jeżeli ResolveToWorldSpace jest używany to przed wyznaczeniem róznicy tranformacja
poprzedniej pozycji jest przekształcana do przestrzeni globalnej.
▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂
*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = "True", Category = "Melee Combat Core|Config|Custom Warping", ClampMin = "-1.0", ClampMax = "50.0", 
		EditCondition = "OpponentMotionSolverType == CMaleeCombat_WarpingPositionSolver::AccumulatedMotion"))
	float MaxDeltaBetweenPrevPosition = -1.0;

/*
▶ 𝘊𝘜𝘚𝘛𝘖𝘔 𝙒𝘼𝙍𝙋𝙄𝙉𝙂 ◀ ᴍᴇʟᴇᴇ ᴄᴏᴍʙᴀᴛ
*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = "True", Category = "Melee Combat Core|Debug"))
	bool bDrawWarpingDebug = false;


	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = "True", Category = "Melee Combat Core|Main"))
	bool bStartedMeleeCombat = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = "True", Category = "Melee Combat Core|Main"))
	FMeleeCombat_TargetingResult TargetedEnemyData;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = "True", Category = "Melee Combat Core|Main"))
	FMeleeCombat_TargetingResult CurrentlyActiveEnemy;

	UPROPERTY(BlueprintReadWrite, meta = (AllowPrivateAccess = "True", Category = "Melee Combat Core|Widgets Core"))
	UWidget* WidgetInstance01 = nullptr;

	UPROPERTY(BlueprintReadWrite, meta = (AllowPrivateAccess = "True", Category = "Melee Combat Core|Widgets Core"))
	UWidget* WidgetInstance02 = nullptr;

	UPROPERTY(BlueprintReadWrite, meta = (AllowPrivateAccess = "True", Category = "Melee Combat Core|Main"))
	CMaleeCombat_CurrentAction CurrentPerformedAction = CMaleeCombat_CurrentAction::None;

	UPROPERTY(BlueprintReadWrite, meta = (AllowPrivateAccess = "True", Category = "Melee Combat Core|Main"))
	CMaleeCombat_CurrentAction PrevPerformedAction = CMaleeCombat_CurrentAction::None;

	UPROPERTY(BlueprintReadWrite, meta = (AllowPrivateAccess = "True", Category = "Melee Combat Core|Code Flow"))
	bool bLoadingAsset01 = false;

	UPROPERTY(BlueprintReadWrite, meta = (AllowPrivateAccess = "True", Category = "Melee Combat Core|Code Flow"))
	bool bLoadingAsset02 = false;

	UPROPERTY(BlueprintReadWrite, meta = (AllowPrivateAccess = "True", Category = "Melee Combat Core|Code Flow"))
	bool bInputRefreshHold_Attack = false;

	UPROPERTY(BlueprintReadWrite, meta = (AllowPrivateAccess = "True", Category = "Melee Combat Core|Code Flow"))
	bool bInputRefreshHold_Dodge = false;

	UPROPERTY(BlueprintReadWrite, meta = (AllowPrivateAccess = "True", Category = "Melee Combat Core|Code Flow"))
	FTimerHandle TimerHandle01;

	UPROPERTY(BlueprintReadWrite, meta = (AllowPrivateAccess = "True", Category = "Melee Combat Core|Anims Ref"))
	UCombatAnimSetup* ChoosedAnimSetup = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = "True", Category = "Melee Combat Core|Anims Ref"))
	TArray<UCombatAnimsDatabase*> DatabasesForSearch;

	UPROPERTY(BlueprintReadWrite, meta = (AllowPrivateAccess = "True", Category = "Melee Combat Core|Transforms"))
	FCALS_ComponentAndTransform DesiredPositionOwner;

	UPROPERTY(BlueprintReadWrite, meta = (AllowPrivateAccess = "True", Category = "Melee Combat Core|Transforms"))
	FCALS_ComponentAndTransform DesiredPositionEnemy;

	UPROPERTY(BlueprintReadWrite, meta = (AllowPrivateAccess = "True", Category = "Melee Combat Core|Fighting Poses", ClampMin = "0.0", ClampMax = "1.0"))
	float DesiredFightingPosesAlpha = 0.0;

	UPROPERTY(BlueprintReadWrite, meta = (AllowPrivateAccess = "True", Category = "Melee Combat Core|Fighting Poses", ClampMin = "-1.0", ClampMax = "1.0"))
	float FightingPosesBlendMode = 0.0;

	UPROPERTY(BlueprintReadWrite, meta = (AllowPrivateAccess = "True", Category = "Melee Combat Core|Fighting Poses", ClampMin = "-1.0"))
	float FightingPosesElapsedTime = -1;

	UPROPERTY(BlueprintReadWrite, meta = (AllowPrivateAccess = "True", Category = "Melee Combat Core|Damage"))
	FHitResult DamageHitResult;



	UPROPERTY(BlueprintReadWrite, meta = (AllowPrivateAccess = "True", Category = "Melee Combat Core|Focusing"))
	float FocusingOnOpponentStrength = 0.0;

	UPROPERTY(BlueprintReadWrite, meta = (AllowPrivateAccess = "True", Category = "Melee Combat Core|Focusing"))
	FVector FocusingPosition = FVector(0, 0, 0);

	UPROPERTY(BlueprintReadWrite, meta = (AllowPrivateAccess = "True", Category = "Melee Combat Core|Focusing"))
	float OpponentIsFocusingAlpha = 0.0;

	UPROPERTY(BlueprintReadWrite, meta = (AllowPrivateAccess = "True", Category = "Melee Combat Core|Timers"))
	float MeleeCombatPostureElapsedTime = -1;

	UPROPERTY(BlueprintReadWrite, meta = (AllowPrivateAccess = "True", Category = "Melee Combat Core|Timers"))
	float CheckCombatModeElapsedTime = -1;


//░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░
public:

	UPROPERTY(BlueprintAssignable, Category = "Melee Combat Core")
	FOnMontageLatendAction OnMontageLatendAction;

	UPROPERTY(BlueprintAssignable, Category = "Melee Combat Core")
	FMeleeCombatInputTrigger_Attack MeleeCombatInputTrigger_Attack;

	UPROPERTY(BlueprintAssignable, Category = "Melee Combat Core")
	FMeleeCombatInputTrigger_Dodge MeleeCombatInputTrigger_Dodge;

	UFUNCTION(BlueprintCallable, Category = "Melee Combat Core", meta = (DisplayName = "Call Montagel Action", Keywords = "Montage,Latend,Melee,Delegate"))
	void CallMontagelAction(CE_MontageDelegateActions Type, FName NotifyName);

	UFUNCTION(BlueprintCallable, Category = "Melee Combat Core|Inputs", meta = (CompactNodeTitle = "AttackInputTrigger", Keywords = "Melee,Delegate,Input,Attack"))
	void CallAttackInputDelegate(bool InputValue);

	UFUNCTION(BlueprintCallable, Category = "Melee Combat Core|Inputs", meta = (CompactNodeTitle = "AttackDodgeTrigger", Keywords = "Melee,Delegate,Input,Dodge"))
	void CallDodgeInputDelegate(bool InputValue);

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UFUNCTION(BlueprintPure, Category = "Melee Combat Core", meta = (BlueprintThreadSafe, WorldContext = "WorldContextObject", DisplayName = "Try Get Melee Combat Component", Keywords = "Melee,Combat,Component"))
	static UMeleeCombatCoreComponent* GryGetMeleeCombatComponent(UObject* WorldContextObject, ACharacter* Target);


public:

	UFUNCTION(BlueprintNativeEvent, BlueprintPure, Category = "Melee Combat Core", meta = (DisplayName = "Get Movement Input", Keywords = "Melee,Combat,Character,Input,Movement"))
	FVector GetCurrentMovementInput();
	virtual FVector GetCurrentMovementInput_Implementation();

	UFUNCTION(BlueprintNativeEvent, BlueprintPure, Category = "Melee Combat Core", meta = (DisplayName = "Get Character States", Keywords = "Melee,Combat,Character,States,Interface", AdvancedDisplay = 2))
	void GetCharacterMainStates
	(
		int FromEnemy,
		CALS_MovementState& MovementState,
		CALS_MovementAction& MovementAction,
		CALS_RotationMode& RotationMode,
		CALS_Gait& ActualGait,
		CALS_Stance& ActualStance,
		CALS_OverlayState& OverlayState
	);

	virtual void GetCharacterMainStates_Implementation
	(
		int FromEnemy,
		CALS_MovementState& MovementState,
		CALS_MovementAction& MovementAction,
		CALS_RotationMode& RotationMode,
		CALS_Gait& ActualGait,
		CALS_Stance& ActualStance,
		CALS_OverlayState& OverlayState
	);

	UFUNCTION(BlueprintNativeEvent, BlueprintPure, Category = "Melee Combat Core", meta = (DisplayName = "Should Mark Targeted Enemy For Finisher", Keywords = "Melee,Combat,Character,Finisher,Targeting"))
	bool ShouldMarkTargetedEnemyForSF(ACharacter* InCharacter);
	virtual bool ShouldMarkTargetedEnemyForSF_Implementation(ACharacter* InCharacter);


	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Melee Combat Core", meta = (ForceAsFunction, Keywords = "Melee,Combat,Character,State"))
	bool TrySetCurrentCombatActionState(CMaleeCombat_CurrentAction NewState);
	virtual bool TrySetCurrentCombatActionState_Implementation(CMaleeCombat_CurrentAction NewState);


	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Melee Combat Core | Searcher", meta = (ForceAsFunction, Keywords = "Melee,Combat,Searcher,CombatAnimSetup,Database", AdvancedDisplay = 2))
	bool RunSearchingAnimSetFromDatabases
	(
		UCombatAnimSetup*& ReturnAnimSetup, 
		const TArray<UCombatAnimsDatabase*>& DatabasesToSearch,
		UCombatAnimSetup* CurrentActivatedSet,
		bool bFastSearch, 
		float SearchPropertyA, 
		float SearchPropertyB, 
		int DebugType = 0, 
		float DebugDuration = 0.5
	);
	virtual bool RunSearchingAnimSetFromDatabases_Implementation(UCombatAnimSetup*& ReturnAnimSetup, const TArray<UCombatAnimsDatabase*>& DatabasesToSearch, UCombatAnimSetup* CurrentActivatedSet, bool bFastSearch,
		float SearchPropertyA, float SearchPropertyB, int DebugType = 0, float DebugDuration = 0.5);


	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Melee Combat Core | Searcher", meta = (ForceAsFunction, DisplayName = "Run Searching AnimSet Environment Context", 
		Keywords = "Melee,Combat,Searcher,CombatAnimSetup,Database", AdvancedDisplay = 2))
	bool RunSearchingAnimSetEnvContext(UCombatAnimSetup*& ReturnAnimSetup, bool bFastSearch, int DebugType = 0, float DebugDuration = 0.5);
	virtual bool RunSearchingAnimSetEnvContext_Implementation(UCombatAnimSetup*& ReturnAnimSetup, bool bFastSearch, int DebugType = 0, float DebugDuration = 0.5);


	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Melee Combat Core | Searcher", meta = (ForceAsFunction, Keywords = "Melee,Combat,Searcher,Hit,Reaction,Animation", AdvancedDisplay = 7))
	bool RunSearchingBestHitReactionAnim
	(
		UAnimSequenceBase*& ReturnAnimSeq,
		float& ReturnPlayRate,
		float& ReturnStartTime,
		ACharacter* InCharacter,
		const TArray<UCombatHitReactionsDatabase*>& DatabasesToSearch,
		FHitResult HitReactionResult,
		bool bFastSearch,
		FRotator CustomInCharRot,
		float SearchPropertyB,
		int DebugType = 0,
		float DebugDuration = 0.5
	);
	virtual bool RunSearchingBestHitReactionAnim_Implementation(UAnimSequenceBase*& ReturnAnimSeq, float& ReturnPlayRate, float& ReturnStartTime, ACharacter* InCharacter,
		const TArray<UCombatHitReactionsDatabase*>& DatabasesToSearch, FHitResult HitReactionResult, bool bFastSearch, FRotator CustomInCharRot, float SearchPropertyB, int DebugType = 0, float DebugDuration = 0.5);


	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Melee Combat Core", meta = (Keywords = "Melee,Combat,Character,Animation,Layer,Link"))
	bool TryLinkCombatAnimLayer(bool bForEnemy);
	virtual bool TryLinkCombatAnimLayer_Implementation(bool bForEnemy);

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Melee Combat Core", meta = (Keywords = "Melee,Combat,Character,Start,Attack"))
	bool StartMeleeAttackAction();
	virtual bool StartMeleeAttackAction_Implementation();

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Melee Combat Core", meta = (Keywords = "Melee,Combat,Character,MoveTo,Attack,Capsule"))
	void MoveActorsCapsulesTo(float Duration, float StopTime, bool bEase);
	virtual void MoveActorsCapsulesTo_Implementation(float Duration, float StopTime, bool bEase);

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Melee Combat Core", meta = (Keywords = "Melee,Combat,Animation,Montage,Notify,Attack"))
	bool CheckEarlyBlendOutActionMontage(UAnimSequenceBase* InAnimation, float InCurrentAnimTime, float InCurrentAnimTimeRatio, float InNotifyStateTime, bool InNotifyBlendOut);
	virtual bool CheckEarlyBlendOutActionMontage_Implementation(UAnimSequenceBase* InAnimation, float InCurrentAnimTime, float InCurrentAnimTimeRatio, float InNotifyStateTime, bool InNotifyBlendOut);

	//INPUT EVENT FUNCTION
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Melee Combat Core", meta = (Keywords = "Melee,Combat,Input,,Attack"))
	void ActualAttackInputExecute();
	virtual void ActualAttackInputExecute_Implementation();

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Melee Combat Core", meta = (Keywords = "Melee,Combat,Trace,Hit,Event", AdvancedDisplay = 1))
	void TryApplyDamageForOpponent(bool HitValid, FHitResult HitResult, float NotifyTime, float AnimTime);
	virtual void TryApplyDamageForOpponent_Implementation(bool HitValid, FHitResult HitResult, float NotifyTime, float AnimTime);


	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Melee Combat Core", meta = (Keywords = "Melee,Combat,rotation", AdvancedDisplay = 2))
	void GetFocusingOnOpponentValues(bool& RequireFocusing, FRotator& FocusingRotation, float& InterpSpeedBias, FRotator OptionalInDesiredRot);
	virtual void GetFocusingOnOpponentValues_Implementation(bool& RequireFocusing, FRotator& FocusingRotation, float& InterpSpeedBias, FRotator OptionalInDesiredRot);


	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Melee Combat Core", meta = (Keywords = "Melee,Combat,Task,Event,Gameplay"))
	void RunPerOpponentGameplayTask(bool bRun);
	virtual void RunPerOpponentGameplayTask_Implementation(bool bRun);


	//░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░
public:

/*
▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂
ENG:
Start a timer that corrects the position during CombatAnim activation. This mechanism is an equivalent of Motion Warping.
Its purpose is to calculate appropriate positions for RefChar and ChoosedOpponent so that the attack animation fits the current context. 
The algorithm is computationally quite complex due to the number of vector and matrix operations involved.

The function StartCustomWarpingPositions() initializes the entire algorithm. Below is a list of several parameters that can be configured 
during initialization:
➊ Duration – The most important value. It defines the duration of CustomWarping.
➋ FInterpMode – Currently an unused parameter.
➌ InterpSpeedLoc – Position interpolation speed. It is important when bUseInterpingPerTickWarping == true. It does not affect the 
  accumulation of RootMotion or the relative offset of the opponent’s capsule.
➍ InterpSpeedRot – Works the same way as InterpSpeedLoc, but applies to rotation interpolation.
➎ GetPositionMaxTime – Defines the time at which CustomWarping stops. This value specifies how long it takes before StopCustomWarpingUpdating() 
  is triggered. If it is < 0, the time is taken from Duration.
➏ CustomInterpCurve – Allows setting a custom interpolation curve. This is relevant when bUseInterpingPerTickWarping == false. If NULL, the 
  interpolation curve is taken from the active CombatAnimSetup.
▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂
PL:
Rozpocznij timer korygujący pozycję podczas aktywacji CombatAnim. Jest to odpowiednik mechanizmu MotionWarping. 
Mechanizm ten ma na celu obliczenie takiej pozycji RefChar oraz ChoosedOpponent aby Animacja ataku pasowała do kontekstu
Jest to dość złożony obliczeniowo algorytm po względem dokonywanych operacji na wektorach oraz macierzach.

Funcja StartCustomWarpingPositions() inicuje cały algorytm. Poniżej lista kilku parametrów możliwych do konfiguracji podczas inicjacji:
➊ Duration - Najważniejsza wartość. Określa czas trwania CustomWarping
➋ FInterpMode - Obecnie nie używany parametr
➌ InterpSpeedLoc - Szybkość interpolacji pozycji. Jest istotny w przypadku kiedy bUseInterpingPerTickWarping == true. Nie ma wpływu na 
  akumulowanie RootMotion oraz relatywnego przesunięcha kapsuły przeciwnika
➍ InterpSpeedRot - Działa tak samo jak InterpSpeedLoc, jednak dotyczy interpolacji rotacji
➎ GetPositionMaxTime - Określa czas zatrzymania CustomWarping. Wartość ta oznacza jaki czas ma upłynąć do aktywacji 
  StopCustomWarpingUpdating(). Jeżeli jest < 0 to czas ten brany jest z Duration
➏ CustomInterpCurve - Możliwość ustawienia customowej krzywej interpolacji. Istotny w przypadku kiedy bUseInterpingPerTickWarping 
  == false. Jeżeli NULL to krzywa interpolacji brana jest z aktywnego CombatAnimSetup
▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂
*/
	UFUNCTION(BlueprintCallable, Category = "Melee Combat Core", meta = (Keywords = "Melee,Combat,Movement,Timer,Warping", AdvancedDisplay = 1))
	bool StartCustomWarpingPositions(float Duration = 0.3, bool FInterpMode = false, float InterpSpeedLoc = 10, float InterpSpeedRot = 10, float GetPositionMaxTime = -1, UCurveFloat* CustomInterpCurve = nullptr);

/*
▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂
ENG:
Function responsible for updating CustomWarping. It must be executed on the TickEvent. It becomes active when StartCustomWarpingPositions()
is called. It is also possible to override its definition in Blueprint.
The final movement is composed of several factors, such as:

➊ Interpolation to the target value in DesiredPositionOwner. If ResolveToWorldSpace == true, the transform is also converted to world space.

➋ Root Motion accumulation. When CombatAnimSetup is valid, it is possible to retrieve the RootBone position from the animation. Based on this,
  a displacement delta can be calculated: GetBoneTransform('root', Time + dt) - GetBoneTransform('root', Time). This value is added to
  DesiredPositionOwner.Transform.GetLocation().

➌ Local displacement of the opponent’s capsule. This can be calculated either as an accumulated value or as the difference between the current
  position and the position at the time CustomWarping was initialized.

▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂
PL:
Funkcja aktualizująca CustomWarping. Musi być wykonywana na TickEvent. Jest aktywna w momencie wywołania StartCustomWarpingPositions().
Istnieje możliwość nadpisania definicji w Blueprint.

Na finalny ruch składa się wiele czynników takch jak:
➊ Interpolacja do zadanej wartość w DesiredPositionOwner. Jeżeli ResolveToWorldSpace == true zachodzi również zmiana transformacji na
   przestrzeń globalną

➋ Akumulowanie RootMotion. Kiedy CombatAnimSetup jest valid możliwe jest pobranie a animacji pozycję RootBone. Na tej podstawie można
   wyznaczyć Deltę przemieszczenia GetBoneTransform('root', Time + dt) - GetBoneTransform('root', Time). Wartość ta jest dodawana do
   DesiredPositionOwner.Transform.GetLocation()

➌ Lokalne przemieszczenie kapsuły przeciwnika. Może być obliczany jako wartość akumulowana lub jako różnica obecnej pozycji względem
   pozycji podczas inicjacji CustomWarping
▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂
*/
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Melee Combat Core", meta = (ForceAsFunction, Keywords = "Melee,Combat,Movement,Timer,Warping,Tick", AdvancedDisplay = 1))
	void UpdateCustomWarpingCharacters(float dt, bool UseFInterpModeUpdating, bool ResolveToWorldSpace, float RootDeltaApplyScale = 1.0);
	virtual void UpdateCustomWarpingCharacters_Implementation(float dt, bool UseFInterpModeUpdating, bool ResolveToWorldSpace, float RootDeltaApplyScale = 1.0);

/*
Easily stop further Warping of a position by stopping the Timer function.
*/
	UFUNCTION(BlueprintCallable, Category = "Melee Combat Core", meta = (Keywords = "Melee,Combat,Movement,Timer,Warping"))
	bool StopCustomWarpingUpdating();

/*
Returns the elapsed time of the timer associated with CustomWarping. The time is not normalized.
*/
	UFUNCTION(BlueprintPure, Category = "Melee Combat Core", meta = (Keywords = "Melee,Combat,Movement,Timer,Warping"))
	bool GetCustomWarpingElapsedTime(float& ReturnTime);

/*
ENG:
A function that affects the scale of the displacement associated with GetChoosedEnemyChar(). Several conditions are required for 
this value to influence CustomWarping behavior. By default, when the value is 1.0, the accumulated position difference of 
GetChoosedEnemyChar() will translate all movement into the warping of RefChar. If it is 0.5, then the position delta (as 
CurrentPosition - PrevPosition) will be multiplied by 0.5 from the moment it is set.

PL:
Funkcja wpływająca na skalę przemieszczenia związanego z GetChoosedEnemyChar(). Wymagane jest kilka warunków do tego aby ta wartość 
wpływała na zachowanie podczas CustomWarping. Domyślnie kiedy wartość to 1.0 w przypadku akumulacji różnicy pozycji 
GetChoosedEnemyChar() cały ruch będzie przekładany na Warping dla RefChar. Jeżeli będzie to 0.5 to wtedy od momentu ustawienia delta 
pozycji (jako CurrentPosition - PrevPosition) będzie pomnożona przez 0.5.
*/
	UFUNCTION(BlueprintCallable, Category = "Melee Combat Core", meta = (Keywords = "Melee,Combat,Movement,Timer,Warping"))
	void SetCustomWarpingFollowOpponentMotion(float MotionFollowingScale = 1.0);

	UFUNCTION(BlueprintCallable, Category = "Melee Combat Core", meta = (Keywords = "Melee,Combat,Movement,Transforms,Warping", AdvancedDisplay = 4))
	void CalculateRquiredWarpingPositions(bool& ReturnSolved, FCALS_ComponentAndTransform& ReturnAttacker, FCALS_ComponentAndTransform& ReturnOpponent, float FindingFloorTraceUp = 40, float FindingFloorTraceDown = 30, 
		float FreeSpaceRadiusCheck = 60, float FreeSpaceHeightCheck = 30);

	UPrimitiveComponent* SimpleFindFloorComponentByTrace(FVector CheckLocation, float TraceUp = 40, float TraceDown = 30);
	FVector ApplySafeMoveToLocation(AActor* InActor, FVector DesiredPosition, float InterpScale = 1.0, float DeltaT = 0.01);

private:
	//Custom Warping Values
	float UpdatingWarpingDuration = 0.3;
	FTimerHandle UpdatingWarpingTimer;
	FTransform InitWarpingPositionOwner = FTransform::Identity;
	FTransform InitWarpingPositionEnemy = FTransform::Identity;
	FVector UpdatingWarpingParams = FVector(0, 0, 0);
	int CurrentUpdateRotationsMode = 0;
	FVector AccumulatedRootOffset = FVector(0, 0, 0);
	FVector AccumulatedEnemyOffset = FVector::ZeroVector;
	FVector EnemyPositionDelta = FVector::ZeroVector;
	FCALS_ComponentAndTransform OnWarpingStartEnemyPosition;
	UCurveFloat* CustomWarpingCurve = nullptr;
	float OpponentMotionFollowScale = 1.0;
	void OnWapingUpdatingFinished() {};
	//░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░



public:
	UFUNCTION(BlueprintPure, Category = "Melee Combat Core", meta = (Keywords = "Melee,Combat,Character,Actor"))
	ACharacter* GetTargetedCharacter() const;

	UFUNCTION(BlueprintPure, Category = "Melee Combat Core", meta = (Keywords = "Melee,Combat,Character,Actor"))
	ACharacter* GetChoosedEnemyChar() const;

	UFUNCTION(BlueprintPure, Category = "Melee Combat Core", meta = (Keywords = "Melee,Combat,Character,Actor"))
	float GetOwnerLinkedAnimClassElapsedTime() const;

	UFUNCTION(BlueprintPure, Category = "Melee Combat Core", meta = (Keywords = "Melee,Combat,Character,Rotatation,Focus"))
	bool GetShouldFocusOnOpponent() const;

	UFUNCTION(BlueprintCallable, Category = "Utility", meta = (DisplayName = "Simple Timer Updater By Ref", Keywords = "Melee,Combat,Timer,Counter,Elapsed"))
	void SimpleTimerUpdater(bool& IsRunning, bool& IsStopped, UPARAM(ref) float& InTimerVar, float Delta);

private:

	UPROPERTY(Transient)
	TObjectPtr<UAnimInstance> PendingAnimInstance = nullptr;

	TSharedPtr<FStreamableHandle> CombatLayerLoadHandle;
	bool bCombatLayerLoadInProgress = false;
	float LinkedAnimLayerTimeElapsed = -1.0;
	void OnCombatAnimLayerClassLoaded();
	void UnlinkAnimLayer(bool ForEnemy);

	float CustomFloatInterp(float A, float B, float Alpha, CustomWarpingInterpFunction FunctionType = CustomWarpingInterpFunction::Linear);

	template<typename T>
	FORCEINLINE T& IgnoreOut()
	{
		// Oddzielny bufor na wątek; bezpieczny dla Game Thread.
		static thread_local T Dummy{};   // wymaga domyślnego konstruktora / trivialnego T
		return Dummy;
	}
		
};
