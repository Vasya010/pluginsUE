#include "UI/ZonefallHUDWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Brushes/SlateRoundedBoxBrush.h"
#include "Components/Border.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Image.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/ProgressBar.h"
#include "Components/SizeBox.h"
#include "Components/Spacer.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Engine/Font.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Materials/MaterialInterface.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Styling/SlateTypes.h"

#include "Character/ZonefallPlayerCharacter.h"
#include "Weapons/ZonefallWeaponInventoryComponent.h"
#include "World/ZonefallMinimapCapture.h"
#include "World/ZonefallCharacterPortraitCapture.h"

namespace
{
	namespace ZHud
	{
		static const FLinearColor GlassFill = FLinearColor(0.02f, 0.04f, 0.07f, 0.78f);
		static const FLinearColor GlassFillDeep = FLinearColor(0.01f, 0.02f, 0.04f, 0.90f);
		static const FLinearColor GlassOutline = FLinearColor(1.0f, 1.0f, 1.0f, 0.14f);
		static const FLinearColor TextPrimary = FLinearColor(0.94f, 0.97f, 1.0f, 1.0f);
		static const FLinearColor TextMuted = FLinearColor(0.62f, 0.70f, 0.80f, 1.0f);
		static const FLinearColor HealthHigh = FLinearColor(0.22f, 0.88f, 0.62f, 1.0f);
		static const FLinearColor HealthMid = FLinearColor(0.95f, 0.78f, 0.28f, 1.0f);
		static const FLinearColor HealthLow = FLinearColor(0.92f, 0.22f, 0.20f, 1.0f);
		static const FLinearColor Stamina = FLinearColor(0.40f, 0.72f, 0.98f, 1.0f);
	}

	FSlateFontInfo MakeHudFont(int32 Size, bool bBold = false)
	{
		FSlateFontInfo Font;
		Font.Size = FMath::Clamp(Size, 8, 72);
		Font.TypefaceFontName = bBold ? FName(TEXT("Bold")) : FName(TEXT("Regular"));
		Font.FontObject = LoadObject<UObject>(nullptr, TEXT("/Engine/EngineFonts/Roboto.Roboto"));
		return Font;
	}

	FLinearColor HealthColorForFraction(float Fraction)
	{
		const float F = FMath::Clamp(Fraction, 0.0f, 1.0f);
		if (F > 0.55f)
		{
			return FMath::Lerp(ZHud::HealthMid, ZHud::HealthHigh, (F - 0.55f) / 0.45f);
		}
		return FMath::Lerp(ZHud::HealthLow, ZHud::HealthMid, F / 0.55f);
	}

	UProgressBar* MakeMeter(UWidgetTree* Tree, FName Name, const FLinearColor& FillColor)
	{
		UProgressBar* Bar = Tree->ConstructWidget<UProgressBar>(UProgressBar::StaticClass(), Name);
		FProgressBarStyle Style = Bar->GetWidgetStyle();
		Style.BackgroundImage = FSlateRoundedBoxBrush(FLinearColor(0.0f, 0.0f, 0.0f, 0.55f), 5.0f, ZHud::GlassOutline, 1.0f);
		Style.FillImage = FSlateRoundedBoxBrush(FillColor, 5.0f);
		Bar->SetWidgetStyle(Style);
		Bar->SetPercent(1.0f);
		return Bar;
	}
}

UZonefallHUDWidget::UZonefallHUDWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

TSharedRef<SWidget> UZonefallHUDWidget::RebuildWidget()
{
	if (!WidgetTree || !WidgetTree->RootWidget)
	{
		BuildLayout();
	}
	return Super::RebuildWidget();
}

void UZonefallHUDWidget::BindToCharacter(AZonefallPlayerCharacter* InCharacter)
{
	Character = InCharacter;
}

void UZonefallHUDWidget::BuildLayout()
{
	if (!WidgetTree)
	{
		return;
	}

	WidgetTree->RootWidget = nullptr;

	UCanvasPanel* Root = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("HUDRoot"));
	WidgetTree->RootWidget = Root;

	// Safe-zone margin from screen edges so nothing ever clips off-frame.
	const float Margin = 48.0f;

	// ============================================================================
	// Bottom-LEFT: polished round minimap (double ring + compass + rotating blip).
	// ============================================================================
	{
		UOverlay* MapOverlay = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass(), TEXT("MinimapOverlay"));

		MinimapImage = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("MinimapImage"));
		if (UOverlaySlot* ImgSlot = MapOverlay->AddChildToOverlay(MinimapImage))
		{
			ImgSlot->SetHorizontalAlignment(HAlign_Fill);
			ImgSlot->SetVerticalAlignment(VAlign_Fill);
		}

		// Thick dark bezel.
		UBorder* Ring = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("MinimapRing"));
		Ring->SetBrush(FSlateRoundedBoxBrush(FLinearColor(0, 0, 0, 0), MinimapSize * 0.5f, FLinearColor(0.0f, 0.0f, 0.0f, 0.95f), 9.0f));
		if (UOverlaySlot* RingSlot = MapOverlay->AddChildToOverlay(Ring))
		{
			RingSlot->SetHorizontalAlignment(HAlign_Fill);
			RingSlot->SetVerticalAlignment(VAlign_Fill);
		}

		// Bright accent ring just inside the bezel.
		UBorder* RingAccent = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("MinimapRingAccent"));
		RingAccent->SetBrush(FSlateRoundedBoxBrush(FLinearColor(0, 0, 0, 0), MinimapSize * 0.5f, AccentColor * FLinearColor(1, 1, 1, 0.75f), 2.5f));
		if (UOverlaySlot* RingAccentSlot = MapOverlay->AddChildToOverlay(RingAccent))
		{
			RingAccentSlot->SetHorizontalAlignment(HAlign_Fill);
			RingAccentSlot->SetVerticalAlignment(VAlign_Fill);
		}

		// Compass "N" pinned to the top of the ring.
		CompassNorthText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("CompassNorth"));
		CompassNorthText->SetText(NSLOCTEXT("ZonefallHUD", "North", "N"));
		CompassNorthText->SetFont(MakeHudFont(13, true));
		CompassNorthText->SetColorAndOpacity(FSlateColor(AccentColor));
		if (UOverlaySlot* NorthSlot = MapOverlay->AddChildToOverlay(CompassNorthText))
		{
			NorthSlot->SetHorizontalAlignment(HAlign_Center);
			NorthSlot->SetVerticalAlignment(VAlign_Top);
			NorthSlot->SetPadding(FMargin(0.0f, 4.0f, 0.0f, 0.0f));
		}

		// Player blip (an arrow-ish marker) at the centre.
		MinimapBlip = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("MinimapBlip"));
		MinimapBlip->SetBrush(FSlateRoundedBoxBrush(AccentColor, 2.0f));
		MinimapBlip->SetDesiredSizeOverride(FVector2D(8.0f, 18.0f));
		if (UOverlaySlot* BlipSlot = MapOverlay->AddChildToOverlay(MinimapBlip))
		{
			BlipSlot->SetHorizontalAlignment(HAlign_Center);
			BlipSlot->SetVerticalAlignment(VAlign_Center);
		}

		USizeBox* MapSize = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("MinimapSizeBox"));
		MapSize->SetWidthOverride(MinimapSize);
		MapSize->SetHeightOverride(MinimapSize);
		MapSize->AddChild(MapOverlay);

		if (UCanvasPanelSlot* MapCanvas = Root->AddChildToCanvas(MapSize))
		{
			MapCanvas->SetAnchors(FAnchors(0.0f, 1.0f));
			MapCanvas->SetAlignment(FVector2D(0.0f, 1.0f));
			MapCanvas->SetAutoSize(true);
			MapCanvas->SetPosition(FVector2D(Margin, -Margin));
		}
	}

	// ============================================================================
	// Bottom-RIGHT: rich VITALS CARD — live portrait + health + stamina + weapon/ammo.
	// ============================================================================
	{
		UHorizontalBox* CardRow = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("VitalsRow"));

		// ---------- Portrait block (the hero "draws himself") ----------
		{
			UOverlay* PortraitOverlay = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass(), TEXT("PortraitOverlay"));

			PortraitImage = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("PortraitImage"));
			// Placeholder fill until the render target is live.
			PortraitImage->SetBrush(FSlateRoundedBoxBrush(FLinearColor(0.06f, 0.09f, 0.13f, 1.0f), 8.0f));
			if (UOverlaySlot* PSlot = PortraitOverlay->AddChildToOverlay(PortraitImage))
			{
				PSlot->SetHorizontalAlignment(HAlign_Fill);
				PSlot->SetVerticalAlignment(VAlign_Fill);
			}

			// Frame around the portrait.
			UBorder* PortraitFrame = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("PortraitFrame"));
			PortraitFrame->SetBrush(FSlateRoundedBoxBrush(FLinearColor(0, 0, 0, 0), 8.0f, AccentColor * FLinearColor(1, 1, 1, 0.8f), 2.0f));
			if (UOverlaySlot* FSlot = PortraitOverlay->AddChildToOverlay(PortraitFrame))
			{
				FSlot->SetHorizontalAlignment(HAlign_Fill);
				FSlot->SetVerticalAlignment(VAlign_Fill);
			}

			USizeBox* PortraitSizeBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("PortraitSizeBox"));
			PortraitSizeBox->SetWidthOverride(PortraitSize);
			PortraitSizeBox->SetHeightOverride(PortraitSize);
			PortraitSizeBox->AddChild(PortraitOverlay);

			if (UHorizontalBoxSlot* PortSlot = CardRow->AddChildToHorizontalBox(PortraitSizeBox))
			{
				PortSlot->SetVerticalAlignment(VAlign_Center);
				PortSlot->SetPadding(FMargin(0.0f, 0.0f, 14.0f, 0.0f));
			}
		}

		// ---------- Info column: name, health, stamina, weapon/ammo ----------
		{
			UVerticalBox* InfoCol = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("VitalsInfoCol"));

			// Name.
			PlayerNameText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("PlayerName"));
			PlayerNameText->SetText(PlayerDisplayName.IsEmpty() ? NSLOCTEXT("ZonefallHUD", "Protagonist", "PROTAGONIST") : PlayerDisplayName);
			PlayerNameText->SetFont(MakeHudFont(13, true));
			PlayerNameText->SetColorAndOpacity(FSlateColor(ZHud::TextPrimary));
			InfoCol->AddChildToVerticalBox(PlayerNameText);

			// HEALTH label row.
			UHorizontalBox* HpRow = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("HpLabelRow"));
			UTextBlock* HpHeader = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("HpHeader"));
			HpHeader->SetText(NSLOCTEXT("ZonefallHUD", "Health", "HEALTH"));
			HpHeader->SetFont(MakeHudFont(10, true));
			HpHeader->SetColorAndOpacity(FSlateColor(ZHud::TextMuted));
			if (UHorizontalBoxSlot* HHSlot = HpRow->AddChildToHorizontalBox(HpHeader))
			{
				HHSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
				HHSlot->SetVerticalAlignment(VAlign_Center);
			}
			HealthText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("HealthText"));
			HealthText->SetText(FText::FromString(TEXT("100 / 100")));
			HealthText->SetFont(MakeHudFont(12, true));
			HealthText->SetColorAndOpacity(FSlateColor(ZHud::TextPrimary));
			HealthText->SetJustification(ETextJustify::Right);
			if (UHorizontalBoxSlot* HTSlot = HpRow->AddChildToHorizontalBox(HealthText))
			{
				HTSlot->SetVerticalAlignment(VAlign_Center);
			}
			if (UVerticalBoxSlot* HpRowSlot = InfoCol->AddChildToVerticalBox(HpRow))
			{
				HpRowSlot->SetPadding(FMargin(0.0f, 8.0f, 0.0f, 0.0f));
			}

			// Health meter.
			USizeBox* HpBarSize = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("HpBarSize"));
			HpBarSize->SetWidthOverride(VitalsWidth);
			HpBarSize->SetHeightOverride(13.0f);
			HealthBar = MakeMeter(WidgetTree, TEXT("HealthBar"), ZHud::HealthHigh);
			HpBarSize->AddChild(HealthBar);
			if (UVerticalBoxSlot* HpBarSlot = InfoCol->AddChildToVerticalBox(HpBarSize))
			{
				HpBarSlot->SetPadding(FMargin(0.0f, 4.0f, 0.0f, 0.0f));
			}

			// Stamina / Dead-Eye meter (thinner).
			USizeBox* StamBarSize = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("StaminaBarSize"));
			StamBarSize->SetWidthOverride(VitalsWidth);
			StamBarSize->SetHeightOverride(7.0f);
			StaminaBar = MakeMeter(WidgetTree, TEXT("StaminaBar"), ZHud::Stamina);
			StamBarSize->AddChild(StaminaBar);
			if (UVerticalBoxSlot* StamSlot = InfoCol->AddChildToVerticalBox(StamBarSize))
			{
				StamSlot->SetPadding(FMargin(0.0f, 5.0f, 0.0f, 0.0f));
			}

			// Weapon name + ammo row.
			UHorizontalBox* WeaponRow = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("WeaponRow"));
			WeaponNameText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("WeaponName"));
			WeaponNameText->SetText(NSLOCTEXT("ZonefallHUD", "Unarmed", "Unarmed"));
			WeaponNameText->SetFont(MakeHudFont(12));
			WeaponNameText->SetColorAndOpacity(FSlateColor(ZHud::TextMuted));
			if (UHorizontalBoxSlot* WNSlot = WeaponRow->AddChildToHorizontalBox(WeaponNameText))
			{
				WNSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
				WNSlot->SetVerticalAlignment(VAlign_Center);
			}
			AmmoText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("AmmoText"));
			AmmoText->SetText(FText::GetEmpty());
			AmmoText->SetFont(MakeHudFont(18, true));
			AmmoText->SetColorAndOpacity(FSlateColor(AccentColor));
			AmmoText->SetJustification(ETextJustify::Right);
			if (UHorizontalBoxSlot* AmmoSlot = WeaponRow->AddChildToHorizontalBox(AmmoText))
			{
				AmmoSlot->SetVerticalAlignment(VAlign_Center);
			}
			if (UVerticalBoxSlot* WeaponRowSlot = InfoCol->AddChildToVerticalBox(WeaponRow))
			{
				WeaponRowSlot->SetPadding(FMargin(0.0f, 12.0f, 0.0f, 0.0f));
			}

			if (UHorizontalBoxSlot* InfoSlot = CardRow->AddChildToHorizontalBox(InfoCol))
			{
				InfoSlot->SetVerticalAlignment(VAlign_Center);
			}
		}

		VitalsPanel = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("VitalsPanel"));
		VitalsPanel->SetBrush(FSlateRoundedBoxBrush(ZHud::GlassFill, 12.0f, ZHud::GlassOutline, 1.0f));
		VitalsPanel->SetPadding(FMargin(16.0f, 14.0f));
		VitalsPanel->SetContent(CardRow);

		if (UCanvasPanelSlot* VitalsCanvas = Root->AddChildToCanvas(VitalsPanel))
		{
			VitalsCanvas->SetAnchors(FAnchors(1.0f, 1.0f));
			VitalsCanvas->SetAlignment(FVector2D(1.0f, 1.0f));
			VitalsCanvas->SetAutoSize(true);
			VitalsCanvas->SetPosition(FVector2D(-Margin, -Margin));
		}
	}
}

void UZonefallHUDWidget::RefreshMinimapBrush()
{
	if (bMinimapBrushSet || !MinimapImage || !MinimapCapture)
	{
		return;
	}

	UTextureRenderTarget2D* RT = MinimapCapture->GetRenderTarget();
	if (!RT)
	{
		return;
	}

	if (MinimapMaterial)
	{
		MinimapMID = UMaterialInstanceDynamic::Create(MinimapMaterial, this);
		if (MinimapMID)
		{
			MinimapMID->SetTextureParameterValue(TEXT("Texture"), RT);
			MinimapImage->SetBrushFromMaterial(MinimapMID);
		}
	}
	else
	{
		FSlateBrush Brush;
		Brush.SetResourceObject(RT);
		Brush.ImageSize = FVector2D(MinimapSize, MinimapSize);
		Brush.DrawAs = ESlateBrushDrawType::Image;
		Brush.TintColor = FSlateColor(FLinearColor::White);
		MinimapImage->SetBrush(Brush);
	}

	MinimapImage->SetColorAndOpacity(FLinearColor::White);
	bMinimapBrushSet = true;
}

void UZonefallHUDWidget::RefreshPortraitBrush()
{
	if (bPortraitBrushSet || !PortraitImage || !PortraitCapture)
	{
		return;
	}

	UTextureRenderTarget2D* RT = PortraitCapture->GetRenderTarget();
	if (!RT)
	{
		return;
	}

	FSlateBrush Brush;
	Brush.SetResourceObject(RT);
	Brush.ImageSize = FVector2D(PortraitSize, PortraitSize);
	Brush.DrawAs = ESlateBrushDrawType::Image;
	Brush.TintColor = FSlateColor(FLinearColor::White);
	PortraitImage->SetBrush(Brush);
	PortraitImage->SetColorAndOpacity(FLinearColor::White);
	bPortraitBrushSet = true;
}

void UZonefallHUDWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	HudPulseTime += InDeltaTime;

	if (Character)
	{
		const float HealthFrac = Character->GetHealthFraction();
		const int32 CurrentHp = FMath::RoundToInt(Character->GetHealth());
		const int32 MaxHp = FMath::Max(1, FMath::RoundToInt(Character->GetMaxHealth()));

		if (HealthBar)
		{
			HealthBar->SetPercent(HealthFrac);

			const FLinearColor FillColor = HealthColorForFraction(HealthFrac);
			const float Pulse = HealthFrac < 0.25f ? (0.78f + 0.22f * FMath::Sin(HudPulseTime * 8.0f)) : 1.0f;
			HealthBar->SetFillColorAndOpacity(FillColor * FLinearColor(1, 1, 1, Pulse));
		}

		if (HealthText)
		{
			HealthText->SetText(FText::FromString(FString::Printf(TEXT("%d / %d"), CurrentHp, MaxHp)));
		}

		if (StaminaBar)
		{
			StaminaBar->SetPercent(Character->GetDeadEyeFraction());
		}

		// Low-health: pulse the card outline red.
		if (VitalsPanel)
		{
			if (HealthFrac < 0.25f)
			{
				const float OutlineAlpha = 0.35f + 0.30f * FMath::Sin(HudPulseTime * 8.0f);
				VitalsPanel->SetBrush(FSlateRoundedBoxBrush(ZHud::GlassFill, 12.0f, ZHud::HealthLow * FLinearColor(1, 1, 1, OutlineAlpha), 2.0f));
			}
			else
			{
				VitalsPanel->SetBrush(FSlateRoundedBoxBrush(ZHud::GlassFill, 12.0f, ZHud::GlassOutline, 1.0f));
			}
		}

		if (UZonefallWeaponInventoryComponent* WInv = Character->GetWeapons())
		{
			if (WInv->HasEquippedWeapon())
			{
				const FZonefallWeaponItem W = WInv->GetEquippedWeapon();
				if (WeaponNameText)
				{
					WeaponNameText->SetText(W.DisplayName.IsEmpty() ? FText::FromName(W.WeaponId) : W.DisplayName);
				}
				if (AmmoText)
				{
					if (W.Slot == EZonefallWeaponSlot::Melee)
					{
						AmmoText->SetText(NSLOCTEXT("ZonefallHUD", "Melee", "—"));
						AmmoText->SetColorAndOpacity(FSlateColor(ZHud::TextMuted));
					}
					else if (W.AmmoInClip <= 0 && W.AmmoReserve <= 0)
					{
						AmmoText->SetText(NSLOCTEXT("ZonefallHUD", "NoAmmo", "EMPTY"));
						AmmoText->SetColorAndOpacity(FSlateColor(ZHud::HealthLow));
					}
					else if (W.AmmoInClip <= 0)
					{
						AmmoText->SetText(NSLOCTEXT("ZonefallHUD", "Reload", "RELOAD [R]"));
						AmmoText->SetColorAndOpacity(FSlateColor(FLinearColor(0.95f, 0.7f, 0.2f, 1.0f)));
					}
					else
					{
						AmmoText->SetText(FText::FromString(FString::Printf(TEXT("%d  |  %d"), W.AmmoInClip, W.AmmoReserve)));
						AmmoText->SetColorAndOpacity(FSlateColor(AccentColor));
					}
				}
			}
			else
			{
				if (WeaponNameText)
				{
					WeaponNameText->SetText(NSLOCTEXT("ZonefallHUD", "Unarmed", "Unarmed"));
				}
				if (AmmoText)
				{
					AmmoText->SetText(FText::GetEmpty());
				}
			}
		}
	}

	if (!MinimapCapture)
	{
		MinimapCapture = AZonefallMinimapCapture::Get(GetWorld());
	}
	RefreshMinimapBrush();

	if (!PortraitCapture)
	{
		PortraitCapture = AZonefallCharacterPortraitCapture::Get(GetWorld());
	}
	RefreshPortraitBrush();

	if (MinimapBlip && Character)
	{
		const float Yaw = Character->GetControlRotation().Yaw;
		MinimapBlip->SetRenderTransformAngle(Yaw);
	}
}
