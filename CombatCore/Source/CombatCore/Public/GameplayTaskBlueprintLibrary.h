// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "GameplayTaskBlueprintLibrary.generated.h"

class UGameplayTaskBlueprint;

/**
 * 
 */
UCLASS()
class COMBATCORE_API UGameplayTaskBlueprintLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:

	/** Returns all active GameplayTaskBlueprint tasks for a given Owner. */
	UFUNCTION(BlueprintCallable, Category = "GameplayTaskBlueprint|Registry", meta = (DisplayName = "Get Active Gameplay Task"))
	static void GetActiveGameplayTaskBlueprints(UObject* Owner, TArray<UGameplayTaskBlueprint*>& OutTasks);

	/** Stops all active tasks for Owner (optionally filtered by Tag). */
	UFUNCTION(BlueprintCallable, Category = "GameplayTaskBlueprint|Registry", meta = (DisplayName = "Stop All Gameplay Task"))
	static int32 StopAllGameplayTaskBlueprints(UObject* Owner, FName OptionalTagFilter);

	/** Stops active tasks of a specific class (and optional tag). */
	UFUNCTION(BlueprintCallable, Category = "GameplayTaskBlueprint|Registry", meta = (DisplayName = "Stop Gameplay Task By Class"))
	static int32 StopGameplayTaskBlueprintsByClass(UObject* Owner, TSubclassOf<UGameplayTaskBlueprint> TaskClass, FName OptionalTagFilter);

	UFUNCTION(BlueprintCallable, Category = "GameplayTaskBlueprint|Registry")
	static UGameplayTaskBlueprint* GetActiveGameplayTaskByClass(
		UObject* Owner,
		TSubclassOf<UGameplayTaskBlueprint> TaskClass,
		FName OptionalTagFilter,
		bool& bFound);

	UFUNCTION(BlueprintCallable, Category = "GameplayTaskBlueprint|Registry")
	static void GetAllActiveGameplayTasksByClass(
		UObject* Owner,
		TSubclassOf<UGameplayTaskBlueprint> TaskClass,
		FName OptionalTagFilter,
		TArray<UGameplayTaskBlueprint*>& OutTasks);

};
