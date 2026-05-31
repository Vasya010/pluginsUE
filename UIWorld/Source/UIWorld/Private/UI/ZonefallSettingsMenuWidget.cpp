#include "ZonefallSettingsMenuWidget.h"

#include "UIWorldMenuGameInstance.h"
#include "ZonefallSettingsDataObject.h"
#include "Blueprint/WidgetTree.h"
#include "Brushes/SlateRoundedBoxBrush.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/ComboBoxString.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/ProgressBar.h"
#include "Components/SizeBox.h"
#include "Components/Spacer.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Styling/SlateTypes.h"

namespace
{
UButton* FindFirstButtonInUserWidget(UUserWidget* InUserWidget)
{
	if (!InUserWidget || !InUserWidget->WidgetTree)
	{
		return nullptr;
	}

	UButton* FoundButton = nullptr;
	InUserWidget->WidgetTree->ForEachWidget([&FoundButton](UWidget* ChildWidget)
	{
		if (!FoundButton)
		{
			FoundButton = Cast<UButton>(ChildWidget);
		}
	});

	return FoundButton;
}

UButton* ResolveButtonFromWidgetName(UWidgetTree* InWidgetTree, const FName& WidgetName)
{
	if (!InWidgetTree || WidgetName.IsNone())
	{
		return nullptr;
	}

	if (UWidget* FoundWidget = InWidgetTree->FindWidget(WidgetName))
	{
		if (UButton* AsButton = Cast<UButton>(FoundWidget))
		{
			return AsButton;
		}
		if (UUserWidget* AsUserWidget = Cast<UUserWidget>(FoundWidget))
		{
			return FindFirstButtonInUserWidget(AsUserWidget);
		}
	}

	return nullptr;
}
}

UZonefallSettingsMenuWidget::UZonefallSettingsMenuWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	DisplayModeComboBoxName = TEXT("DisplayModeCombo");
	GraphicsPresetComboBoxName = TEXT("GraphicsPresetCombo");
	OverallQualityComboBoxName = TEXT("OverallQualityCombo");
	ResolutionScaleComboBoxName = TEXT("ResolutionScaleCombo");
	ScreenResolutionComboBoxName = TEXT("Fullscreen");
	VSyncComboBoxName = TEXT("VSyncCombo");
	FPSLimitComboBoxName = TEXT("FPSLimitCombo");
	LumenComboBoxName = TEXT("LumenCombo");
	DLSSComboBoxName = TEXT("DLSSCombo");
	FrameGenerationComboBoxName = TEXT("FrameGenerationCombo");
	FSRComboBoxName = TEXT("FSRCombo");
	FSRFrameGenerationComboBoxName = TEXT("FSRFrameGenerationCombo");
	ApplyButtonName = TEXT("ApplyButton");
	ResetButtonName = TEXT("ResetButton");
	BackButtonName = TEXT("BackButton");
	MemoryUsageProgressBarName = TEXT("MemoryUsageProgress");
	MemoryUsageTextName = TEXT("MemoryUsageText");
	ApplyStatusTextName = TEXT("ApplyStatusText");
	bAutoApplyDisplayModeAndResolution = true;
	bHasPendingChanges = false;
	bIsRefreshingFromSettings = false;
}

void UZonefallSettingsMenuWidget::NativePreConstruct()
{
	Super::NativePreConstruct();

	ResolveNamedWidgets();
	BuildLayoutIfNeeded();
	ResolveNamedWidgets();
	ApplyModernVisualTheme();
	PopulateComboOptionsIfNeeded();
}

void UZonefallSettingsMenuWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (!SettingsObject)
	{
		SettingsObject = NewObject<UZonefallSettingsDataObject>(this);
		SettingsObject->SetDefaults();
	}

	ResolveNamedWidgets();
	BuildLayoutIfNeeded();
	ResolveNamedWidgets();
	ApplyModernVisualTheme();
	PopulateComboOptionsIfNeeded();
	PopulateScreenResolutionsIfNeeded();
	BindEvents();
	RefreshFromSettings();
}

void UZonefallSettingsMenuWidget::RefreshFromSettings()
{
	if (!SettingsObject)
	{
		return;
	}

	bIsRefreshingFromSettings = true;
	SettingsObject->LoadFromSystem();
	SettingsObject->SanitizeSettings();

	if (DisplayModeComboBox) { DisplayModeComboBox->SetSelectedOption(SettingsObject->DisplayMode); }
	if (GraphicsPresetComboBox) { GraphicsPresetComboBox->SetSelectedOption(TEXT("Custom")); }
	if (OverallQualityComboBox) { OverallQualityComboBox->SetSelectedOption(SettingsObject->OverallQuality); }
	if (ResolutionScaleComboBox)
	{
		EnsureComboHasOption(ResolutionScaleComboBox, SettingsObject->ResolutionScale);
		ResolutionScaleComboBox->SetSelectedOption(SettingsObject->ResolutionScale);
	}
	if (ScreenResolutionComboBox)
	{
		const FString CurrentScreenResolution = SettingsObject->GetCurrentScreenResolutionString();
		EnsureComboHasOption(ScreenResolutionComboBox, CurrentScreenResolution);
		ScreenResolutionComboBox->SetSelectedOption(CurrentScreenResolution);
	}
	if (VSyncComboBox) { VSyncComboBox->SetSelectedOption(SettingsObject->VSync); }
	if (FPSLimitComboBox)
	{
		EnsureComboHasOption(FPSLimitComboBox, SettingsObject->FPSLimit);
		FPSLimitComboBox->SetSelectedOption(SettingsObject->FPSLimit);
	}
	if (LumenComboBox) { LumenComboBox->SetSelectedOption(SettingsObject->Lumen); }
	if (DLSSComboBox) { EnsureComboHasOption(DLSSComboBox, SettingsObject->DLSSMode); DLSSComboBox->SetSelectedOption(SettingsObject->DLSSMode.IsEmpty() ? TEXT("Off") : SettingsObject->DLSSMode); }
	if (FrameGenerationComboBox) { EnsureComboHasOption(FrameGenerationComboBox, SettingsObject->FrameGeneration); FrameGenerationComboBox->SetSelectedOption(SettingsObject->FrameGeneration.IsEmpty() ? TEXT("Off") : SettingsObject->FrameGeneration); }
	if (FSRComboBox) { EnsureComboHasOption(FSRComboBox, SettingsObject->FSRMode); FSRComboBox->SetSelectedOption(SettingsObject->FSRMode.IsEmpty() ? TEXT("Off") : SettingsObject->FSRMode); }
	if (FSRFrameGenerationComboBox) { EnsureComboHasOption(FSRFrameGenerationComboBox, SettingsObject->FSRFrameGeneration); FSRFrameGenerationComboBox->SetSelectedOption(SettingsObject->FSRFrameGeneration.IsEmpty() ? TEXT("Off") : SettingsObject->FSRFrameGeneration); }

	UpdateFeatureAvailabilityUI();
	UpdatePerformanceEstimateUI();
	UpdateTexts();
	bHasPendingChanges = false;
	UpdateApplyButtonState();
	SetApplyStatusMessage(FText::FromString(TEXT("Settings synced")), FLinearColor(0.60f, 0.86f, 0.64f, 1.0f));
	bIsRefreshingFromSettings = false;
}

void UZonefallSettingsMenuWidget::ApplySettingsNow()
{
	if (!SettingsObject)
	{
		return;
	}

	SettingsObject->ApplyToSystem(this);
	bHasPendingChanges = false;
	UpdateApplyButtonState();
	SetApplyStatusMessage(FText::FromString(TEXT("Settings applied")), FLinearColor(0.66f, 0.92f, 0.72f, 1.0f));
	OnSettingsApplied.Broadcast();
}

void UZonefallSettingsMenuWidget::ResetToDefaults()
{
	if (!SettingsObject)
	{
		return;
	}

	SettingsObject->SetDefaults();
	bIsRefreshingFromSettings = true;
	if (DisplayModeComboBox) { DisplayModeComboBox->SetSelectedOption(SettingsObject->DisplayMode); }
	if (GraphicsPresetComboBox) { GraphicsPresetComboBox->SetSelectedOption(TEXT("Custom")); }
	if (OverallQualityComboBox) { OverallQualityComboBox->SetSelectedOption(SettingsObject->OverallQuality); }
	if (ResolutionScaleComboBox) { ResolutionScaleComboBox->SetSelectedOption(SettingsObject->ResolutionScale); }
	if (ScreenResolutionComboBox) { EnsureComboHasOption(ScreenResolutionComboBox, SettingsObject->ScreenResolution); ScreenResolutionComboBox->SetSelectedOption(SettingsObject->ScreenResolution); }
	if (VSyncComboBox) { VSyncComboBox->SetSelectedOption(SettingsObject->VSync); }
	if (FPSLimitComboBox) { EnsureComboHasOption(FPSLimitComboBox, SettingsObject->FPSLimit); FPSLimitComboBox->SetSelectedOption(SettingsObject->FPSLimit); }
	if (LumenComboBox) { LumenComboBox->SetSelectedOption(SettingsObject->Lumen); }
	if (DLSSComboBox) { EnsureComboHasOption(DLSSComboBox, SettingsObject->DLSSMode); DLSSComboBox->SetSelectedOption(SettingsObject->DLSSMode); }
	if (FrameGenerationComboBox) { EnsureComboHasOption(FrameGenerationComboBox, SettingsObject->FrameGeneration); FrameGenerationComboBox->SetSelectedOption(SettingsObject->FrameGeneration); }
	if (FSRComboBox) { EnsureComboHasOption(FSRComboBox, SettingsObject->FSRMode); FSRComboBox->SetSelectedOption(SettingsObject->FSRMode); }
	if (FSRFrameGenerationComboBox) { EnsureComboHasOption(FSRFrameGenerationComboBox, SettingsObject->FSRFrameGeneration); FSRFrameGenerationComboBox->SetSelectedOption(SettingsObject->FSRFrameGeneration); }
	bIsRefreshingFromSettings = false;
	UpdateFeatureAvailabilityUI();
	UpdatePerformanceEstimateUI();
	UpdateTexts();
	MarkSettingsDirty();
	SetApplyStatusMessage(FText::FromString(TEXT("Defaults loaded (press APPLY)")), FLinearColor(0.95f, 0.80f, 0.30f, 1.0f));
}

void UZonefallSettingsMenuWidget::BuildLayoutIfNeeded()
{
	if (!WidgetTree || WidgetTree->RootWidget)
	{
		return;
	}

	RootBorder = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("SettingsRootBorder"));
	RootBorder->SetBrushColor(FLinearColor(0.03f, 0.05f, 0.04f, 0.90f));
	RootBorder->SetPadding(FMargin(18.0f));
	WidgetTree->RootWidget = RootBorder;

	RootBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("SettingsRootBox"));
	RootBorder->SetContent(RootBox);

	TitleText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("SettingsTitle"));
	TitleText->SetText(FText::FromString(TEXT("SETTINGS")));
	FSlateFontInfo TitleFont;
	TitleFont.Size = 44;
	TitleText->SetFont(TitleFont);
	TitleText->SetColorAndOpacity(FSlateColor(FLinearColor(0.90f, 0.95f, 0.85f, 1.0f)));
	if (UVerticalBoxSlot* TitleSlot = RootBox->AddChildToVerticalBox(TitleText))
	{
		TitleSlot->SetPadding(FMargin(4, 4, 4, 16));
	}

	auto AddSectionHeader = [this](const TCHAR* HeaderText)
	{
		if (!RootBox)
		{
			return;
		}

		UTextBlock* Header = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
		FSlateFontInfo HeaderFont;
		HeaderFont.Size = 22;
		Header->SetFont(HeaderFont);
		Header->SetText(FText::FromString(HeaderText));
		Header->SetColorAndOpacity(FSlateColor(FLinearColor(0.72f, 0.84f, 0.98f, 1.0f)));
		Header->SetShadowOffset(FVector2D(0.0f, 1.0f));
		Header->SetShadowColorAndOpacity(FLinearColor(0.0f, 0.0f, 0.0f, 0.70f));
		if (UVerticalBoxSlot* Slot = RootBox->AddChildToVerticalBox(Header))
		{
			Slot->SetPadding(FMargin(0, 14, 0, 6));
		}
	};

	AddSectionHeader(TEXT("DISPLAY"));
	DisplayModeComboBox = CreateOptionCombo(TEXT("DisplayModeCombo"), DisplayModeText);
	ScreenResolutionComboBox = CreateOptionCombo(TEXT("Fullscreen"), ScreenResolutionText);

	AddSectionHeader(TEXT("GRAPHICS"));
	GraphicsPresetComboBox = CreateOptionCombo(TEXT("GraphicsPresetCombo"), GraphicsPresetText);
	OverallQualityComboBox = CreateOptionCombo(TEXT("OverallQualityCombo"), OverallQualityText);
	ResolutionScaleComboBox = CreateOptionCombo(TEXT("ResolutionScaleCombo"), ResolutionScaleText);
	VSyncComboBox = CreateOptionCombo(TEXT("VSyncCombo"), VSyncText);
	FPSLimitComboBox = CreateOptionCombo(TEXT("FPSLimitCombo"), FPSLimitText);
	LumenComboBox = CreateOptionCombo(TEXT("LumenCombo"), LumenText);

	AddSectionHeader(TEXT("UPSCALING"));
	DLSSComboBox = CreateOptionCombo(TEXT("DLSSCombo"), DLSSText);
	FrameGenerationComboBox = CreateOptionCombo(TEXT("FrameGenerationCombo"), FrameGenerationText);
	FSRComboBox = CreateOptionCombo(TEXT("FSRCombo"), FSRText);
	FSRFrameGenerationComboBox = CreateOptionCombo(TEXT("FSRFrameGenerationCombo"), FSRFrameGenerationText);

	MemoryUsageText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("MemoryUsageText"));
	MemoryUsageText->SetText(FText::FromString(TEXT("Estimated Memory Usage")));
	MemoryUsageText->SetColorAndOpacity(FSlateColor(FLinearColor(0.80f, 0.88f, 0.78f, 1.0f)));
	FSlateFontInfo MemoryFont;
	MemoryFont.Size = 22;
	MemoryUsageText->SetFont(MemoryFont);
	if (UVerticalBoxSlot* MemoryTextSlot = RootBox->AddChildToVerticalBox(MemoryUsageText))
	{
		MemoryTextSlot->SetPadding(FMargin(0, 10, 0, 4));
	}

	MemoryUsageProgressBar = WidgetTree->ConstructWidget<UProgressBar>(UProgressBar::StaticClass(), TEXT("MemoryUsageProgress"));
	MemoryUsageProgressBar->SetFillColorAndOpacity(FLinearColor(0.20f, 0.75f, 0.35f, 1.0f));
	MemoryUsageProgressBar->SetPercent(0.5f);
	if (UVerticalBoxSlot* MemoryBarSlot = RootBox->AddChildToVerticalBox(MemoryUsageProgressBar))
	{
		MemoryBarSlot->SetPadding(FMargin(0, 0, 0, 8));
	}

	TObjectPtr<UTextBlock> UnusedText = nullptr;
	auto CreateActionButton = [this, &UnusedText](const TCHAR* Name, const TCHAR* Label) -> UButton*
	{
		UComboBoxString* IgnoreCombo = nullptr;
		UButton* NewButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), Name);
		ApplyButtonStyle(NewButton);
		if (UVerticalBoxSlot* ActionSlot = RootBox->AddChildToVerticalBox(NewButton))
		{
			ActionSlot->SetPadding(FMargin(0, 6, 0, 6));
		}

		TObjectPtr<UTextBlock> ActionText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
		FSlateFontInfo Font;
		Font.Size = 28;
		ActionText->SetFont(Font);
		ActionText->SetColorAndOpacity(FSlateColor(FLinearColor(0.85f, 0.90f, 0.82f, 1.0f)));
		ActionText->SetText(FText::FromString(Label));
		NewButton->AddChild(ActionText);
		return NewButton;
	};

	ApplyButton = CreateActionButton(TEXT("ApplyButton"), TEXT("APPLY"));
	ResetButton = CreateActionButton(TEXT("ResetButton"), TEXT("RESET DEFAULTS"));
	BackButton = CreateActionButton(TEXT("BackButton"), TEXT("BACK"));
}

void UZonefallSettingsMenuWidget::BindEvents()
{
	if (DisplayModeComboBox)
	{
		DisplayModeComboBox->OnSelectionChanged.AddUniqueDynamic(this, &UZonefallSettingsMenuWidget::HandleDisplayModeSelectionChanged);
	}
	if (GraphicsPresetComboBox)
	{
		GraphicsPresetComboBox->OnSelectionChanged.AddUniqueDynamic(this, &UZonefallSettingsMenuWidget::HandleGraphicsPresetSelectionChanged);
	}
	if (OverallQualityComboBox)
	{
		OverallQualityComboBox->OnSelectionChanged.AddUniqueDynamic(this, &UZonefallSettingsMenuWidget::HandleOverallQualitySelectionChanged);
	}
	if (ResolutionScaleComboBox)
	{
		ResolutionScaleComboBox->OnSelectionChanged.AddUniqueDynamic(this, &UZonefallSettingsMenuWidget::HandleResolutionScaleSelectionChanged);
	}
	if (ScreenResolutionComboBox)
	{
		ScreenResolutionComboBox->OnSelectionChanged.AddUniqueDynamic(this, &UZonefallSettingsMenuWidget::HandleScreenResolutionSelectionChanged);
	}
	if (VSyncComboBox)
	{
		VSyncComboBox->OnSelectionChanged.AddUniqueDynamic(this, &UZonefallSettingsMenuWidget::HandleVSyncSelectionChanged);
	}
	if (FPSLimitComboBox)
	{
		FPSLimitComboBox->OnSelectionChanged.AddUniqueDynamic(this, &UZonefallSettingsMenuWidget::HandleFPSLimitSelectionChanged);
	}
	if (LumenComboBox)
	{
		LumenComboBox->OnSelectionChanged.AddUniqueDynamic(this, &UZonefallSettingsMenuWidget::HandleLumenSelectionChanged);
	}
	if (DLSSComboBox)
	{
		DLSSComboBox->OnSelectionChanged.AddUniqueDynamic(this, &UZonefallSettingsMenuWidget::HandleDLSSSelectionChanged);
	}
	if (FrameGenerationComboBox)
	{
		FrameGenerationComboBox->OnSelectionChanged.AddUniqueDynamic(this, &UZonefallSettingsMenuWidget::HandleFrameGenerationSelectionChanged);
	}
	if (FSRComboBox)
	{
		FSRComboBox->OnSelectionChanged.AddUniqueDynamic(this, &UZonefallSettingsMenuWidget::HandleFSRSelectionChanged);
	}
	if (FSRFrameGenerationComboBox)
	{
		FSRFrameGenerationComboBox->OnSelectionChanged.AddUniqueDynamic(this, &UZonefallSettingsMenuWidget::HandleFSRFrameGenerationSelectionChanged);
	}

	if (ApplyButton)
	{
		ApplyButton->OnClicked.AddUniqueDynamic(this, &UZonefallSettingsMenuWidget::HandleApplyClicked);
	}
	if (ResetButton)
	{
		ResetButton->OnClicked.AddUniqueDynamic(this, &UZonefallSettingsMenuWidget::HandleResetClicked);
	}
	if (BackButton)
	{
		BackButton->OnClicked.AddUniqueDynamic(this, &UZonefallSettingsMenuWidget::HandleBackClicked);
	}
}

void UZonefallSettingsMenuWidget::ResolveNamedWidgets()
{
	if (!WidgetTree)
	{
		return;
	}

	auto FindComboByAliases = [this](const TArray<FName>& Aliases) -> UComboBoxString*
	{
		for (const FName& Alias : Aliases)
		{
			if (Alias.IsNone())
			{
				continue;
			}

			if (UComboBoxString* Found = Cast<UComboBoxString>(WidgetTree->FindWidget(Alias)))
			{
				return Found;
			}
		}
		return nullptr;
	};

	if (!DisplayModeComboBox && !DisplayModeComboBoxName.IsNone())
	{
		DisplayModeComboBox = Cast<UComboBoxString>(WidgetTree->FindWidget(DisplayModeComboBoxName));
	}
	if (!GraphicsPresetComboBox && !GraphicsPresetComboBoxName.IsNone())
	{
		GraphicsPresetComboBox = Cast<UComboBoxString>(WidgetTree->FindWidget(GraphicsPresetComboBoxName));
	}
	if (!OverallQualityComboBox && !OverallQualityComboBoxName.IsNone())
	{
		OverallQualityComboBox = Cast<UComboBoxString>(WidgetTree->FindWidget(OverallQualityComboBoxName));
	}
	if (!ResolutionScaleComboBox && !ResolutionScaleComboBoxName.IsNone())
	{
		ResolutionScaleComboBox = Cast<UComboBoxString>(WidgetTree->FindWidget(ResolutionScaleComboBoxName));
	}
	if (!ScreenResolutionComboBox && !ScreenResolutionComboBoxName.IsNone())
	{
		ScreenResolutionComboBox = Cast<UComboBoxString>(WidgetTree->FindWidget(ScreenResolutionComboBoxName));
	}
	if (!VSyncComboBox && !VSyncComboBoxName.IsNone())
	{
		VSyncComboBox = Cast<UComboBoxString>(WidgetTree->FindWidget(VSyncComboBoxName));
	}
	if (!FPSLimitComboBox && !FPSLimitComboBoxName.IsNone())
	{
		FPSLimitComboBox = Cast<UComboBoxString>(WidgetTree->FindWidget(FPSLimitComboBoxName));
	}
	if (!LumenComboBox && !LumenComboBoxName.IsNone())
	{
		LumenComboBox = Cast<UComboBoxString>(WidgetTree->FindWidget(LumenComboBoxName));
	}
	if (!DLSSComboBox && !DLSSComboBoxName.IsNone())
	{
		DLSSComboBox = Cast<UComboBoxString>(WidgetTree->FindWidget(DLSSComboBoxName));
	}
	if (!FrameGenerationComboBox && !FrameGenerationComboBoxName.IsNone())
	{
		FrameGenerationComboBox = Cast<UComboBoxString>(WidgetTree->FindWidget(FrameGenerationComboBoxName));
	}
	if (!FSRComboBox && !FSRComboBoxName.IsNone())
	{
		FSRComboBox = Cast<UComboBoxString>(WidgetTree->FindWidget(FSRComboBoxName));
	}
	if (!FSRFrameGenerationComboBox && !FSRFrameGenerationComboBoxName.IsNone())
	{
		FSRFrameGenerationComboBox = Cast<UComboBoxString>(WidgetTree->FindWidget(FSRFrameGenerationComboBoxName));
	}
	if (!FSRFrameGenerationComboBox)
	{
		// Be tolerant to common naming variants in UMG.
		FSRFrameGenerationComboBox = FindComboByAliases({
			FSRFrameGenerationComboBoxName,
			TEXT("FSRFrameGenerationCombo"),
			TEXT("FSRFrameGeneration"),
			TEXT("FsrFrameGeneration"),
			TEXT("Fsr FrameGeneration"),
			TEXT("Fsr Frame Generation"),
			TEXT("FSR FrameGeneration"),
			TEXT("FSR Frame Generation")
		});
	}

	if (!ApplyButton && !ApplyButtonName.IsNone())
	{
		ApplyButton = ResolveButtonFromWidgetName(WidgetTree, ApplyButtonName);
	}
	if (!ResetButton && !ResetButtonName.IsNone())
	{
		ResetButton = ResolveButtonFromWidgetName(WidgetTree, ResetButtonName);
	}
	if (!BackButton && !BackButtonName.IsNone())
	{
		BackButton = ResolveButtonFromWidgetName(WidgetTree, BackButtonName);
	}

	if (!MemoryUsageProgressBar && !MemoryUsageProgressBarName.IsNone())
	{
		MemoryUsageProgressBar = Cast<UProgressBar>(WidgetTree->FindWidget(MemoryUsageProgressBarName));
	}
	if (!MemoryUsageText && !MemoryUsageTextName.IsNone())
	{
		MemoryUsageText = Cast<UTextBlock>(WidgetTree->FindWidget(MemoryUsageTextName));
	}
	if (!ApplyStatusText && !ApplyStatusTextName.IsNone())
	{
		ApplyStatusText = Cast<UTextBlock>(WidgetTree->FindWidget(ApplyStatusTextName));
	}
}

void UZonefallSettingsMenuWidget::PopulateComboOptionsIfNeeded()
{
	if (DisplayModeComboBox && DisplayModeComboBox->GetOptionCount() == 0)
	{
		DisplayModeComboBox->AddOption(TEXT("Fullscreen"));
		DisplayModeComboBox->AddOption(TEXT("Windowed"));
		DisplayModeComboBox->AddOption(TEXT("Windowed Fullscreen"));
	}
	if (GraphicsPresetComboBox && GraphicsPresetComboBox->GetOptionCount() == 0)
	{
		GraphicsPresetComboBox->AddOption(TEXT("Custom"));
		GraphicsPresetComboBox->AddOption(TEXT("Competitive"));
		GraphicsPresetComboBox->AddOption(TEXT("Balanced"));
		GraphicsPresetComboBox->AddOption(TEXT("Quality"));
	}

	if (OverallQualityComboBox && OverallQualityComboBox->GetOptionCount() == 0)
	{
		OverallQualityComboBox->AddOption(TEXT("Low"));
		OverallQualityComboBox->AddOption(TEXT("Medium"));
		OverallQualityComboBox->AddOption(TEXT("High"));
		OverallQualityComboBox->AddOption(TEXT("Epic"));
		OverallQualityComboBox->AddOption(TEXT("Cinematic"));
	}

	if (ResolutionScaleComboBox && ResolutionScaleComboBox->GetOptionCount() == 0)
	{
		ResolutionScaleComboBox->AddOption(TEXT("50%"));
		ResolutionScaleComboBox->AddOption(TEXT("60%"));
		ResolutionScaleComboBox->AddOption(TEXT("70%"));
		ResolutionScaleComboBox->AddOption(TEXT("80%"));
		ResolutionScaleComboBox->AddOption(TEXT("90%"));
		ResolutionScaleComboBox->AddOption(TEXT("100%"));
	}

	if (VSyncComboBox && VSyncComboBox->GetOptionCount() == 0)
	{
		VSyncComboBox->AddOption(TEXT("Off"));
		VSyncComboBox->AddOption(TEXT("On"));
	}

	if (FPSLimitComboBox && FPSLimitComboBox->GetOptionCount() == 0)
	{
		FPSLimitComboBox->AddOption(TEXT("30"));
		FPSLimitComboBox->AddOption(TEXT("60"));
		FPSLimitComboBox->AddOption(TEXT("120"));
		FPSLimitComboBox->AddOption(TEXT("Unlimited"));
	}

	if (LumenComboBox && LumenComboBox->GetOptionCount() == 0)
	{
		LumenComboBox->AddOption(TEXT("Off"));
		LumenComboBox->AddOption(TEXT("On"));
	}
	if (DLSSComboBox)
	{
		EnsureComboHasOption(DLSSComboBox, TEXT("Off"));
		EnsureComboHasOption(DLSSComboBox, TEXT("Unavailable"));
		EnsureComboHasOption(DLSSComboBox, TEXT("Quality"));
		EnsureComboHasOption(DLSSComboBox, TEXT("Balanced"));
		EnsureComboHasOption(DLSSComboBox, TEXT("Performance"));
		EnsureComboHasOption(DLSSComboBox, TEXT("Ultra Performance"));
		EnsureComboHasOption(DLSSComboBox, TEXT("DLAA"));
	}
	if (FrameGenerationComboBox)
	{
		EnsureComboHasOption(FrameGenerationComboBox, TEXT("Off"));
		EnsureComboHasOption(FrameGenerationComboBox, TEXT("On"));
		EnsureComboHasOption(FrameGenerationComboBox, TEXT("Unavailable"));
	}
	if (FSRComboBox)
	{
		EnsureComboHasOption(FSRComboBox, TEXT("Off"));
		EnsureComboHasOption(FSRComboBox, TEXT("Unavailable"));
		EnsureComboHasOption(FSRComboBox, TEXT("Native AA"));
		EnsureComboHasOption(FSRComboBox, TEXT("Quality"));
		EnsureComboHasOption(FSRComboBox, TEXT("Balanced"));
		EnsureComboHasOption(FSRComboBox, TEXT("Performance"));
		EnsureComboHasOption(FSRComboBox, TEXT("Ultra Performance"));
	}
	if (FSRFrameGenerationComboBox)
	{
		EnsureComboHasOption(FSRFrameGenerationComboBox, TEXT("Off"));
		EnsureComboHasOption(FSRFrameGenerationComboBox, TEXT("On"));
		EnsureComboHasOption(FSRFrameGenerationComboBox, TEXT("Unavailable"));
	}
}

void UZonefallSettingsMenuWidget::PopulateScreenResolutionsIfNeeded()
{
	if (!SettingsObject || !ScreenResolutionComboBox)
	{
		return;
	}

	TArray<FString> AvailableResolutions;
	SettingsObject->GetAvailableScreenResolutions(AvailableResolutions, false);
	for (const FString& Resolution : AvailableResolutions)
	{
		EnsureComboHasOption(ScreenResolutionComboBox, Resolution);
	}

	// Always keep current runtime resolution selectable even if it is not in detected fullscreen list.
	const FString CurrentScreenResolution = SettingsObject->GetCurrentScreenResolutionString();
	EnsureComboHasOption(ScreenResolutionComboBox, CurrentScreenResolution);
}

void UZonefallSettingsMenuWidget::ApplyButtonStyle(UButton* Button) const
{
	if (!Button)
	{
		return;
	}

	FButtonStyle Style = Button->GetStyle();
	Style.SetNormal(FSlateRoundedBoxBrush(FLinearColor(0.07f, 0.12f, 0.20f, 0.97f), 10.0f));
	Style.SetHovered(FSlateRoundedBoxBrush(FLinearColor(0.12f, 0.24f, 0.40f, 1.0f), 10.0f));
	Style.SetPressed(FSlateRoundedBoxBrush(FLinearColor(0.05f, 0.09f, 0.16f, 1.0f), 10.0f));
	Style.SetDisabled(FSlateRoundedBoxBrush(FLinearColor(0.05f, 0.06f, 0.08f, 0.75f), 10.0f));
	Style.NormalPadding = FMargin(14.0f, 10.0f);
	Style.PressedPadding = FMargin(14.0f, 12.0f, 14.0f, 8.0f);
	Button->SetStyle(Style);
}

void UZonefallSettingsMenuWidget::ApplyComboBoxStyle(UComboBoxString* ComboBox) const
{
	if (!ComboBox)
	{
		return;
	}

	FComboBoxStyle ComboStyle = ComboBox->GetWidgetStyle();
	FButtonStyle ComboButtonStyle = ComboStyle.ComboButtonStyle.ButtonStyle;
	ComboButtonStyle.SetNormal(FSlateRoundedBoxBrush(FLinearColor(0.05f, 0.10f, 0.18f, 0.98f), 9.0f));
	ComboButtonStyle.SetHovered(FSlateRoundedBoxBrush(FLinearColor(0.10f, 0.20f, 0.33f, 1.0f), 9.0f));
	ComboButtonStyle.SetPressed(FSlateRoundedBoxBrush(FLinearColor(0.04f, 0.08f, 0.14f, 1.0f), 9.0f));
	ComboButtonStyle.SetDisabled(FSlateRoundedBoxBrush(FLinearColor(0.04f, 0.05f, 0.08f, 0.72f), 9.0f));
	ComboButtonStyle.NormalPadding = FMargin(14.0f, 8.0f);
	ComboButtonStyle.PressedPadding = FMargin(14.0f, 9.0f, 14.0f, 7.0f);
	ComboStyle.ComboButtonStyle.SetButtonStyle(ComboButtonStyle);
	ComboStyle.ComboButtonStyle.SetContentPadding(FMargin(10.0f, 6.0f));
	ComboStyle.ComboButtonStyle.SetMenuBorderBrush(FSlateRoundedBoxBrush(FLinearColor(0.03f, 0.06f, 0.10f, 0.98f), 8.0f));
	ComboStyle.ComboButtonStyle.SetMenuBorderPadding(FMargin(6.0f));
	ComboBox->SetWidgetStyle(ComboStyle);

	FTableRowStyle RowStyle = ComboBox->GetItemStyle();
	RowStyle.SetActiveBrush(FSlateRoundedBoxBrush(FLinearColor(0.12f, 0.24f, 0.40f, 1.0f), 6.0f));
	RowStyle.SetActiveHoveredBrush(FSlateRoundedBoxBrush(FLinearColor(0.16f, 0.30f, 0.48f, 1.0f), 6.0f));
	RowStyle.SetInactiveBrush(FSlateRoundedBoxBrush(FLinearColor(0.05f, 0.09f, 0.14f, 1.0f), 6.0f));
	RowStyle.SetInactiveHoveredBrush(FSlateRoundedBoxBrush(FLinearColor(0.10f, 0.18f, 0.28f, 1.0f), 6.0f));
	RowStyle.SetEvenRowBackgroundBrush(FSlateRoundedBoxBrush(FLinearColor(0.03f, 0.06f, 0.10f, 0.98f), 4.0f));
	RowStyle.SetEvenRowBackgroundHoveredBrush(FSlateRoundedBoxBrush(FLinearColor(0.09f, 0.17f, 0.26f, 1.0f), 4.0f));
	RowStyle.SetOddRowBackgroundBrush(FSlateRoundedBoxBrush(FLinearColor(0.04f, 0.07f, 0.11f, 0.98f), 4.0f));
	RowStyle.SetOddRowBackgroundHoveredBrush(FSlateRoundedBoxBrush(FLinearColor(0.10f, 0.19f, 0.29f, 1.0f), 4.0f));
	ComboBox->SetItemStyle(RowStyle);

}

void UZonefallSettingsMenuWidget::ApplyLabelStyle(UTextBlock* LabelTextBlock, int32 FontSize) const
{
	if (!LabelTextBlock)
	{
		return;
	}

	FSlateFontInfo Font = LabelTextBlock->GetFont();
	Font.Size = FontSize;
	LabelTextBlock->SetFont(Font);
	LabelTextBlock->SetColorAndOpacity(FSlateColor(FLinearColor(0.88f, 0.94f, 1.0f, 1.0f)));
	LabelTextBlock->SetShadowOffset(FVector2D(0.0f, 1.0f));
	LabelTextBlock->SetShadowColorAndOpacity(FLinearColor(0.0f, 0.0f, 0.0f, 0.75f));
}

void UZonefallSettingsMenuWidget::ApplyModernVisualTheme()
{
	if (RootBorder)
	{
		RootBorder->SetBrush(FSlateRoundedBoxBrush(FLinearColor(0.01f, 0.02f, 0.04f, 0.95f), 14.0f));
		RootBorder->SetPadding(FMargin(20.0f));
	}

	if (TitleText)
	{
		FSlateFontInfo TitleFont = TitleText->GetFont();
		TitleFont.Size = 44;
		TitleText->SetFont(TitleFont);
		TitleText->SetColorAndOpacity(FSlateColor(FLinearColor(0.95f, 0.98f, 1.0f, 1.0f)));
		TitleText->SetShadowOffset(FVector2D(0.0f, 1.0f));
		TitleText->SetShadowColorAndOpacity(FLinearColor(0.0f, 0.0f, 0.0f, 0.82f));
	}

	const TArray<UComboBoxString*> AllCombos = {
		DisplayModeComboBox,
		GraphicsPresetComboBox,
		OverallQualityComboBox,
		ResolutionScaleComboBox,
		ScreenResolutionComboBox,
		VSyncComboBox,
		FPSLimitComboBox,
		LumenComboBox,
		DLSSComboBox,
		FrameGenerationComboBox,
		FSRComboBox,
		FSRFrameGenerationComboBox
	};
	for (UComboBoxString* ComboBox : AllCombos)
	{
		ApplyComboBoxStyle(ComboBox);
	}

	const TArray<UTextBlock*> AllLabels = {
		DisplayModeText,
		GraphicsPresetText,
		OverallQualityText,
		ResolutionScaleText,
		ScreenResolutionText,
		VSyncText,
		FPSLimitText,
		LumenText,
		DLSSText,
		FrameGenerationText,
		FSRText,
		FSRFrameGenerationText
	};
	for (UTextBlock* LabelTextBlock : AllLabels)
	{
		ApplyLabelStyle(LabelTextBlock, 20);
	}

	ApplyLabelStyle(MemoryUsageText, 20);
	ApplyLabelStyle(ApplyStatusText, 18);
	if (ApplyStatusText)
	{
		ApplyStatusText->SetColorAndOpacity(FSlateColor(FLinearColor(0.72f, 0.84f, 0.98f, 1.0f)));
	}

	if (MemoryUsageProgressBar)
	{
		FProgressBarStyle ProgressStyle = MemoryUsageProgressBar->GetWidgetStyle();
		ProgressStyle.SetBackgroundImage(FSlateRoundedBoxBrush(FLinearColor(0.03f, 0.06f, 0.09f, 0.96f), 7.0f));
		ProgressStyle.SetFillImage(FSlateRoundedBoxBrush(FLinearColor(0.11f, 0.76f, 0.98f, 1.0f), 7.0f));
		ProgressStyle.SetMarqueeImage(FSlateRoundedBoxBrush(FLinearColor(0.86f, 0.95f, 1.0f, 1.0f), 7.0f));
		MemoryUsageProgressBar->SetWidgetStyle(ProgressStyle);
	}

	ApplyButtonStyle(ApplyButton);
	ApplyButtonStyle(ResetButton);
	ApplyButtonStyle(BackButton);
}

UComboBoxString* UZonefallSettingsMenuWidget::CreateOptionCombo(const FName Name, TObjectPtr<UTextBlock>& OutTextBlock)
{
	// AAA-ish row: label + control on the same line in a rounded panel.
	UBorder* RowPanel = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass());
	RowPanel->SetBrush(FSlateRoundedBoxBrush(FLinearColor(0.02f, 0.04f, 0.07f, 0.65f), 12.0f));
	RowPanel->SetPadding(FMargin(12.0f, 10.0f));

	UHorizontalBox* Row = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
	RowPanel->SetContent(Row);

	OutTextBlock = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	ApplyLabelStyle(OutTextBlock, 19);

	USizeBox* LabelSize = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
	LabelSize->SetWidthOverride(260.0f);
	LabelSize->SetContent(OutTextBlock);

	if (UHorizontalBoxSlot* LabelSlot = Row->AddChildToHorizontalBox(LabelSize))
	{
		LabelSlot->SetPadding(FMargin(0, 0, 12, 0));
		LabelSlot->SetHorizontalAlignment(HAlign_Left);
		LabelSlot->SetVerticalAlignment(VAlign_Center);
	}

	USpacer* Spacer = WidgetTree->ConstructWidget<USpacer>(USpacer::StaticClass());
	if (UHorizontalBoxSlot* SpacerSlot = Row->AddChildToHorizontalBox(Spacer))
	{
		SpacerSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	}

	UComboBoxString* Combo = WidgetTree->ConstructWidget<UComboBoxString>(UComboBoxString::StaticClass(), Name);
	ApplyComboBoxStyle(Combo);
	USizeBox* ComboSize = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
	ComboSize->SetMinDesiredWidth(340.0f);
	ComboSize->SetContent(Combo);
	if (UHorizontalBoxSlot* ComboSlot = Row->AddChildToHorizontalBox(ComboSize))
	{
		ComboSlot->SetHorizontalAlignment(HAlign_Fill);
		ComboSlot->SetVerticalAlignment(VAlign_Center);
	}

	if (UVerticalBoxSlot* RowSlot = RootBox->AddChildToVerticalBox(RowPanel))
	{
		RowSlot->SetPadding(FMargin(0, 4, 0, 6));
	}

	return Combo;
}

void UZonefallSettingsMenuWidget::EnsureComboHasOption(UComboBoxString* ComboBox, const FString& Option) const
{
	if (!ComboBox || Option.IsEmpty())
	{
		return;
	}

	if (ComboBox->FindOptionIndex(Option) == INDEX_NONE)
	{
		ComboBox->AddOption(Option);
	}
}

void UZonefallSettingsMenuWidget::UpdateFeatureAvailabilityUI()
{
	if (!SettingsObject)
	{
		return;
	}

	if (FrameGenerationComboBox)
	{
		if (!SettingsObject->bFrameGenerationSupported)
		{
			SettingsObject->FrameGeneration = TEXT("Unavailable");
			EnsureComboHasOption(FrameGenerationComboBox, TEXT("Unavailable"));
			FrameGenerationComboBox->SetSelectedOption(TEXT("Unavailable"));
			FrameGenerationComboBox->SetIsEnabled(false);
		}
		else
		{
			if (SettingsObject->FrameGeneration == TEXT("Unavailable"))
			{
				SettingsObject->FrameGeneration = TEXT("Off");
			}
			FrameGenerationComboBox->SetIsEnabled(true);
			FrameGenerationComboBox->SetSelectedOption(SettingsObject->FrameGeneration.IsEmpty() ? TEXT("Off") : SettingsObject->FrameGeneration);
		}
	}
}

float UZonefallSettingsMenuWidget::CalculateEstimatedMemoryUsageMB() const
{
	if (!SettingsObject)
	{
		return 0.0f;
	}

	float EstimatedMB = 1200.0f;

	if (SettingsObject->OverallQuality == TEXT("Low")) { EstimatedMB += 300.0f; }
	else if (SettingsObject->OverallQuality == TEXT("Medium")) { EstimatedMB += 550.0f; }
	else if (SettingsObject->OverallQuality == TEXT("High")) { EstimatedMB += 900.0f; }
	else if (SettingsObject->OverallQuality == TEXT("Epic")) { EstimatedMB += 1300.0f; }
	else { EstimatedMB += 1700.0f; }

	const FString ScaleNumeric = SettingsObject->ResolutionScale.Replace(TEXT("%"), TEXT(""));
	const float ResolutionScale = FMath::Clamp(FCString::Atof(*ScaleNumeric), 50.0f, 100.0f);
	EstimatedMB += (ResolutionScale - 50.0f) * 22.0f;

	if (SettingsObject->Lumen == TEXT("On"))
	{
		EstimatedMB += 650.0f;
	}
	if (SettingsObject->DLSSMode == TEXT("Quality")) { EstimatedMB -= 120.0f; }
	else if (SettingsObject->DLSSMode == TEXT("Balanced")) { EstimatedMB -= 220.0f; }
	else if (SettingsObject->DLSSMode == TEXT("Performance")) { EstimatedMB -= 320.0f; }
	else if (SettingsObject->DLSSMode == TEXT("Ultra Performance")) { EstimatedMB -= 420.0f; }
	else if (SettingsObject->DLSSMode == TEXT("DLAA")) { EstimatedMB += 90.0f; }

	if (SettingsObject->FrameGeneration == TEXT("On"))
	{
		EstimatedMB += 180.0f;
	}
	if (SettingsObject->FSRMode == TEXT("Quality")) { EstimatedMB -= 100.0f; }
	else if (SettingsObject->FSRMode == TEXT("Balanced")) { EstimatedMB -= 200.0f; }
	else if (SettingsObject->FSRMode == TEXT("Performance")) { EstimatedMB -= 300.0f; }
	else if (SettingsObject->FSRMode == TEXT("Ultra Performance")) { EstimatedMB -= 380.0f; }
	else if (SettingsObject->FSRMode == TEXT("Native AA")) { EstimatedMB += 60.0f; }

	if (SettingsObject->FSRFrameGeneration == TEXT("On"))
	{
		EstimatedMB += 140.0f;
	}

	if (SettingsObject->DisplayMode == TEXT("Fullscreen"))
	{
		EstimatedMB += 120.0f;
	}

	return EstimatedMB;
}

void UZonefallSettingsMenuWidget::UpdatePerformanceEstimateUI()
{
	const float EstimatedMB = CalculateEstimatedMemoryUsageMB();
	const float MaxReferenceMB = 6000.0f;
	const float Normalized = FMath::Clamp(EstimatedMB / MaxReferenceMB, 0.0f, 1.0f);

	if (MemoryUsageProgressBar)
	{
		MemoryUsageProgressBar->SetVisibility(ESlateVisibility::Visible);
		MemoryUsageProgressBar->SetPercent(Normalized);
		if (Normalized > 0.8f)
		{
			MemoryUsageProgressBar->SetFillColorAndOpacity(FLinearColor(0.86f, 0.20f, 0.20f, 1.0f));
		}
		else if (Normalized > 0.55f)
		{
			MemoryUsageProgressBar->SetFillColorAndOpacity(FLinearColor(0.92f, 0.66f, 0.18f, 1.0f));
		}
		else
		{
			MemoryUsageProgressBar->SetFillColorAndOpacity(FLinearColor(0.20f, 0.75f, 0.35f, 1.0f));
		}
	}

	if (MemoryUsageText)
	{
		MemoryUsageText->SetText(FText::FromString(FString::Printf(TEXT("Memory: %.0f MB"), EstimatedMB)));
	}
}

void UZonefallSettingsMenuWidget::UpdateTexts()
{
	if (GraphicsPresetText) { GraphicsPresetText->SetText(FText::FromString(TEXT("Graphics Preset"))); }
	if (DisplayModeText) { DisplayModeText->SetText(FText::FromString(TEXT("Display Mode"))); }
	if (OverallQualityText) { OverallQualityText->SetText(FText::FromString(TEXT("Overall Quality"))); }
	if (ResolutionScaleText) { ResolutionScaleText->SetText(FText::FromString(TEXT("Resolution Scale"))); }
	if (ScreenResolutionText) { ScreenResolutionText->SetText(FText::FromString(TEXT("Screen Resolution"))); }
	if (VSyncText) { VSyncText->SetText(FText::FromString(TEXT("VSync"))); }
	if (FPSLimitText) { FPSLimitText->SetText(FText::FromString(TEXT("FPS Limit"))); }
	if (LumenText) { LumenText->SetText(FText::FromString(TEXT("Lumen"))); }
	if (DLSSText) { DLSSText->SetText(FText::FromString(TEXT("Zonefall DLSS"))); }
	if (FrameGenerationText) { FrameGenerationText->SetText(FText::FromString(TEXT("Zonefall DLSS Frame Gen"))); }
	if (FSRText) { FSRText->SetText(FText::FromString(TEXT("Zonefall FSR"))); }
	if (FSRFrameGenerationText) { FSRFrameGenerationText->SetText(FText::FromString(TEXT("Zonefall FSR Frame Gen"))); }
	if (MemoryUsageText && MemoryUsageText->GetText().IsEmpty())
	{
		MemoryUsageText->SetText(FText::FromString(TEXT("Estimated Memory Usage")));
	}
}

void UZonefallSettingsMenuWidget::MarkSettingsDirty()
{
	if (bIsRefreshingFromSettings)
	{
		return;
	}

	bHasPendingChanges = true;
	UpdateApplyButtonState();
	SetApplyStatusMessage(FText::FromString(TEXT("Pending changes")), FLinearColor(0.95f, 0.80f, 0.30f, 1.0f));
}

void UZonefallSettingsMenuWidget::SetApplyStatusMessage(const FText& InMessage, const FLinearColor& InColor)
{
	if (!ApplyStatusText)
	{
		return;
	}

	ApplyStatusText->SetText(InMessage);
	ApplyStatusText->SetColorAndOpacity(FSlateColor(InColor));
}

void UZonefallSettingsMenuWidget::UpdateApplyButtonState()
{
	if (ApplyButton)
	{
		ApplyButton->SetIsEnabled(bHasPendingChanges);
	}
}

void UZonefallSettingsMenuWidget::HandleDisplayModeSelectionChanged(FString SelectedItem, ESelectInfo::Type SelectionType)
{
	if (SettingsObject)
	{
		SettingsObject->DisplayMode = SelectedItem;
		UpdatePerformanceEstimateUI();
		if (bAutoApplyDisplayModeAndResolution)
		{
			SettingsObject->ApplyDisplayModeAndResolution(false);
		}
		MarkSettingsDirty();
	}
}

void UZonefallSettingsMenuWidget::HandleGraphicsPresetSelectionChanged(FString SelectedItem, ESelectInfo::Type SelectionType)
{
	if (!SettingsObject || SelectedItem == TEXT("Custom"))
	{
		return;
	}

	EZonefallGraphicsPreset Preset = EZonefallGraphicsPreset::Balanced;
	if (SelectedItem == TEXT("Competitive"))
	{
		Preset = EZonefallGraphicsPreset::Competitive;
	}
	else if (SelectedItem == TEXT("Quality"))
	{
		Preset = EZonefallGraphicsPreset::Quality;
	}

	SettingsObject->ApplyGraphicsPreset(Preset);

	bIsRefreshingFromSettings = true;
	if (DisplayModeComboBox) { DisplayModeComboBox->SetSelectedOption(SettingsObject->DisplayMode); }
	if (OverallQualityComboBox) { OverallQualityComboBox->SetSelectedOption(SettingsObject->OverallQuality); }
	if (ResolutionScaleComboBox) { ResolutionScaleComboBox->SetSelectedOption(SettingsObject->ResolutionScale); }
	if (ScreenResolutionComboBox) { EnsureComboHasOption(ScreenResolutionComboBox, SettingsObject->ScreenResolution); ScreenResolutionComboBox->SetSelectedOption(SettingsObject->ScreenResolution); }
	if (VSyncComboBox) { VSyncComboBox->SetSelectedOption(SettingsObject->VSync); }
	if (FPSLimitComboBox) { EnsureComboHasOption(FPSLimitComboBox, SettingsObject->FPSLimit); FPSLimitComboBox->SetSelectedOption(SettingsObject->FPSLimit); }
	if (LumenComboBox) { LumenComboBox->SetSelectedOption(SettingsObject->Lumen); }
	if (DLSSComboBox) { EnsureComboHasOption(DLSSComboBox, SettingsObject->DLSSMode); DLSSComboBox->SetSelectedOption(SettingsObject->DLSSMode); }
	if (FrameGenerationComboBox) { EnsureComboHasOption(FrameGenerationComboBox, SettingsObject->FrameGeneration); FrameGenerationComboBox->SetSelectedOption(SettingsObject->FrameGeneration); }
	if (FSRComboBox) { EnsureComboHasOption(FSRComboBox, SettingsObject->FSRMode); FSRComboBox->SetSelectedOption(SettingsObject->FSRMode); }
	if (FSRFrameGenerationComboBox) { EnsureComboHasOption(FSRFrameGenerationComboBox, SettingsObject->FSRFrameGeneration); FSRFrameGenerationComboBox->SetSelectedOption(SettingsObject->FSRFrameGeneration); }
	bIsRefreshingFromSettings = false;

	UpdateFeatureAvailabilityUI();
	UpdatePerformanceEstimateUI();
	MarkSettingsDirty();
	SetApplyStatusMessage(FText::FromString(FString::Printf(TEXT("Preset selected: %s (press APPLY)"), *SelectedItem)), FLinearColor(0.75f, 0.86f, 1.0f, 1.0f));
}

void UZonefallSettingsMenuWidget::HandleOverallQualitySelectionChanged(FString SelectedItem, ESelectInfo::Type SelectionType)
{
	if (SettingsObject)
	{
		SettingsObject->OverallQuality = SelectedItem;
		UpdatePerformanceEstimateUI();
		MarkSettingsDirty();
	}
}

void UZonefallSettingsMenuWidget::HandleResolutionScaleSelectionChanged(FString SelectedItem, ESelectInfo::Type SelectionType)
{
	if (SettingsObject)
	{
		SettingsObject->ResolutionScale = SelectedItem;
		UpdatePerformanceEstimateUI();
		MarkSettingsDirty();
	}
}

void UZonefallSettingsMenuWidget::HandleVSyncSelectionChanged(FString SelectedItem, ESelectInfo::Type SelectionType)
{
	if (SettingsObject)
	{
		SettingsObject->VSync = SelectedItem;
		MarkSettingsDirty();
	}
}

void UZonefallSettingsMenuWidget::HandleScreenResolutionSelectionChanged(FString SelectedItem, ESelectInfo::Type SelectionType)
{
	if (SettingsObject)
	{
		SettingsObject->ScreenResolution = SelectedItem;
		UpdatePerformanceEstimateUI();
		if (bAutoApplyDisplayModeAndResolution)
		{
			SettingsObject->ApplyDisplayModeAndResolution(false);
		}
		MarkSettingsDirty();
	}
}

void UZonefallSettingsMenuWidget::HandleFPSLimitSelectionChanged(FString SelectedItem, ESelectInfo::Type SelectionType)
{
	if (!SettingsObject || bIsRefreshingFromSettings)
	{
		return;
	}

	const FString CleanSelected = SelectedItem.TrimStartAndEnd();
	if (CleanSelected.IsEmpty())
	{
		return;
	}

	if (SettingsObject)
	{
		SettingsObject->FPSLimit = CleanSelected;
		SettingsObject->SanitizeSettings();
		EnsureComboHasOption(FPSLimitComboBox, SettingsObject->FPSLimit);
		if (FPSLimitComboBox)
		{
			FPSLimitComboBox->SetSelectedOption(SettingsObject->FPSLimit);
		}
		MarkSettingsDirty();
	}
}

void UZonefallSettingsMenuWidget::HandleLumenSelectionChanged(FString SelectedItem, ESelectInfo::Type SelectionType)
{
	if (SettingsObject)
	{
		SettingsObject->Lumen = SelectedItem;
		UpdatePerformanceEstimateUI();
		MarkSettingsDirty();
	}
}

void UZonefallSettingsMenuWidget::HandleDLSSSelectionChanged(FString SelectedItem, ESelectInfo::Type SelectionType)
{
	if (SettingsObject)
	{
		SettingsObject->DLSSMode = SelectedItem;
		const bool bEnableDLSS = (SelectedItem != TEXT("Off") && SelectedItem != TEXT("Unavailable"));
		if (bEnableDLSS)
		{
			// Selecting DLSS should immediately disable FSR upscaler mode.
			SettingsObject->FSRMode = TEXT("Off");
		}
		SettingsObject->SanitizeSettings();
		if (DLSSComboBox) { DLSSComboBox->SetSelectedOption(SettingsObject->DLSSMode); }
		if (FSRComboBox) { FSRComboBox->SetSelectedOption(SettingsObject->FSRMode); }
		if (FrameGenerationComboBox) { FrameGenerationComboBox->SetSelectedOption(SettingsObject->FrameGeneration); }
		if (FSRFrameGenerationComboBox) { FSRFrameGenerationComboBox->SetSelectedOption(SettingsObject->FSRFrameGeneration); }
		UpdatePerformanceEstimateUI();
		MarkSettingsDirty();
	}
}

void UZonefallSettingsMenuWidget::HandleFrameGenerationSelectionChanged(FString SelectedItem, ESelectInfo::Type SelectionType)
{
	if (SettingsObject)
	{
		if (!SettingsObject->bFrameGenerationSupported)
		{
			SettingsObject->FrameGeneration = TEXT("Unavailable");
			if (FrameGenerationComboBox)
			{
				FrameGenerationComboBox->SetSelectedOption(TEXT("Unavailable"));
				FrameGenerationComboBox->SetIsEnabled(false);
			}
			return;
		}

		SettingsObject->FrameGeneration = SelectedItem;
		SettingsObject->SanitizeSettings();
		if (FrameGenerationComboBox) { FrameGenerationComboBox->SetSelectedOption(SettingsObject->FrameGeneration); }
		if (FSRFrameGenerationComboBox) { FSRFrameGenerationComboBox->SetSelectedOption(SettingsObject->FSRFrameGeneration); }
		UpdatePerformanceEstimateUI();
		MarkSettingsDirty();
	}
}

void UZonefallSettingsMenuWidget::HandleFSRSelectionChanged(FString SelectedItem, ESelectInfo::Type SelectionType)
{
	if (SettingsObject)
	{
		SettingsObject->FSRMode = SelectedItem;
		const bool bEnableFSR = (SelectedItem != TEXT("Off") && SelectedItem != TEXT("Unavailable"));
		if (bEnableFSR)
		{
			// Selecting FSR should immediately disable DLSS upscaler mode.
			SettingsObject->DLSSMode = TEXT("Off");
		}
		SettingsObject->SanitizeSettings();
		if (DLSSComboBox) { DLSSComboBox->SetSelectedOption(SettingsObject->DLSSMode); }
		if (FSRComboBox) { FSRComboBox->SetSelectedOption(SettingsObject->FSRMode); }
		if (FrameGenerationComboBox) { FrameGenerationComboBox->SetSelectedOption(SettingsObject->FrameGeneration); }
		if (FSRFrameGenerationComboBox) { FSRFrameGenerationComboBox->SetSelectedOption(SettingsObject->FSRFrameGeneration); }
		UpdatePerformanceEstimateUI();
		MarkSettingsDirty();
	}
}

void UZonefallSettingsMenuWidget::HandleFSRFrameGenerationSelectionChanged(FString SelectedItem, ESelectInfo::Type SelectionType)
{
	if (SettingsObject)
	{
		SettingsObject->FSRFrameGeneration = SelectedItem;
		SettingsObject->SanitizeSettings();
		if (FSRFrameGenerationComboBox) { FSRFrameGenerationComboBox->SetSelectedOption(SettingsObject->FSRFrameGeneration); }
		if (FrameGenerationComboBox) { FrameGenerationComboBox->SetSelectedOption(SettingsObject->FrameGeneration); }
		UpdatePerformanceEstimateUI();
		MarkSettingsDirty();
	}
}

void UZonefallSettingsMenuWidget::HandleApplyClicked()
{
	ApplySettingsNow();
}

void UZonefallSettingsMenuWidget::HandleResetClicked()
{
	ResetToDefaults();
}

void UZonefallSettingsMenuWidget::HandleBackClicked()
{
	const UWorld* World = GetWorld();
	UE_LOG(
		LogTemp,
		Log,
		TEXT("[PauseFlow] Settings Back clicked. Bound=%d Paused=%d"),
		OnBackRequested.IsBound() ? 1 : 0,
		(World && World->IsPaused()) ? 1 : 0
	);

	if (bHasPendingChanges)
	{
		// Back should return without forcing apply: restore last saved runtime settings.
		RefreshFromSettings();
		SetApplyStatusMessage(FText::FromString(TEXT("Changes discarded")), FLinearColor(0.78f, 0.84f, 0.92f, 1.0f));
	}

	OnBackRequested.Broadcast();

	// Fallback path for cases when Back isn't wired in BP.
	if (!OnBackRequested.IsBound())
	{
		if (UWorld* CurrentWorld = GetWorld())
		{
			if (UUIWorldMenuGameInstance* MenuGameInstance = Cast<UUIWorldMenuGameInstance>(CurrentWorld->GetGameInstance()))
			{
				UE_LOG(LogTemp, Log, TEXT("[MenuFlow] Settings Back fallback -> BackFromSettingsMenuSmart"));
				MenuGameInstance->BackFromSettingsMenuSmart(false);
			}
		}
	}
}

