// Fill out your copyright notice in the Description page of Project Settings.


#include "GameplayTaskBlueprintLibrary.h"
#include "GameplayTaskBlueprint.h"

void UGameplayTaskBlueprintLibrary::GetActiveGameplayTaskBlueprints(UObject* Owner, TArray<UGameplayTaskBlueprint*>& OutTasks)
{
	UGameplayTaskBlueprint::GetActiveTasksForOwner(Owner, OutTasks);
}

int32 UGameplayTaskBlueprintLibrary::StopAllGameplayTaskBlueprints(UObject* Owner, FName OptionalTagFilter)
{
	TArray<UGameplayTaskBlueprint*> Tasks;
	UGameplayTaskBlueprint::GetActiveTasksForOwner(Owner, Tasks);

	int32 Stopped = 0;
	for (UGameplayTaskBlueprint* Task : Tasks)
	{
		if (!Task) continue;

		if (OptionalTagFilter != NAME_None && Task->TaskTag != OptionalTagFilter)
			continue;

		Task->StopTask();
		Stopped++;
	}

	return Stopped;
}

int32 UGameplayTaskBlueprintLibrary::StopGameplayTaskBlueprintsByClass(UObject* Owner, TSubclassOf<UGameplayTaskBlueprint> TaskClass, FName OptionalTagFilter)
{
	TArray<UGameplayTaskBlueprint*> Tasks;
	UGameplayTaskBlueprint::GetActiveTasksForOwner(Owner, Tasks);

	int32 Stopped = 0;
	for (UGameplayTaskBlueprint* Task : Tasks)
	{
		if (!Task) continue;
		if (!Task->IsA(TaskClass)) continue;

		if (OptionalTagFilter != NAME_None && Task->TaskTag != OptionalTagFilter)
			continue;

		Task->StopTask();
		Stopped++;
	}

	return Stopped;
}

UGameplayTaskBlueprint* UGameplayTaskBlueprintLibrary::GetActiveGameplayTaskByClass(
	UObject* Owner,
	TSubclassOf<UGameplayTaskBlueprint> TaskClass,
	FName OptionalTagFilter,
	bool& bFound)
{
	bFound = false;

	if (!Owner || !*TaskClass)
	{
		return nullptr;
	}

	TArray<UGameplayTaskBlueprint*> Tasks;
	UGameplayTaskBlueprint::GetActiveTasksForOwner(Owner, Tasks);

	for (UGameplayTaskBlueprint* Task : Tasks)
	{
		if (!Task) continue;

		if (!Task->IsA(TaskClass))
			continue;

		if (OptionalTagFilter != NAME_None && Task->TaskTag != OptionalTagFilter)
			continue;

		bFound = true;
		return Task;
	}

	return nullptr;
}

void UGameplayTaskBlueprintLibrary::GetAllActiveGameplayTasksByClass(
	UObject* Owner,
	TSubclassOf<UGameplayTaskBlueprint> TaskClass,
	FName OptionalTagFilter,
	TArray<UGameplayTaskBlueprint*>& OutTasks)
{
	OutTasks.Reset();

	if (!Owner || !*TaskClass)
		return;

	TArray<UGameplayTaskBlueprint*> Tasks;
	UGameplayTaskBlueprint::GetActiveTasksForOwner(Owner, Tasks);

	for (UGameplayTaskBlueprint* Task : Tasks)
	{
		if (!Task) continue;

		if (!Task->IsA(TaskClass))
			continue;

		if (OptionalTagFilter != NAME_None && Task->TaskTag != OptionalTagFilter)
			continue;

		OutTasks.Add(Task);
	}
}