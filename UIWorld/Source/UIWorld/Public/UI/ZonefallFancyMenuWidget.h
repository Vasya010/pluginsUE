#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Types/SlateEnums.h"
#include "ZonefallFancyMenuWidget.generated.h"

class UButton;
class UBorder;
class UCanvasPanel;
class UComboBoxString;
class UTextBlock;
class UWidget;
class UZonefallModernButtonWidget;
class UUIWorldMenuGameInstance;
struct FUIWorldOnlineSessionResult;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnZonefallMainButtonClicked);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnZonefallContinueButtonClicked);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnZonefallOnlineHostClicked);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnZonefallOnlineFindClicked);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnZonefallOnlineJoinClicked, int32, SessionIndex);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnZonefallOnlineLeaveClicked);

UENUM(BlueprintType)
enum class EZonefallMenuMoodPreset : uint8
{
	Custom UMETA(DisplayName = "Custom"),
	WastelandHorror UMETA(DisplayName = "Wasteland Horror")
};

UCLASS(BlueprintType, Blueprintable)
class ZONEFALL_API UZonefallFancyMenuWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UZonefallFancyMenuWidget(const FObjectInitializer& ObjectInitializer);

	virtual void NativeConstruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
	virtual FReply NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;

	UPROPERTY(BlueprintAssignable, Category = "Zonefall|UI")
	FOnZonefallMainButtonClicked OnMainButtonClicked;

	UPROPERTY(BlueprintAssignable, Category = "Zonefall|UI")
	FOnZonefallContinueButtonClicked OnContinueButtonClicked;

	// Optional external hooks (if you want BP to react on top of built-in flow).
	UPROPERTY(BlueprintAssignable, Category = "Zonefall|UI|Online")
	FOnZonefallOnlineHostClicked OnOnlineHostClicked;

	UPROPERTY(BlueprintAssignable, Category = "Zonefall|UI|Online")
	FOnZonefallOnlineFindClicked OnOnlineFindClicked;

	UPROPERTY(BlueprintAssignable, Category = "Zonefall|UI|Online")
	FOnZonefallOnlineJoinClicked OnOnlineJoinClicked;

	UPROPERTY(BlueprintAssignable, Category = "Zonefall|UI|Online")
	FOnZonefallOnlineLeaveClicked OnOnlineLeaveClicked;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zonefall|UI|Button")
	FText ButtonText;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zonefall|UI|Button")
	FText ContinueButtonText;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zonefall|UI|Button")
	FLinearColor NormalTextColor;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zonefall|UI|Button")
	FLinearColor HoverTextColor;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zonefall|UI|Button")
	FLinearColor PressedTextColor;

	// If true, C++ creates a fallback button only when widget has no root in designer.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zonefall|UI|Button")
	bool bAllowFallbackButtonBuild;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zonefall|UI|Font", meta = (ClampMin = "8", ClampMax = "128"))
	int32 FontSize;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zonefall|UI|Font")
	FSlateFontInfo ButtonFont;

	// Horror-style background post FX for main menu atmosphere.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zonefall|UI|HorrorFX")
	EZonefallMenuMoodPreset MoodPreset;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zonefall|UI|HorrorFX")
	bool bEnableHorrorFX;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zonefall|UI|HorrorFX", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float PulseOpacityStrength;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zonefall|UI|HorrorFX", meta = (ClampMin = "0.05", ClampMax = "10.0"))
	float PulseSpeed;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zonefall|UI|HorrorFX", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float FlickerChancePerSecond;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zonefall|UI|HorrorFX", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float FlickerDarkenStrength;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zonefall|UI|HorrorFX", meta = (ClampMin = "0.0", ClampMax = "20.0"))
	float JitterPixels;

	// Optional: name of border widget used as horror background in UMG designer.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zonefall|UI|HorrorFX")
	FName HorrorBackgroundWidgetName;

	// -------- Online UI auto-bind names (UMG Designer) --------
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zonefall|UI|Online")
	FName HostButtonWidgetName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zonefall|UI|Online")
	FName FindButtonWidgetName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zonefall|UI|Online")
	FName JoinButtonWidgetName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zonefall|UI|Online")
	FName LeaveButtonWidgetName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zonefall|UI|Online")
	FName SessionsComboBoxWidgetName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zonefall|UI|Online")
	FName OnlineStatusTextWidgetName;

	// Optional containers for "Main menu block" and "Online block".
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zonefall|UI|Online")
	FName MainMenuPanelWidgetName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zonefall|UI|Online")
	FName OnlinePanelWidgetName;

	// Optional buttons for quick switching between panels.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zonefall|UI|Online")
	FName OpenOnlineButtonWidgetName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zonefall|UI|Online")
	FName BackToMainMenuButtonWidgetName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zonefall|UI|Online")
	FName ServerNameTextWidgetName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zonefall|UI|Online")
	FName ServerPlayersTextWidgetName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zonefall|UI|Online")
	FName ServerPingTextWidgetName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zonefall|UI|Online")
	FName LocalIpTextWidgetName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zonefall|UI|Online")
	bool bApplyAaaAutoStyleToOnlineButtons;

	// Texts configurable from Details (no code edits needed).
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zonefall|UI|Online|Text")
	FText OnlineHostButtonText;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zonefall|UI|Online|Text")
	FText OnlineFindButtonText;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zonefall|UI|Online|Text")
	FText OnlineJoinButtonText;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zonefall|UI|Online|Text")
	FText OnlineLeaveButtonText;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zonefall|UI|Online|Text")
	FText OnlineStatusIdleText;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zonefall|UI|Online|Text")
	FText OnlineStatusHostingText;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zonefall|UI|Online|Text")
	FText OnlineStatusSearchingText;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zonefall|UI|Online|Text")
	FText OnlineStatusJoiningText;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zonefall|UI|Online|Text")
	FText OnlineStatusLeavingText;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zonefall|UI|Online|Text")
	FText OnlineStatusHostSuccessText;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zonefall|UI|Online|Text")
	FText OnlineStatusJoinSuccessText;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zonefall|UI|Online|Text")
	FText OnlineStatusLeaveSuccessText;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zonefall|UI|Online|Text")
	FText OnlineStatusSelectServerText;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zonefall|UI|Online|Text")
	FText OnlineNoSessionsText;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zonefall|UI|Online|Text")
	FText ServerNamePrefixText;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zonefall|UI|Online|Text")
	FText ServerPlayersPrefixText;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zonefall|UI|Online|Text")
	FText ServerPingPrefixText;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zonefall|UI|Online|Text")
	FText ServerLocalIpPrefixText;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zonefall|UI|Online")
	bool bUseLanForOnline;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zonefall|UI|Online", meta = (ClampMin = "1", ClampMax = "64"))
	int32 OnlineHostMaxPlayers;

	UFUNCTION(BlueprintCallable, Category = "Zonefall|UI")
	void SetButtonLabel(const FText& NewText);

	UFUNCTION(BlueprintCallable, Category = "Zonefall|UI")
	void ApplyStyleNow();

	UFUNCTION(BlueprintCallable, Category = "Zonefall|UI")
	void SetContinueButtonVisible(bool bVisible);

	UFUNCTION(BlueprintCallable, Category = "Zonefall|UI|Online")
	void RefreshOnlineSessionsUI();

	UFUNCTION(BlueprintCallable, Category = "Zonefall|UI|Online")
	void SetOnlineStatusText(const FText& NewStatus);

	UFUNCTION(BlueprintCallable, Category = "Zonefall|UI|Online")
	void UpdateSelectedServerCard();

	UFUNCTION(BlueprintCallable, Category = "Zonefall|UI|Online")
	void OpenOnlinePanel();

	UFUNCTION(BlueprintCallable, Category = "Zonefall|UI|Online")
	void BackToMainMenuPanel();

	// Diagnostic helper: shows which online widgets are bound by current names.
	UFUNCTION(BlueprintPure, Category = "Zonefall|UI|Online")
	FString GetOnlineBindingReport() const;

	UFUNCTION(BlueprintCallable, Category = "Zonefall|UI|HorrorFX")
	void ApplyMoodPreset();

protected:
	UPROPERTY(Transient, BlueprintReadOnly, Category = "Zonefall|UI")
	TObjectPtr<UCanvasPanel> RootPanel;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "Zonefall|UI")
	TObjectPtr<UButton> MainButton;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "Zonefall|UI")
	TObjectPtr<UTextBlock> MainButtonText;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "Zonefall|UI")
	TObjectPtr<UButton> ContinueButton;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "Zonefall|UI")
	TObjectPtr<UTextBlock> ContinueButtonTextBlock;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "Zonefall|UI")
	TObjectPtr<UWidget> ContinueWidgetFallback;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "Zonefall|UI")
	TObjectPtr<UBorder> HorrorBackgroundWidget;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "Zonefall|UI|Online")
	TObjectPtr<UButton> HostButton;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "Zonefall|UI|Online")
	TObjectPtr<UButton> FindButton;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "Zonefall|UI|Online")
	TObjectPtr<UButton> JoinButton;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "Zonefall|UI|Online")
	TObjectPtr<UButton> LeaveButton;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "Zonefall|UI|Online")
	TObjectPtr<UComboBoxString> SessionsComboBox;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "Zonefall|UI|Online")
	TObjectPtr<UTextBlock> OnlineStatusText;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "Zonefall|UI|Online")
	TObjectPtr<UWidget> MainMenuPanelWidget;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "Zonefall|UI|Online")
	TObjectPtr<UWidget> OnlinePanelWidget;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "Zonefall|UI|Online")
	TObjectPtr<UButton> OpenOnlineButton;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "Zonefall|UI|Online")
	TObjectPtr<UButton> BackToMainMenuButton;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "Zonefall|UI|Online")
	TObjectPtr<UZonefallModernButtonWidget> HostModernButton;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "Zonefall|UI|Online")
	TObjectPtr<UZonefallModernButtonWidget> FindModernButton;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "Zonefall|UI|Online")
	TObjectPtr<UZonefallModernButtonWidget> JoinModernButton;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "Zonefall|UI|Online")
	TObjectPtr<UZonefallModernButtonWidget> LeaveModernButton;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "Zonefall|UI|Online")
	TObjectPtr<UTextBlock> ServerNameText;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "Zonefall|UI|Online")
	TObjectPtr<UTextBlock> ServerPlayersText;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "Zonefall|UI|Online")
	TObjectPtr<UTextBlock> ServerPingText;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "Zonefall|UI|Online")
	TObjectPtr<UTextBlock> LocalIpText;

private:
	void BuildDefaultLayout();
	void ApplyTextStyle(const FLinearColor& Color);
	void UpdateHorrorFX(float InDeltaTime);
	void ApplyWastelandHorrorPreset();
	void BindOnlineUiEvents();
	UUIWorldMenuGameInstance* GetMenuGameInstance() const;
	void ApplyAaaStyleToOnlineButtons();
	void SetJoinEnabled(bool bEnabled);
	void ApplyOnlineButtonTexts();
	void SetNativeButtonLabel(UButton* NativeButton, const FText& Label);

	UFUNCTION()
	void HandleMainButtonClicked();

	UFUNCTION()
	void HandleMainButtonHovered();

	UFUNCTION()
	void HandleMainButtonUnhovered();

	UFUNCTION()
	void HandleMainButtonPressed();

	UFUNCTION()
	void HandleMainButtonReleased();

	UFUNCTION()
	void HandleContinueButtonClicked();

	UFUNCTION()
	void HandleContinueButtonHovered();

	UFUNCTION()
	void HandleContinueButtonUnhovered();

	UFUNCTION()
	void HandleHostButtonClicked();

	UFUNCTION()
	void HandleFindButtonClicked();

	UFUNCTION()
	void HandleJoinButtonClicked();

	UFUNCTION()
	void HandleLeaveButtonClicked();

	UFUNCTION()
	void HandleHostCompleted(bool bSuccess, const FString& Message);

	UFUNCTION()
	void HandleJoinCompleted(bool bSuccess, const FString& Message);

	UFUNCTION()
	void HandleLeaveCompleted(bool bSuccess, const FString& Message);

	UFUNCTION()
	void HandleSessionsFound(const TArray<FUIWorldOnlineSessionResult>& Results);

	UFUNCTION()
	void HandleSessionSelectionChanged(FString SelectedItem, ESelectInfo::Type SelectionType);

	UFUNCTION()
	void HandleOpenOnlineClicked();

	UFUNCTION()
	void HandleBackToMainMenuClicked();

	float HorrorFXTimeSeconds;
	float FlickerDarkenAlpha;
	float FlickerRecoverSpeed;
	bool bMainActionInProgress;
	bool bContinueActionInProgress;
	int32 CachedSessionCount;
	TArray<FUIWorldOnlineSessionResult> CachedSessionResults;
};

