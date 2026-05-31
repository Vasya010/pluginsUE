#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/Button.h"
#include "UIWorldMenuGameInstance.h"
#include "ZonefallOnlineLobbyWidget.generated.h"

class UBorder;
class UButton;
class UEditableTextBox;
class UCheckBox;
class UImage;
class UProgressBar;
class UScrollBox;
class USpinBox;
class UTextBlock;
class UVerticalBox;
class UHorizontalBox;
class UWidgetSwitcher;
class UComboBoxString;
class UZonefallSessionCardButton;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnZonefallSessionCardClicked, int32, CardIndex);

/**
 * A UButton that remembers which session row it represents and re-broadcasts its
 * click carrying that index. Dynamic delegates can't capture an index by themselves,
 * so this lets the lobby map a click back to the correct search result reliably.
 */
UCLASS()
class UIWORLD_API UZonefallSessionCardButton : public UButton
{
	GENERATED_BODY()

public:
	UZonefallSessionCardButton();

	/** Index into the lobby's cached session list that this card represents. */
	UPROPERTY(Transient)
	int32 CardIndex = INDEX_NONE;

	/** Fired when this card is clicked, carrying CardIndex. */
	UPROPERTY(BlueprintAssignable)
	FOnZonefallSessionCardClicked OnCardClicked;

private:
	UFUNCTION()
	void HandleInternalClicked();
};

/**
 * Fully self-assembled (no Blueprint required) online lobby UI for UIWorld.
 *
 *  - Modern card-based session list (server name, players, ping, LAN/Online badge)
 *  - Refresh + Auto-refresh
 *  - Host with name / max players / LAN toggle
 *  - Join by selection
 *  - Status bar with live messages
 *  - Hooks directly into UUIWorldMenuGameInstance online APIs
 *
 * Drop it in as MainMenuGameInstance->OnlineMenuWidgetClass = UZonefallOnlineLobbyWidget::StaticClass();
 */
UCLASS(BlueprintType, Blueprintable)
class UIWORLD_API UZonefallOnlineLobbyWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UZonefallOnlineLobbyWidget(const FObjectInitializer& ObjectInitializer);

	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zonefall|Lobby|Style")
	FLinearColor BackgroundTint = FLinearColor(0.04f, 0.06f, 0.09f, 0.94f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zonefall|Lobby|Style")
	FLinearColor AccentColor = FLinearColor(0.32f, 0.86f, 0.99f, 1.0f);

	// Translucent glass card fill (frosted look).
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zonefall|Lobby|Style")
	FLinearColor CardTint = FLinearColor(1.0f, 1.0f, 1.0f, 0.04f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zonefall|Lobby|Style")
	FLinearColor CardHoverTint = FLinearColor(1.0f, 1.0f, 1.0f, 0.10f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zonefall|Lobby|Style")
	int32 TitleFontSize = 38;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zonefall|Lobby|Style")
	int32 BodyFontSize = 16;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zonefall|Lobby|Behavior")
	bool bAutoRefreshOnConstruct = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zonefall|Lobby|Behavior", meta = (ClampMin = "0.0"))
	float AutoRefreshIntervalSeconds = 8.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zonefall|Lobby|Behavior", meta = (ClampMin = "1", ClampMax = "100"))
	int32 MaxSessionResults = 50;

	// Default OFF: with Steam as the platform service the lobby uses internet
	// sessions/lobbies by default. Tick LAN in the UI for local-network play.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zonefall|Lobby|Behavior")
	bool bLanByDefault = true;

	// Maps selectable in the lobby. Used both for hosting and for "Open Level".
	// Use short names ("Menu") or full package paths ("/Game/Maps/Arena").
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zonefall|Lobby|Maps")
	TArray<FString> AvailableMaps;

	UFUNCTION(BlueprintCallable, Category = "Zonefall|Lobby")
	void RefreshSessionList();

	/** GTA-style: auto-pick best open session (lowest ping, same build). */
	UFUNCTION(BlueprintCallable, Category = "Zonefall|Lobby")
	void QuickJoinFromUI();

	UFUNCTION(BlueprintCallable, Category = "Zonefall|Lobby")
	void HostFromUI();

	UFUNCTION(BlueprintCallable, Category = "Zonefall|Lobby")
	void JoinSelectedSession();

	UFUNCTION(BlueprintCallable, Category = "Zonefall|Lobby")
	void LeaveCurrentSession();

private:
	UUIWorldMenuGameInstance* ResolveGameInstance() const;

	void BuildLayout();
	void UpdateSteamBanner();
	void RebuildSessionList(const TArray<FUIWorldOnlineSessionResult>& Results);
	void SetStatusMessage(const FString& Message, bool bSuccess = true);
	void UpdateCardSelectionVisuals();
	void SetSelectedSessionIndex(int32 NewIndex);
	void UpdateActionButtonStates();
	void UpdateNetworkModeBadge();

	UFUNCTION()
	void HandleCardClicked(int32 CardIndex);

	UFUNCTION()
	void HandleRefreshClicked();

	UFUNCTION()
	void HandleQuickJoinClicked();

	UFUNCTION()
	void HandleHostClicked();

	UFUNCTION()
	void HandleJoinClicked();

	UFUNCTION()
	void HandleLeaveClicked();

	UFUNCTION()
	void HandleBackClicked();

	UFUNCTION()
	void HandleOpenLevelClicked();

	UFUNCTION()
	void HandleJoinByIdClicked();

	// Resolves the currently selected map name from the combo (falls back to GI default).
	FName GetSelectedMapName() const;

	UFUNCTION()
	void HandleAutoRefreshTick();

	UFUNCTION()
	void HandleSessionsFound(const TArray<FUIWorldOnlineSessionResult>& Results);

	UFUNCTION()
	void HandleHostCompleted(bool bSuccess, const FString& Message);

	UFUNCTION()
	void HandleJoinCompleted(bool bSuccess, const FString& Message);

	UFUNCTION()
	void HandleLeaveCompleted(bool bSuccess, const FString& Message);

	UFUNCTION()
	void HandleOnlineMatchReady(UWorld* World);

	UFUNCTION()
	void HandleServerNameChanged(const FText& NewText, ETextCommit::Type CommitType);

	UFUNCTION()
	void HandleLanCheckChanged(bool bIsChecked);

	UPROPERTY(Transient) TObjectPtr<UBorder> RootBorder;
	UPROPERTY(Transient) TObjectPtr<UVerticalBox> RootBox;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> TitleText;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> SubtitleText;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> PlayerBannerText;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> ModeBadgeText;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> SessionListHeaderText;
	UPROPERTY(Transient) TObjectPtr<UHorizontalBox> ContentRow;
	UPROPERTY(Transient) TObjectPtr<UVerticalBox> HostColumn;
	UPROPERTY(Transient) TObjectPtr<UHorizontalBox> HostBar;
	UPROPERTY(Transient) TObjectPtr<UEditableTextBox> ServerNameInput;
	UPROPERTY(Transient) TObjectPtr<UComboBoxString> MapSelectBox;
	UPROPERTY(Transient) TObjectPtr<UComboBoxString> MaxPlayersBox;
	UPROPERTY(Transient) TObjectPtr<UCheckBox> LanCheck;
	UPROPERTY(Transient) TObjectPtr<UComboBoxString> PrivacyBox;
	UPROPERTY(Transient) TObjectPtr<UEditableTextBox> HostPasswordInput;
	UPROPERTY(Transient) TObjectPtr<UEditableTextBox> JoinPasswordInput;
	UPROPERTY(Transient) TObjectPtr<UCheckBox> HideFullCheck;
	UPROPERTY(Transient) TObjectPtr<UButton> QuickJoinButton;
	UPROPERTY(Transient) TObjectPtr<UButton> HostButton;
	UPROPERTY(Transient) TObjectPtr<UButton> RefreshButton;
	UPROPERTY(Transient) TObjectPtr<UButton> JoinButton;
	UPROPERTY(Transient) TObjectPtr<UButton> LeaveButton;
	UPROPERTY(Transient) TObjectPtr<UButton> OpenLevelButton;
	UPROPERTY(Transient) TObjectPtr<UButton> BackButton;
	// Direct connect by typed address / ID (public join).
	UPROPERTY(Transient) TObjectPtr<UEditableTextBox> JoinByIdInput;
	UPROPERTY(Transient) TObjectPtr<UButton> JoinByIdButton;
	UPROPERTY(Transient) TObjectPtr<UScrollBox> SessionList;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> StatusText;
	UPROPERTY(Transient) TObjectPtr<UProgressBar> BusyBar;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UZonefallSessionCardButton>> SessionCardButtons;

	UPROPERTY(Transient)
	TArray<FUIWorldOnlineSessionResult> CachedSessions;

	UPROPERTY(Transient)
	int32 SelectedSessionIndex = INDEX_NONE;

	UPROPERTY(Transient)
	/** Host / join / leave in progress — blocks JOIN. */
	bool bBusy = false;
	/** Session list refresh — does not block JOIN when a row is selected. */
	bool bSearchingSessions = false;

	UPROPERTY(Transient)
	float TimeSinceLastRefresh = 0.0f;

	// Double-click-to-join tracking.
	UPROPERTY(Transient)
	int32 LastClickedCardIndex = INDEX_NONE;

	double LastCardClickTime = 0.0;

	// A click on the already-selected card within this window joins immediately.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zonefall|Lobby|Behavior", meta = (ClampMin = "0.1", ClampMax = "1.0", AllowPrivateAccess = "true"))
	float DoubleClickJoinSeconds = 0.35f;

	FTimerHandle AutoRefreshTimerHandle;
};
