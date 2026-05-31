// Fill out your copyright notice in the Description page of Project Settings.


#include "GameplayTaskBlueprint.h"
#include "GameplayTasksComponent.h"
#include "GameFramework/Actor.h"

namespace
{
	// Owner -> active tasks (weak)
	static TMultiMap<TWeakObjectPtr<UObject>, TWeakObjectPtr<UGameplayTaskBlueprint>> GActiveTasks;
}



//UGameplayTaskBlueprint::UGameplayTaskBlueprint() { }

UGameplayTaskBlueprint* UGameplayTaskBlueprint::StartTask(
	TScriptInterface<IGameplayTaskOwnerInterface> TaskOwner,
	TSubclassOf<UGameplayTaskBlueprint> TaskClass,
	int32 Priority,
	FName InTaskTag)
{
	if (!TaskOwner.GetObject() || !*TaskClass)
	{
		return nullptr;
	}

	UObject* OwnerObj = TaskOwner.GetObject();

	UGameplayTaskBlueprint* Task = NewObject<UGameplayTaskBlueprint>(OwnerObj, TaskClass);
	if (!Task)
	{
		return nullptr;
	}

	Task->TaskTag = InTaskTag;

	// InitTask wires owner & priority into the GameplayTasks system
	Task->InitTask(*TaskOwner, Priority);

	// schedules activation via component/task system
	Task->ReadyForActivation();
	return Task;
}

void UGameplayTaskBlueprint::StopTask()
{
	// EndTask is the official way to stop/finish a gameplay task
	EndTask();
}

bool UGameplayTaskBlueprint::IsTaskActive_BP() const
{
	return bIsActiveInternal;
}

AActor* UGameplayTaskBlueprint::GetTaskOwnerObject_BP() const
{
	IGameplayTaskOwnerInterface* OwnerInterface = const_cast<UGameplayTaskBlueprint*>(this)->GetTaskOwner();
	if (!OwnerInterface)
	{
		return nullptr;
	}

	AActor* OwnerObject = OwnerInterface->GetGameplayTaskOwner(this);
	return OwnerObject;

}

UGameplayTasksComponent* UGameplayTaskBlueprint::GetGameplayTasksComponent_BP() const
{
	UObject* OwnerObj = GetTaskOwnerObject_BP();
	if (!OwnerObj) return nullptr;

	// If owner is an Actor, tasks component can be found on it
	if (AActor* OwnerActor = Cast<AActor>(OwnerObj))
	{
		return OwnerActor->FindComponentByClass<UGameplayTasksComponent>();
	}

	// Some owners implement interface and provide component; base has helper:
	return GetGameplayTasksComponent();
}

void UGameplayTaskBlueprint::CallToCustomEvent(int32 AdditiveId)
{
	// Opcjonalnie: ignoruj jeœli task ju¿ nieaktywny
	if (!bIsActiveInternal)
		return;

	OnCustomExecuteBP(AdditiveId);
}

void UGameplayTaskBlueprint::SetBoolParam(FName Key, bool bValue)
{
	BoolParams.Add(Key, bValue);
}

void UGameplayTaskBlueprint::SetFloatParam(FName Key, float Value)
{
	FloatParams.Add(Key, Value);
}

void UGameplayTaskBlueprint::SetVectorParam(FName Key, FVector Value)
{
	VectorParams.Add(Key, Value);
}

void UGameplayTaskBlueprint::SetTransformParam(FName Key, FTransform Value)
{
	TransformParams.Add(Key, Value);
}

void UGameplayTaskBlueprint::SetObjectParam(FName Key, UObject* Value)
{
	ObjectParams.Add(Key, Value);
}


bool UGameplayTaskBlueprint::GetBoolParam(FName Key, bool& bOutFound) const
{
	if (const bool* Ptr = BoolParams.Find(Key))
	{
		bOutFound = true;
		return *Ptr;
	}
	bOutFound = false;
	return false;
}

float UGameplayTaskBlueprint::GetFloatParam(FName Key, bool& bOutFound) const
{
	if (const float* Ptr = FloatParams.Find(Key))
	{
		bOutFound = true;
		return *Ptr;
	}
	bOutFound = false;
	return 0.0f;
}

FVector UGameplayTaskBlueprint::GetVectorParam(FName Key, bool& bOutFound) const
{
	if (const FVector* Ptr = VectorParams.Find(Key))
	{
		bOutFound = true;
		return *Ptr;
	}
	bOutFound = false;
	return FVector(0, 0, 0);
}

FTransform UGameplayTaskBlueprint::GetTransformParam(FName Key, bool& bOutFound) const
{
	if (const FTransform* Ptr = TransformParams.Find(Key))
	{
		bOutFound = true;
		return *Ptr;
	}
	bOutFound = false;
	return FTransform::Identity;
}

UObject* UGameplayTaskBlueprint::GetObjectParam(FName Key, bool& bOutFound) const
{
	if (UObject* Ptr = *ObjectParams.Find(Key))
	{
		bOutFound = true;
		return Ptr;
	}
	bOutFound = false;
	return nullptr;
}


void UGameplayTaskBlueprint::OnActivatedBP_Implementation()
{
}

void UGameplayTaskBlueprint::OnEndedBP_Implementation(bool OwnerFinished)
{
}

void UGameplayTaskBlueprint::OnTickBP_Implementation(float DeltaTime)
{
}

void UGameplayTaskBlueprint::OnCustomExecuteBP_Implementation(int32 AdditiveId)
{
}

void UGameplayTaskBlueprint::Activate()
{
	Super::Activate();

	bTickingTask = bEnableTick;     // <- w³¹cza TickTask
	bSimulatedTask = bUseTickOnSimThread; // opcjonalnie, jeœli nie chcesz tickowania w sim

	bIsActiveInternal = true;
	RegisterActiveTask(this);

	// Blueprint event
	OnActivatedBP();
}

void UGameplayTaskBlueprint::OnDestroy(bool bInOwnerFinished)
{
	// Unregister first
	bIsActiveInternal = false;
	UnregisterActiveTask(this);

	// Blueprint event (still safe here)
	OnEndedBP(bInOwnerFinished);

	Super::OnDestroy(bInOwnerFinished);
}

void UGameplayTaskBlueprint::TickTask(float DeltaTime)
{
	Super::TickTask(DeltaTime);
	// Event BP
	OnTickBP(DeltaTime);
}

void UGameplayTaskBlueprint::RegisterActiveTask(UGameplayTaskBlueprint* Task)
{
	if (!Task) return;

	UObject* OwnerObj = Task->GetTaskOwnerObject_BP();
	if (!OwnerObj) return;

	GActiveTasks.Add(OwnerObj, Task);
}

void UGameplayTaskBlueprint::UnregisterActiveTask(UGameplayTaskBlueprint* Task)
{
	if (!Task) return;

	UObject* OwnerObj = Task->GetTaskOwnerObject_BP();
	if (!OwnerObj) return;

	// Remove all matching pairs (Owner -> Task)
	TArray<TWeakObjectPtr<UGameplayTaskBlueprint>> Found;
	GActiveTasks.MultiFind(OwnerObj, Found);

	for (const TWeakObjectPtr<UGameplayTaskBlueprint>& WeakTask : Found)
	{
		if (WeakTask.Get() == Task)
		{
			GActiveTasks.RemoveSingle(OwnerObj, WeakTask);
		}
	}

	// Optionally: cleanup stale entries for that owner
	TArray<TWeakObjectPtr<UGameplayTaskBlueprint>> Remaining;
	GActiveTasks.MultiFind(OwnerObj, Remaining);
	for (const TWeakObjectPtr<UGameplayTaskBlueprint>& WeakTask : Remaining)
	{
		if (!WeakTask.IsValid())
		{
			GActiveTasks.RemoveSingle(OwnerObj, WeakTask);
		}
	}
}

void UGameplayTaskBlueprint::GetActiveTasksForOwner(UObject* Owner, TArray<UGameplayTaskBlueprint*>& OutTasks)
{
	OutTasks.Reset();

	if (!Owner) return;

	TArray<TWeakObjectPtr<UGameplayTaskBlueprint>> Found;
	GActiveTasks.MultiFind(Owner, Found);

	for (const TWeakObjectPtr<UGameplayTaskBlueprint>& WeakTask : Found)
	{
		if (UGameplayTaskBlueprint* Task = WeakTask.Get())
		{
			OutTasks.Add(Task);
		}
	}
}