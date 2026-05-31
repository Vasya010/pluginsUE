// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTask.h"
#include "GameplayTaskBlueprint.generated.h"

/**
 * Blueprint-friendly GameplayTask base class.
 * - Can be subclassed in Blueprint
 * - Exposes Activated/Ended events
 * - Allows external stop
 * - Allows simple Bool/Float runtime params
 * - Keeps static registry of active tasks per Owner (for external control)
 */
UCLASS(BlueprintType, Blueprintable, EditInlineNew)
class COMBATCORE_API UGameplayTaskBlueprint : public UGameplayTask
{
	GENERATED_BODY()

public:
	//UGameplayTaskBlueprint();

	/** Optional tag for grouping / finding tasks from outside. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gameplay Task")
	FName TaskTag = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gameplay Task")
	bool bEnableTick = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gameplay Task", meta = (EditCondition = "bEnableTick"))
	bool bUseTickOnSimThread = false;


	/** Starts/creates task instance (factory). */
	UFUNCTION(BlueprintCallable, Category = "GameplayTaskBlueprint",
		meta = (DefaultToSelf = "TaskOwner", BlueprintInternalUseOnly = "true"))
	static UGameplayTaskBlueprint* StartTask(
		TScriptInterface<IGameplayTaskOwnerInterface> TaskOwner,
		TSubclassOf<UGameplayTaskBlueprint> TaskClass,
		int32 Priority = 0,
		FName InTaskTag = NAME_None);

	/** Stops (ends) this task. Safe to call from outside. */
	UFUNCTION(BlueprintCallable, Category = "GameplayTaskBlueprint")
	void StopTask();

	/** Is this task currently active (activated and not ended). */
	UFUNCTION(BlueprintPure, Category = "GameplayTaskBlueprint", meta = (DisplayName = "Is Task Active"))
	bool IsTaskActive_BP() const;

	/** Owner object (most often Actor/AIController/Component). */
	UFUNCTION(BlueprintPure, Category = "GameplayTaskBlueprint", meta = (DisplayName = "Get Task Owner"))
	AActor* GetTaskOwnerObject_BP() const;

	/** GameplayTasksComponent used to run tasks (if available). */
	UFUNCTION(BlueprintPure, Category = "GameplayTaskBlueprint")
	class UGameplayTasksComponent* GetGameplayTasksComponent_BP() const;

	UFUNCTION(BlueprintCallable, Category = "GameplayTaskBlueprint")
	void CallToCustomEvent(int32 AdditiveId);

	// ----- Simple param system -----

	UFUNCTION(BlueprintCallable, Category = "GameplayTaskBlueprint|Params")
	void SetBoolParam(FName Key, bool bValue);

	UFUNCTION(BlueprintCallable, Category = "GameplayTaskBlueprint|Params")
	void SetFloatParam(FName Key, float Value);

	UFUNCTION(BlueprintCallable, Category = "GameplayTaskBlueprint|Params")
	void SetVectorParam(FName Key, FVector Value);

	UFUNCTION(BlueprintCallable, Category = "GameplayTaskBlueprint|Params")
	void SetTransformParam(FName Key, FTransform Value);

	UFUNCTION(BlueprintCallable, Category = "GameplayTaskBlueprint|Params")
	void SetObjectParam(FName Key, UObject* Value);

	UFUNCTION(BlueprintPure, Category = "GameplayTaskBlueprint|Params")
	bool GetBoolParam(FName Key, bool& bOutFound) const;

	UFUNCTION(BlueprintPure, Category = "GameplayTaskBlueprint|Params")
	float GetFloatParam(FName Key, bool& bOutFound) const;

	UFUNCTION(BlueprintPure, Category = "GameplayTaskBlueprint|Params")
	FVector GetVectorParam(FName Key, bool& bOutFound) const;

	UFUNCTION(BlueprintPure, Category = "GameplayTaskBlueprint|Params")
	FTransform GetTransformParam(FName Key, bool& bOutFound) const;

	UFUNCTION(BlueprintPure, Category = "GameplayTaskBlueprint|Params")
	UObject* GetObjectParam(FName Key, bool& bOutFound) const;

	// ----- Blueprint events -----

	/** Called when task is activated (equivalent to "OnActivated"). */
	UFUNCTION(BlueprintNativeEvent, Category = "GameplayTaskBlueprint|Events", meta = (DisplayName = "On Activated"))
	void OnActivatedBP();
	virtual void OnActivatedBP_Implementation();

	/**
	 * Called when task ends/destroys.
	 * bOwnerFinished = owner ended (actor destroyed/end play) or task manager finished.
	 */
	UFUNCTION(BlueprintNativeEvent, Category = "GameplayTaskBlueprint|Events", meta = (DisplayName = "On Ended"))
	void OnEndedBP(bool OwnerFinished);
	virtual void OnEndedBP_Implementation(bool OwnerFinished);

	UFUNCTION(BlueprintNativeEvent, Category = "GameplayTaskBlueprint|Events", meta = (DisplayName = "Tick"))
	void OnTickBP(float DeltaTime);
	virtual void OnTickBP_Implementation(float DeltaTime);

	UFUNCTION(BlueprintNativeEvent, Category = "GameplayTaskBlueprint|Events", meta = (DisplayName = "Custom Execute"))
	void OnCustomExecuteBP(int32 AdditiveId);
	virtual void OnCustomExecuteBP_Implementation(int32 AdditiveId);

protected:
	// UGameplayTask overrides
	virtual void Activate() override;
	virtual void OnDestroy(bool bInOwnerFinished) override;
	virtual void TickTask(float DeltaTime) override;

private:
	// Runtime params
	UPROPERTY(Transient)
	TMap<FName, bool> BoolParams;

	UPROPERTY(Transient)
	TMap<FName, float> FloatParams;

	UPROPERTY(Transient)
	TMap<FName, FVector> VectorParams;

	UPROPERTY(Transient)
	TMap<FName, FTransform> TransformParams;

	UPROPERTY(Transient)
	TMap<FName, UObject*> ObjectParams;


	// Registry helpers
	static void RegisterActiveTask(UGameplayTaskBlueprint* Task);
	static void UnregisterActiveTask(UGameplayTaskBlueprint* Task);

	// cached active flag (because UGameplayTask has internal state but BP-friendly flag helps)
	UPROPERTY(Transient)
	bool bIsActiveInternal = false;

public:
	// Registry access used by Blueprint Library
	static void GetActiveTasksForOwner(UObject* Owner, TArray<UGameplayTaskBlueprint*>& OutTasks);
	
};
