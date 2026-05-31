#include "BTTask_ZonefallSetFocus.h"

#include "AIController.h"
#include "BehaviorTree/BehaviorTreeComponent.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "BehaviorTree/Blackboard/BlackboardKeyType_Object.h"
#include "BehaviorTree/Blackboard/BlackboardKeyType_Vector.h"

UBTTask_ZonefallSetFocus::UBTTask_ZonefallSetFocus()
{
	NodeName = TEXT("Zonefall Set Focus");
	BlackboardKey.AddObjectFilter(this, GET_MEMBER_NAME_CHECKED(UBTTask_ZonefallSetFocus, BlackboardKey), AActor::StaticClass());
	BlackboardKey.AddVectorFilter(this, GET_MEMBER_NAME_CHECKED(UBTTask_ZonefallSetFocus, BlackboardKey));
}

void UBTTask_ZonefallSetFocus::SetCodedBlackboardKey(const FBlackboardKeySelector& InBlackboardKey)
{
	BlackboardKey = InBlackboardKey;
}

EBTNodeResult::Type UBTTask_ZonefallSetFocus::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	AAIController* AIController = OwnerComp.GetAIOwner();
	UBlackboardComponent* Blackboard = OwnerComp.GetBlackboardComponent();
	if (!AIController || !Blackboard)
	{
		return EBTNodeResult::Failed;
	}

	if (bClearFocus)
	{
		AIController->ClearFocus(EAIFocusPriority::Gameplay);
		return EBTNodeResult::Succeeded;
	}

	if (AActor* FocusActor = Cast<AActor>(Blackboard->GetValueAsObject(BlackboardKey.SelectedKeyName)))
	{
		AIController->SetFocus(FocusActor, EAIFocusPriority::Gameplay);
		return EBTNodeResult::Succeeded;
	}

	if (BlackboardKey.SelectedKeyType == UBlackboardKeyType_Vector::StaticClass())
	{
		AIController->SetFocalPoint(Blackboard->GetValueAsVector(BlackboardKey.SelectedKeyName), EAIFocusPriority::Gameplay);
		return EBTNodeResult::Succeeded;
	}

	return EBTNodeResult::Failed;
}
