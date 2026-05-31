#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "UIWorldSimpleMainMenuWidget.generated.h"

class UButton;
class UTextBlock;
class UVerticalBox;
class UUIWorldMenuGameInstance;

UCLASS(BlueprintType, Blueprintable)
class UIWORLD_API UUIWorldSimpleMainMenuWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual void NativeConstruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UIWorld|MainMenu")
	FName NewGameLevelName = FName(TEXT("Menu"));

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UIWorld|MainMenu")
	bool bBuildFallbackLayout = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UIWorld|MainMenu|Binding")
	FName NewGameButtonName = FName(TEXT("NewGameButton"));

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UIWorld|MainMenu|Binding")
	FName ContinueButtonName = FName(TEXT("ContinueButton"));

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UIWorld|MainMenu|Binding")
	FName SettingsButtonName = FName(TEXT("SettingsButton"));

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UIWorld|MainMenu|Binding")
	FName OnlineButtonName = FName(TEXT("OnlineButton"));

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UIWorld|MainMenu|Binding")
	FName QuitButtonName = FName(TEXT("QuitButton"));

	// Animation and visual tuning for a gritty menu vibe.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UIWorld|MainMenu|Style")
	FLinearColor IdleTextColor = FLinearColor(0.72f, 0.74f, 0.68f, 1.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UIWorld|MainMenu|Style")
	FLinearColor HoverTextColor = FLinearColor(0.90f, 0.80f, 0.42f, 1.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UIWorld|MainMenu|Style")
	FLinearColor AccentTextColor = FLinearColor(0.78f, 0.86f, 0.50f, 1.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UIWorld|MainMenu|Style")
	float IntroDurationSeconds = 0.45f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UIWorld|MainMenu|Style")
	float HoverScale = 1.07f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UIWorld|MainMenu|Style")
	float HoverInterpSpeed = 10.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UIWorld|MainMenu|Style")
	float PulseSpeed = 1.8f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UIWorld|MainMenu|Style")
	float PulseAmplitude = 0.08f;

private:
	void BuildLayoutIfNeeded();
	void ResolveButtons();
	void BindEvents();
	void ApplyBaseVisualStyle();
	void UpdateAnimatedVisuals(float DeltaSeconds);
	void SetButtonHoveredState(UButton* Button, bool bHovered);
	UTextBlock* GetButtonText(UButton* Button) const;
	UButton* CreateMenuButton(const FName Name, const FText& Label, UVerticalBox* ParentBox);
	UUIWorldMenuGameInstance* GetMenuGameInstance() const;

	UFUNCTION()
	void HandleNewGameClicked();

	UFUNCTION()
	void HandleContinueClicked();

	UFUNCTION()
	void HandleSettingsClicked();

	UFUNCTION()
	void HandleOnlineClicked();

	UFUNCTION()
	void HandleQuitClicked();

	UFUNCTION()
	void HandleNewGameHovered();
	UFUNCTION()
	void HandleNewGameUnhovered();

	UFUNCTION()
	void HandleContinueHovered();
	UFUNCTION()
	void HandleContinueUnhovered();

	UFUNCTION()
	void HandleSettingsHovered();
	UFUNCTION()
	void HandleSettingsUnhovered();

	UFUNCTION()
	void HandleOnlineHovered();
	UFUNCTION()
	void HandleOnlineUnhovered();

	UFUNCTION()
	void HandleQuitHovered();
	UFUNCTION()
	void HandleQuitUnhovered();

	UPROPERTY(Transient)
	TObjectPtr<UButton> NewGameButton;

	UPROPERTY(Transient)
	TObjectPtr<UButton> ContinueButton;

	UPROPERTY(Transient)
	TObjectPtr<UButton> SettingsButton;

	UPROPERTY(Transient)
	TObjectPtr<UButton> OnlineButton;

	UPROPERTY(Transient)
	TObjectPtr<UButton> QuitButton;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> NewGameText;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> ContinueText;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> SettingsText;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> OnlineText;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> QuitText;

	float IntroProgress = 0.0f;
	float HoverNewGame = 0.0f;
	float HoverContinue = 0.0f;
	float HoverSettings = 0.0f;
	float HoverOnline = 0.0f;
	float HoverQuit = 0.0f;

	bool bNewGameHovered = false;
	bool bContinueHovered = false;
	bool bSettingsHovered = false;
	bool bOnlineHovered = false;
	bool bQuitHovered = false;
};
