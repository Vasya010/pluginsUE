// Fill out your copyright notice in the Description page of Project Settings.


#include "AGLS_BlueprintFunctionsLibraryP2.h"

FTransformTrajectory UAGLS_BlueprintFunctionsLibraryP2::OverrideTrajectoryFacing(UPARAM(ref) FTransformTrajectory& InTrajectoryData, FRotator InFacing, float NewFacingAlpha, bool bIncludeHistory)
{
	const FQuat FacingQuat = InFacing.Quaternion();
	TArray<FTransformTrajectorySample> NewSamples;

	for (int i = 0; i < InTrajectoryData.Samples.Num() - 1; i++)
	{
		FTransformTrajectorySample Sample = InTrajectoryData.Samples[i];
		FTransformTrajectorySample NewSample = Sample;

		if (bIncludeHistory == false)
		{
			if (Sample.TimeInSeconds < 0.0) continue;
		}

		//InTrajectoryData.Samples[0].Facing = FQuat::Slerp(Sample.Facing, FacingQuat, NewFacingAlpha);
		NewSample.Facing = FQuat::Slerp(Sample.Facing, FacingQuat, NewFacingAlpha);
		NewSamples.Add(NewSample);
	}

	InTrajectoryData.Samples = NewSamples;

	return InTrajectoryData;
}
