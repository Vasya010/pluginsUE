


#include "CombatAnimsDatabase.h"

void UCombatAnimsDatabase::GetNormalizedValues(FVector2D& VictimRefXY, FVector2D& VictimRefZ, FVector& AttackPeakXY, FVector& AttackPeakZ)
{
	VictimRefXY = NormalizedVictimRefXY;
	VictimRefZ = NormalizedVictimRefZ;
	AttackPeakXY = NormalizedAttackPeakXY;
	AttackPeakZ = NormalizedAttackPeakXY;
}

#if WITH_EDITOR
void UCombatAnimsDatabase::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	//if (bNormalizeData == false) return;
	//if (CombatAnimAssets.Num() <= 1) return;

	//float MinDistance = -9999;
	//float MaxDistance = 9999;
	//float CurrentDist01 = 0.0;
	//float CurrentDist02 = 0.0;

	//FTransform ReferencePos = FTransform::Identity;

	//for (int i = 0; i < CombatAnimAssets.Num(); i++)
	//{
	//	UCombatAnimSetup* Setup = CombatAnimAssets[i];

	//	FCombatAnimTrajectorySavedData* Data = Setup->PreDefinedData.Find(FName(TEXT("MainRef")));
	//	if (Data)
	//	{
	//		if (Data->SampledBoneTransforms.Num() > 0)
	//		{
	//			const FVector Loc = Data->SampledBoneTransforms[0].GetLocation();
	//			CurrentDist01 = FVector::Dist2D(ReferencePos.GetLocation(), Loc);
	//			CurrentDist02 = Loc.Z - ReferencePos.GetLocation().Z;
	//		}
	//	}

	//}
}
#endif
