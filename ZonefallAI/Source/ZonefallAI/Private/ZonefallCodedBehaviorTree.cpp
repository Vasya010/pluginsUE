#include "ZonefallCodedBehaviorTree.h"

#include "BTDecorator_ZonefallHasLineOfSight.h"
#include "BTDecorator_ZonefallIsSquadRole.h"
#include "BTDecorator_ZonefallIsSuppressed.h"
#include "BTDecorator_ZonefallShouldInvestigate.h"
#include "BTService_ZonefallReportToSquad.h"
#include "BTService_ZonefallUpdateTarget.h"
#include "BTTask_ZonefallBark.h"
#include "BTTask_ZonefallClearMemoryState.h"
#include "BTTask_ZonefallFindPatrolPoint.h"
#include "BTTask_ZonefallFlankTarget.h"
#include "BTTask_ZonefallMoveToCover.h"
#include "BTTask_ZonefallPickInvestigationPoint.h"
#include "BTTask_ZonefallRequestSquadRole.h"
#include "BTTask_ZonefallSeedBlackboard.h"
#include "BTTask_ZonefallSetFocus.h"
#include "BTTask_ZonefallSmartMoveTo.h"
#include "BTTask_ZonefallSuppressFire.h"
#include "BehaviorTree/BehaviorTree.h"
#include "BehaviorTree/Blackboard/BlackboardKeyType_Bool.h"
#include "BehaviorTree/Blackboard/BlackboardKeyType_Enum.h"
#include "BehaviorTree/Blackboard/BlackboardKeyType_Float.h"
#include "BehaviorTree/Blackboard/BlackboardKeyType_Int.h"
#include "BehaviorTree/Blackboard/BlackboardKeyType_Name.h"
#include "BehaviorTree/Blackboard/BlackboardKeyType_Object.h"
#include "BehaviorTree/Blackboard/BlackboardKeyType_Vector.h"
#include "BehaviorTree/BlackboardData.h"
#include "BehaviorTree/Composites/BTComposite_Selector.h"
#include "BehaviorTree/Composites/BTComposite_Sequence.h"
#include "BehaviorTree/Tasks/BTTask_Wait.h"
#include "GameFramework/Actor.h"

namespace ZonefallCodedBT
{
	template <typename KeyType>
	KeyType* AddKey(UBlackboardData* BlackboardAsset, const FName KeyName)
	{
		return BlackboardAsset ? BlackboardAsset->UpdatePersistentKey<KeyType>(KeyName) : nullptr;
	}

	FBlackboardKeySelector MakeSelector(const FName KeyName, TSubclassOf<UBlackboardKeyType> KeyType)
	{
		FBlackboardKeySelector Selector;
		Selector.SelectedKeyName = KeyName;
		Selector.SelectedKeyType = KeyType;
		return Selector;
	}

	FBTCompositeChild MakeCompositeChild(UBTCompositeNode* ChildComposite)
	{
		FBTCompositeChild Child;
		Child.ChildComposite = ChildComposite;
		return Child;
	}

	FBTCompositeChild MakeTaskChild(UBTTaskNode* ChildTask)
	{
		FBTCompositeChild Child;
		Child.ChildTask = ChildTask;
		return Child;
	}

	void AddDecorator(FBTCompositeChild& Child, UBTDecorator* Decorator)
	{
		Child.Decorators.Add(Decorator);
		Child.DecoratorOps.Add(FBTDecoratorLogic(EBTDecoratorLogic::Test, 0));
	}

	template <typename NodeType>
	NodeType* NewNode(UBehaviorTree* Tree, const TCHAR* Name)
	{
		NodeType* Node = NewObject<NodeType>(Tree, FName(Name));
		Node->SetFlags(RF_Transactional);
		return Node;
	}
}

UBlackboardData* UZonefallCodedBehaviorTreeLibrary::CreateAdventureBlackboard(UObject* Outer, const FZonefallAIBlackboardKeys& Keys)
{
	UObject* SafeOuter = Outer ? Outer : GetTransientPackage();
	UBlackboardData* BlackboardAsset = NewObject<UBlackboardData>(SafeOuter, TEXT("BB_Zonefall_CodedAdventure"));
	BlackboardAsset->SetFlags(RF_Transactional);

	if (UBlackboardKeyType_Object* TargetActorKey = ZonefallCodedBT::AddKey<UBlackboardKeyType_Object>(BlackboardAsset, Keys.TargetActor))
	{
		TargetActorKey->BaseClass = AActor::StaticClass();
	}
	if (UBlackboardKeyType_Object* FocusActorKey = ZonefallCodedBT::AddKey<UBlackboardKeyType_Object>(BlackboardAsset, Keys.FocusActor))
	{
		FocusActorKey->BaseClass = AActor::StaticClass();
	}

	ZonefallCodedBT::AddKey<UBlackboardKeyType_Bool>(BlackboardAsset, Keys.HasTarget);
	ZonefallCodedBT::AddKey<UBlackboardKeyType_Vector>(BlackboardAsset, Keys.LastKnownTargetLocation);
	ZonefallCodedBT::AddKey<UBlackboardKeyType_Vector>(BlackboardAsset, Keys.LastHeardLocation);
	ZonefallCodedBT::AddKey<UBlackboardKeyType_Vector>(BlackboardAsset, Keys.InvestigationLocation);
	ZonefallCodedBT::AddKey<UBlackboardKeyType_Float>(BlackboardAsset, Keys.LastSeenTime);
	ZonefallCodedBT::AddKey<UBlackboardKeyType_Bool>(BlackboardAsset, Keys.HasLineOfSight);
	ZonefallCodedBT::AddKey<UBlackboardKeyType_Vector>(BlackboardAsset, Keys.MoveLocation);
	ZonefallCodedBT::AddKey<UBlackboardKeyType_Vector>(BlackboardAsset, Keys.HomeLocation);
	ZonefallCodedBT::AddKey<UBlackboardKeyType_Vector>(BlackboardAsset, Keys.PatrolAnchor);
	ZonefallCodedBT::AddKey<UBlackboardKeyType_Float>(BlackboardAsset, Keys.PatrolRadius);
	ZonefallCodedBT::AddKey<UBlackboardKeyType_Float>(BlackboardAsset, Keys.SearchRadius);
	ZonefallCodedBT::AddKey<UBlackboardKeyType_Float>(BlackboardAsset, Keys.PatrolAcceptanceRadius);
	ZonefallCodedBT::AddKey<UBlackboardKeyType_Float>(BlackboardAsset, Keys.PatrolWaitTime);
	ZonefallCodedBT::AddKey<UBlackboardKeyType_Float>(BlackboardAsset, Keys.Alertness);
	ZonefallCodedBT::AddKey<UBlackboardKeyType_Float>(BlackboardAsset, Keys.MemoryConfidence);
	ZonefallCodedBT::AddKey<UBlackboardKeyType_Float>(BlackboardAsset, Keys.Suspicion);
	ZonefallCodedBT::AddKey<UBlackboardKeyType_Float>(BlackboardAsset, Keys.TimeSinceLastStimulus);
	ZonefallCodedBT::AddKey<UBlackboardKeyType_Bool>(BlackboardAsset, Keys.ShouldInvestigate);
	ZonefallCodedBT::AddKey<UBlackboardKeyType_Bool>(BlackboardAsset, Keys.IsInvestigating);
	ZonefallCodedBT::AddKey<UBlackboardKeyType_Float>(BlackboardAsset, Keys.DesiredSpeed);
	ZonefallCodedBT::AddKey<UBlackboardKeyType_Name>(BlackboardAsset, Keys.SelfArchetypeTag);
	ZonefallCodedBT::AddKey<UBlackboardKeyType_Name>(BlackboardAsset, Keys.SelfRoleTag);
	ZonefallCodedBT::AddKey<UBlackboardKeyType_Name>(BlackboardAsset, Keys.SelfFactionTag);
	ZonefallCodedBT::AddKey<UBlackboardKeyType_Name>(BlackboardAsset, Keys.SelfDispositionTag);
	ZonefallCodedBT::AddKey<UBlackboardKeyType_Name>(BlackboardAsset, Keys.SelfRelationshipTag);
	ZonefallCodedBT::AddKey<UBlackboardKeyType_Name>(BlackboardAsset, Keys.TargetArchetypeTag);
	ZonefallCodedBT::AddKey<UBlackboardKeyType_Name>(BlackboardAsset, Keys.TargetRoleTag);
	ZonefallCodedBT::AddKey<UBlackboardKeyType_Name>(BlackboardAsset, Keys.TargetFactionTag);
	ZonefallCodedBT::AddKey<UBlackboardKeyType_Name>(BlackboardAsset, Keys.TargetDispositionTag);
	ZonefallCodedBT::AddKey<UBlackboardKeyType_Name>(BlackboardAsset, Keys.TargetRelationshipTag);
	ZonefallCodedBT::AddKey<UBlackboardKeyType_Float>(BlackboardAsset, Keys.TargetThreatScore);

	if (UBlackboardKeyType_Enum* ThreatStateKey = ZonefallCodedBT::AddKey<UBlackboardKeyType_Enum>(BlackboardAsset, Keys.ThreatState))
	{
		ThreatStateKey->EnumType = StaticEnum<EZonefallAIThreatState>();
		ThreatStateKey->EnumName = TEXT("EZonefallAIThreatState");
	}
	if (UBlackboardKeyType_Enum* StimulusTypeKey = ZonefallCodedBT::AddKey<UBlackboardKeyType_Enum>(BlackboardAsset, Keys.StimulusType))
	{
		StimulusTypeKey->EnumType = StaticEnum<EZonefallAIStimulusType>();
		StimulusTypeKey->EnumName = TEXT("EZonefallAIStimulusType");
	}
	if (UBlackboardKeyType_Enum* SquadRoleKey = ZonefallCodedBT::AddKey<UBlackboardKeyType_Enum>(BlackboardAsset, Keys.SquadRole))
	{
		SquadRoleKey->EnumType = StaticEnum<EZonefallAISquadRole>();
		SquadRoleKey->EnumName = TEXT("EZonefallAISquadRole");
	}
	if (UBlackboardKeyType_Enum* TacticalIntentKey = ZonefallCodedBT::AddKey<UBlackboardKeyType_Enum>(BlackboardAsset, Keys.TacticalIntent))
	{
		TacticalIntentKey->EnumType = StaticEnum<EZonefallAITacticalIntent>();
		TacticalIntentKey->EnumName = TEXT("EZonefallAITacticalIntent");
	}
	if (UBlackboardKeyType_Enum* FlankSideKey = ZonefallCodedBT::AddKey<UBlackboardKeyType_Enum>(BlackboardAsset, Keys.FlankSide))
	{
		FlankSideKey->EnumType = StaticEnum<EZonefallAIFlankSide>();
		FlankSideKey->EnumName = TEXT("EZonefallAIFlankSide");
	}

	ZonefallCodedBT::AddKey<UBlackboardKeyType_Bool>(BlackboardAsset, Keys.HasAttackToken);
	ZonefallCodedBT::AddKey<UBlackboardKeyType_Bool>(BlackboardAsset, Keys.HasCover);
	ZonefallCodedBT::AddKey<UBlackboardKeyType_Bool>(BlackboardAsset, Keys.IsSuppressed);
	ZonefallCodedBT::AddKey<UBlackboardKeyType_Bool>(BlackboardAsset, Keys.IsRetreating);
	ZonefallCodedBT::AddKey<UBlackboardKeyType_Float>(BlackboardAsset, Keys.SquadCohesion);
	ZonefallCodedBT::AddKey<UBlackboardKeyType_Float>(BlackboardAsset, Keys.CoverScore);
	ZonefallCodedBT::AddKey<UBlackboardKeyType_Float>(BlackboardAsset, Keys.SuppressionLevel);
	ZonefallCodedBT::AddKey<UBlackboardKeyType_Float>(BlackboardAsset, Keys.HealthFraction);
	ZonefallCodedBT::AddKey<UBlackboardKeyType_Int>(BlackboardAsset, Keys.SquadMemberCount);
	ZonefallCodedBT::AddKey<UBlackboardKeyType_Vector>(BlackboardAsset, Keys.CoverLocation);
	ZonefallCodedBT::AddKey<UBlackboardKeyType_Vector>(BlackboardAsset, Keys.CoverFacing);
	ZonefallCodedBT::AddKey<UBlackboardKeyType_Vector>(BlackboardAsset, Keys.FlankPivotLocation);

	if (UBlackboardKeyType_Object* SquadLeaderKey = ZonefallCodedBT::AddKey<UBlackboardKeyType_Object>(BlackboardAsset, Keys.SquadLeader))
	{
		SquadLeaderKey->BaseClass = AActor::StaticClass();
	}
	if (UBlackboardKeyType_Object* SuppressionSourceKey = ZonefallCodedBT::AddKey<UBlackboardKeyType_Object>(BlackboardAsset, Keys.SuppressionSource))
	{
		SuppressionSourceKey->BaseClass = AActor::StaticClass();
	}

	BlackboardAsset->UpdateKeyIDs();
	BlackboardAsset->UpdateIfHasSynchronizedKeys();
	return BlackboardAsset;
}

UBehaviorTree* UZonefallCodedBehaviorTreeLibrary::CreateAdventureBehaviorTree(UObject* Outer, UBlackboardData* BlackboardAsset, const FZonefallCodedBehaviorTreeSettings& Settings)
{
	UObject* SafeOuter = Outer ? Outer : GetTransientPackage();
	UBehaviorTree* Tree = NewObject<UBehaviorTree>(SafeOuter, TEXT("BT_Zonefall_CodedAdventure"));
	Tree->SetFlags(RF_Transactional);
	Tree->BlackboardAsset = BlackboardAsset ? BlackboardAsset : CreateAdventureBlackboard(Tree, Settings.BlackboardKeys);

	UBTComposite_Sequence* RootSequence = ZonefallCodedBT::NewNode<UBTComposite_Sequence>(Tree, TEXT("Root_Sequence"));
	Tree->RootNode = RootSequence;

	UBTComposite_Selector* DecisionSelector = ZonefallCodedBT::NewNode<UBTComposite_Selector>(Tree, TEXT("Selector_Decision"));

	UBTService_ZonefallUpdateTarget* UpdateTargetService = ZonefallCodedBT::NewNode<UBTService_ZonefallUpdateTarget>(Tree, TEXT("Service_UpdateTarget"));
	UpdateTargetService->BlackboardKeys = Settings.BlackboardKeys;
	UpdateTargetService->MaxTargetDistance = Settings.MaxTargetDistance;
	UpdateTargetService->LostTargetGraceTime = Settings.LostTargetGraceTime;
	UpdateTargetService->bRequireLineOfSight = false;
	RootSequence->Services.Add(UpdateTargetService);

	UBTTask_ZonefallSeedBlackboard* SeedTask = ZonefallCodedBT::NewNode<UBTTask_ZonefallSeedBlackboard>(Tree, TEXT("Task_SeedBlackboard"));
	SeedTask->BlackboardKeys = Settings.BlackboardKeys;

	UBTComposite_Sequence* CombatSequence = ZonefallCodedBT::NewNode<UBTComposite_Sequence>(Tree, TEXT("Sequence_Combat"));
	UBTDecorator_ZonefallHasLineOfSight* CombatDecorator = ZonefallCodedBT::NewNode<UBTDecorator_ZonefallHasLineOfSight>(Tree, TEXT("Decorator_CombatLineOfSight"));
	CombatDecorator->SetCodedBlackboardKey(ZonefallCodedBT::MakeSelector(Settings.BlackboardKeys.TargetActor, UBlackboardKeyType_Object::StaticClass()));
	CombatDecorator->MaxDistance = Settings.MaxTargetDistance;

	UBTService_ZonefallReportToSquad* ReportToSquadService = ZonefallCodedBT::NewNode<UBTService_ZonefallReportToSquad>(Tree, TEXT("Service_ReportToSquad"));
	ReportToSquadService->BlackboardKeys = Settings.BlackboardKeys;
	ReportToSquadService->bAutoRegister = true;
	CombatSequence->Services.Add(ReportToSquadService);

	UBTTask_ZonefallRequestSquadRole* RequestRoleTask = ZonefallCodedBT::NewNode<UBTTask_ZonefallRequestSquadRole>(Tree, TEXT("Task_RequestSquadRole"));
	RequestRoleTask->BlackboardKeys = Settings.BlackboardKeys;

	UBTTask_ZonefallSetFocus* FocusTargetTask = ZonefallCodedBT::NewNode<UBTTask_ZonefallSetFocus>(Tree, TEXT("Task_FocusTarget"));
	FocusTargetTask->SetCodedBlackboardKey(ZonefallCodedBT::MakeSelector(Settings.BlackboardKeys.TargetActor, UBlackboardKeyType_Object::StaticClass()));

	UBTComposite_Selector* TacticalRoleSelector = ZonefallCodedBT::NewNode<UBTComposite_Selector>(Tree, TEXT("Selector_TacticalRole"));

	// --- Suppressed branch: take cover and cower
	UBTComposite_Sequence* SuppressedSequence = ZonefallCodedBT::NewNode<UBTComposite_Sequence>(Tree, TEXT("Sequence_Suppressed"));
	UBTTask_ZonefallMoveToCover* SuppressedCoverTask = ZonefallCodedBT::NewNode<UBTTask_ZonefallMoveToCover>(Tree, TEXT("Task_SuppressedCover"));
	SuppressedCoverTask->BlackboardKeys = Settings.BlackboardKeys;
	UBTTask_ZonefallSmartMoveTo* SuppressedMoveTask = ZonefallCodedBT::NewNode<UBTTask_ZonefallSmartMoveTo>(Tree, TEXT("Task_MoveToSuppressedCover"));
	SuppressedMoveTask->SetCodedBlackboardKey(ZonefallCodedBT::MakeSelector(Settings.BlackboardKeys.MoveLocation, UBlackboardKeyType_Vector::StaticClass()));
	SuppressedMoveTask->DesiredSpeedKey = ZonefallCodedBT::MakeSelector(Settings.BlackboardKeys.DesiredSpeed, UBlackboardKeyType_Float::StaticClass());
	SuppressedMoveTask->AcceptanceRadius = Settings.CombatAcceptanceRadius * 0.6f;
	SuppressedMoveTask->DesiredSpeed = Settings.CombatMoveSpeed * 1.1f;
	UBTTask_Wait* SuppressedWaitTask = ZonefallCodedBT::NewNode<UBTTask_Wait>(Tree, TEXT("Task_SuppressedWait"));
	SuppressedWaitTask->WaitTime = FValueOrBBKey_Float(1.25f);
	SuppressedWaitTask->RandomDeviation = FValueOrBBKey_Float(0.5f);
	SuppressedSequence->Children.Add(ZonefallCodedBT::MakeTaskChild(SuppressedCoverTask));
	SuppressedSequence->Children.Add(ZonefallCodedBT::MakeTaskChild(SuppressedMoveTask));
	SuppressedSequence->Children.Add(ZonefallCodedBT::MakeTaskChild(SuppressedWaitTask));
	FBTCompositeChild SuppressedChild = ZonefallCodedBT::MakeCompositeChild(SuppressedSequence);
	UBTDecorator_ZonefallIsSuppressed* SuppressedDecorator = ZonefallCodedBT::NewNode<UBTDecorator_ZonefallIsSuppressed>(Tree, TEXT("Decorator_IsSuppressed"));
	SuppressedDecorator->BlackboardKeys = Settings.BlackboardKeys;
	SuppressedDecorator->MinimumLevel = 0.55f;
	ZonefallCodedBT::AddDecorator(SuppressedChild, SuppressedDecorator);

	// --- Flanker branch: choose flank point and move there
	UBTComposite_Sequence* FlankerSequence = ZonefallCodedBT::NewNode<UBTComposite_Sequence>(Tree, TEXT("Sequence_Flanker"));
	UBTTask_ZonefallBark* FlankerBarkTask = ZonefallCodedBT::NewNode<UBTTask_ZonefallBark>(Tree, TEXT("Task_BarkFlanking"));
	FlankerBarkTask->Category = EZonefallAIBarkCategory::Flanking;
	UBTTask_ZonefallFlankTarget* FlankTask = ZonefallCodedBT::NewNode<UBTTask_ZonefallFlankTarget>(Tree, TEXT("Task_FlankTarget"));
	FlankTask->BlackboardKeys = Settings.BlackboardKeys;
	UBTTask_ZonefallSmartMoveTo* FlankerMoveTask = ZonefallCodedBT::NewNode<UBTTask_ZonefallSmartMoveTo>(Tree, TEXT("Task_MoveToFlank"));
	FlankerMoveTask->SetCodedBlackboardKey(ZonefallCodedBT::MakeSelector(Settings.BlackboardKeys.MoveLocation, UBlackboardKeyType_Vector::StaticClass()));
	FlankerMoveTask->DesiredSpeedKey = ZonefallCodedBT::MakeSelector(Settings.BlackboardKeys.DesiredSpeed, UBlackboardKeyType_Float::StaticClass());
	FlankerMoveTask->AcceptanceRadius = Settings.CombatAcceptanceRadius;
	FlankerMoveTask->DesiredSpeed = Settings.CombatMoveSpeed;
	FlankerSequence->Children.Add(ZonefallCodedBT::MakeTaskChild(FlankerBarkTask));
	FlankerSequence->Children.Add(ZonefallCodedBT::MakeTaskChild(FlankTask));
	FlankerSequence->Children.Add(ZonefallCodedBT::MakeTaskChild(FlankerMoveTask));
	FBTCompositeChild FlankerChild = ZonefallCodedBT::MakeCompositeChild(FlankerSequence);
	UBTDecorator_ZonefallIsSquadRole* FlankerDecorator = ZonefallCodedBT::NewNode<UBTDecorator_ZonefallIsSquadRole>(Tree, TEXT("Decorator_IsFlanker"));
	FlankerDecorator->BlackboardKeys = Settings.BlackboardKeys;
	FlankerDecorator->RequiredRole = EZonefallAISquadRole::Flanker;
	ZonefallCodedBT::AddDecorator(FlankerChild, FlankerDecorator);

	// --- Suppressor branch: stay in cover and suppress the target/LKP
	UBTComposite_Sequence* SuppressorSequence = ZonefallCodedBT::NewNode<UBTComposite_Sequence>(Tree, TEXT("Sequence_Suppressor"));
	UBTTask_ZonefallMoveToCover* SuppressorCoverTask = ZonefallCodedBT::NewNode<UBTTask_ZonefallMoveToCover>(Tree, TEXT("Task_SuppressorCover"));
	SuppressorCoverTask->BlackboardKeys = Settings.BlackboardKeys;
	UBTTask_ZonefallSmartMoveTo* SuppressorMoveTask = ZonefallCodedBT::NewNode<UBTTask_ZonefallSmartMoveTo>(Tree, TEXT("Task_MoveToSuppressorCover"));
	SuppressorMoveTask->SetCodedBlackboardKey(ZonefallCodedBT::MakeSelector(Settings.BlackboardKeys.MoveLocation, UBlackboardKeyType_Vector::StaticClass()));
	SuppressorMoveTask->DesiredSpeedKey = ZonefallCodedBT::MakeSelector(Settings.BlackboardKeys.DesiredSpeed, UBlackboardKeyType_Float::StaticClass());
	SuppressorMoveTask->AcceptanceRadius = Settings.CombatAcceptanceRadius * 0.8f;
	SuppressorMoveTask->DesiredSpeed = Settings.CombatMoveSpeed * 0.85f;
	UBTTask_ZonefallSuppressFire* SuppressFireTask = ZonefallCodedBT::NewNode<UBTTask_ZonefallSuppressFire>(Tree, TEXT("Task_SuppressFire"));
	SuppressFireTask->BlackboardKeys = Settings.BlackboardKeys;
	UBTTask_Wait* SuppressorHoldTask = ZonefallCodedBT::NewNode<UBTTask_Wait>(Tree, TEXT("Task_SuppressorHold"));
	SuppressorHoldTask->WaitTime = FValueOrBBKey_Float(1.5f);
	SuppressorHoldTask->RandomDeviation = FValueOrBBKey_Float(0.6f);
	SuppressorSequence->Children.Add(ZonefallCodedBT::MakeTaskChild(SuppressorCoverTask));
	SuppressorSequence->Children.Add(ZonefallCodedBT::MakeTaskChild(SuppressorMoveTask));
	SuppressorSequence->Children.Add(ZonefallCodedBT::MakeTaskChild(SuppressFireTask));
	SuppressorSequence->Children.Add(ZonefallCodedBT::MakeTaskChild(SuppressorHoldTask));
	FBTCompositeChild SuppressorChild = ZonefallCodedBT::MakeCompositeChild(SuppressorSequence);
	UBTDecorator_ZonefallIsSquadRole* SuppressorDecorator = ZonefallCodedBT::NewNode<UBTDecorator_ZonefallIsSquadRole>(Tree, TEXT("Decorator_IsSuppressor"));
	SuppressorDecorator->BlackboardKeys = Settings.BlackboardKeys;
	SuppressorDecorator->RequiredRole = EZonefallAISquadRole::Suppressor;
	ZonefallCodedBT::AddDecorator(SuppressorChild, SuppressorDecorator);

	// --- Attacker (default): charge the target
	UBTComposite_Sequence* AttackerSequence = ZonefallCodedBT::NewNode<UBTComposite_Sequence>(Tree, TEXT("Sequence_Attacker"));
	UBTTask_ZonefallSmartMoveTo* MoveToTargetTask = ZonefallCodedBT::NewNode<UBTTask_ZonefallSmartMoveTo>(Tree, TEXT("Task_MoveToTarget"));
	MoveToTargetTask->SetCodedBlackboardKey(ZonefallCodedBT::MakeSelector(Settings.BlackboardKeys.TargetActor, UBlackboardKeyType_Object::StaticClass()));
	MoveToTargetTask->DesiredSpeedKey = ZonefallCodedBT::MakeSelector(Settings.BlackboardKeys.DesiredSpeed, UBlackboardKeyType_Float::StaticClass());
	MoveToTargetTask->AcceptanceRadius = Settings.CombatAcceptanceRadius;
	MoveToTargetTask->DesiredSpeed = Settings.CombatMoveSpeed;
	MoveToTargetTask->bAllowBlackboardDesiredSpeedOverride = true;
	MoveToTargetTask->bTrackMovingGoalActor = true;
	AttackerSequence->Children.Add(ZonefallCodedBT::MakeTaskChild(MoveToTargetTask));

	TacticalRoleSelector->Children.Add(SuppressedChild);
	TacticalRoleSelector->Children.Add(FlankerChild);
	TacticalRoleSelector->Children.Add(SuppressorChild);
	TacticalRoleSelector->Children.Add(ZonefallCodedBT::MakeCompositeChild(AttackerSequence));

	CombatSequence->Children.Add(ZonefallCodedBT::MakeTaskChild(RequestRoleTask));
	CombatSequence->Children.Add(ZonefallCodedBT::MakeTaskChild(FocusTargetTask));
	CombatSequence->Children.Add(ZonefallCodedBT::MakeCompositeChild(TacticalRoleSelector));

	UBTComposite_Sequence* InvestigateSequence = ZonefallCodedBT::NewNode<UBTComposite_Sequence>(Tree, TEXT("Sequence_Investigate"));
	UBTTask_ZonefallPickInvestigationPoint* PickInvestigationPointTask = ZonefallCodedBT::NewNode<UBTTask_ZonefallPickInvestigationPoint>(Tree, TEXT("Task_PickInvestigationPoint"));
	PickInvestigationPointTask->BlackboardKeys = Settings.BlackboardKeys;
	PickInvestigationPointTask->MoveLocationKey = ZonefallCodedBT::MakeSelector(Settings.BlackboardKeys.MoveLocation, UBlackboardKeyType_Vector::StaticClass());

	UBTTask_ZonefallSetFocus* FocusLastKnownTask = ZonefallCodedBT::NewNode<UBTTask_ZonefallSetFocus>(Tree, TEXT("Task_FocusLastKnownLocation"));
	FocusLastKnownTask->SetCodedBlackboardKey(ZonefallCodedBT::MakeSelector(Settings.BlackboardKeys.MoveLocation, UBlackboardKeyType_Vector::StaticClass()));

	UBTTask_ZonefallSmartMoveTo* MoveToLastKnownTask = ZonefallCodedBT::NewNode<UBTTask_ZonefallSmartMoveTo>(Tree, TEXT("Task_MoveToLastKnownLocation"));
	MoveToLastKnownTask->SetCodedBlackboardKey(ZonefallCodedBT::MakeSelector(Settings.BlackboardKeys.MoveLocation, UBlackboardKeyType_Vector::StaticClass()));
	MoveToLastKnownTask->DesiredSpeedKey = ZonefallCodedBT::MakeSelector(Settings.BlackboardKeys.DesiredSpeed, UBlackboardKeyType_Float::StaticClass());
	MoveToLastKnownTask->AcceptanceRadius = Settings.InvestigationAcceptanceRadius;
	MoveToLastKnownTask->DesiredSpeed = Settings.InvestigationMoveSpeed;

	UBTTask_ZonefallClearMemoryState* ClearMemoryStateTask = ZonefallCodedBT::NewNode<UBTTask_ZonefallClearMemoryState>(Tree, TEXT("Task_ClearMemoryState"));
	ClearMemoryStateTask->BlackboardKeys = Settings.BlackboardKeys;

	InvestigateSequence->Children.Add(ZonefallCodedBT::MakeTaskChild(PickInvestigationPointTask));
	InvestigateSequence->Children.Add(ZonefallCodedBT::MakeTaskChild(FocusLastKnownTask));
	InvestigateSequence->Children.Add(ZonefallCodedBT::MakeTaskChild(MoveToLastKnownTask));
	InvestigateSequence->Children.Add(ZonefallCodedBT::MakeTaskChild(ClearMemoryStateTask));

	UBTComposite_Sequence* PatrolSequence = ZonefallCodedBT::NewNode<UBTComposite_Sequence>(Tree, TEXT("Sequence_Patrol"));
	UBTTask_ZonefallFindPatrolPoint* FindPatrolPointTask = ZonefallCodedBT::NewNode<UBTTask_ZonefallFindPatrolPoint>(Tree, TEXT("Task_FindPatrolPoint"));
	FindPatrolPointTask->MoveLocationKey = ZonefallCodedBT::MakeSelector(Settings.BlackboardKeys.MoveLocation, UBlackboardKeyType_Vector::StaticClass());
	FindPatrolPointTask->AnchorLocationKey = ZonefallCodedBT::MakeSelector(Settings.BlackboardKeys.PatrolAnchor, UBlackboardKeyType_Vector::StaticClass());
	FindPatrolPointTask->RadiusKey = ZonefallCodedBT::MakeSelector(Settings.BlackboardKeys.PatrolRadius, UBlackboardKeyType_Float::StaticClass());
	FindPatrolPointTask->AcceptanceRadiusKey = ZonefallCodedBT::MakeSelector(Settings.BlackboardKeys.PatrolAcceptanceRadius, UBlackboardKeyType_Float::StaticClass());
	FindPatrolPointTask->WaitTimeKey = ZonefallCodedBT::MakeSelector(Settings.BlackboardKeys.PatrolWaitTime, UBlackboardKeyType_Float::StaticClass());

	UBTTask_ZonefallSetFocus* FocusPatrolTask = ZonefallCodedBT::NewNode<UBTTask_ZonefallSetFocus>(Tree, TEXT("Task_FocusPatrolPoint"));
	FocusPatrolTask->SetCodedBlackboardKey(ZonefallCodedBT::MakeSelector(Settings.BlackboardKeys.MoveLocation, UBlackboardKeyType_Vector::StaticClass()));

	UBTTask_ZonefallSmartMoveTo* MoveToPatrolPointTask = ZonefallCodedBT::NewNode<UBTTask_ZonefallSmartMoveTo>(Tree, TEXT("Task_MoveToPatrolPoint"));
	MoveToPatrolPointTask->SetCodedBlackboardKey(ZonefallCodedBT::MakeSelector(Settings.BlackboardKeys.MoveLocation, UBlackboardKeyType_Vector::StaticClass()));
	MoveToPatrolPointTask->AcceptanceRadiusKey = ZonefallCodedBT::MakeSelector(Settings.BlackboardKeys.PatrolAcceptanceRadius, UBlackboardKeyType_Float::StaticClass());
	MoveToPatrolPointTask->DesiredSpeedKey = ZonefallCodedBT::MakeSelector(Settings.BlackboardKeys.DesiredSpeed, UBlackboardKeyType_Float::StaticClass());
	MoveToPatrolPointTask->AcceptanceRadius = Settings.PatrolAcceptanceRadius;
	MoveToPatrolPointTask->DesiredSpeed = Settings.PatrolMoveSpeed;

	UBTTask_Wait* WaitPatrolTask = ZonefallCodedBT::NewNode<UBTTask_Wait>(Tree, TEXT("Task_PatrolWait"));
	WaitPatrolTask->WaitTime = FValueOrBBKey_Float(Settings.PatrolWaitTime);
	WaitPatrolTask->WaitTime.SetKey(Settings.BlackboardKeys.PatrolWaitTime);
	WaitPatrolTask->RandomDeviation = FValueOrBBKey_Float(Settings.PatrolWaitRandomDeviation);

	PatrolSequence->Children.Add(ZonefallCodedBT::MakeTaskChild(FindPatrolPointTask));
	PatrolSequence->Children.Add(ZonefallCodedBT::MakeTaskChild(FocusPatrolTask));
	PatrolSequence->Children.Add(ZonefallCodedBT::MakeTaskChild(MoveToPatrolPointTask));
	PatrolSequence->Children.Add(ZonefallCodedBT::MakeTaskChild(WaitPatrolTask));

	FBTCompositeChild CombatChild = ZonefallCodedBT::MakeCompositeChild(CombatSequence);
	ZonefallCodedBT::AddDecorator(CombatChild, CombatDecorator);
	FBTCompositeChild InvestigateChild = ZonefallCodedBT::MakeCompositeChild(InvestigateSequence);
	UBTDecorator_ZonefallShouldInvestigate* InvestigateDecorator = ZonefallCodedBT::NewNode<UBTDecorator_ZonefallShouldInvestigate>(Tree, TEXT("Decorator_ShouldInvestigate"));
	InvestigateDecorator->BlackboardKeys = Settings.BlackboardKeys;
	ZonefallCodedBT::AddDecorator(InvestigateChild, InvestigateDecorator);
	FBTCompositeChild PatrolChild = ZonefallCodedBT::MakeCompositeChild(PatrolSequence);

	DecisionSelector->Children.Add(CombatChild);
	DecisionSelector->Children.Add(InvestigateChild);
	DecisionSelector->Children.Add(PatrolChild);

	RootSequence->Children.Add(ZonefallCodedBT::MakeTaskChild(SeedTask));
	RootSequence->Children.Add(ZonefallCodedBT::MakeCompositeChild(DecisionSelector));

	return Tree;
}
