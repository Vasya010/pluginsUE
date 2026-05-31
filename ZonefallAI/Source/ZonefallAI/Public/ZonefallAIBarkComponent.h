#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ZonefallAIBarkComponent.generated.h"

class USoundBase;
class AAIController;

UENUM(BlueprintType)
enum class EZonefallAIBarkCategory : uint8
{
	Contact UMETA(DisplayName = "Contact"),
	LostTarget UMETA(DisplayName = "Lost Target"),
	Flanking UMETA(DisplayName = "Flanking"),
	Suppressing UMETA(DisplayName = "Suppressing"),
	CoverMe UMETA(DisplayName = "Cover Me"),
	Reload UMETA(DisplayName = "Reload"),
	Grenade UMETA(DisplayName = "Grenade"),
	Wounded UMETA(DisplayName = "Wounded"),
	Retreat UMETA(DisplayName = "Retreat"),
	Acknowledge UMETA(DisplayName = "Acknowledge"),
	Curious UMETA(DisplayName = "Curious"),
	Investigate UMETA(DisplayName = "Investigate")
};

USTRUCT(BlueprintType)
struct ZONEFALLAI_API FZonefallBarkLine
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bark")
	EZonefallAIBarkCategory Category = EZonefallAIBarkCategory::Contact;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bark")
	FName BarkId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bark", meta = (MultiLine = true))
	FText Text;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bark")
	TObjectPtr<USoundBase> Sound;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bark", meta = (ClampMin = "0.0"))
	float CooldownSeconds = 6.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bark", meta = (ClampMin = "0.0"))
	float SquadCooldownSeconds = 2.5f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bark", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float Priority = 0.5f;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FZonefallBarkPlayedEvent, const FZonefallBarkLine&, BarkLine, AActor*, Speaker);

UCLASS(ClassGroup = (ZonefallAI), Blueprintable, meta = (BlueprintSpawnableComponent, DisplayName = "Zonefall AI Bark"))
class ZONEFALLAI_API UZonefallAIBarkComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UZonefallAIBarkComponent();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bark|Library")
	TArray<FZonefallBarkLine> BarkLibrary;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bark|Behavior")
	bool bRouteThroughSquadSubsystem = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bark|Behavior")
	bool bPrintToScreenAsFallback = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bark|Behavior", meta = (ClampMin = "0.0"))
	float ScreenBarkDuration = 2.0f;

	UPROPERTY(BlueprintAssignable, Category = "Bark|Events")
	FZonefallBarkPlayedEvent OnBarkPlayed;

	UFUNCTION(BlueprintCallable, Category = "Zonefall AI|Bark")
	bool TryBarkCategory(EZonefallAIBarkCategory Category);

	UFUNCTION(BlueprintCallable, Category = "Zonefall AI|Bark")
	bool TryBarkId(FName BarkId);

	UFUNCTION(BlueprintPure, Category = "Zonefall AI|Bark")
	bool HasBarkForCategory(EZonefallAIBarkCategory Category) const;

private:
	TMap<FName, float> LastBarkTimes;

	bool ResolveBarkLine(EZonefallAIBarkCategory Category, FName BarkId, FZonefallBarkLine& OutLine) const;
	bool PerformBark(const FZonefallBarkLine& Line);
	AAIController* ResolveOwningController() const;
	float NowSeconds() const;
};
