#include "ZonefallAIBarkComponent.h"

#include "AIController.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"
#include "ZonefallAISquadSubsystem.h"

UZonefallAIBarkComponent::UZonefallAIBarkComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

bool UZonefallAIBarkComponent::TryBarkCategory(EZonefallAIBarkCategory Category)
{
	FZonefallBarkLine Line;
	if (!ResolveBarkLine(Category, NAME_None, Line))
	{
		return false;
	}
	return PerformBark(Line);
}

bool UZonefallAIBarkComponent::TryBarkId(FName BarkId)
{
	if (BarkId.IsNone())
	{
		return false;
	}
	FZonefallBarkLine Line;
	if (!ResolveBarkLine(EZonefallAIBarkCategory::Contact, BarkId, Line))
	{
		return false;
	}
	return PerformBark(Line);
}

bool UZonefallAIBarkComponent::HasBarkForCategory(EZonefallAIBarkCategory Category) const
{
	for (const FZonefallBarkLine& Line : BarkLibrary)
	{
		if (Line.Category == Category)
		{
			return true;
		}
	}
	return false;
}

bool UZonefallAIBarkComponent::ResolveBarkLine(EZonefallAIBarkCategory Category, FName BarkId, FZonefallBarkLine& OutLine) const
{
	TArray<const FZonefallBarkLine*> Matches;
	for (const FZonefallBarkLine& Line : BarkLibrary)
	{
		if (!BarkId.IsNone())
		{
			if (Line.BarkId == BarkId)
			{
				Matches.Add(&Line);
			}
		}
		else if (Line.Category == Category)
		{
			Matches.Add(&Line);
		}
	}

	if (Matches.Num() == 0)
	{
		return false;
	}

	const int32 Pick = FMath::RandRange(0, Matches.Num() - 1);
	OutLine = *Matches[Pick];
	return true;
}

bool UZonefallAIBarkComponent::PerformBark(const FZonefallBarkLine& Line)
{
	const float Now = NowSeconds();
	const float LastTime = LastBarkTimes.FindRef(Line.BarkId.IsNone() ? FName(*UEnum::GetValueAsString(Line.Category)) : Line.BarkId);
	if (Now - LastTime < Line.CooldownSeconds)
	{
		return false;
	}

	AAIController* Controller = ResolveOwningController();
	FName CalloutId = Line.BarkId.IsNone() ? FName(*UEnum::GetValueAsString(Line.Category)) : Line.BarkId;

	if (bRouteThroughSquadSubsystem && Controller)
	{
		if (UWorld* World = GetWorld())
		{
			if (UZonefallAISquadSubsystem* SquadSubsystem = World->GetSubsystem<UZonefallAISquadSubsystem>())
			{
				if (!SquadSubsystem->TryEmitSquadCallout(Controller, CalloutId, Line.SquadCooldownSeconds, Line.CooldownSeconds))
				{
					return false;
				}
			}
		}
	}

	LastBarkTimes.Add(CalloutId, Now);

	if (Line.Sound)
	{
		AActor* SpeakerActor = Controller ? static_cast<AActor*>(Controller->GetPawn()) : GetOwner();
		const FVector SoundLocation = SpeakerActor ? SpeakerActor->GetActorLocation() : FVector::ZeroVector;
		UGameplayStatics::PlaySoundAtLocation(this, Line.Sound, SoundLocation);
	}

	if (bPrintToScreenAsFallback && GEngine && !Line.Text.IsEmpty())
	{
		const AActor* Owner = GetOwner();
		const FString OwnerName = Owner ? Owner->GetName() : TEXT("AI");
		const FString Message = FString::Printf(TEXT("[%s] %s"), *OwnerName, *Line.Text.ToString());
		GEngine->AddOnScreenDebugMessage(-1, ScreenBarkDuration, FColor::Cyan, Message);
	}

	AActor* SpeakerActor = Controller ? static_cast<AActor*>(Controller->GetPawn()) : GetOwner();
	OnBarkPlayed.Broadcast(Line, SpeakerActor);
	return true;
}

AAIController* UZonefallAIBarkComponent::ResolveOwningController() const
{
	AActor* Owner = GetOwner();
	if (AAIController* AsAI = Cast<AAIController>(Owner))
	{
		return AsAI;
	}
	if (APawn* Pawn = Cast<APawn>(Owner))
	{
		return Cast<AAIController>(Pawn->GetController());
	}
	return nullptr;
}

float UZonefallAIBarkComponent::NowSeconds() const
{
	const UWorld* World = GetWorld();
	return World ? World->GetTimeSeconds() : 0.0f;
}
