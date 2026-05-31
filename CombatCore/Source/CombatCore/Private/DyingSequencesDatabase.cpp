// Fill out your copyright notice in the Description page of Project Settings.


#include "DyingSequencesDatabase.h"
#include "Kismet/KismetSystemLibrary.h"
#include "GameFramework/Character.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Algo/MinElement.h"

const TArray<FTransform> UDyingSequencesDatabase::GetHitBonePositionFromDyingAnim(int AnimIndex)
{
    if (FAnimExtractionResultType* Result = ExtractedAniamtionsData.Find(AnimIndex))
    {
        const TArray<FTransform>& Transforms = Result->ExtractedBonePositions02;
        return Transforms;
    }
	return TArray<FTransform>();
}


const TArray<FTransform> UDyingSequencesDatabase::GetRootBonePositionFromDyingAnim(int AnimIndex)
{
    if (FAnimExtractionResultType* Result = ExtractedAniamtionsData.Find(AnimIndex))
    {
        const TArray<FTransform>& Transforms = Result->ExtractedBonePositions01;
        return Transforms;
    }
    return TArray<FTransform>();
}


bool UDyingSequencesDatabase::RunDyingSequenceSearcher(UAnimSequenceBase*& ReturnAnimation, float& ReturnPlayRate, float& ReturnStartTime, ACharacter* InCharacter, 
    const TArray<UDyingSequencesDatabase*>& InDataAssets, FTransform InHitTransform, FRotator CustomCharacterRot, int SearchProperty01, int SearchProperty02, bool bCanDrawDebugs)
{
    if (InDataAssets.Num() == 0 || !InCharacter) return false;
    
    TArray<float> PerAnimWeight;
    TArray<float> DistanceWeights;
    TArray<float> AngleWeights;
    TArray< UAnimSequenceBase*> ValidAnims;

    int32 TotalAnimCount = 0;
    for (const UDyingSequencesDatabase* DB : InDataAssets)
    {
        if (DB) { TotalAnimCount += DB->DyingSequencesAsSoftRef.Num(); } //calculate the total number of iterations over all animations
    } 

    PerAnimWeight.Reserve(TotalAnimCount);
    DistanceWeights.Reserve(TotalAnimCount);
    AngleWeights.Reserve(TotalAnimCount);
    ValidAnims.Reserve(TotalAnimCount);

    float DistanceMin = 9999;
    float DistanceMax = -9999;
    float TextPositionZ = 0.0; //For Debugging
    
    float FinalMinValue = 999;
    int FinalMinIndex = -1;

    const float CAPSULEHEIGHT = InCharacter->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
    const FVector ACTORLOC = InCharacter->GetActorLocation();
    const FRotator ACTORROT = InCharacter->GetActorRotation();
    const TArray<AActor*> ActorsToIgnore;


    for (int i = 0; i < InDataAssets.Num(); i++) //➊
    {
        UDyingSequencesDatabase* CurrentDatabase = InDataAssets[i];
        if (CurrentDatabase->DyingSequences.Num() == 0) { return false; }

        for (int ii = 0; ii < CurrentDatabase->DyingSequences.Num(); ii++) //➋
        {
            UAnimSequenceBase* CurrentAnim = CurrentDatabase->DyingSequences[ii];
            FTransform PrevBonePositionWS = FTransform::Identity;

            float CurrentWeight = 0.0; //!!!!!!!

            const TArray<FTransform>& RootPositions = CurrentDatabase->GetRootBonePositionFromDyingAnim(ii);

            FRotator CurrentActorRot = ACTORROT;
            if (!CustomCharacterRot.IsZero())
            {
                CurrentActorRot = CustomCharacterRot;
            }

            //▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒ -> Calculate Anim Collide Weight (Not Normalized)
            for (int iii = 0; iii < RootPositions.Num(); iii++) //➌
            {
                FHitResult CapsuleResult;
                

                FTransform CurrentBonePosition = RootPositions[iii];
                CurrentBonePosition = FTransform
                (CurrentBonePosition.Rotator(),
                    FVector
                    (CurrentBonePosition.GetLocation().Y * 1,
                        CurrentBonePosition.GetLocation().X * -1.0,
                        CurrentBonePosition.GetLocation().Z
                    ),
                    FVector(1, 1, 1)
                ); //Invert Bone

                //To World Space 
                const FTransform BonePositionWS = CurrentBonePosition *
                    FTransform
                    (
                        CurrentActorRot,
                        ACTORLOC - FVector(0, 0, CAPSULEHEIGHT),
                        FVector(1, 1, 1)
                    );

                const FVector TraceStart = BonePositionWS.GetLocation() + FVector(0, 0, (CAPSULEHEIGHT * CurrentDatabase->CapsuleCollisionHalfHeightScale) + 20);
                const FVector TraceEnd = TraceStart + FVector(0, 0, -2);

                const bool bHitValid = UKismetSystemLibrary::CapsuleTraceSingleForObjects
                (
                    InCharacter,
                    TraceStart,
                    TraceEnd,
                    CurrentDatabase->CapsuleCollisionRadius,
                    CAPSULEHEIGHT * CurrentDatabase->CapsuleCollisionHalfHeightScale,
                    CurrentDatabase->TraceObjectTypes,
                    false,
                    ActorsToIgnore,
                    EDrawDebugTrace::None,
                    CapsuleResult,
                    true,
                    CurrentDatabase->ShapesColorA,
                    CurrentDatabase->ShapesColorB,
                    2.0
                );


                //▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓
                //░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░
                //▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓
                if (bCanDrawDebugs && CurrentDatabase->bDrawDebugInfo)
                {
                    const float NormalizedArrayLoop = FMath::GetMappedRangeValueClamped(FVector2D(0.0, CurrentDatabase->DyingSequences.Num() * 1.0), FVector2D(0.0, 1.0), ii * 1.0);

                    FLinearColor CapColor = FMath::Lerp<FLinearColor>(CurrentDatabase->ShapesColorB, CurrentDatabase->ShapesColorC, NormalizedArrayLoop);
                    if (bHitValid) CapColor = CurrentDatabase->ShapesColorA;

                    if (CurrentDatabase->bDrawCollisionShapes)
                    {
                        DrawDebugCapsule
                        (
                            InCharacter->GetWorld(),
                            TraceStart,
                            CAPSULEHEIGHT * CurrentDatabase->CapsuleCollisionHalfHeightScale,
                            CurrentDatabase->CapsuleCollisionRadius,
                            CurrentActorRot.Quaternion(),
                            CapColor.ToFColor(true),
                            false,
                            CurrentDatabase->DebugDrawTime,
                            CurrentDatabase->DrawShapesDepth,
                            0.15
                        );
                    }

                    if (PrevBonePositionWS.GetLocation().IsNearlyZero() == false)
                    {
                        FLinearColor LineColor = CapColor;
                        LineColor.A = 0.5;
                        DrawDebugLine(InCharacter->GetWorld(), PrevBonePositionWS.GetLocation(), BonePositionWS.GetLocation(), LineColor.ToFColor(true), 
                            false, CurrentDatabase->DebugDrawTime, CurrentDatabase->DrawShapesDepth, 1.0);
                    }

                }
                //▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░
                //▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀ 

                if (bHitValid && CurrentDatabase->bCheckColliding)
                {
                    if (InCharacter->GetCharacterMovement()->IsWalkable(CapsuleResult))
                    {
                        CurrentWeight += (CurrentDatabase->AnimCollideAddValueToWeight * 0.5);
                    }
                    else if (ACharacter* AsCharacter = Cast<ACharacter>(CapsuleResult.GetActor()))
                    {
                        CurrentWeight += CurrentDatabase->AnimCollideAddValueToWeight * CurrentDatabase->WhenHitCharacterCollideValueScale;
                    }
                    else
                    {
                        CurrentWeight += CurrentDatabase->AnimCollideAddValueToWeight;
                    }
                }

                PrevBonePositionWS = BonePositionWS;

            }

            //▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒ -> Calculate HitPosition Weight (Not Normalized)
            // Sprawdzenie i ewentualne wyliczenie wagi związaniej z porównaniem odleglości względem zadanej pozycji Hit i referencyjnej pozycji z animacji
            if (CurrentDatabase->ReferenceHitPositionBoneName != NAME_None && CurrentDatabase->GetHitBonePositionFromDyingAnim(ii).Num() > 0)
            {
                FTransform HitBonePosition = CurrentDatabase->GetHitBonePositionFromDyingAnim(ii)[0];
                HitBonePosition = FTransform //Wykonanie transformacji kosci (odwrócenie osi pozycji)
                (HitBonePosition.Rotator(),
                    FVector
                    (HitBonePosition.GetLocation().Y * 1,
                        HitBonePosition.GetLocation().X * -1.0,
                        HitBonePosition.GetLocation().Z
                    ),
                    FVector(1, 1, 1)
                ); //Invert Bone

                //To World Space
                const FTransform BonePositionWS = HitBonePosition *
                    FTransform
                    (
                        CurrentActorRot,
                        ACTORLOC - FVector(0, 0, CAPSULEHEIGHT),
                        FVector(1, 1, 1)
                    );


                //▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓
                //░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░
                //▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓
                if (bCanDrawDebugs && CurrentDatabase->bDrawDebugInfo)
                {
                    DrawDebugPoint(InCharacter->GetWorld(), BonePositionWS.GetLocation(), 18.0, FColor::Cyan, false, CurrentDatabase->DebugDrawTime, CurrentDatabase->DrawShapesDepth);
                    DrawDebugLine(InCharacter->GetWorld(), InHitTransform.GetLocation(), BonePositionWS.GetLocation(), FColor(0, 125, 255, 150), false, CurrentDatabase->DebugDrawTime, CurrentDatabase->DrawShapesDepth, 0.6);
                }
                //▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓
                //▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀


                float CurrentDistance = FVector::Distance(InHitTransform.GetLocation(), BonePositionWS.GetLocation()) * 0.1;
                //░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░ ●
                DistanceWeights.Add(CurrentDistance); //Dodanie do tabilcy wagi odleglości (wartość w postaci nie znormalizowanej)
                if (CurrentDistance > DistanceMax) //Poszukiwanie wartości dystansu Min i Max w celu późniejszej normalizacji do przedziału 0 - 1
                {
                    DistanceMax = CurrentDistance;
                }
                //░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░ ●
                if (CurrentDistance < DistanceMin)
                {
                    DistanceMin = CurrentDistance;
                }
                //░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░ ●
                // ANGLE WEIGHT CALCULATION
                if (CurrentDatabase->HitReferenceAngleLerp > 0.0)
                {
                    const FVector BoneUpVector = FRotationMatrix(BonePositionWS.Rotator()).GetUnitAxis(EAxis::Z);
                    const FVector RefHitUpVector = FRotationMatrix(InHitTransform.Rotator()).GetUnitAxis(EAxis::Z) * -1.0;

                    const float CurrentAngle = FMath::GetMappedRangeValueClamped(FVector2D(-0.5, 1.0), FVector2D(1.0, 0.0), FVector::DotProduct(BoneUpVector, RefHitUpVector));
                    AngleWeights.Add(CurrentAngle);
                }

            }

            // ▽  ▼ ▼ ▼  ▽  ▼ ▼ ▼  ▽  ▼ ▼ ▼  ▽  ▼ ▼ ▼  ▽  ▼ ▼ ▼  ▽  ▼ ▼ ▼  ▽  ▼ ▼ ▼  ▽  ▼ ▼ ▼  ▽  ▼ ▼ ▼  ▽  ▼ ▼ ▼  ▽  ▼ ▼ ▼  ▽  ▼ ▼ ▼ 
            const float RandomBias = FMath::FRandRange(0.0, CurrentDatabase->RandomizationSearchingBias); //Zaaplikowanie randomowej wartosci do finalnej wagi animacji
            PerAnimWeight.Add(CurrentWeight + CurrentDatabase->DatabaseBaseConstSearchBias + RandomBias); //Add Weight Per Asset. Also apply Bias and RandomValue
            ValidAnims.Add(CurrentAnim);
        }

        //▋▋▋▋▋▋▋▋▋▋▋▋▋▋▋▋▋▋▋▋▋▋▋▋▋▋▋▋▋▋▋▋▋▋▋▋▋▋ Normalize Weights ▋▋▋▋▋▋▋▋▋▋▋▋▋▋▋▋▋▋▋▋▋▋▋▋▋▋▋▋▋▋▋▋▋▋▋▋▋▋▋▋▋
        if (PerAnimWeight.Num() == DistanceWeights.Num() && CurrentDatabase->bNormalizeData)
        {
            for (int DD = 0; DD < DistanceWeights.Num(); DD++)
            {
                const float NormalizedDistance = FMath::GetMappedRangeValueClamped(FVector2D(DistanceMin, DistanceMax), FVector2D(0.0, 1.0), DistanceWeights[DD]);

                if (CurrentDatabase->HitReferenceAngleLerp > 0.0 && DistanceWeights.Num() == AngleWeights.Num()) //Check should apply Angle Weight per ANIM
                {
                    PerAnimWeight[DD] += FMath::Lerp<float>(NormalizedDistance * CurrentDatabase->HitReferenceWeightScale, AngleWeights[DD], CurrentDatabase->HitReferenceAngleLerp);
                }
                else
                {
                    PerAnimWeight[DD] += (NormalizedDistance * CurrentDatabase->HitReferenceWeightScale);
                }

                //FIND MIN WEIGHT:
                if (PerAnimWeight[DD] < FinalMinValue)
                {
                    FinalMinValue = PerAnimWeight[DD];
                    FinalMinIndex = DD;
                }
            }
        }
        //▛▜▛▜▛▜▛▜▛▜▛▜▛▜▛▜▛▜▛▜▛▜▛▜▛▜▛▜▛▜▛▜▛▜▛▜▛▜▛▜▛▜▛▜▛▜▛▜▛▜▛▜▛▜▛▜▛▜▛▜▛▜▛▜▛▜▛▜▛▜▛▜▛▜▛▜▛▜▛▜▛▜▛▜▛▜▛▜▛▜▛▜▛▜▛▜▛▜▛▜▛▜▛▜▛▜▛▜▛▜▛▜▛▜▛▜▛▜▛▜▛▜▛▜▛▜▛▜▛▜▛▜▛▜▛▜▛▜▛▜▛▜▛▜▛▜▛▜▛▜▛▜▛▜▛▜▛▜▛▜▛▜▛▜▛▜▛▜▛▜▛▜▛▜▛▜▛▜▛▜▛▜▛▜
        //▙▟▙▟▙▟▙▟▙▟▙▟▙▟▙▟▙▟▙▟▙▟▙▟▙▟▙▟▙▟▙▟▙▟▙▟▙▟▙▟▙▟▙▟▙▟▙▟▙▟▙▟▙▟▙▟▙▟▙▟▙▟▙▟▙▟▙▟▙▟▙▟▙▟▙▟▙▟▙▟▙▟▙▟▙▟▙▟▙▟▙▟▙▟▙▟▙▟▙▟▙▟▙▟▙▟▙▟▙▟▙▟▙▟▙▟▙▟▙▟▙▟▙▟▙▟▙▟▙▟▙▟▙▟▙▟▙▟▙▟▙▟▙▟▙▟▙▟▙▟▙▟▙▟▙▟▙▟▙▟▙▟▙▟▙▟▙▟▙▟▙▟▙▟▙▟▙▟▙▟▙▟▙▟


#if WITH_EDITOR
    //░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░
    //▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓
        if (bCanDrawDebugs && CurrentDatabase->bDrawDebugInfo && PerAnimWeight.Num() > 0)
        {
            for (int iiii = 0; iiii < PerAnimWeight.Num(); iiii++)
            {
                const FVector TextPosition = ACTORLOC + FVector(0, 0, TextPositionZ) + (InCharacter->GetActorRightVector() * 30);
                const float ForColorLerp = FMath::GetMappedRangeValueClamped(FVector2D(0.0, 1.2), FVector2D(0.0, 1.0), PerAnimWeight[iiii]);
                FLinearColor TextColor = FLinearColor::LerpUsingHSV(FLinearColor(0, 1, 0, 1), FColor(1, 0.0, 0.6, 1), ForColorLerp);
                const FString TextToDisplay =
                    FString::FromInt(iiii) + TEXT(") ") +
                    ValidAnims[iiii]->GetFName().ToString() +
                    TEXT(" = ") +
                    FString::SanitizeFloat(PerAnimWeight[iiii]
                    );
                DrawDebugString(InCharacter->GetWorld(), TextPosition, TextToDisplay, nullptr, TextColor.ToFColor(true), CurrentDatabase->DebugDrawTime, true, 0.96);
                TextPositionZ = TextPositionZ + 8;
            }     
        }
        //▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓
        //▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀
#endif // WITH_EDITOR

    }

    if (PerAnimWeight.Num() > 0 && ValidAnims.Num() == PerAnimWeight.Num())
    {
        if (FinalMinIndex != -1)
        {
            ReturnAnimation = ValidAnims[FinalMinIndex];
            ReturnPlayRate = 1.0;
            ReturnStartTime = 0.0;
            return true;
        }
        else
        {
            const float* MinPtr = Algo::MinElement(PerAnimWeight);
            int32 MinIndex = PerAnimWeight.IndexOfByKey(*MinPtr);
            //float MinValue = *MinPtr;
            ReturnAnimation = ValidAnims[MinIndex];
            ReturnPlayRate = 1.0;
            ReturnStartTime = 0.0;
            return true;
        }

    }
    return false;
}






bool UDyingSequencesDatabase::RunDyingSequenceSearcherSoftRef(TSoftObjectPtr<UAnimSequence>& ReturnAnimationRef, float& ReturnPlayRate, float& ReturnStartTime,
    ACharacter* InCharacter, const TArray<UDyingSequencesDatabase*>& InDataAssets, FTransform InHitTransform, FRotator CustomCharacterRot, int SearchProperty01, int SearchProperty02, bool bCanDrawDebugs)
{
    if (InDataAssets.Num() == 0 || !InCharacter) return false;

    TArray<float> PerAnimWeight;
    TArray<float> DistanceWeights;
    TArray<float> AngleWeights;
    TArray<TSoftObjectPtr<UAnimSequence>> ValidAnims;

    int32 TotalAnimCount = 0;
    for (const UDyingSequencesDatabase* DB : InDataAssets)
    { if (DB) { TotalAnimCount += DB->DyingSequencesAsSoftRef.Num(); } } //calculate the total number of iterations over all animations

    PerAnimWeight.Reserve(TotalAnimCount);
    DistanceWeights.Reserve(TotalAnimCount);
    AngleWeights.Reserve(TotalAnimCount);
    ValidAnims.Reserve(TotalAnimCount);

    float DistanceMin = 9999;
    float DistanceMax = -9999;
    float TextPositionZ = 0.0; //For Debugging

    const float CAPSULEHEIGHT = InCharacter->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
    const FVector ACTORLOC = InCharacter->GetActorLocation();
    const FRotator ACTORROT = InCharacter->GetActorRotation();
    const TArray<AActor*> ActorsToIgnore;

    for (int i = 0; i < InDataAssets.Num(); i++) //➊
    {
        UDyingSequencesDatabase* CurrentDatabase = InDataAssets[i];
        if (CurrentDatabase->DyingSequencesAsSoftRef.Num() == 0) { return false; }

        for (int ii = 0; ii < CurrentDatabase->DyingSequencesAsSoftRef.Num(); ii++) //➋
        {
            TSoftObjectPtr<UAnimSequence> CurrentAnim = CurrentDatabase->DyingSequencesAsSoftRef[ii];
            FTransform PrevBonePositionWS = FTransform::Identity;

            float CurrentWeight = 0.0; //!!!!!!!

            const TArray<FTransform>& RootPositions = CurrentDatabase->GetRootBonePositionFromDyingAnim(ii);

            FRotator CurrentActorRot = ACTORROT;
            if (!CustomCharacterRot.IsZero())
            {
                CurrentActorRot = CustomCharacterRot;
            }

            //▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒ -> Calculate Anim Collide Weight (Not Normalized)
            for (int iii = 0; iii < RootPositions.Num(); iii++) //➌
            {
                FHitResult CapsuleResult;

                if (CurrentDatabase->bCheckColliding == false) continue;

                FTransform CurrentBonePosition = RootPositions[iii];
                CurrentBonePosition = FTransform
                (CurrentBonePosition.Rotator(),
                    FVector
                    (CurrentBonePosition.GetLocation().Y * 1,
                        CurrentBonePosition.GetLocation().X * -1.0,
                        CurrentBonePosition.GetLocation().Z
                    ),
                    FVector(1, 1, 1)
                ); //Invert Bone

                //To World Space 
                const FTransform BonePositionWS = CurrentBonePosition *
                    FTransform
                    (
                        CurrentActorRot,
                        ACTORLOC - FVector(0, 0, CAPSULEHEIGHT),
                        FVector(1, 1, 1)
                    );

                const FVector TraceStart = BonePositionWS.GetLocation() + FVector(0, 0, (CAPSULEHEIGHT * CurrentDatabase->CapsuleCollisionHalfHeightScale) + 20);
                const FVector TraceEnd = TraceStart + FVector(0, 0, -2);


                //TRACE
                const bool bHitValid = UKismetSystemLibrary::CapsuleTraceSingleForObjects
                (
                    InCharacter,
                    TraceStart,
                    TraceEnd,
                    CurrentDatabase->CapsuleCollisionRadius,
                    CAPSULEHEIGHT * CurrentDatabase->CapsuleCollisionHalfHeightScale,
                    CurrentDatabase->TraceObjectTypes,
                    false,
                    ActorsToIgnore,
                    EDrawDebugTrace::None,
                    CapsuleResult,
                    true,
                    CurrentDatabase->ShapesColorA,
                    CurrentDatabase->ShapesColorB,
                    2.0
                );


                //▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓
                //░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░
                //▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓
                if (bCanDrawDebugs && CurrentDatabase->bDrawDebugInfo)
                {
                    const float NormalizedArrayLoop = FMath::GetMappedRangeValueClamped(FVector2D(0.0, CurrentDatabase->DyingSequencesAsSoftRef.Num() * 1.0), FVector2D(0.0, 1.0), ii * 1.0);

                    FLinearColor CapColor = FMath::Lerp<FLinearColor>(CurrentDatabase->ShapesColorB, CurrentDatabase->ShapesColorC, NormalizedArrayLoop);
                    if (bHitValid) CapColor = CurrentDatabase->ShapesColorA;

                    if (CurrentDatabase->bDrawCollisionShapes)
                    {
                        DrawDebugCapsule
                        (
                            InCharacter->GetWorld(),
                            TraceStart,
                            CAPSULEHEIGHT * CurrentDatabase->CapsuleCollisionHalfHeightScale,
                            CurrentDatabase->CapsuleCollisionRadius,
                            CurrentActorRot.Quaternion(),
                            CapColor.ToFColor(true),
                            false,
                            CurrentDatabase->DebugDrawTime,
                            CurrentDatabase->DrawShapesDepth,
                            0.15
                        );
                    }

                    if (PrevBonePositionWS.GetLocation().IsNearlyZero() == false)
                    {
                        FLinearColor LineColor = CapColor;
                        LineColor.A = 0.5;
                        DrawDebugLine(InCharacter->GetWorld(), PrevBonePositionWS.GetLocation(), BonePositionWS.GetLocation(), LineColor.ToFColor(true),
                            false, CurrentDatabase->DebugDrawTime, CurrentDatabase->DrawShapesDepth, 1.0);
                    }

                }
                //▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░
                //▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀ 

                if (bHitValid && CurrentDatabase->bCheckColliding)
                {
                    if (InCharacter->GetCharacterMovement()->IsWalkable(CapsuleResult))
                    {
                        CurrentWeight += (CurrentDatabase->AnimCollideAddValueToWeight * 0.5);
                    }
                    else if(ACharacter* AsCharacter = Cast<ACharacter>(CapsuleResult.GetActor()))
                    {
                        CurrentWeight += CurrentDatabase->AnimCollideAddValueToWeight * CurrentDatabase->WhenHitCharacterCollideValueScale;
                    }
                    else
                    {
                        CurrentWeight += CurrentDatabase->AnimCollideAddValueToWeight;
                    }
                }

                PrevBonePositionWS = BonePositionWS;

            }

            //▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒ -> Calculate HitPosition Weight (Not Normalized)
            // Sprawdzenie i ewentualne wyliczenie wagi związaniej z porównaniem odleglości względem zadanej pozycji Hit i referencyjnej pozycji z animacji
            if (CurrentDatabase->ReferenceHitPositionBoneName != NAME_None && CurrentDatabase->GetHitBonePositionFromDyingAnim(ii).Num() > 0)
            {
                FTransform HitBonePosition = CurrentDatabase->GetHitBonePositionFromDyingAnim(ii)[0];
                HitBonePosition = FTransform //Wykonanie transformacji kosci (odwrócenie osi pozycji)
                (HitBonePosition.Rotator(),
                    FVector
                    (HitBonePosition.GetLocation().Y * 1,
                        HitBonePosition.GetLocation().X * -1.0,
                        HitBonePosition.GetLocation().Z
                    ),
                    FVector(1, 1, 1)
                ); //Invert Bone

                //To World Space
                const FTransform BonePositionWS = HitBonePosition *
                    FTransform
                    (
                        CurrentActorRot,
                        ACTORLOC - FVector(0, 0, CAPSULEHEIGHT),
                        FVector(1, 1, 1)
                    );


                //▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓
                //░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░
                //▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓
                if (bCanDrawDebugs && CurrentDatabase->bDrawDebugInfo)
                {
                    DrawDebugPoint(InCharacter->GetWorld(), BonePositionWS.GetLocation(), 18.0, FColor::Cyan, false, CurrentDatabase->DebugDrawTime, CurrentDatabase->DrawShapesDepth);
                    DrawDebugLine(InCharacter->GetWorld(), InHitTransform.GetLocation(), BonePositionWS.GetLocation(), FColor(0, 125, 255, 150), false, CurrentDatabase->DebugDrawTime, CurrentDatabase->DrawShapesDepth, 0.6);
                }
                //▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓
                //▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀


                float CurrentDistance = FVector::Distance(InHitTransform.GetLocation(), BonePositionWS.GetLocation()) * 0.1;
                //░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░ ●
                DistanceWeights.Add(CurrentDistance); //Dodanie do tabilcy wagi odleglości (wartość w postaci nie znormalizowanej)
                if (CurrentDistance > DistanceMax) //Poszukiwanie wartości dystansu Min i Max w celu późniejszej normalizacji do przedziału 0 - 1
                {
                    DistanceMax = CurrentDistance;
                }
                //░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░ ●
                if (CurrentDistance < DistanceMin)
                {
                    DistanceMin = CurrentDistance;
                }
                //░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░ ●
                // ANGLE WEIGHT CALCULATION
                if (CurrentDatabase->HitReferenceAngleLerp > 0.0)
                {
                    const FVector BoneUpVector = FRotationMatrix(BonePositionWS.Rotator()).GetUnitAxis(EAxis::Z);
                    const FVector RefHitUpVector = FRotationMatrix(InHitTransform.Rotator()).GetUnitAxis(EAxis::Z) * -1.0;

                    const float CurrentAngle = FMath::GetMappedRangeValueClamped(FVector2D(-0.5, 1.0), FVector2D(1.0, 0.0), FVector::DotProduct(BoneUpVector, RefHitUpVector));
                    AngleWeights.Add(CurrentAngle);
                }

            }

            // ▽  ▼ ▼ ▼  ▽  ▼ ▼ ▼  ▽  ▼ ▼ ▼  ▽  ▼ ▼ ▼  ▽  ▼ ▼ ▼  ▽  ▼ ▼ ▼  ▽  ▼ ▼ ▼  ▽  ▼ ▼ ▼  ▽  ▼ ▼ ▼  ▽  ▼ ▼ ▼  ▽  ▼ ▼ ▼  ▽  ▼ ▼ ▼ 
            const float RandomBias = FMath::FRandRange(0.0, CurrentDatabase->RandomizationSearchingBias); //Zaaplikowanie randomowej wartosci do finalnej wagi animacji
            PerAnimWeight.Add(CurrentWeight + CurrentDatabase->DatabaseBaseConstSearchBias + RandomBias); //Add Weight Per Asset. Also apply Bias and RandomValue
            ValidAnims.Add(CurrentAnim);
        }

        //▋▋▋▋▋▋▋▋▋▋▋▋▋▋▋▋▋▋▋▋▋▋▋▋▋▋▋▋▋▋▋▋▋▋▋▋▋▋ Normalize Weights ▋▋▋▋▋▋▋▋▋▋▋▋▋▋▋▋▋▋▋▋▋▋▋▋▋▋▋▋▋▋▋▋▋▋▋▋▋▋▋▋▋
        if (PerAnimWeight.Num() == DistanceWeights.Num() && CurrentDatabase->bNormalizeData)
        {
            for (int DD = 0; DD < DistanceWeights.Num(); DD++)
            {
                const float NormalizedDistance = FMath::GetMappedRangeValueClamped(FVector2D(DistanceMin, DistanceMax), FVector2D(0.0, 1.0), DistanceWeights[DD]);

                if (CurrentDatabase->HitReferenceAngleLerp > 0.0 && DistanceWeights.Num() == AngleWeights.Num()) //Check should apply Angle Weight per ANIM
                {
                    PerAnimWeight[DD] += FMath::Lerp<float>(NormalizedDistance * CurrentDatabase->HitReferenceWeightScale, AngleWeights[DD], CurrentDatabase->HitReferenceAngleLerp);
                }
                else
                {
                    PerAnimWeight[DD] += (NormalizedDistance * CurrentDatabase->HitReferenceWeightScale);
                } 
            }
        }
        //▛▜▛▜▛▜▛▜▛▜▛▜▛▜▛▜▛▜▛▜▛▜▛▜▛▜▛▜▛▜▛▜▛▜▛▜▛▜▛▜▛▜▛▜▛▜▛▜▛▜▛▜▛▜▛▜▛▜▛▜▛▜▛▜▛▜▛▜▛▜▛▜▛▜▛▜▛▜▛▜▛▜▛▜▛▜▛▜▛▜▛▜▛▜▛▜▛▜▛▜▛▜▛▜▛▜▛▜▛▜▛▜▛▜▛▜▛▜▛▜▛▜▛▜▛▜▛▜▛▜▛▜▛▜▛▜▛▜▛▜▛▜▛▜▛▜▛▜▛▜▛▜▛▜▛▜▛▜▛▜▛▜▛▜▛▜▛▜▛▜▛▜▛▜▛▜▛▜▛▜▛▜▛▜
        //▙▟▙▟▙▟▙▟▙▟▙▟▙▟▙▟▙▟▙▟▙▟▙▟▙▟▙▟▙▟▙▟▙▟▙▟▙▟▙▟▙▟▙▟▙▟▙▟▙▟▙▟▙▟▙▟▙▟▙▟▙▟▙▟▙▟▙▟▙▟▙▟▙▟▙▟▙▟▙▟▙▟▙▟▙▟▙▟▙▟▙▟▙▟▙▟▙▟▙▟▙▟▙▟▙▟▙▟▙▟▙▟▙▟▙▟▙▟▙▟▙▟▙▟▙▟▙▟▙▟▙▟▙▟▙▟▙▟▙▟▙▟▙▟▙▟▙▟▙▟▙▟▙▟▙▟▙▟▙▟▙▟▙▟▙▟▙▟▙▟▙▟▙▟▙▟▙▟▙▟▙▟▙▟


        if (PerAnimWeight.Num() > 0 && ValidAnims.Num() == PerAnimWeight.Num())
        {
            const float* MinPtr = Algo::MinElement(PerAnimWeight);
            int32 MinIndex = PerAnimWeight.IndexOfByKey(*MinPtr);
            //float MinValue = *MinPtr;
            ReturnAnimationRef = ValidAnims[MinIndex];
            ReturnPlayRate = 1.0;
            ReturnStartTime = 0.0;

#if WITH_EDITOR
            //░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░
            //▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓
            if (bCanDrawDebugs && CurrentDatabase->bDrawDebugInfo && PerAnimWeight.Num() > 0)
            {
                for (int iiii = 0; iiii < PerAnimWeight.Num(); iiii++)
                {
                    const FVector TextPosition = ACTORLOC + FVector(0, 0, TextPositionZ) + (InCharacter->GetActorRightVector() * 30);
                    const float ForColorLerp = FMath::GetMappedRangeValueClamped(FVector2D(*MinPtr, 1.5), FVector2D(0.0, 1.0), PerAnimWeight[iiii]);
                    FColor TextColor = FMath::Lerp(
                        FLinearColor(FColor(0, 255, 0, 255)),
                        FLinearColor(FColor(255, 10, 20, 255)),
                        ForColorLerp
                    ).ToFColor(true);

                    FString AnimName = TEXT("Unreachable Name");
                    if (TSoftObjectPtr<UAnimSequence> AssetToName = ValidAnims[iiii])
                    {
                        AnimName = AssetToName.GetAssetName();
                    }
                    FString TextToDisplay =
                        FString::FromInt(iiii) + TEXT(") ") +
                        AnimName +
                        TEXT(" = ") +
                        FString::SanitizeFloat(PerAnimWeight[iiii]);

                    if (iiii == MinIndex)
                    {
                        TextToDisplay = TextToDisplay + TEXT(" <--- CHOOSED ANIM");
                    }

                    DrawDebugString(InCharacter->GetWorld(), TextPosition, TextToDisplay, nullptr, TextColor, CurrentDatabase->DebugDrawTime, true, 0.96);
                    TextPositionZ = TextPositionZ + 8;
                }
            }
            //▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓░▓
            //▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀
#endif // WITH_EDITOR

            return true; // RETURN <----------------------------------------
        }
    }
    return false;
}





#if WITH_EDITOR
void UDyingSequencesDatabase::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{

	ExtractedAniamtionsData.Empty();
    
    TArray<UAnimSequenceBase*> LoadedAnimsSet;

    if (DyingSequences.Num() > 0) 
    {
        LoadedAnimsSet = DyingSequences;
    }
    else if (DyingSequencesAsSoftRef.Num() > 0)
    {
        for (int AnimI = 0; AnimI < DyingSequencesAsSoftRef.Num(); AnimI++)
        {
            if (DyingSequencesAsSoftRef[AnimI].IsValid())
            {
                UAnimSequenceBase* AnimSeq = DyingSequencesAsSoftRef[AnimI].Get(); //Get Asset
                if (AnimSeq)
                {
                    LoadedAnimsSet.Add(AnimSeq);
                }
            }
            else
            {
                UAnimSequenceBase* AnimSeq = DyingSequencesAsSoftRef[AnimI].LoadSynchronous(); //Load Asset
                LoadedAnimsSet.Add(AnimSeq);
            }
        }
    }
    else
    {
        return;
    }

    for (int i = 0; i < LoadedAnimsSet.Num(); i++)
    {
        UAnimSequenceBase* CurrentAnim = LoadedAnimsSet[i];
        if (!CurrentAnim) { continue; }
        
        UAnimMontage* AsMontage = Cast<UAnimMontage>(CurrentAnim);
        if (AsMontage)
        {
            CurrentAnim = UForAnimsSimpleMacros::GetAnimSequenceFromMontage(AsMontage, 0);
        }

        if (!CurrentAnim)
        {
            continue;
        }

        FAnimExtractionResultType ExtrationResult;
        ExtrationResult.BoneName01 = RootBoneName;
        ExtrationResult.BoneName02 = ReferenceHitPositionBoneName;

        if (ExtrationResult.BoneName02 != NAME_None)
        {
            const FTransform BonePos02 = UForAnimsSimpleMacros::GetBonePositionAtTimeFromSeq(CurrentAnim, 0.0, ExtrationResult.BoneName02);
            ExtrationResult.ExtractedBonePositions02.Add(BonePos02);
        }

        if (ExtrationResult.BoneName01 == NAME_None) 
        { 
            ExtractedAniamtionsData.Add(i, ExtrationResult); //Add key
            continue; 
        }

        TArray<FTransform> RootBonePositions;

        for (int ii = 0; ii < RootMotionExtractPositionNumber; ii++)
        {
            const float TimeNormalized = FMath::GetMappedRangeValueClamped(FVector2D(0.0, (RootMotionExtractPositionNumber - 1) * 1.0), RootMotionExtractTimeRange, ii * 1.0);
            float TimeFinal = FMath::GetMappedRangeValueClamped(FVector2D(0.0, 1.0), FVector2D(0.0, CurrentAnim->GetPlayLength()), TimeNormalized);

            FTransform BonePos01 = UForAnimsSimpleMacros::GetBonePositionAtTimeFromSeq(CurrentAnim, TimeFinal, ExtrationResult.BoneName01);
            
            RootBonePositions.Add(BonePos01);
        }

        ExtrationResult.ExtractedBonePositions01 = RootBonePositions;

        ExtractedAniamtionsData.Add(i, ExtrationResult); //Add key
    }

}

#endif
