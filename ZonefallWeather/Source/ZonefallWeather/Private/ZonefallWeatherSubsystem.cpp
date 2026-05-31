#include "ZonefallWeatherSubsystem.h"

#include "Engine/World.h"
#include "ZonefallWeatherController.h"

void UZonefallWeatherSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	PressureRandom.Initialize(PressureRandomSeed);
	PressureSystems.Reset();
}

void UZonefallWeatherSubsystem::Deinitialize()
{
	PressureSystems.Reset();
	ActiveController.Reset();
	Super::Deinitialize();
}

bool UZonefallWeatherSubsystem::DoesSupportWorldType(const EWorldType::Type WorldType) const
{
	return WorldType == EWorldType::Game || WorldType == EWorldType::PIE || WorldType == EWorldType::Editor;
}

TStatId UZonefallWeatherSubsystem::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(UZonefallWeatherSubsystem, STATGROUP_Tickables);
}

void UZonefallWeatherSubsystem::Tick(float DeltaTime)
{
	if (bAutoSpawnPressureSystems && PressureSystems.Num() == 0 && InitialPressureSystemCount > 0)
	{
		SpawnInitialPressureSystems();
	}

	for (int32 Index = PressureSystems.Num() - 1; Index >= 0; --Index)
	{
		FZonefallPressureSystem& System = PressureSystems[Index];
		System.Center += System.Velocity * DeltaTime;
		System.LifeRemaining -= DeltaTime;

		const float HalfExtent = SpawnAreaHalfExtent * 1.5f;
		if (System.LifeRemaining <= 0.0f ||
			FMath::Abs(System.Center.X) > HalfExtent ||
			FMath::Abs(System.Center.Y) > HalfExtent)
		{
			PressureSystems.RemoveAtSwap(Index);
			if (bAutoSpawnPressureSystems)
			{
				PressureSystems.Add(MakeRandomPressureSystem());
			}
		}
	}
}

void UZonefallWeatherSubsystem::RegisterController(AZonefallWeatherController* Controller)
{
	ActiveController = Controller;
}

void UZonefallWeatherSubsystem::UnregisterController(AZonefallWeatherController* Controller)
{
	if (ActiveController.Get() == Controller)
	{
		ActiveController.Reset();
	}
}

AZonefallWeatherController* UZonefallWeatherSubsystem::GetActiveController() const
{
	return ActiveController.Get();
}

void UZonefallWeatherSubsystem::AddPressureSystem(const FZonefallPressureSystem& System)
{
	PressureSystems.Add(System);
}

void UZonefallWeatherSubsystem::ClearPressureSystems()
{
	PressureSystems.Reset();
}

void UZonefallWeatherSubsystem::SamplePressureAt(FVector2D WorldLocationXY, float& OutTemperatureBias, float& OutHumidityBias, float& OutStormBoost) const
{
	OutTemperatureBias = 0.0f;
	OutHumidityBias = 0.0f;
	OutStormBoost = 0.0f;

	for (const FZonefallPressureSystem& System : PressureSystems)
	{
		const float Distance = FVector2D::Distance(System.Center, WorldLocationXY);
		if (Distance >= System.Radius)
		{
			continue;
		}

		const float Alpha = 1.0f - (Distance / System.Radius);
		const float Falloff = Alpha * Alpha * (3.0f - 2.0f * Alpha);

		// Low pressure (Strength < 0): cooler, more humid, more storms
		// High pressure (Strength > 0): warmer, drier, calmer
		OutTemperatureBias += -System.Strength * 4.0f * Falloff;
		OutHumidityBias += -System.Strength * 0.32f * Falloff;
		if (System.Strength < 0.0f)
		{
			OutStormBoost += -System.Strength * 0.5f * Falloff;
		}
	}
}

int32 UZonefallWeatherSubsystem::GetActivePressureSystemCount() const
{
	return PressureSystems.Num();
}

TArray<FZonefallPressureSystem> UZonefallWeatherSubsystem::GetPressureSystems() const
{
	return PressureSystems;
}

void UZonefallWeatherSubsystem::SpawnInitialPressureSystems()
{
	const int32 Count = FMath::Clamp(InitialPressureSystemCount, 1, 16);
	for (int32 Index = 0; Index < Count; ++Index)
	{
		PressureSystems.Add(MakeRandomPressureSystem());
	}
}

FZonefallPressureSystem UZonefallWeatherSubsystem::MakeRandomPressureSystem()
{
	FZonefallPressureSystem System;
	System.Center = FVector2D(
		PressureRandom.FRandRange(-SpawnAreaHalfExtent, SpawnAreaHalfExtent),
		PressureRandom.FRandRange(-SpawnAreaHalfExtent, SpawnAreaHalfExtent));
	const float Heading = PressureRandom.FRandRange(0.0f, UE_TWO_PI);
	const float Speed = PressureRandom.FRandRange(60.0f, 220.0f);
	System.Velocity = FVector2D(FMath::Cos(Heading), FMath::Sin(Heading)) * Speed;
	System.Radius = PressureRandom.FRandRange(8000.0f, 22000.0f);
	System.Strength = PressureRandom.FRandRange(-0.85f, 0.85f);
	System.LifeRemaining = PressureRandom.FRandRange(MinPressureLifeSeconds, MaxPressureLifeSeconds);
	return System;
}
