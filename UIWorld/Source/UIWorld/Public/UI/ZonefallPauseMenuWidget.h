#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/Button.h"
#include "ZonefallPauseMenuWidget.generated.h"

class UBorder;
class UButton;
class UImage;
class UTextBlock;
class UVerticalBox;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnPauseResumeRequested);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnPauseSaveRequested);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnPauseSettingsRequested);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnPauseMainMenuRequested);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnPauseQuitRequested);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnPauseItemClicked, int32, ItemId);

/** Menu button that remembers its row id and re-broadcasts clicks with it. */
UCLASS()
class UIWORLD_API UZonefallPauseItemButton : public UButton
{
	GENERATED_BODY()

public:
	UZonefallPauseItemButton();

	UPROPERTY(Transient)
	int32 ItemId = INDEX_NONE;

	UPROPERTY(BlueprintAssignable)
	FOnPauseItemClicked OnItemClicked;

private:
	UFUNCTION()
	void HandleInternalClicked();
};

/**
 * Fully self-assembling pause menu (no Blueprint required): dim backdrop, animated panel,
 * Resume / Save / Settings / Main Menu / Quit. All actions route through the game instance
 * with safe fallbacks, so it works out of the box. Esc resumes.
 */
UCLASS(BlueprintType, Blueprintable)
class UIWORLD_API UZonefallPauseMenuWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UZonefallPauseMenuWidget(const FObjectInitializer& ObjectInitializer);

	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeConstruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
	virtual FReply NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;

	UPROPERTY(BlueprintAssignable, Category = "Zonefall|Pause") FOnPauseResumeRequested OnResumeRequested;
	UPROPERTY(BlueprintAssignable, Category = "Zonefall|Pause") FOnPauseSaveRequested OnSaveRequested;
	UPROPERTY(BlueprintAssignable, Category = "Zonefall|Pause") FOnPauseSettingsRequested OnSettingsRequested;
	UPROPERTY(BlueprintAssignable, Category = "Zonefall|Pause") FOnPauseMainMenuRequested OnMainMenuRequested;
	UPROPERTY(BlueprintAssignable, Category = "Zonefall|Pause") FOnPauseQuitRequested OnQuitRequested;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zonefall|Pause|Text") FText TitleText = FText::FromString(TEXT("PAUSED"));

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zonefall|Pause|Style") FLinearColor BackdropTint = FLinearColor(0.0f, 0.0f, 0.0f, 0.7f);
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zonefall|Pause|Style") FLinearColor PanelTint = FLinearColor(0.05f, 0.08f, 0.12f, 0.97f);
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zonefall|Pause|Style") FLinearColor AccentColor = FLinearColor(0.27f, 0.85f, 0.96f, 1.0f);
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zonefall|Pause|Style") int32 TitleFontSize = 36;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zonefall|Pause|Style") int32 BodyFontSize = 18;

private:
	void BuildLayout();
	UZonefallPauseItemButton* AddMenuButton(UVerticalBox* Parent, int32 ItemId, const FText& Label);
	void SetStatus(const FText& Text);
	class UUIWorldMenuGameInstance* ResolveGameInstance() const;

	UFUNCTION() void HandleItemClicked(int32 ItemId);

	UPROPERTY(Transient) TObjectPtr<UBorder> Backdrop;
	UPROPERTY(Transient) TObjectPtr<UBorder> Panel;
	UPROPERTY(Transient) TObjectPtr<UImage> AccentLine;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> TitleLabel;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> StatusLabel;
	UPROPERTY(Transient) TArray<TObjectPtr<UZonefallPauseItemButton>> MenuButtons;

	float IntroTime = 0.0f;
};
