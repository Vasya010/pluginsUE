// Jakub W 2026


#include "CharacterFocusingComponent.h"
#include "Kismet/KismetMathLibrary.h"

// Sets default values for this component's properties
UCharacterFocusingComponent::UCharacterFocusingComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;
	SetComponentTickEnabled(false);

	ComponentTags.Add("CustomFocusing");
}


// Called when the game starts
void UCharacterFocusingComponent::BeginPlay()
{
	Super::BeginPlay();
	//Set Main Reference
	if (GetOwner()) { RefChar = Cast<ACharacter>(GetOwner()); }
	SetComponentTickEnabled(false);

}


// Called every frame
void UCharacterFocusingComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	DeltaT = DeltaTime;

	//Timer
	if (bEnableFocusing == false && TickDisableElapsedTime > -0.1 && TickDisableElapsedTime > -0.9)
	{
		if (TickDisableElapsedTime > 0.0)
		{
			TickDisableElapsedTime = TickDisableElapsedTime - DeltaTime;
		}
		else
		{
			TickDisableElapsedTime = -1;
			SetComponentTickEnabled(false);
		}
	}


	if (bAutoUpdateRotationOnTick == false) { return; }

	FocusDesiredRotation = FRotator(0, UKismetMathLibrary::FindLookAtRotation(RefChar->GetActorLocation(), GetCorrectFocusPosition()).Yaw, 0.0);
	FocusCurrentRotation = UKismetMathLibrary::RLerp(GetDefaultDesiredRotation(bCharacterUsingVelocityRotation), FocusDesiredRotation, FocusingStrength, true);

	if (RotationInterpSpeed > 0.0 && RefChar->HasAnyRootMotion() == false) 
	{ 
		UpdateCharacterRotation(FocusCurrentRotation, RotationInterpSpeed, DeltaTime); 
	}
	
	if (bDrawDebugs)
	{
		FColor DrawsColor = FColor::Yellow;
		if (CustomFocusActor) { DrawsColor = FColor::Cyan; }

		DrawDebugSphere(GetWorld(), GetCorrectFocusPosition(), 8.0, 8, DrawsColor, false, 0.0, -1, 1);
		DrawDebugString(GetWorld(), GetCorrectFocusPosition() + FVector(0, 0, 10), TEXT("FP"), nullptr, DrawsColor, 0, true, 1);
	}

}


void UCharacterFocusingComponent::SetUseCharacterFocusing(bool bEnable, bool bInstantChangeTick)
{
	bEnableFocusing = bEnable;

	if (bEnable)
	{
		TickDisableElapsedTime = -1;
		SetComponentTickEnabled(true);
	}
	else
	{
		if (bInstantChangeTick)
		{
			SetComponentTickEnabled(false);
		}
		else
		{
			TickDisableElapsedTime = 4;
		}
	}
}


bool UCharacterFocusingComponent::GetCharacterUsingFocusing() const
{
	return bEnableFocusing && FocusingStrength > 0.01;
}


bool UCharacterFocusingComponent::GetCharacterEnabledFocusing() const
{
	return bEnableFocusing;
}


bool UCharacterFocusingComponent::UpdateCharacterRotation(FRotator InRotation, float InterpSpeed, float dt)
{
	if (!RefChar) return false;

	const FRotator CurrentRot = RefChar->GetActorRotation();
	const FRotator NewRot = UKismetMathLibrary::RInterpTo(CurrentRot, InRotation, dt, InterpSpeed);

	RefChar->SetActorRotation(NewRot);
	return true;
}	


void UCharacterFocusingComponent::SetCharacterFocusingStrength(float Alpha)
{
	FocusingStrength = FMath::Clamp<float>(Alpha, 0.0, 1.0);
}


void UCharacterFocusingComponent::GetCharacterFocusingValues(float& ReturnFocusingStrength, FRotator& ReturnFocusCurrentRotation, FRotator& ReturnFocusDesiredRotation, 
	FVector& ReturnFocalPoint) const
{
	ReturnFocusingStrength = FocusingStrength;
	ReturnFocusCurrentRotation = FocusCurrentRotation;
	ReturnFocusDesiredRotation = FocusDesiredRotation;
	ReturnFocalPoint = CustomFocalPoint;
}


void UCharacterFocusingComponent::SetCharacterFocusingPosition(FVector InPosition)
{
	CustomFocalPoint = InPosition;
}


void UCharacterFocusingComponent::SetCharacterFocusOnActor(AActor* InActor)
{
	CustomFocusActor = InActor;
}

FRotator UCharacterFocusingComponent::GetFocusingAsTrajectoryFacing() const
{
	const FVector RV = UKismetMathLibrary::GetRightVector(FocusCurrentRotation) * -1.0;
	return UKismetMathLibrary::MakeRotFromX(RV);
}


void UCharacterFocusingComponent::UpdateRotationControl()
{
	FocusDesiredRotation = FRotator(0, UKismetMathLibrary::FindLookAtRotation(RefChar->GetActorLocation(), GetCorrectFocusPosition()).Yaw, 0.0);
	FocusCurrentRotation = UKismetMathLibrary::RLerp(GetDefaultDesiredRotation(bCharacterUsingVelocityRotation), FocusDesiredRotation, FocusingStrength, true);

	if (RotationInterpSpeed > 0.0) { UpdateCharacterRotation(FocusCurrentRotation, RotationInterpSpeed, DeltaT); }

	if (bDrawDebugs)
	{
		FColor DrawsColor = FColor::Yellow;
		if (CustomFocusActor) { DrawsColor = FColor::Cyan; }

		DrawDebugSphere(GetWorld(), GetCorrectFocusPosition(), 8.0, 8, DrawsColor, false, 0.0, -1, 1);
		DrawDebugString(GetWorld(), GetCorrectFocusPosition() + FVector(0, 0, 10), TEXT("FP"), nullptr, DrawsColor, 0, true, 1);
	}
}


FVector UCharacterFocusingComponent::GetCorrectFocusPosition()
{
	if (CustomFocusActor)
	{
		return CustomFocusActor->GetActorLocation();
	}
	else
	{
		return CustomFocalPoint;
	}
}

FRotator UCharacterFocusingComponent::GetDefaultDesiredRotation(bool UsingVelocityRotation)
{
	if (UsingVelocityRotation)
	{
		FVector VelocityXY = FVector(RefChar->GetVelocity().X, RefChar->GetVelocity().Y, 0.0);
		if (VelocityXY.Length() < 0.5)
		{
			return RefChar->GetActorRotation();
		}
		else
		{
			VelocityXY.Normalize();
			return UKismetMathLibrary::MakeRotFromX(VelocityXY);
		}
	}
	else
	{
		return FRotator(0.0, RefChar->GetControlRotation().Yaw, 0.0);
	}
}

