#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "ZonefallStartupIntroWidget.generated.h"

class UBorder;
class UMediaPlayer;
class UMediaSource;
class UImage;
class UMediaTexture;
class UTextBlock;
class UWidget;
class UMaterialInterface;
class UMaterialInstanceDynamic;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnZonefallStartupIntroFinished);

UCLASS(BlueprintType, Blueprintable)
class ZONEFALL_API UZonefallStartupIntroWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UZonefallStartupIntroWidget(const FObjectInitializer& ObjectInitializer);

	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	UPROPERTY(BlueprintAssignable, Category = "Zonefall|UI|Startup")
	FOnZonefallStartupIntroFinished OnIntroFinished;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zonefall|UI|Startup|Intro")
	TObjectPtr<UMediaPlayer> IntroMediaPlayer;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zonefall|UI|Startup|Intro")
	TObjectPtr<UMediaSource> IntroMediaSource;

	// Assign MediaTexture created from IntroMediaPlayer (Video Output Media Texture).
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zonefall|UI|Startup|Intro")
	TObjectPtr<UMediaTexture> IntroMediaTexture;

	// Optional. Preferred for UMG rendering stability: UI domain material with a Texture parameter.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zonefall|UI|Startup|Intro")
	TObjectPtr<UMaterialInterface> IntroVideoUIMaterial;

	// Texture parameter name inside IntroVideoUIMaterial.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zonefall|UI|Startup|Intro")
	FName IntroVideoTextureParameterName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zonefall|UI|Startup|Intro", meta = (ClampMin = "0.5", ClampMax = "60.0"))
	float FallbackIntroDuration;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zonefall|UI|Startup|Intro")
	bool bAutoPlayOnConstruct;

	// Fallback text if video source/texture is not ready.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zonefall|UI|Startup|Intro")
	FText IntroFallbackText;

	UFUNCTION(BlueprintCallable, Category = "Zonefall|UI|Startup|Intro")
	void StartIntro();

	UFUNCTION(BlueprintCallable, Category = "Zonefall|UI|Startup|Intro")
	void FinishIntro();

protected:
	UPROPERTY(Transient, BlueprintReadOnly, Category = "Zonefall|UI|Startup|Intro", meta = (BindWidgetOptional))
	TObjectPtr<UBorder> RootBorder;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "Zonefall|UI|Startup|Intro", meta = (BindWidgetOptional))
	TObjectPtr<UImage> VideoImage;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "Zonefall|UI|Startup|Intro", meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> FallbackText;

private:
	void ShowFallbackIntroText();
	void TryStartPlayback(const TCHAR* ReasonTag);

	UFUNCTION()
	void HandleMediaOpened(FString OpenedUrl);

	UFUNCTION()
	void HandleMediaOpenFailed(FString FailedUrl);

	UFUNCTION()
	void HandlePlaybackRetryTick();

	UFUNCTION()
	void HandleMediaEndReached();

	FTimerHandle FallbackFinishTimerHandle;
	FTimerHandle PlaybackRetryTimerHandle;
	TObjectPtr<UMaterialInstanceDynamic> IntroVideoMaterialMID;
	int32 PlaybackRetryCount;
	bool bFinished;
};

