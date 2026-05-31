// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "PoseSearch/PoseSearchLibrary.h"
#include "PoseSearch/PoseSearchTrajectoryLibrary.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "AGLS_BlueprintFunctionsLibraryP2.generated.h"

/**
 * 
 */
UCLASS()
class HELPFULFUNCTIONS_API UAGLS_BlueprintFunctionsLibraryP2 : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

	UFUNCTION(BlueprintCallable, meta = (BlueprintThreadSafe, DisplayName = "Override Trajectory Facing", Keywords = "Pose,Search,Trajectory,Facing"), Category = "Animation|PoseSearch")
	static FTransformTrajectory OverrideTrajectoryFacing(UPARAM(ref) FTransformTrajectory& InTrajectoryData, FRotator InFacing, float NewFacingAlpha = 1.0, bool bIncludeHistory = true);
	
};
