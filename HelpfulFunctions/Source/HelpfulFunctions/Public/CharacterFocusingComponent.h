// Jakub W 2026

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/ActorComponent.h"
#include "CharacterFocusingComponent.generated.h"

/*
▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒
ENG:
An additional component for Characters to properly configure the focus on a specific target. Using this component is helpful 
when CharacterMesh uses Motion Matching technology to animate the character. In such cases, it's also necessary to properly 
override values ​​during trajectory generation.

▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒
PL:
Dodatkowy komponent przeznaczony dla Character w celu poprawnego skonfigurowania opcji skupiania się na określanym celu. 
Użycie tego komponentu jest pomocne w przypadku kiedy CharacterMesh korzysta z technologi Motion Matching do animowania 
postaci. W takim przypadku konieczne jest również prawidłowe nadpisanie wartości podczas generowania trajektorii.

▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒
*/
UCLASS(Blueprintable, ClassGroup = (Gameplay), meta = (BlueprintSpawnableComponent))
class HELPFULFUNCTIONS_API UCharacterFocusingComponent : public UActorComponent
{
	GENERATED_BODY()

public:	

	UCharacterFocusingComponent();

protected:

	virtual void BeginPlay() override;
	float DeltaT = 0.01;

public:	

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

public:

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = "True", Category = "Custom Character Focusing|Reference"))
	ACharacter* RefChar = nullptr;

	//Warning if this value <= 0.0 auto setting rotation option is disabled
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = "True", Category = "Custom Character Focusing|Config"))
	float RotationInterpSpeed = 10.0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = "True", Category = "Custom Character Focusing|Config"))
	bool bAutoUpdateRotationOnTick = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = "True", Category = "Custom Character Focusing|Config"))
	bool bCharacterUsingVelocityRotation = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = "True", Category = "Custom Character Focusing|Config"))
	bool bDrawDebugs = false;


	bool bEnableFocusing = false;
	float FocusingStrength = 1.0;
	FRotator FocusCurrentRotation = FRotator(0, 0, 0);
	FRotator FocusDesiredRotation = FRotator(0, 0, 0);

	float TickDisableElapsedTime = -1;

	AActor* CustomFocusActor = nullptr;
	FVector CustomFocalPoint = FVector(0, 0, 0);

	UFUNCTION(BlueprintCallable, Category = "Custom Character Focusing", meta = (Keywords = "Focus,Character,Rotation,Motion,Matching"))
	void SetUseCharacterFocusing(bool bEnable, bool bInstantChangeTick = true);

	UFUNCTION(BlueprintCallable, Category = "Custom Character Focusing", meta = (BlueprintThreadSafe, Keywords = "Focus,Character,Rotation,Motion,Matching"))
	bool GetCharacterUsingFocusing() const;

	UFUNCTION(BlueprintCallable, Category = "Custom Character Focusing", meta = (BlueprintThreadSafe, Keywords = "Focus,Character,Rotation,Motion,Matching"))
	bool GetCharacterEnabledFocusing() const;

	bool UpdateCharacterRotation(FRotator InRotation, float InterpSpeed, float dt);

	UFUNCTION(BlueprintCallable, Category = "Custom Character Focusing", meta = (Keywords = "Focus,Character,Rotation,Motion,Matching"))
	void SetCharacterFocusingStrength(float Alpha);

	UFUNCTION(BlueprintPure, Category = "Custom Character Focusing", meta = (BlueprintThreadSafe, Keywords = "Focus,Character,Rotation,Motion,Matching", AdvancedDisplay = 1))
	void GetCharacterFocusingValues(float& ReturnFocusingStrength, FRotator& ReturnFocusCurrentRotation, FRotator& ReturnFocusDesiredRotation, FVector& ReturnFocalPoint) const;

	UFUNCTION(BlueprintCallable, Category = "Custom Character Focusing", meta = (Keywords = "Focus,Character,Rotation,Motion,Matching"))
	void SetCharacterFocusingPosition(FVector InPosition);

	UFUNCTION(BlueprintCallable, Category = "Custom Character Focusing", meta = (Keywords = "Focus,Character,Rotation,Motion,Matching"))
	void SetCharacterFocusOnActor(AActor* InActor);

	UFUNCTION(BlueprintPure, Category = "Custom Character Focusing", meta = (BlueprintThreadSafe, Keywords = "Focus,Character,Rotation,Motion,Matching"))
	FRotator GetFocusingAsTrajectoryFacing() const;

	UFUNCTION(BlueprintCallable, Category = "Custom Character Focusing", meta = (Keywords = "Focus,Character,Rotation,Motion,Matching"))
	void UpdateRotationControl();

	FVector GetCorrectFocusPosition();

	FRotator GetDefaultDesiredRotation(bool UsingVelocityRotation = false);
};
