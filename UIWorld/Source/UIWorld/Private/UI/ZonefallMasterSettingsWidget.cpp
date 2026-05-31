#include "UI/ZonefallMasterSettingsWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Brushes/SlateRoundedBoxBrush.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/ButtonSlot.h"
#include "Components/CheckBox.h"
#include "Components/ComboBoxString.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/ScrollBox.h"
#include "Components/SizeBox.h"
#include "Components/Slider.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Components/WidgetSwitcher.h"
#include "Engine/Font.h"
#include "Styling/SlateTypes.h"

#include "Character/ZonefallPlayerCharacter.h"
#include "Localization/ZonefallLocalizationSubsystem.h"
#include "UI/ZonefallConfirmDialogWidget.h"
#include "UI/ZonefallSettingsDataObject.h"
#include "UIWorldMenuGameInstance.h"

namespace
{
	// ---------------------------------------------------------------------------
	// Glassmorphism design system. Frosted translucent panels, hairline outlines
	// and a single cyan accent — shared by every control on the settings screen.
	// ---------------------------------------------------------------------------
	namespace ZGlass
	{
		static const FLinearColor Accent       = FLinearColor(0.32f, 0.86f, 0.99f, 1.0f);
		static const FLinearColor AccentSoft    = FLinearColor(0.32f, 0.86f, 0.99f, 0.22f);
		static const FLinearColor Panel        = FLinearColor(1.0f, 1.0f, 1.0f, 0.045f);
		static const FLinearColor PanelStrong  = FLinearColor(1.0f, 1.0f, 1.0f, 0.075f);
		static const FLinearColor Row          = FLinearColor(1.0f, 1.0f, 1.0f, 0.035f);
		static const FLinearColor RowHover     = FLinearColor(1.0f, 1.0f, 1.0f, 0.08f);
		static const FLinearColor Outline      = FLinearColor(1.0f, 1.0f, 1.0f, 0.14f);
		static const FLinearColor OutlineSoft  = FLinearColor(1.0f, 1.0f, 1.0f, 0.07f);
		static const FLinearColor TextPrimary  = FLinearColor(0.94f, 0.97f, 1.0f, 1.0f);
		static const FLinearColor TextSecondary= FLinearColor(0.66f, 0.74f, 0.84f, 1.0f);
		static const FLinearColor Backdrop     = FLinearColor(0.012f, 0.022f, 0.04f, 0.9f);
		static const FLinearColor MenuFill     = FLinearColor(0.03f, 0.05f, 0.09f, 0.98f);
	}

	FSlateFontInfo MakeSetFont(int32 Size)
	{
		FSlateFontInfo Font;
		Font.Size = FMath::Clamp(Size, 8, 96);
		Font.FontObject = LoadObject<UObject>(nullptr, TEXT("/Engine/EngineFonts/Roboto.Roboto"));
		return Font;
	}

	UTextBlock* MakeSetText(UWidgetTree* Tree, FName Name, const FText& InText, int32 Size, FLinearColor Color)
	{
		UTextBlock* T = Tree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), Name);
		T->SetText(InText);
		T->SetFont(MakeSetFont(Size));
		T->SetColorAndOpacity(FSlateColor(Color));
		return T;
	}

	// Frosted rounded panel (translucent fill + hairline outline).
	UBorder* MakeGlassPanel(UWidgetTree* Tree, FName Name, const FLinearColor& Fill, float Radius, const FLinearColor& OutlineColor, float OutlineWidth = 1.0f)
	{
		UBorder* B = Tree->ConstructWidget<UBorder>(UBorder::StaticClass(), Name);
		B->SetBrush(FSlateRoundedBoxBrush(Fill, Radius, OutlineColor, OutlineWidth));
		return B;
	}

	void StyleSetButton(UButton* B, FLinearColor Base, FLinearColor Hover, FLinearColor Pressed)
	{
		if (!B) { return; }
		FButtonStyle Style = B->GetStyle();
		Style.Normal   = FSlateRoundedBoxBrush(Base,    8.0f, ZGlass::OutlineSoft, 1.0f);
		Style.Hovered  = FSlateRoundedBoxBrush(Hover,   8.0f, ZGlass::Outline,     1.0f);
		Style.Pressed  = FSlateRoundedBoxBrush(Pressed, 8.0f, ZGlass::Outline,     1.0f);
		Style.Disabled = FSlateRoundedBoxBrush(FLinearColor(1.0f, 1.0f, 1.0f, 0.03f), 8.0f, ZGlass::OutlineSoft, 1.0f);
		B->SetStyle(Style);
	}

	UButton* MakeTextButton(UWidgetTree* Tree, FName Name, const FText& Label, int32 FontSize, FLinearColor Bg, FLinearColor Hover, FLinearColor Pressed)
	{
		UButton* B = Tree->ConstructWidget<UButton>(UButton::StaticClass(), Name);
		StyleSetButton(B, Bg, Hover, Pressed);
		UTextBlock* T = Tree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), FName(*(Name.ToString() + TEXT("_Lbl"))));
		T->SetText(Label);
		T->SetFont(MakeSetFont(FontSize));
		T->SetColorAndOpacity(FSlateColor(ZGlass::TextPrimary));
		B->AddChild(T);
		if (UButtonSlot* Slot = Cast<UButtonSlot>(T->Slot))
		{
			Slot->SetHorizontalAlignment(HAlign_Center);
			Slot->SetVerticalAlignment(VAlign_Center);
			Slot->SetPadding(FMargin(16.0f, 9.0f));
		}
		return B;
	}

	// Frosted combo box: translucent button, hairline outline, accent-highlighted rows.
	void StyleGlassCombo(UComboBoxString* Combo)
	{
		if (!Combo) { return; }

		FComboBoxStyle ComboStyle = Combo->GetWidgetStyle();
		FButtonStyle Btn = ComboStyle.ComboButtonStyle.ButtonStyle;
		Btn.SetNormal  (FSlateRoundedBoxBrush(FLinearColor(1.0f, 1.0f, 1.0f, 0.05f), 8.0f, ZGlass::OutlineSoft, 1.0f));
		Btn.SetHovered (FSlateRoundedBoxBrush(FLinearColor(1.0f, 1.0f, 1.0f, 0.10f), 8.0f, ZGlass::Outline,     1.0f));
		Btn.SetPressed (FSlateRoundedBoxBrush(FLinearColor(1.0f, 1.0f, 1.0f, 0.07f), 8.0f, ZGlass::Outline,     1.0f));
		Btn.SetDisabled(FSlateRoundedBoxBrush(FLinearColor(1.0f, 1.0f, 1.0f, 0.02f), 8.0f, ZGlass::OutlineSoft, 1.0f));
		Btn.NormalPadding  = FMargin(14.0f, 7.0f);
		Btn.PressedPadding = FMargin(14.0f, 8.0f, 14.0f, 6.0f);
		ComboStyle.ComboButtonStyle.SetButtonStyle(Btn);
		ComboStyle.ComboButtonStyle.SetContentPadding(FMargin(10.0f, 5.0f));
		ComboStyle.ComboButtonStyle.SetMenuBorderBrush(FSlateRoundedBoxBrush(ZGlass::MenuFill, 8.0f, ZGlass::Outline, 1.0f));
		ComboStyle.ComboButtonStyle.SetMenuBorderPadding(FMargin(6.0f));
		Combo->SetWidgetStyle(ComboStyle);

		FTableRowStyle Row = Combo->GetItemStyle();
		Row.SetActiveBrush(FSlateRoundedBoxBrush(ZGlass::Accent * 0.45f, 6.0f));
		Row.SetActiveHoveredBrush(FSlateRoundedBoxBrush(ZGlass::Accent * 0.55f, 6.0f));
		Row.SetInactiveBrush(FSlateRoundedBoxBrush(FLinearColor(1.0f, 1.0f, 1.0f, 0.04f), 6.0f));
		Row.SetInactiveHoveredBrush(FSlateRoundedBoxBrush(FLinearColor(1.0f, 1.0f, 1.0f, 0.10f), 6.0f));
		Row.SetEvenRowBackgroundBrush(FSlateRoundedBoxBrush(ZGlass::MenuFill, 4.0f));
		Row.SetEvenRowBackgroundHoveredBrush(FSlateRoundedBoxBrush(FLinearColor(1.0f, 1.0f, 1.0f, 0.08f), 4.0f));
		Row.SetOddRowBackgroundBrush(FSlateRoundedBoxBrush(ZGlass::MenuFill, 4.0f));
		Row.SetOddRowBackgroundHoveredBrush(FSlateRoundedBoxBrush(FLinearColor(1.0f, 1.0f, 1.0f, 0.08f), 4.0f));
		Combo->SetItemStyle(Row);
	}

	// Slim accent slider with a rounded glass track.
	void StyleGlassSlider(USlider* Slider, const FLinearColor& Accent)
	{
		if (!Slider) { return; }
		FSliderStyle Style = Slider->GetWidgetStyle();
		Style.SetNormalBarImage(FSlateRoundedBoxBrush(FLinearColor(1.0f, 1.0f, 1.0f, 0.12f), 4.0f));
		Style.SetHoveredBarImage(FSlateRoundedBoxBrush(FLinearColor(1.0f, 1.0f, 1.0f, 0.16f), 4.0f));
		Style.SetDisabledBarImage(FSlateRoundedBoxBrush(FLinearColor(1.0f, 1.0f, 1.0f, 0.06f), 4.0f));
		Style.SetNormalThumbImage(FSlateRoundedBoxBrush(Accent, 9.0f));
		Style.SetHoveredThumbImage(FSlateRoundedBoxBrush(Accent * 1.1f, 9.0f));
		Style.SetDisabledThumbImage(FSlateRoundedBoxBrush(FLinearColor(0.4f, 0.4f, 0.4f, 1.0f), 9.0f));
		Style.SetBarThickness(6.0f);
		Slider->SetWidgetStyle(Style);
		Slider->SetSliderHandleColor(Accent);
		Slider->SetSliderBarColor(FLinearColor(1.0f, 1.0f, 1.0f, 0.12f));
	}

	// Rounded toggle: translucent when off, accent-filled when on.
	void StyleGlassCheck(UCheckBox* Check, const FLinearColor& Accent)
	{
		if (!Check) { return; }
		FCheckBoxStyle Style = Check->GetWidgetStyle();
		Style.SetCheckBoxType(ESlateCheckBoxType::ToggleButton);

		const FSlateBrush Off        = FSlateRoundedBoxBrush(FLinearColor(1.0f, 1.0f, 1.0f, 0.06f), 6.0f, ZGlass::Outline, 1.0f);
		const FSlateBrush OffHover    = FSlateRoundedBoxBrush(FLinearColor(1.0f, 1.0f, 1.0f, 0.12f), 6.0f, ZGlass::Outline, 1.0f);
		const FSlateBrush On         = FSlateRoundedBoxBrush(Accent, 6.0f, ZGlass::Outline, 1.0f);
		const FSlateBrush OnHover     = FSlateRoundedBoxBrush(Accent * 1.1f, 6.0f, ZGlass::Outline, 1.0f);

		Style.SetUncheckedImage(Off);
		Style.SetUncheckedHoveredImage(OffHover);
		Style.SetUncheckedPressedImage(OffHover);
		Style.SetCheckedImage(On);
		Style.SetCheckedHoveredImage(OnHover);
		Style.SetCheckedPressedImage(On);
		Style.SetUndeterminedImage(Off);
		Style.SetPadding(FMargin(20.0f, 12.0f));
		Check->SetWidgetStyle(Style);
	}
}

UZonefallKeybindButton::UZonefallKeybindButton()
{
	OnClicked.AddDynamic(this, &UZonefallKeybindButton::HandleInternalClicked);
}

void UZonefallKeybindButton::HandleInternalClicked()
{
	OnKeybindClicked.Broadcast(ActionId);
}

UZonefallMasterSettingsWidget::UZonefallMasterSettingsWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

TSharedRef<SWidget> UZonefallMasterSettingsWidget::RebuildWidget()
{
	if (!WidgetTree || !WidgetTree->RootWidget)
	{
		BuildLayout();
	}
	return Super::RebuildWidget();
}

void UZonefallMasterSettingsWidget::NativeConstruct()
{
	Super::NativeConstruct();
	SetIsFocusable(true);
	RefreshFromSettings();
	ShowTab(0);

	// Gamepad / keyboard navigation: land focus on the first category so a controller can
	// immediately move through the menu (deferred a frame so the Slate tree is realised).
	if (UWorld* World = GetWorld())
	{
		FTimerHandle FocusHandle;
		World->GetTimerManager().SetTimerForNextTick([WeakThis = TWeakObjectPtr<UZonefallMasterSettingsWidget>(this)]()
		{
			if (WeakThis.IsValid() && WeakThis->TabGraphicsButton)
			{
				WeakThis->TabGraphicsButton->SetFocus();
			}
		});
	}
}

AZonefallPlayerCharacter* UZonefallMasterSettingsWidget::ResolvePlayerCharacter() const
{
	if (APawn* Pawn = GetOwningPlayerPawn())
	{
		return Cast<AZonefallPlayerCharacter>(Pawn);
	}
	return nullptr;
}

void UZonefallMasterSettingsWidget::BuildLayout()
{
	if (!WidgetTree)
	{
		return;
	}

	if (!SettingsObject)
	{
		SettingsObject = NewObject<UZonefallSettingsDataObject>(this);
		SettingsObject->LoadFromSystem();
	}

	WidgetTree->RootWidget = nullptr;

	// Full-screen frosted backdrop.
	RootBorder = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("SettingsRoot"));
	RootBorder->SetBrush(FSlateRoundedBoxBrush(ZGlass::Backdrop, 0.0f));
	RootBorder->SetPadding(FMargin(90.0f, 54.0f));
	RootBorder->SetHorizontalAlignment(HAlign_Fill);
	RootBorder->SetVerticalAlignment(VAlign_Fill);
	WidgetTree->RootWidget = RootBorder;

	// Centered frosted glass card that holds the whole screen.
	UBorder* Card = MakeGlassPanel(WidgetTree, TEXT("SettingsCard"), ZGlass::PanelStrong, 20.0f, ZGlass::Outline, 1.0f);
	Card->SetPadding(FMargin(36.0f, 30.0f));
	RootBorder->SetContent(Card);

	UVerticalBox* MainBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("SettingsMainBox"));
	Card->SetContent(MainBox);

	// --- Header: title + accent underline ---
	UTextBlock* Title = MakeSetText(WidgetTree, TEXT("SettingsTitle"), NSLOCTEXT("ZonefallSettings", "Title", "SETTINGS"), TitleFontSize, ZGlass::TextPrimary);
	Title->SetShadowOffset(FVector2D(0.0f, 1.0f));
	Title->SetShadowColorAndOpacity(FLinearColor(0.0f, 0.0f, 0.0f, 0.6f));
	if (UVerticalBoxSlot* TitleSlot = MainBox->AddChildToVerticalBox(Title))
	{
		TitleSlot->SetPadding(FMargin(2, 0, 0, 6));
	}

	UBorder* TitleRule = MakeGlassPanel(WidgetTree, TEXT("SettingsTitleRule"), ZGlass::Accent, 2.0f, ZGlass::Accent, 0.0f);
	if (USizeBox* RuleSize = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("SettingsTitleRuleSize")))
	{
		RuleSize->SetWidthOverride(96.0f);
		RuleSize->SetHeightOverride(3.0f);
		RuleSize->AddChild(TitleRule);
		if (UVerticalBoxSlot* RuleSlot = MainBox->AddChildToVerticalBox(RuleSize))
		{
			RuleSlot->SetPadding(FMargin(2, 0, 0, 18));
			RuleSlot->SetHorizontalAlignment(HAlign_Left);
		}
	}

	// Content row: left category sidebar + right body panel.
	UHorizontalBox* ContentRow = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("SettingsContentRow"));
	if (UVerticalBoxSlot* ContentSlot = MainBox->AddChildToVerticalBox(ContentRow))
	{
		ContentSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
		ContentSlot->SetPadding(FMargin(0, 0, 0, 16));
	}

	// --- Left vertical category list (glass) ---
	UBorder* Sidebar = MakeGlassPanel(WidgetTree, TEXT("SettingsSidebar"), ZGlass::Panel, 14.0f, ZGlass::OutlineSoft, 1.0f);
	Sidebar->SetPadding(FMargin(12.0f, 16.0f));
	Sidebar->SetVerticalAlignment(VAlign_Top);

	USizeBox* SidebarSize = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("SettingsSidebarSize"));
	SidebarSize->SetWidthOverride(264.0f);
	SidebarSize->AddChild(Sidebar);
	if (UHorizontalBoxSlot* SbSlot = ContentRow->AddChildToHorizontalBox(SidebarSize))
	{
		SbSlot->SetPadding(FMargin(0, 0, 16, 0));
		SbSlot->SetVerticalAlignment(VAlign_Fill);
	}

	UVerticalBox* SidebarBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("SettingsSidebarBox"));
	Sidebar->SetContent(SidebarBox);

	UZonefallLocalizationSubsystem* SidebarLoc = UZonefallLocalizationSubsystem::Get(this);
	auto SidebarLT = [SidebarLoc](const TCHAR* Key, const FText& Fallback) -> FText
	{
		return SidebarLoc ? SidebarLoc->GetText(FName(Key)) : Fallback;
	};

	TabGraphicsButton = MakeTextButton(WidgetTree, TEXT("TabGraphics"), SidebarLT(TEXT("settings.graphics"), NSLOCTEXT("ZonefallSettings", "Graphics", "GRAPHICS")), BodyFontSize + 1, ZGlass::Row, ZGlass::RowHover, ZGlass::AccentSoft);
	TabAudioButton = MakeTextButton(WidgetTree, TEXT("TabAudio"), SidebarLT(TEXT("settings.audio"), NSLOCTEXT("ZonefallSettings", "Audio", "AUDIO")), BodyFontSize + 1, ZGlass::Row, ZGlass::RowHover, ZGlass::AccentSoft);
	TabControlsButton = MakeTextButton(WidgetTree, TEXT("TabControls"), SidebarLT(TEXT("settings.controls"), NSLOCTEXT("ZonefallSettings", "Controls", "CONTROLS")), BodyFontSize + 1, ZGlass::Row, ZGlass::RowHover, ZGlass::AccentSoft);
	TabLanguageButton = MakeTextButton(WidgetTree, TEXT("TabLanguage"), SidebarLT(TEXT("settings.language"), NSLOCTEXT("ZonefallSettings", "Language", "LANGUAGE")), BodyFontSize + 1, ZGlass::Row, ZGlass::RowHover, ZGlass::AccentSoft);
	TabGraphicsButton->OnClicked.AddDynamic(this, &UZonefallMasterSettingsWidget::HandleTabGraphics);
	TabAudioButton->OnClicked.AddDynamic(this, &UZonefallMasterSettingsWidget::HandleTabAudio);
	TabControlsButton->OnClicked.AddDynamic(this, &UZonefallMasterSettingsWidget::HandleTabControls);
	TabLanguageButton->OnClicked.AddDynamic(this, &UZonefallMasterSettingsWidget::HandleTabLanguage);

	for (UButton* Tab : { TabGraphicsButton.Get(), TabAudioButton.Get(), TabControlsButton.Get(), TabLanguageButton.Get() })
	{
		if (UVerticalBoxSlot* TS = SidebarBox->AddChildToVerticalBox(Tab))
		{
			TS->SetPadding(FMargin(0, 0, 0, 8));
			TS->SetHorizontalAlignment(HAlign_Fill);
		}
	}

	// --- Right body panel with a switcher (glass) ---
	UBorder* BodyPanel = MakeGlassPanel(WidgetTree, TEXT("SettingsBodyPanel"), ZGlass::Panel, 14.0f, ZGlass::OutlineSoft, 1.0f);
	BodyPanel->SetPadding(FMargin(24.0f, 18.0f));
	if (UHorizontalBoxSlot* BodySlot = ContentRow->AddChildToHorizontalBox(BodyPanel))
	{
		BodySlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	}

	BodySwitcher = WidgetTree->ConstructWidget<UWidgetSwitcher>(UWidgetSwitcher::StaticClass(), TEXT("SettingsSwitcher"));
	BodyPanel->SetContent(BodySwitcher);

	UScrollBox* GraphicsBody = WidgetTree->ConstructWidget<UScrollBox>(UScrollBox::StaticClass(), TEXT("GraphicsBody"));
	UScrollBox* AudioBody = WidgetTree->ConstructWidget<UScrollBox>(UScrollBox::StaticClass(), TEXT("AudioBody"));
	UScrollBox* ControlsBody = WidgetTree->ConstructWidget<UScrollBox>(UScrollBox::StaticClass(), TEXT("ControlsBody"));
	UScrollBox* LanguageBody = WidgetTree->ConstructWidget<UScrollBox>(UScrollBox::StaticClass(), TEXT("LanguageBody"));
	BodySwitcher->AddChild(GraphicsBody);
	BodySwitcher->AddChild(AudioBody);
	BodySwitcher->AddChild(ControlsBody);
	BodySwitcher->AddChild(LanguageBody);

	BuildGraphicsTab(GraphicsBody);
	BuildAudioTab(AudioBody);
	BuildControlsTab(ControlsBody);
	BuildLanguageTab(LanguageBody);

	// Thin divider above the footer.
	UBorder* FooterRule = MakeGlassPanel(WidgetTree, TEXT("SettingsFooterRule"), ZGlass::OutlineSoft, 1.0f, ZGlass::OutlineSoft, 0.0f);
	if (USizeBox* FRuleSize = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("SettingsFooterRuleSize")))
	{
		FRuleSize->SetHeightOverride(1.0f);
		FRuleSize->AddChild(FooterRule);
		if (UVerticalBoxSlot* FRuleSlot = MainBox->AddChildToVerticalBox(FRuleSize))
		{
			FRuleSlot->SetPadding(FMargin(0, 0, 0, 12));
		}
	}

	// Footer: status + Reset + Apply + Back.
	UHorizontalBox* Footer = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("SettingsFooter"));

	StatusText = MakeSetText(WidgetTree, TEXT("SettingsStatus"), NSLOCTEXT("ZonefallSettings", "Ready", "Ready."), BodyFontSize, ZGlass::TextSecondary);
	if (UHorizontalBoxSlot* SS = Footer->AddChildToHorizontalBox(StatusText))
	{
		SS->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
		SS->SetVerticalAlignment(VAlign_Center);
	}

	UButton* ResetButton = MakeTextButton(WidgetTree, TEXT("SettingsReset"), NSLOCTEXT("ZonefallSettings", "Reset", "RESET"), BodyFontSize, ZGlass::Row, ZGlass::RowHover, ZGlass::AccentSoft);
	ResetButton->OnClicked.AddDynamic(this, &UZonefallMasterSettingsWidget::HandleResetClicked);
	if (UHorizontalBoxSlot* RS = Footer->AddChildToHorizontalBox(ResetButton)) { RS->SetPadding(FMargin(0, 0, 10, 0)); }

	UButton* ApplyButton = MakeTextButton(WidgetTree, TEXT("SettingsApply"), NSLOCTEXT("ZonefallSettings", "Apply", "APPLY"), BodyFontSize, AccentColor * 0.5f, AccentColor * 0.68f, AccentColor * 0.85f);
	ApplyButton->OnClicked.AddDynamic(this, &UZonefallMasterSettingsWidget::HandleApplyClicked);
	if (UHorizontalBoxSlot* AS = Footer->AddChildToHorizontalBox(ApplyButton)) { AS->SetPadding(FMargin(0, 0, 10, 0)); }

	UButton* BackButton = MakeTextButton(WidgetTree, TEXT("SettingsBack"), NSLOCTEXT("ZonefallSettings", "Back", "BACK"), BodyFontSize, ZGlass::Row, ZGlass::RowHover, ZGlass::AccentSoft);
	BackButton->OnClicked.AddDynamic(this, &UZonefallMasterSettingsWidget::HandleBackClicked);
	Footer->AddChildToHorizontalBox(BackButton);

	MainBox->AddChildToVerticalBox(Footer);
}

UComboBoxString* UZonefallMasterSettingsWidget::AddComboRow(UVerticalBox* Parent, const FText& Label, const TArray<FString>& Options, const FString& Selected)
{
	UBorder* RowPanel = MakeGlassPanel(WidgetTree, NAME_None, ZGlass::Row, 10.0f, ZGlass::OutlineSoft, 1.0f);
	RowPanel->SetPadding(FMargin(14.0f, 9.0f));

	UHorizontalBox* Row = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
	RowPanel->SetContent(Row);

	UTextBlock* LabelText = MakeSetText(WidgetTree, NAME_None, Label, BodyFontSize, ZGlass::TextPrimary);
	if (UHorizontalBoxSlot* LS = Row->AddChildToHorizontalBox(LabelText))
	{
		LS->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
		LS->SetVerticalAlignment(VAlign_Center);
	}

	UComboBoxString* Combo = WidgetTree->ConstructWidget<UComboBoxString>(UComboBoxString::StaticClass());
	for (const FString& Opt : Options)
	{
		Combo->AddOption(Opt);
	}
	if (!Selected.IsEmpty())
	{
		if (Combo->FindOptionIndex(Selected) == INDEX_NONE)
		{
			Combo->AddOption(Selected);
		}
		Combo->SetSelectedOption(Selected);
	}
	else if (Options.Num() > 0)
	{
		Combo->SetSelectedOption(Options[0]);
	}
	StyleGlassCombo(Combo);

	USizeBox* ComboSize = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
	ComboSize->SetMinDesiredWidth(280.0f);
	ComboSize->SetContent(Combo);
	if (UHorizontalBoxSlot* CS = Row->AddChildToHorizontalBox(ComboSize))
	{
		CS->SetVerticalAlignment(VAlign_Center);
	}

	if (UVerticalBoxSlot* RowSlot = Parent->AddChildToVerticalBox(RowPanel))
	{
		RowSlot->SetPadding(FMargin(0, 0, 0, 8));
	}
	return Combo;
}

USlider* UZonefallMasterSettingsWidget::AddSliderRow(UVerticalBox* Parent, const FText& Label, float MinValue, float MaxValue, float Value, FName ValueLabelName)
{
	UBorder* RowPanel = MakeGlassPanel(WidgetTree, NAME_None, ZGlass::Row, 10.0f, ZGlass::OutlineSoft, 1.0f);
	RowPanel->SetPadding(FMargin(14.0f, 9.0f));

	UHorizontalBox* Row = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
	RowPanel->SetContent(Row);

	UTextBlock* LabelText = MakeSetText(WidgetTree, NAME_None, Label, BodyFontSize, ZGlass::TextPrimary);
	if (UHorizontalBoxSlot* LS = Row->AddChildToHorizontalBox(LabelText))
	{
		LS->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
		LS->SetVerticalAlignment(VAlign_Center);
	}

	USlider* Slider = WidgetTree->ConstructWidget<USlider>(USlider::StaticClass());
	Slider->SetMinValue(MinValue);
	Slider->SetMaxValue(MaxValue);
	Slider->SetValue(FMath::Clamp(Value, MinValue, MaxValue));
	Slider->SetStepSize((MaxValue - MinValue) / 100.0f);
	StyleGlassSlider(Slider, AccentColor);
	Slider->OnValueChanged.AddDynamic(this, &UZonefallMasterSettingsWidget::HandleSliderChanged);

	USizeBox* SliderSize = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
	SliderSize->SetMinDesiredWidth(240.0f);
	SliderSize->SetContent(Slider);
	if (UHorizontalBoxSlot* SLS = Row->AddChildToHorizontalBox(SliderSize))
	{
		SLS->SetVerticalAlignment(VAlign_Center);
		SLS->SetPadding(FMargin(12, 0, 12, 0));
	}

	UTextBlock* ValueText = MakeSetText(WidgetTree, ValueLabelName, FText::FromString(TEXT("--")), BodyFontSize, AccentColor);
	USizeBox* ValueSize = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
	ValueSize->SetMinDesiredWidth(56.0f);
	ValueSize->SetContent(ValueText);
	if (UHorizontalBoxSlot* VS = Row->AddChildToHorizontalBox(ValueSize))
	{
		VS->SetVerticalAlignment(VAlign_Center);
	}

	SliderValueLabels.Add(Slider, ValueText);

	if (UVerticalBoxSlot* RowSlot = Parent->AddChildToVerticalBox(RowPanel))
	{
		RowSlot->SetPadding(FMargin(0, 0, 0, 8));
	}
	return Slider;
}

void UZonefallMasterSettingsWidget::AddSectionHeader(UVerticalBox* Parent, const FText& Text)
{
	if (!Parent)
	{
		return;
	}

	// Header row: accent pill marker + label.
	UHorizontalBox* HeaderRow = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());

	UBorder* Pill = MakeGlassPanel(WidgetTree, NAME_None, AccentColor, 2.0f, AccentColor, 0.0f);
	if (USizeBox* PillSize = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass()))
	{
		PillSize->SetWidthOverride(4.0f);
		PillSize->SetHeightOverride(18.0f);
		PillSize->SetContent(Pill);
		if (UHorizontalBoxSlot* PS = HeaderRow->AddChildToHorizontalBox(PillSize))
		{
			PS->SetPadding(FMargin(0, 0, 10, 0));
			PS->SetVerticalAlignment(VAlign_Center);
		}
	}

	UTextBlock* Header = MakeSetText(WidgetTree, NAME_None, Text, BodyFontSize + 3, AccentColor);
	if (UHorizontalBoxSlot* HS = HeaderRow->AddChildToHorizontalBox(Header))
	{
		HS->SetVerticalAlignment(VAlign_Center);
	}

	if (UVerticalBoxSlot* HeaderSlot = Parent->AddChildToVerticalBox(HeaderRow))
	{
		HeaderSlot->SetPadding(FMargin(0, 14, 0, 8));
	}
}

UCheckBox* UZonefallMasterSettingsWidget::AddCheckRow(UVerticalBox* Parent, const FText& Label, bool bChecked)
{
	UBorder* RowPanel = MakeGlassPanel(WidgetTree, NAME_None, ZGlass::Row, 10.0f, ZGlass::OutlineSoft, 1.0f);
	RowPanel->SetPadding(FMargin(14.0f, 9.0f));

	UHorizontalBox* Row = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
	RowPanel->SetContent(Row);

	UTextBlock* LabelText = MakeSetText(WidgetTree, NAME_None, Label, BodyFontSize, ZGlass::TextPrimary);
	if (UHorizontalBoxSlot* LS = Row->AddChildToHorizontalBox(LabelText))
	{
		LS->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
		LS->SetVerticalAlignment(VAlign_Center);
	}

	UCheckBox* Check = WidgetTree->ConstructWidget<UCheckBox>(UCheckBox::StaticClass());
	StyleGlassCheck(Check, AccentColor);
	Check->SetIsChecked(bChecked);
	if (UHorizontalBoxSlot* CS = Row->AddChildToHorizontalBox(Check))
	{
		CS->SetVerticalAlignment(VAlign_Center);
	}

	if (UVerticalBoxSlot* RowSlot = Parent->AddChildToVerticalBox(RowPanel))
	{
		RowSlot->SetPadding(FMargin(0, 0, 0, 8));
	}
	return Check;
}

void UZonefallMasterSettingsWidget::BuildGraphicsTab(UScrollBox* Body)
{
	if (!Body || !SettingsObject)
	{
		return;
	}

	UVerticalBox* Box = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("GraphicsVBox"));
	Body->AddChild(Box);

	// Small muted helper line under a section, so each group explains itself.
	auto AddNote = [this, Box](const FText& Note)
	{
		UTextBlock* N = MakeSetText(WidgetTree, NAME_None, Note, BodyFontSize - 3, ZGlass::TextSecondary);
		N->SetAutoWrapText(true);
		if (UVerticalBoxSlot* NS = Box->AddChildToVerticalBox(N))
		{
			NS->SetPadding(FMargin(14, 0, 0, 10));
		}
	};

	TArray<FString> Resolutions;
	SettingsObject->GetAvailableScreenResolutions(Resolutions, false);

	// Quality preset at the top — picking one reconfigures everything below.
	AddSectionHeader(Box, NSLOCTEXT("ZonefallSettings", "SecPreset", "QUALITY PRESET"));
	AddNote(NSLOCTEXT("ZonefallSettings", "NotePreset", "Pick a preset to set everything below at once. Tweaking any option switches the preset to Custom. Press APPLY to save."));
	PresetCombo = AddComboRow(Box, NSLOCTEXT("ZonefallSettings", "Preset", "Quality Preset"),
		{ TEXT("Competitive"), TEXT("Balanced"), TEXT("Quality"), TEXT("Ultra"), TEXT("Auto Detect") }, TEXT("Balanced"));
	if (PresetCombo)
	{
		PresetCombo->OnSelectionChanged.AddDynamic(this, &UZonefallMasterSettingsWidget::HandlePresetChanged);
	}

	AddSectionHeader(Box, NSLOCTEXT("ZonefallSettings", "SecDisplay", "DISPLAY"));
	DisplayModeCombo = AddComboRow(Box, NSLOCTEXT("ZonefallSettings", "DisplayMode", "Display Mode"),
		{ TEXT("Fullscreen"), TEXT("Windowed Fullscreen"), TEXT("Windowed") }, SettingsObject->DisplayMode);
	ResolutionCombo = AddComboRow(Box, NSLOCTEXT("ZonefallSettings", "Resolution", "Resolution"),
		Resolutions, SettingsObject->ScreenResolution.IsEmpty() ? SettingsObject->GetCurrentScreenResolutionString() : SettingsObject->ScreenResolution);
	AddSectionHeader(Box, NSLOCTEXT("ZonefallSettings", "SecRendering", "RENDERING API & RTX"));
	DirectXCombo = AddComboRow(Box, NSLOCTEXT("ZonefallSettings", "DirectX", "Graphics API (restart)"),
		{ TEXT("DirectX 12"), TEXT("DirectX 11") }, SettingsObject->DirectXVersion);
	RayTracingCombo = AddComboRow(Box, NSLOCTEXT("ZonefallSettings", "RTX", "Ray Tracing (RTX)"),
		{ TEXT("On"), TEXT("Off") }, SettingsObject->RayTracing);

	AddSectionHeader(Box, NSLOCTEXT("ZonefallSettings", "SecQuality", "QUALITY"));
	QualityCombo = AddComboRow(Box, NSLOCTEXT("ZonefallSettings", "Quality", "Overall Quality"),
		{ TEXT("Low"), TEXT("Medium"), TEXT("High"), TEXT("Epic"), TEXT("Cinematic") }, SettingsObject->OverallQuality);
	ResScaleCombo = AddComboRow(Box, NSLOCTEXT("ZonefallSettings", "ResScale", "Resolution Scale"),
		{ TEXT("50%"), TEXT("60%"), TEXT("70%"), TEXT("80%"), TEXT("90%"), TEXT("100%") }, SettingsObject->ResolutionScale);
	VSyncCombo = AddComboRow(Box, NSLOCTEXT("ZonefallSettings", "VSync", "V-Sync"),
		{ TEXT("On"), TEXT("Off") }, SettingsObject->VSync);
	FPSLimitCombo = AddComboRow(Box, NSLOCTEXT("ZonefallSettings", "FPSLimit", "FPS Limit"),
		{ TEXT("30"), TEXT("60"), TEXT("120"), TEXT("144"), TEXT("240"), TEXT("Unlimited") }, SettingsObject->FPSLimit);
	LumenCombo = AddComboRow(Box, NSLOCTEXT("ZonefallSettings", "Lumen", "Lumen GI"),
		{ TEXT("On"), TEXT("Off") }, SettingsObject->Lumen);
	CloudsCombo = AddComboRow(Box, NSLOCTEXT("ZonefallSettings", "Clouds", "Volumetric Clouds"),
		{ TEXT("Off"), TEXT("Low"), TEXT("High"), TEXT("Epic") }, SettingsObject->VolumetricClouds);

	// Upscaling & frame generation — always shown. Unsupported features are visible but
	// disabled with a "(not supported)" tag so the player always sees what exists.
	AddSectionHeader(Box, NSLOCTEXT("ZonefallSettings", "SecUpscaling", "UPSCALING & FRAME GENERATION"));
	AddNote(NSLOCTEXT("ZonefallSettings", "NoteUpscaling", "Zonefall DLSS (NVIDIA RTX) и Zonefall FSR повышают FPS за счёт апскейлинга. Используйте один за раз. Генерация кадров требует RTX 40-серии / FSR 4. Неподдерживаемые опции отображаются серым."));

	auto AddUpscaleRow = [this, Box](const FText& BaseLabel, const TArray<FString>& Opts, const FString& Current, bool bSupported) -> UComboBoxString*
	{
		const FText Label = bSupported
			? BaseLabel
			: FText::Format(NSLOCTEXT("ZonefallSettings", "NotSupportedFmt", "{0}  (not supported)"), BaseLabel);
		UComboBoxString* Combo = AddComboRow(Box, Label, Opts, bSupported ? Current : FString(TEXT("Unavailable")));
		if (Combo)
		{
			Combo->SetIsEnabled(bSupported);
		}
		return Combo;
	};

	DLSSCombo = AddUpscaleRow(NSLOCTEXT("ZonefallSettings", "DLSS", "Zonefall DLSS"),
		{ TEXT("Off"), TEXT("Performance"), TEXT("Balanced"), TEXT("Quality") }, SettingsObject->DLSSMode, SettingsObject->bDLSSSupported);
	FrameGenCombo = AddUpscaleRow(NSLOCTEXT("ZonefallSettings", "FrameGen", "Zonefall DLSS Frame Gen"),
		{ TEXT("On"), TEXT("Off") }, SettingsObject->FrameGeneration, SettingsObject->bFrameGenerationSupported);
	FSRCombo = AddUpscaleRow(NSLOCTEXT("ZonefallSettings", "FSR", "Zonefall FSR"),
		{ TEXT("Off"), TEXT("Performance"), TEXT("Balanced"), TEXT("Quality") }, SettingsObject->FSRMode, SettingsObject->bFSRSupported);
	FSRFrameGenCombo = AddUpscaleRow(NSLOCTEXT("ZonefallSettings", "FSRFrameGen", "Zonefall FSR Frame Gen"),
		{ TEXT("On"), TEXT("Off") }, SettingsObject->FSRFrameGeneration, SettingsObject->bFSRFrameGenerationSupported);

	AddSectionHeader(Box, NSLOCTEXT("ZonefallSettings", "SecCamera", "CAMERA"));
	BrightnessSlider = AddSliderRow(Box, NSLOCTEXT("ZonefallSettings", "Brightness", "Brightness"), 0.5f, 2.0f, SettingsObject->Brightness, TEXT("BrightnessVal"));
	FOVSlider = AddSliderRow(Box, NSLOCTEXT("ZonefallSettings", "FOV", "Field of View"), 60.0f, 120.0f, SettingsObject->FieldOfView, TEXT("FOVVal"));

	// --- Advanced per-group quality ---
	AddSectionHeader(Box, NSLOCTEXT("ZonefallSettings", "SecAdvanced", "ADVANCED GRAPHICS"));
	AdvancedQualityCombos.Reset();
	AdvancedQualityGroups.Reset();
	const TArray<TPair<FName, FText>> Groups =
	{
		{ TEXT("ViewDistance"),       NSLOCTEXT("ZonefallSettings", "GViewDistance", "View Distance") },
		{ TEXT("Shadows"),            NSLOCTEXT("ZonefallSettings", "GShadows", "Shadows") },
		{ TEXT("GlobalIllumination"), NSLOCTEXT("ZonefallSettings", "GGI", "Global Illumination") },
		{ TEXT("Reflections"),        NSLOCTEXT("ZonefallSettings", "GReflections", "Reflections") },
		{ TEXT("PostProcess"),        NSLOCTEXT("ZonefallSettings", "GPostProcess", "Post Processing") },
		{ TEXT("Textures"),           NSLOCTEXT("ZonefallSettings", "GTextures", "Textures") },
		{ TEXT("Effects"),            NSLOCTEXT("ZonefallSettings", "GEffects", "Effects") },
		{ TEXT("Foliage"),            NSLOCTEXT("ZonefallSettings", "GFoliage", "Foliage") },
		{ TEXT("Shading"),            NSLOCTEXT("ZonefallSettings", "GShading", "Shading") },
		{ TEXT("AntiAliasing"),       NSLOCTEXT("ZonefallSettings", "GAntiAliasing", "Anti-Aliasing") },
	};
	for (const TPair<FName, FText>& G : Groups)
	{
		const FString Current = SettingsObject->AdvancedQuality.FindRef(G.Key);
		UComboBoxString* Combo = AddComboRow(Box, G.Value,
			{ TEXT("Low"), TEXT("Medium"), TEXT("High"), TEXT("Epic") }, Current.IsEmpty() ? TEXT("High") : Current);
		AdvancedQualityCombos.Add(Combo);
		AdvancedQualityGroups.Add(G.Key);
	}
}

void UZonefallMasterSettingsWidget::BuildAudioTab(UScrollBox* Body)
{
	if (!Body || !SettingsObject)
	{
		return;
	}

	UVerticalBox* Box = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("AudioVBox"));
	Body->AddChild(Box);

	AddSectionHeader(Box, NSLOCTEXT("ZonefallSettings", "SecVolume", "VOLUME"));
	MasterSlider = AddSliderRow(Box, NSLOCTEXT("ZonefallSettings", "Master", "Master Volume"), 0.0f, 1.0f, SettingsObject->MasterVolume, TEXT("MasterVal"));
	SfxSlider = AddSliderRow(Box, NSLOCTEXT("ZonefallSettings", "Sfx", "SFX Volume"), 0.0f, 1.0f, SettingsObject->SfxVolume, TEXT("SfxVal"));
	MusicSlider = AddSliderRow(Box, NSLOCTEXT("ZonefallSettings", "Music", "Music Volume"), 0.0f, 1.0f, SettingsObject->MusicVolume, TEXT("MusicVal"));
	VoiceSlider = AddSliderRow(Box, NSLOCTEXT("ZonefallSettings", "Voice", "Voice Volume"), 0.0f, 1.0f, SettingsObject->VoiceVolume, TEXT("VoiceVal"));
}

void UZonefallMasterSettingsWidget::BuildControlsTab(UScrollBox* Body)
{
	if (!Body)
	{
		return;
	}

	UVerticalBox* OuterBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("ControlsOuterVBox"));
	Body->AddChild(OuterBox);

	// --- Mouse / look (sensitivity + invert), persisted on the player character. ---
	AddSectionHeader(OuterBox, NSLOCTEXT("ZonefallSettings", "SecMouse", "MOUSE & LOOK"));

	const AZonefallPlayerCharacter* ReadFrom = ResolvePlayerCharacter();
	if (!ReadFrom) { ReadFrom = GetDefault<AZonefallPlayerCharacter>(); }
	const float CurrentSens = ReadFrom ? ReadFrom->LookSensitivity : 1.0f;
	const bool bCurrentInvert = ReadFrom ? ReadFrom->bInvertLookY : false;

	SensitivitySlider = AddSliderRow(OuterBox, NSLOCTEXT("ZonefallSettings", "Sensitivity", "Mouse Sensitivity"), 0.1f, 4.0f, CurrentSens, TEXT("SensVal"));
	InvertYCheck = AddCheckRow(OuterBox, NSLOCTEXT("ZonefallSettings", "InvertY", "Invert Look (Y)"), bCurrentInvert);
	if (InvertYCheck)
	{
		InvertYCheck->OnCheckStateChanged.AddDynamic(this, &UZonefallMasterSettingsWidget::HandleInvertYChanged);
	}

	// --- Key bindings ---
	AddSectionHeader(OuterBox, NSLOCTEXT("ZonefallSettings", "SecKeys", "KEY BINDINGS"));
	ControlsBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("ControlsVBox"));
	OuterBox->AddChildToVerticalBox(ControlsBox);

	RebuildControlsRows();
}

void UZonefallMasterSettingsWidget::RebuildControlsRows()
{
	if (!ControlsBox)
	{
		return;
	}

	ControlsBox->ClearChildren();
	KeybindButtons.Reset();

	// Read bindings from the live pawn if present, otherwise from the class defaults.
	const AZonefallPlayerCharacter* Source = ResolvePlayerCharacter();
	const AZonefallPlayerCharacter* ReadFrom = Source ? Source : GetDefault<AZonefallPlayerCharacter>();
	if (!ReadFrom)
	{
		ControlsBox->AddChildToVerticalBox(MakeSetText(WidgetTree, NAME_None,
			NSLOCTEXT("ZonefallSettings", "NoControls", "Controls unavailable."), BodyFontSize, FLinearColor::White));
		return;
	}

	TArray<FName> Ids;
	TArray<FText> Names;
	ReadFrom->GetBindableActions(Ids, Names);

	if (!Source)
	{
		ControlsBox->AddChildToVerticalBox(MakeSetText(WidgetTree, NAME_None,
			NSLOCTEXT("ZonefallSettings", "RebindInGame", "Showing defaults. Open settings in-game (pause) to rebind."),
			BodyFontSize - 2, FLinearColor(1.0f, 0.82f, 0.4f, 1.0f)));
	}

	for (int32 i = 0; i < Ids.Num(); ++i)
	{
		UHorizontalBox* Row = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());

		UTextBlock* LabelText = MakeSetText(WidgetTree, NAME_None, Names[i], BodyFontSize, FLinearColor(0.85f, 0.92f, 0.98f, 1.0f));
		if (UHorizontalBoxSlot* LS = Row->AddChildToHorizontalBox(LabelText))
		{
			LS->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
			LS->SetVerticalAlignment(VAlign_Center);
		}

		const FKey Key = ReadFrom->GetActionKey(Ids[i]);
		const FText KeyLabel = Key.IsValid() ? Key.GetDisplayName() : NSLOCTEXT("ZonefallSettings", "Unbound", "—");

		UZonefallKeybindButton* Btn = WidgetTree->ConstructWidget<UZonefallKeybindButton>(UZonefallKeybindButton::StaticClass());
		Btn->ActionId = Ids[i];
		StyleSetButton(Btn, ZGlass::Row, ZGlass::RowHover, AccentColor * 0.6f);
		UTextBlock* BtnLabel = MakeSetText(WidgetTree, NAME_None, KeyLabel, BodyFontSize, FLinearColor::White);
		Btn->AddChild(BtnLabel);
		if (UButtonSlot* BSlot = Cast<UButtonSlot>(BtnLabel->Slot))
		{
			BSlot->SetHorizontalAlignment(HAlign_Center);
			BSlot->SetVerticalAlignment(VAlign_Center);
			BSlot->SetPadding(FMargin(18, 6));
		}
		Btn->SetIsEnabled(Source != nullptr);
		Btn->OnKeybindClicked.AddDynamic(this, &UZonefallMasterSettingsWidget::HandleKeybindClicked);
		if (UHorizontalBoxSlot* BS = Row->AddChildToHorizontalBox(Btn))
		{
			BS->SetVerticalAlignment(VAlign_Center);
		}

		KeybindButtons.Add(Btn);

		if (UVerticalBoxSlot* RowSlot = ControlsBox->AddChildToVerticalBox(Row))
		{
			RowSlot->SetPadding(FMargin(0, 0, 0, 8));
		}
	}
}

void UZonefallMasterSettingsWidget::ShowTab(int32 Index)
{
	if (BodySwitcher)
	{
		BodySwitcher->SetActiveWidgetIndex(FMath::Clamp(Index, 0, 3));
	}

	auto StyleTab = [this](UButton* Tab, bool bActive)
	{
		if (!Tab)
		{
			return;
		}
		StyleSetButton(Tab, bActive ? AccentColor * 0.5f : ZGlass::Row, bActive ? AccentColor * 0.6f : ZGlass::RowHover, AccentColor * 0.75f);

		// Brighten the active tab's label.
		if (WidgetTree)
		{
			const FName LabelName(*(Tab->GetName() + TEXT("_Lbl")));
			if (UTextBlock* Lbl = Cast<UTextBlock>(WidgetTree->FindWidget(LabelName)))
			{
				Lbl->SetColorAndOpacity(FSlateColor(bActive ? ZGlass::TextPrimary : ZGlass::TextSecondary));
			}
		}
	};
	StyleTab(TabGraphicsButton, Index == 0);
	StyleTab(TabAudioButton, Index == 1);
	StyleTab(TabControlsButton, Index == 2);
	StyleTab(TabLanguageButton, Index == 3);

	if (Index == 2)
	{
		RebuildControlsRows();
	}
}

void UZonefallMasterSettingsWidget::HandleTabGraphics() { ShowTab(0); }
void UZonefallMasterSettingsWidget::HandleTabAudio() { ShowTab(1); }
void UZonefallMasterSettingsWidget::HandleTabControls() { ShowTab(2); }
void UZonefallMasterSettingsWidget::HandleTabLanguage() { ShowTab(3); }

void UZonefallMasterSettingsWidget::BuildLanguageTab(UScrollBox* Body)
{
	if (!Body || !WidgetTree)
	{
		return;
	}

	UVerticalBox* Box = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("LanguageBox"));
	Body->AddChild(Box);

	UZonefallLocalizationSubsystem* Loc = UZonefallLocalizationSubsystem::Get(this);

	const FText LanguageHeader = Loc ? Loc->GetText(TEXT("settings.language")) : NSLOCTEXT("ZonefallSettings", "Language", "LANGUAGE");
	AddSectionHeader(Box, LanguageHeader);

	const FText LanguageLabel = Loc ? Loc->GetText(TEXT("settings.language.label")) : NSLOCTEXT("ZonefallSettings", "LanguageLabel", "Language");

	TArray<FString> Options = Loc ? Loc->GetLanguageDisplayNames() : TArray<FString>{ TEXT("English"), TEXT("Русский"), TEXT("中文") };
	FString Selected = Options.IsValidIndex(0) ? Options[0] : TEXT("English");
	if (Loc && Options.IsValidIndex((int32)Loc->GetLanguage()))
	{
		Selected = Options[(int32)Loc->GetLanguage()];
	}

	LanguageCombo = AddComboRow(Box, LanguageLabel, Options, Selected);
	if (LanguageCombo)
	{
		LanguageCombo->OnSelectionChanged.AddDynamic(this, &UZonefallMasterSettingsWidget::HandleLanguageComboChanged);
	}

	// Gamepad hint — controls work with a controller here too.
	const FText GamepadHint = Loc ? Loc->GetText(TEXT("controls.gamepad")) : NSLOCTEXT("ZonefallSettings", "GamepadSupported", "Gamepad supported");
	AddSectionHeader(Box, GamepadHint);
}

void UZonefallMasterSettingsWidget::HandleLanguageComboChanged(FString SelectedItem, ESelectInfo::Type SelectionType)
{
	if (SelectionType == ESelectInfo::Direct)
	{
		return; // Programmatic set during build — ignore.
	}

	UZonefallLocalizationSubsystem* Loc = UZonefallLocalizationSubsystem::Get(this);
	if (!Loc || !LanguageCombo)
	{
		return;
	}

	// Map the chosen display name back to a language by its index in the combo.
	const int32 Index = LanguageCombo->FindOptionIndex(SelectedItem);
	const EZonefallLanguage NewLang = static_cast<EZonefallLanguage>(FMath::Clamp(Index, 0, 2));
	Loc->SetLanguage(NewLang);
	// SetLanguage broadcasts OnLanguageChanged; the game instance rebuilds this menu in the new language.
}

void UZonefallMasterSettingsWidget::HandlePresetChanged(FString SelectedItem, ESelectInfo::Type SelectionType)
{
	// Ignore programmatic changes (e.g. RefreshFromSettings) to avoid feedback loops.
	if (SelectionType == ESelectInfo::Direct || !SettingsObject)
	{
		return;
	}

	EZonefallGraphicsPreset Preset = EZonefallGraphicsPreset::Balanced;
	if (SelectedItem == TEXT("Competitive")) { Preset = EZonefallGraphicsPreset::Competitive; }
	else if (SelectedItem == TEXT("Quality")) { Preset = EZonefallGraphicsPreset::Quality; }
	else if (SelectedItem == TEXT("Ultra")) { Preset = EZonefallGraphicsPreset::Ultra; }
	else if (SelectedItem == TEXT("Auto Detect")) { Preset = SettingsObject->DetectRecommendedPreset(); }

	SettingsObject->ApplyGraphicsPreset(Preset);

	// Reflect the preset's values back into all the individual combos/sliders.
	RefreshFromSettings();
	SetStatus(FString::Printf(TEXT("Applied \"%s\" preset — press APPLY to commit."), *SelectedItem), true);
}

void UZonefallMasterSettingsWidget::RefreshSliderValueLabels()
{
	for (const TPair<TObjectPtr<USlider>, TObjectPtr<UTextBlock>>& Pair : SliderValueLabels)
	{
		USlider* Slider = Pair.Key;
		UTextBlock* Label = Pair.Value;
		if (!Slider || !Label)
		{
			continue;
		}
		const float V = Slider->GetValue();
		const float MaxV = Slider->GetMaxValue();
		FString Str;
		if (MaxV <= 1.0f)
		{
			Str = FString::Printf(TEXT("%d%%"), FMath::RoundToInt(V * 100.0f));
		}
		else if (MaxV <= 5.5f)
		{
			Str = FString::Printf(TEXT("%.2f"), V);
		}
		else
		{
			Str = FString::Printf(TEXT("%d"), FMath::RoundToInt(V));
		}
		Label->SetText(FText::FromString(Str));
	}
}

void UZonefallMasterSettingsWidget::HandleSliderChanged(float /*Value*/)
{
	RefreshSliderValueLabels();

	// Apply mouse sensitivity live so the player feels it immediately (persisted on APPLY).
	if (SensitivitySlider)
	{
		if (AZonefallPlayerCharacter* Character = ResolvePlayerCharacter())
		{
			Character->LookSensitivity = SensitivitySlider->GetValue();
		}
	}
}

void UZonefallMasterSettingsWidget::HandleInvertYChanged(bool bIsChecked)
{
	if (AZonefallPlayerCharacter* Character = ResolvePlayerCharacter())
	{
		Character->bInvertLookY = bIsChecked;
	}
}

void UZonefallMasterSettingsWidget::RefreshFromSettings()
{
	if (!SettingsObject)
	{
		return;
	}
	SettingsObject->LoadFromSystem();

	auto SetCombo = [](UComboBoxString* Combo, const FString& Value)
	{
		if (Combo && !Value.IsEmpty())
		{
			if (Combo->FindOptionIndex(Value) == INDEX_NONE) { Combo->AddOption(Value); }
			Combo->SetSelectedOption(Value);
		}
	};

	SetCombo(DisplayModeCombo, SettingsObject->DisplayMode);
	SetCombo(ResolutionCombo, SettingsObject->ScreenResolution.IsEmpty() ? SettingsObject->GetCurrentScreenResolutionString() : SettingsObject->ScreenResolution);
	SetCombo(QualityCombo, SettingsObject->OverallQuality);
	SetCombo(ResScaleCombo, SettingsObject->ResolutionScale);
	SetCombo(VSyncCombo, SettingsObject->VSync);
	SetCombo(FPSLimitCombo, SettingsObject->FPSLimit);
	SetCombo(LumenCombo, SettingsObject->Lumen);
	SetCombo(CloudsCombo, SettingsObject->VolumetricClouds);
	SetCombo(DLSSCombo, SettingsObject->DLSSMode);
	SetCombo(FrameGenCombo, SettingsObject->FrameGeneration);
	SetCombo(FSRCombo, SettingsObject->FSRMode);
	SetCombo(FSRFrameGenCombo, SettingsObject->FSRFrameGeneration);
	SetCombo(DirectXCombo, SettingsObject->DirectXVersion);
	SetCombo(RayTracingCombo, SettingsObject->RayTracing);

	for (int32 i = 0; i < AdvancedQualityCombos.Num(); ++i)
	{
		if (AdvancedQualityGroups.IsValidIndex(i))
		{
			SetCombo(AdvancedQualityCombos[i], SettingsObject->AdvancedQuality.FindRef(AdvancedQualityGroups[i]));
		}
	}

	if (BrightnessSlider) { BrightnessSlider->SetValue(SettingsObject->Brightness); }
	if (FOVSlider) { FOVSlider->SetValue(SettingsObject->FieldOfView); }
	if (MasterSlider) { MasterSlider->SetValue(SettingsObject->MasterVolume); }
	if (SfxSlider) { SfxSlider->SetValue(SettingsObject->SfxVolume); }
	if (MusicSlider) { MusicSlider->SetValue(SettingsObject->MusicVolume); }
	if (VoiceSlider) { VoiceSlider->SetValue(SettingsObject->VoiceVolume); }

	RefreshSliderValueLabels();
}

void UZonefallMasterSettingsWidget::ApplyNow()
{
	if (!SettingsObject)
	{
		return;
	}

	auto ReadCombo = [](UComboBoxString* Combo, FString& Out)
	{
		if (Combo)
		{
			const FString Sel = Combo->GetSelectedOption();
			if (!Sel.IsEmpty()) { Out = Sel; }
		}
	};

	ReadCombo(DisplayModeCombo, SettingsObject->DisplayMode);
	ReadCombo(ResolutionCombo, SettingsObject->ScreenResolution);
	ReadCombo(QualityCombo, SettingsObject->OverallQuality);
	ReadCombo(ResScaleCombo, SettingsObject->ResolutionScale);
	ReadCombo(VSyncCombo, SettingsObject->VSync);
	ReadCombo(FPSLimitCombo, SettingsObject->FPSLimit);
	ReadCombo(LumenCombo, SettingsObject->Lumen);
	ReadCombo(CloudsCombo, SettingsObject->VolumetricClouds);
	ReadCombo(DLSSCombo, SettingsObject->DLSSMode);
	ReadCombo(FrameGenCombo, SettingsObject->FrameGeneration);
	ReadCombo(FSRCombo, SettingsObject->FSRMode);
	ReadCombo(FSRFrameGenCombo, SettingsObject->FSRFrameGeneration);
	const FString PrevDX = SettingsObject->DirectXVersion;
	ReadCombo(DirectXCombo, SettingsObject->DirectXVersion);
	ReadCombo(RayTracingCombo, SettingsObject->RayTracing);
	const bool bDirectXChanged = !PrevDX.IsEmpty() && !SettingsObject->DirectXVersion.IsEmpty() && (PrevDX != SettingsObject->DirectXVersion);

	for (int32 i = 0; i < AdvancedQualityCombos.Num(); ++i)
	{
		if (AdvancedQualityCombos[i] && AdvancedQualityGroups.IsValidIndex(i))
		{
			const FString Sel = AdvancedQualityCombos[i]->GetSelectedOption();
			if (!Sel.IsEmpty())
			{
				SettingsObject->AdvancedQuality.Add(AdvancedQualityGroups[i], Sel);
			}
		}
	}

	if (BrightnessSlider) { SettingsObject->Brightness = BrightnessSlider->GetValue(); }
	if (FOVSlider) { SettingsObject->FieldOfView = FOVSlider->GetValue(); }
	if (MasterSlider) { SettingsObject->MasterVolume = MasterSlider->GetValue(); }
	if (SfxSlider) { SettingsObject->SfxVolume = SfxSlider->GetValue(); }
	if (MusicSlider) { SettingsObject->MusicVolume = MusicSlider->GetValue(); }
	if (VoiceSlider) { SettingsObject->VoiceVolume = VoiceSlider->GetValue(); }

	SettingsObject->SanitizeSettings();
	SettingsObject->ApplyToSystem(this);

	// Persist look/mouse settings on the player character (FKey/sensitivity props are Config).
	if (AZonefallPlayerCharacter* Character = ResolvePlayerCharacter())
	{
		if (SensitivitySlider) { Character->LookSensitivity = SensitivitySlider->GetValue(); }
		if (InvertYCheck) { Character->bInvertLookY = InvertYCheck->IsChecked(); }
		Character->SaveConfig();
	}

	SetStatus(TEXT("Settings applied."), true);

	// DirectX (RHI) can't switch live — offer a restart.
	if (bDirectXChanged)
	{
		if (UZonefallConfirmDialogWidget* Dialog = CreateWidget<UZonefallConfirmDialogWidget>(GetOwningPlayer(), UZonefallConfirmDialogWidget::StaticClass()))
		{
			Dialog->AddToViewport(10000);
			Dialog->Setup(
				NSLOCTEXT("ZonefallSettings", "RestartTitle", "RESTART REQUIRED"),
				FText::Format(NSLOCTEXT("ZonefallSettings", "RestartMsg", "{0} will be used after a restart. Restart the game now?"),
					FText::FromString(SettingsObject->DirectXVersion)),
				NSLOCTEXT("ZonefallSettings", "RestartNow", "RESTART NOW"),
				NSLOCTEXT("ZonefallSettings", "RestartLater", "LATER"));
			Dialog->OnConfirmed.AddDynamic(this, &UZonefallMasterSettingsWidget::HandleRestartConfirmed);
		}
		SetStatus(TEXT("DirectX change will apply after restart."), true);
	}
}

void UZonefallMasterSettingsWidget::HandleRestartConfirmed()
{
	if (UUIWorldMenuGameInstance* GI = GetGameInstance<UUIWorldMenuGameInstance>())
	{
		// RHI preference is already written to config; quitting lets the user relaunch with it.
		GI->QuitGameNow(false);
	}
}

void UZonefallMasterSettingsWidget::ResetToDefaults()
{
	if (!SettingsObject)
	{
		return;
	}
	SettingsObject->SetDefaults();
	RefreshFromSettings();
	SetStatus(TEXT("Reset to defaults."), true);
}

void UZonefallMasterSettingsWidget::SetStatus(const FString& Message, bool bSuccess)
{
	if (StatusText)
	{
		StatusText->SetColorAndOpacity(FSlateColor(bSuccess ? FLinearColor(0.7f, 0.95f, 0.8f, 1.0f) : FLinearColor(1.0f, 0.55f, 0.55f, 1.0f)));
		StatusText->SetText(FText::FromString(Message));
	}
}

void UZonefallMasterSettingsWidget::HandleApplyClicked() { ApplyNow(); }
void UZonefallMasterSettingsWidget::HandleResetClicked() { ResetToDefaults(); }

void UZonefallMasterSettingsWidget::HandleBackClicked()
{
	OnBackRequested.Broadcast();

	// Fallback so BACK works out of the box when shown via the menu flow.
	if (!OnBackRequested.IsBound())
	{
		if (UUIWorldMenuGameInstance* GI = GetGameInstance<UUIWorldMenuGameInstance>())
		{
			GI->BackFromSettingsMenuSmart(false);
		}
	}
}

void UZonefallMasterSettingsWidget::HandleKeybindClicked(FName ActionId)
{
	if (!ResolvePlayerCharacter())
	{
		SetStatus(TEXT("Rebinding is only available in-game (pause menu)."), false);
		return;
	}

	bListeningForKey = true;
	ListeningActionId = ActionId;
	SetStatus(TEXT("Press a key to bind...  (Esc to cancel)"), true);
	SetKeyboardFocus();
}

FReply UZonefallMasterSettingsWidget::NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
	if (bListeningForKey)
	{
		const FKey Pressed = InKeyEvent.GetKey();
		bListeningForKey = false;

		if (Pressed == EKeys::Escape)
		{
			SetStatus(TEXT("Rebind cancelled."), true);
			RebuildControlsRows();
			return FReply::Handled();
		}

		if (AZonefallPlayerCharacter* Character = ResolvePlayerCharacter())
		{
			Character->SetActionKey(ListeningActionId, Pressed);
			SetStatus(FString::Printf(TEXT("Bound to %s."), *Pressed.GetDisplayName().ToString()), true);
		}
		RebuildControlsRows();
		return FReply::Handled();
	}

	return Super::NativeOnKeyDown(InGeometry, InKeyEvent);
}

FReply UZonefallMasterSettingsWidget::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (bListeningForKey)
	{
		const FKey Pressed = InMouseEvent.GetEffectingButton();
		bListeningForKey = false;

		if (AZonefallPlayerCharacter* Character = ResolvePlayerCharacter())
		{
			Character->SetActionKey(ListeningActionId, Pressed);
			SetStatus(FString::Printf(TEXT("Bound to %s."), *Pressed.GetDisplayName().ToString()), true);
		}
		RebuildControlsRows();
		return FReply::Handled();
	}

	return Super::NativeOnMouseButtonDown(InGeometry, InMouseEvent);
}
