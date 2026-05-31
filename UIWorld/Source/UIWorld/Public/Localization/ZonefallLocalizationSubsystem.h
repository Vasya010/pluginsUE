#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "ZonefallLocalizationSubsystem.generated.h"

UENUM(BlueprintType)
enum class EZonefallLanguage : uint8
{
	English UMETA(DisplayName = "English"),
	Russian UMETA(DisplayName = "Русский"),
	Chinese UMETA(DisplayName = "中文")
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FZonefallLanguageChanged, EZonefallLanguage, NewLanguage);

/**
 * Runtime, in-game language switching for Zonefall (English / Russian / Chinese).
 *
 *  - Holds a built-in C++ string table so the self-assembling UMG widgets can localise
 *    without any .locres assets.
 *  - Switches the engine culture (FInternationalization) so engine text follows too.
 *  - Persists the chosen language to GameUserSettings.ini.
 *  - Broadcasts OnLanguageChanged so menus can rebuild themselves live.
 *
 * Look up text with GetText(Key) / the static L() helper. Missing keys fall back to the
 * English entry, then to the key string itself, so nothing ever shows blank.
 */
UCLASS(BlueprintType)
class UIWORLD_API UZonefallLocalizationSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;

	/** Convenience accessor from any UObject with a world. */
	UFUNCTION(BlueprintPure, Category = "Zonefall|Localization", meta = (WorldContext = "WorldContextObject"))
	static UZonefallLocalizationSubsystem* Get(const UObject* WorldContextObject);

	/** Switch language: updates the engine culture, persists, and broadcasts OnLanguageChanged. */
	UFUNCTION(BlueprintCallable, Category = "Zonefall|Localization")
	void SetLanguage(EZonefallLanguage NewLanguage);

	UFUNCTION(BlueprintPure, Category = "Zonefall|Localization")
	EZonefallLanguage GetLanguage() const { return CurrentLanguage; }

	/** Localised text for a key in the current language (falls back to English, then the key). */
	UFUNCTION(BlueprintPure, Category = "Zonefall|Localization")
	FText GetText(FName Key) const;

	/** Static one-liner for widgets: ZLoc(this, "menu.home"). */
	UFUNCTION(BlueprintPure, Category = "Zonefall|Localization", meta = (WorldContext = "WorldContextObject"))
	static FText L(const UObject* WorldContextObject, FName Key);

	/** Display names for the language combo, in EZonefallLanguage order. */
	UFUNCTION(BlueprintPure, Category = "Zonefall|Localization")
	TArray<FString> GetLanguageDisplayNames() const;

	UPROPERTY(BlueprintAssignable, Category = "Zonefall|Localization")
	FZonefallLanguageChanged OnLanguageChanged;

	static FString CultureFor(EZonefallLanguage Language);
	static EZonefallLanguage LanguageFromCulture(const FString& Culture);

private:
	void BuildDictionary();
	void LoadSavedLanguage();
	void SaveLanguage() const;

	EZonefallLanguage CurrentLanguage = EZonefallLanguage::English;

	// Key -> { English, Russian, Chinese } (index matches EZonefallLanguage).
	TMap<FName, TArray<FString>> Dictionary;
};
