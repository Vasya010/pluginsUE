#include "ZonefallModernComboBoxWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Brushes/SlateRoundedBoxBrush.h"
#include "Components/Border.h"
#include "Components/ComboBoxString.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Kismet/KismetSystemLibrary.h"

UZonefallModernComboBoxWidget::UZonefallModernComboBoxWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
	, Label(NSLOCTEXT("ZonefallUI", "ModernComboLabel", "GRAPHICS PRESET"))
	, PlaceholderText(NSLOCTEXT("ZonefallUI", "ModernComboPlaceholder", "Select option"))
	, LabelFontSize(18)
	, ValueFontSize(16)
	, ThemePreset(EZonefallModernComboTheme::Dark)
	, PanelColor(FLinearColor(0.06f, 0.10f, 0.18f, 0.92f))
	, LabelColor(FLinearColor(0.86f, 0.92f, 1.0f, 1.0f))
	, ValueColor(FLinearColor(0.95f, 0.97f, 1.0f, 1.0f))
	, bAutoPopulateFromPreset(false)
	, AutoPreset(EZonefallModernComboPreset::Manual)
	, TargetComboWidgetName(TEXT("ComboBox"))
	, TargetLabelWidgetName(TEXT("LabelText"))
{
	Options = {
		TEXT("Low"),
		TEXT("Medium"),
		TEXT("High"),
		TEXT("Epic")
	};
}

void UZonefallModernComboBoxWidget::NativePreConstruct()
{
	Super::NativePreConstruct();

	ValidateAndBindWidgets();
	ApplyVisualStyle();
	RebuildOptions(true);
}

void UZonefallModernComboBoxWidget::NativeConstruct()
{
	Super::NativeConstruct();

	ValidateAndBindWidgets();
	ApplyVisualStyle();
	RebuildOptions(true);
}

void UZonefallModernComboBoxWidget::SynchronizeProperties()
{
	Super::SynchronizeProperties();

	ValidateAndBindWidgets();
	ApplyVisualStyle();
	RebuildOptions(false);
}

void UZonefallModernComboBoxWidget::SetOptions(const TArray<FString>& InOptions, bool bSelectFirst)
{
	bAutoPopulateFromPreset = false;
	AutoPreset = EZonefallModernComboPreset::Manual;
	Options = InOptions;
	RebuildOptions(bSelectFirst);
}

void UZonefallModernComboBoxWidget::AddOption(const FString& InOption)
{
	const FString Trimmed = InOption.TrimStartAndEnd();
	if (Trimmed.IsEmpty())
	{
		return;
	}

	Options.AddUnique(Trimmed);
	if (ComboBox && ComboBox->FindOptionIndex(Trimmed) == INDEX_NONE)
	{
		ComboBox->AddOption(Trimmed);
	}
}

void UZonefallModernComboBoxWidget::ClearOptions()
{
	Options.Empty();
	if (ComboBox)
	{
		ComboBox->ClearOptions();
		ComboBox->SetSelectedOption(FString());
	}
}

void UZonefallModernComboBoxWidget::SetSelectedOption(const FString& InOption)
{
	const FString Trimmed = InOption.TrimStartAndEnd();
	if (Trimmed.IsEmpty() || !ComboBox)
	{
		return;
	}

	if (ComboBox->FindOptionIndex(Trimmed) == INDEX_NONE)
	{
		ComboBox->AddOption(Trimmed);
		Options.AddUnique(Trimmed);
	}
	ComboBox->SetSelectedOption(Trimmed);
}

FString UZonefallModernComboBoxWidget::GetSelectedOption() const
{
	return ComboBox ? ComboBox->GetSelectedOption() : FString();
}

void UZonefallModernComboBoxWidget::ApplyVisualStyle()
{
	SyncThemeColors();

	if (RootBorder)
	{
		RootBorder->SetBrush(FSlateRoundedBoxBrush(PanelColor, 12.0f));
		RootBorder->SetPadding(FMargin(10.0f));
	}

	if (LabelTextBlock)
	{
		FSlateFontInfo EffectiveFont = LabelFont;
		EffectiveFont.Size = LabelFontSize;
		LabelTextBlock->SetFont(EffectiveFont);
		LabelTextBlock->SetText(Label);
		LabelTextBlock->SetColorAndOpacity(FSlateColor(LabelColor));
		LabelTextBlock->SetShadowOffset(FVector2D(0.0f, 1.0f));
		LabelTextBlock->SetShadowColorAndOpacity(FLinearColor(0.0f, 0.0f, 0.0f, 0.6f));
	}

	if (ComboBox)
	{
		ComboBox->SetToolTipText(PlaceholderText);
	}
}

bool UZonefallModernComboBoxWidget::ValidateAndBindWidgets()
{
	TryBindFromWidgetTree();
	BuildWidgetTree();
	BindComboEvents();
	return ComboBox != nullptr;
}

void UZonefallModernComboBoxWidget::TryBindFromWidgetTree()
{
	if (!WidgetTree)
	{
		return;
	}

	auto FindComboByName = [this](const FName NameToFind) -> UComboBoxString*
	{
		if (NameToFind.IsNone())
		{
			return nullptr;
		}
		return Cast<UComboBoxString>(WidgetTree->FindWidget(NameToFind));
	};

	auto FindTextByName = [this](const FName NameToFind) -> UTextBlock*
	{
		if (NameToFind.IsNone())
		{
			return nullptr;
		}
		return Cast<UTextBlock>(WidgetTree->FindWidget(NameToFind));
	};

	if (!ComboBox)
	{
		ComboBox = FindComboByName(TargetComboWidgetName);
		if (!ComboBox)
		{
			ComboBox = FindComboByName(TEXT("ComboBox"));
		}
		if (!ComboBox)
		{
			ComboBox = FindComboByName(TEXT("ComboBoxString"));
		}
	}

	if (!LabelTextBlock)
	{
		LabelTextBlock = FindTextByName(TargetLabelWidgetName);
		if (!LabelTextBlock)
		{
			LabelTextBlock = FindTextByName(TEXT("LabelText"));
		}
		if (!LabelTextBlock)
		{
			LabelTextBlock = FindTextByName(TEXT("TitleText"));
		}
	}
}

void UZonefallModernComboBoxWidget::BuildWidgetTree()
{
	if (!WidgetTree)
	{
		return;
	}

	if (!RootBorder)
	{
		RootBorder = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("ModernComboRoot"));
		if (!WidgetTree->RootWidget)
		{
			WidgetTree->RootWidget = RootBorder;
		}
	}

	if (!RootBox)
	{
		RootBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("ModernComboRootBox"));
		if (RootBorder)
		{
			RootBorder->SetContent(RootBox);
		}
	}

	if (!LabelTextBlock)
	{
		LabelTextBlock = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("ModernComboLabel"));
		if (RootBox)
		{
			if (UVerticalBoxSlot* LabelSlot = RootBox->AddChildToVerticalBox(LabelTextBlock))
			{
				LabelSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 6.0f));
			}
		}
	}

	if (!ComboBox)
	{
		ComboBox = WidgetTree->ConstructWidget<UComboBoxString>(UComboBoxString::StaticClass(), TEXT("ModernComboBox"));
		if (RootBox)
		{
			RootBox->AddChildToVerticalBox(ComboBox);
		}
	}
}

void UZonefallModernComboBoxWidget::SyncThemeColors()
{
	if (ThemePreset == EZonefallModernComboTheme::Neon)
	{
		PanelColor = FLinearColor(0.02f, 0.16f, 0.23f, 0.96f);
		LabelColor = FLinearColor(0.63f, 0.98f, 1.0f, 1.0f);
		ValueColor = FLinearColor(0.90f, 1.0f, 1.0f, 1.0f);
	}
	else
	{
		PanelColor = FLinearColor(0.06f, 0.10f, 0.18f, 0.92f);
		LabelColor = FLinearColor(0.86f, 0.92f, 1.0f, 1.0f);
		ValueColor = FLinearColor(0.95f, 0.97f, 1.0f, 1.0f);
	}
}

void UZonefallModernComboBoxWidget::RebuildOptions(bool bSelectFirst)
{
	if (!ComboBox)
	{
		return;
	}

	if (bAutoPopulateFromPreset)
	{
		switch (AutoPreset)
		{
		case EZonefallModernComboPreset::OverallQuality:
			Options = { TEXT("Low"), TEXT("Medium"), TEXT("High"), TEXT("Epic"), TEXT("Cinematic") };
			break;
		case EZonefallModernComboPreset::DisplayMode:
			Options = { TEXT("Fullscreen"), TEXT("Windowed"), TEXT("Windowed Fullscreen") };
			break;
		case EZonefallModernComboPreset::VSync:
			Options = { TEXT("Off"), TEXT("On") };
			break;
		case EZonefallModernComboPreset::FPSLimit:
			Options = { TEXT("30"), TEXT("60"), TEXT("120"), TEXT("144"), TEXT("240"), TEXT("Unlimited") };
			break;
		case EZonefallModernComboPreset::ScreenResolution:
		{
			Options.Reset();
			TArray<FIntPoint> Resolutions;
			if (UKismetSystemLibrary::GetSupportedFullscreenResolutions(Resolutions))
			{
				Resolutions.Sort([](const FIntPoint& A, const FIntPoint& B)
				{
					if (A.X == B.X)
					{
						return A.Y < B.Y;
					}
					return A.X < B.X;
				});

				for (const FIntPoint& Resolution : Resolutions)
				{
					Options.AddUnique(FString::Printf(TEXT("%dx%d"), Resolution.X, Resolution.Y));
				}
			}

			if (Options.Num() == 0)
			{
				Options = { TEXT("1280x720"), TEXT("1600x900"), TEXT("1920x1080"), TEXT("2560x1440"), TEXT("3840x2160") };
			}
			break;
		}
		case EZonefallModernComboPreset::DLSS:
			Options = { TEXT("Off"), TEXT("Unavailable"), TEXT("Quality"), TEXT("Balanced"), TEXT("Performance"), TEXT("Ultra Performance"), TEXT("DLAA") };
			break;
		case EZonefallModernComboPreset::DLSSFrameGeneration:
			Options = { TEXT("Off"), TEXT("On"), TEXT("Unavailable") };
			break;
		case EZonefallModernComboPreset::FSR:
			Options = { TEXT("Off"), TEXT("Unavailable"), TEXT("Native AA"), TEXT("Quality"), TEXT("Balanced"), TEXT("Performance"), TEXT("Ultra Performance") };
			break;
		case EZonefallModernComboPreset::FSRFrameGeneration:
			Options = { TEXT("Off"), TEXT("On"), TEXT("Unavailable") };
			break;
		case EZonefallModernComboPreset::Manual:
		default:
			break;
		}
	}

	ComboBox->ClearOptions();
	for (const FString& Option : Options)
	{
		const FString Trimmed = Option.TrimStartAndEnd();
		if (!Trimmed.IsEmpty())
		{
			ComboBox->AddOption(Trimmed);
		}
	}

	if (ComboBox->GetOptionCount() == 0)
	{
		ComboBox->AddOption(TEXT("Off"));
		ComboBox->AddOption(TEXT("On"));
	}

	if (bSelectFirst && ComboBox->GetOptionCount() > 0 && ComboBox->GetSelectedOption().IsEmpty())
	{
		ComboBox->SetSelectedIndex(0);
	}
}

void UZonefallModernComboBoxWidget::BindComboEvents()
{
	if (!ComboBox)
	{
		return;
	}

	ComboBox->OnSelectionChanged.RemoveDynamic(this, &UZonefallModernComboBoxWidget::HandleSelectionChanged);
	ComboBox->OnSelectionChanged.AddDynamic(this, &UZonefallModernComboBoxWidget::HandleSelectionChanged);
}

void UZonefallModernComboBoxWidget::HandleSelectionChanged(FString SelectedItem, ESelectInfo::Type SelectionType)
{
	OnSelectionChanged.Broadcast(SelectedItem);
	BP_OnSelectionChanged(SelectedItem);
}


