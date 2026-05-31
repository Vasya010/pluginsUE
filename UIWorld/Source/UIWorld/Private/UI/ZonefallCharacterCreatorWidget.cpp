#include "UI/ZonefallCharacterCreatorWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Brushes/SlateRoundedBoxBrush.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/ButtonSlot.h"
#include "Components/EditableTextBox.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Image.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/ScrollBox.h"
#include "Components/SizeBox.h"
#include "Components/Slider.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Engine/Font.h"
#include "Engine/TextureRenderTarget2D.h"

#include "Character/ZonefallPlayerCharacter.h"
#include "World/ZonefallCharacterPreviewCapture.h"
#include "UIWorldMenuGameInstance.h"

namespace
{
	enum
	{
		ActBodyPrev = 0, ActBodyNext, ActFacePrev, ActFaceNext, ActHairPrev, ActHairNext,
		ActSkin, ActHairColor, ActPrimary, ActSecondary, ActRandomize, ActReset,
		ActBack, ActCreate, ActRotL, ActRotR
	};

	namespace ZCC
	{
		static const FLinearColor Glass = FLinearColor(0.02f, 0.04f, 0.07f, 0.92f);
		static const FLinearColor GlassDeep = FLinearColor(0.01f, 0.02f, 0.04f, 0.96f);
		static const FLinearColor Outline = FLinearColor(1.0f, 1.0f, 1.0f, 0.14f);
		static const FLinearColor TextHi = FLinearColor(0.95f, 0.97f, 1.0f, 1.0f);
		static const FLinearColor TextLo = FLinearColor(0.62f, 0.70f, 0.80f, 1.0f);
	}

	FSlateFontInfo CcFont(int32 Size, bool bBold = false)
	{
		FSlateFontInfo Font;
		Font.Size = FMath::Clamp(Size, 8, 64);
		Font.TypefaceFontName = bBold ? FName(TEXT("Bold")) : FName(TEXT("Regular"));
		Font.FontObject = LoadObject<UObject>(nullptr, TEXT("/Engine/EngineFonts/Roboto.Roboto"));
		return Font;
	}
}

UZonefallCreatorButton::UZonefallCreatorButton()
{
	OnClicked.AddDynamic(this, &UZonefallCreatorButton::HandleClicked);
}

void UZonefallCreatorButton::HandleClicked()
{
	OnCreatorAction.Broadcast(ActionId, Param);
}

UZonefallCharacterCreatorWidget::UZonefallCharacterCreatorWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	SkinPalette = {
		FLinearColor(0.96f, 0.80f, 0.69f), FLinearColor(0.86f, 0.66f, 0.52f), FLinearColor(0.72f, 0.52f, 0.39f),
		FLinearColor(0.55f, 0.38f, 0.27f), FLinearColor(0.38f, 0.25f, 0.17f), FLinearColor(0.27f, 0.17f, 0.11f)
	};
	HairPalette = {
		FLinearColor(0.04f, 0.03f, 0.02f), FLinearColor(0.20f, 0.12f, 0.06f), FLinearColor(0.45f, 0.28f, 0.12f),
		FLinearColor(0.78f, 0.62f, 0.34f), FLinearColor(0.66f, 0.66f, 0.68f), FLinearColor(0.55f, 0.12f, 0.10f)
	};
	ClothPalette = {
		FLinearColor(0.12f, 0.14f, 0.18f), FLinearColor(0.18f, 0.22f, 0.30f), FLinearColor(0.10f, 0.28f, 0.24f),
		FLinearColor(0.35f, 0.10f, 0.12f), FLinearColor(0.45f, 0.32f, 0.16f), FLinearColor(0.30f, 0.22f, 0.40f),
		FLinearColor(0.70f, 0.55f, 0.20f), FLinearColor(0.75f, 0.76f, 0.80f)
	};
}

TSharedRef<SWidget> UZonefallCharacterCreatorWidget::RebuildWidget()
{
	if (!WidgetTree || !WidgetTree->RootWidget)
	{
		BuildLayout();
	}
	return Super::RebuildWidget();
}

void UZonefallCharacterCreatorWidget::SetupForCharacter(AZonefallPlayerCharacter* InCharacter)
{
	Character = InCharacter;
	PreviewCapture = nullptr;
	bPreviewBrushSet = false;
	if (Character)
	{
		Working = Character->GetAppearance();
		BodyOptionCount = FMath::Max(1, Character->BodyMeshOptions.Num());
	}
	if (Working.CharacterName.IsEmpty())
	{
		Working.CharacterName = TEXT("Drifter");
	}

	// If the layout already exists, push the current values into it.
	if (NameBox)
	{
		NameBox->SetText(FText::FromString(Working.CharacterName));
		if (HeightSlider) { HeightSlider->SetValue(Working.Height); }
		if (BuildSlider) { BuildSlider->SetValue(Working.Build); }
		RefreshDynamicLabels();
	}
	ApplyToPreview();
}

UZonefallCreatorButton* UZonefallCharacterCreatorWidget::MakeButton(FName Name, const FText& Label, int32 ActionId, int32 Param, int32 FontSize)
{
	UZonefallCreatorButton* Btn = WidgetTree->ConstructWidget<UZonefallCreatorButton>(UZonefallCreatorButton::StaticClass(), Name);
	Btn->ActionId = ActionId;
	Btn->Param = Param;
	{
		FButtonStyle Style = Btn->GetStyle();
		Style.Normal = FSlateRoundedBoxBrush(FLinearColor(1, 1, 1, 0.05f), 8.0f, ZCC::Outline, 1.0f);
		Style.Hovered = FSlateRoundedBoxBrush(AccentColor * FLinearColor(1, 1, 1, 0.30f), 8.0f);
		Style.Pressed = FSlateRoundedBoxBrush(AccentColor * FLinearColor(1, 1, 1, 0.45f), 8.0f);
		Btn->SetStyle(Style);
	}
	UTextBlock* Txt = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	Txt->SetText(Label);
	Txt->SetFont(CcFont(FontSize, true));
	Txt->SetColorAndOpacity(FSlateColor(ZCC::TextHi));
	Txt->SetJustification(ETextJustify::Center);
	Btn->AddChild(Txt);
	if (UButtonSlot* BSlot = Cast<UButtonSlot>(Txt->Slot))
	{
		BSlot->SetHorizontalAlignment(HAlign_Center);
		BSlot->SetVerticalAlignment(VAlign_Center);
		BSlot->SetPadding(FMargin(14.0f, 8.0f));
	}
	Btn->OnCreatorAction.AddDynamic(this, &UZonefallCharacterCreatorWidget::HandleAction);
	return Btn;
}

void UZonefallCharacterCreatorWidget::AddSection(UVerticalBox* Col, const FText& Title)
{
	UTextBlock* T = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	T->SetText(Title);
	T->SetFont(CcFont(12, true));
	T->SetColorAndOpacity(FSlateColor(AccentColor));
	if (UVerticalBoxSlot* S = Col->AddChildToVerticalBox(T))
	{
		S->SetPadding(FMargin(0.0f, 16.0f, 0.0f, 6.0f));
	}
}

UTextBlock* UZonefallCharacterCreatorWidget::AddSelectorRow(UVerticalBox* Col, const FText& Label, int32 PrevAction, int32 NextAction)
{
	UHorizontalBox* Row = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());

	UTextBlock* Lbl = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	Lbl->SetText(Label);
	Lbl->SetFont(CcFont(11));
	Lbl->SetColorAndOpacity(FSlateColor(ZCC::TextLo));
	if (UHorizontalBoxSlot* LS = Row->AddChildToHorizontalBox(Lbl))
	{
		LS->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
		LS->SetVerticalAlignment(VAlign_Center);
	}

	UZonefallCreatorButton* Prev = MakeButton(NAME_None, FText::FromString(TEXT("<")), PrevAction, 0, 12);
	Row->AddChildToHorizontalBox(Prev);

	UTextBlock* Value = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	Value->SetText(FText::FromString(TEXT("1")));
	Value->SetFont(CcFont(12, true));
	Value->SetColorAndOpacity(FSlateColor(ZCC::TextHi));
	Value->SetJustification(ETextJustify::Center);
	USizeBox* ValBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
	ValBox->SetWidthOverride(54.0f);
	ValBox->AddChild(Value);
	if (UHorizontalBoxSlot* VS = Row->AddChildToHorizontalBox(ValBox))
	{
		VS->SetVerticalAlignment(VAlign_Center);
	}

	UZonefallCreatorButton* Next = MakeButton(NAME_None, FText::FromString(TEXT(">")), NextAction, 0, 12);
	Row->AddChildToHorizontalBox(Next);

	if (UVerticalBoxSlot* RS = Col->AddChildToVerticalBox(Row))
	{
		RS->SetPadding(FMargin(0.0f, 4.0f));
	}
	return Value;
}

void UZonefallCharacterCreatorWidget::AddSwatchRow(UVerticalBox* Col, const FText& Label, int32 SwatchAction, const TArray<FLinearColor>& Palette)
{
	UTextBlock* Lbl = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	Lbl->SetText(Label);
	Lbl->SetFont(CcFont(11));
	Lbl->SetColorAndOpacity(FSlateColor(ZCC::TextLo));
	if (UVerticalBoxSlot* LS = Col->AddChildToVerticalBox(Lbl))
	{
		LS->SetPadding(FMargin(0.0f, 8.0f, 0.0f, 4.0f));
	}

	UHorizontalBox* Row = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
	for (int32 i = 0; i < Palette.Num(); ++i)
	{
		UZonefallCreatorButton* Sw = WidgetTree->ConstructWidget<UZonefallCreatorButton>(UZonefallCreatorButton::StaticClass());
		Sw->ActionId = SwatchAction;
		Sw->Param = i;
		FButtonStyle Style = Sw->GetStyle();
		Style.Normal = FSlateRoundedBoxBrush(Palette[i], 6.0f, ZCC::Outline, 1.0f);
		Style.Hovered = FSlateRoundedBoxBrush(Palette[i], 6.0f, AccentColor, 2.0f);
		Style.Pressed = FSlateRoundedBoxBrush(Palette[i], 6.0f, AccentColor, 2.5f);
		Sw->SetStyle(Style);
		Sw->OnCreatorAction.AddDynamic(this, &UZonefallCharacterCreatorWidget::HandleAction);

		USizeBox* Box = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
		Box->SetWidthOverride(30.0f);
		Box->SetHeightOverride(30.0f);
		Box->AddChild(Sw);
		if (UHorizontalBoxSlot* SS = Row->AddChildToHorizontalBox(Box))
		{
			SS->SetPadding(FMargin(0.0f, 0.0f, 6.0f, 0.0f));
		}
	}
	Col->AddChildToVerticalBox(Row);
}

USlider* UZonefallCharacterCreatorWidget::AddSliderRow(UVerticalBox* Col, const FText& Label, float MinV, float MaxV, float Value)
{
	UHorizontalBox* Row = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());

	UTextBlock* Lbl = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	Lbl->SetText(Label);
	Lbl->SetFont(CcFont(11));
	Lbl->SetColorAndOpacity(FSlateColor(ZCC::TextLo));
	USizeBox* LblBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
	LblBox->SetWidthOverride(80.0f);
	LblBox->AddChild(Lbl);
	if (UHorizontalBoxSlot* LS = Row->AddChildToHorizontalBox(LblBox))
	{
		LS->SetVerticalAlignment(VAlign_Center);
	}

	USlider* Slider = WidgetTree->ConstructWidget<USlider>(USlider::StaticClass());
	Slider->SetMinValue(MinV);
	Slider->SetMaxValue(MaxV);
	Slider->SetValue(Value);
	Slider->SetSliderHandleColor(AccentColor);
	Slider->SetSliderBarColor(FLinearColor(1, 1, 1, 0.18f));
	if (UHorizontalBoxSlot* SS = Row->AddChildToHorizontalBox(Slider))
	{
		SS->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
		SS->SetVerticalAlignment(VAlign_Center);
	}

	if (UVerticalBoxSlot* RS = Col->AddChildToVerticalBox(Row))
	{
		RS->SetPadding(FMargin(0.0f, 4.0f));
	}
	return Slider;
}

void UZonefallCharacterCreatorWidget::BuildLayout()
{
	if (!WidgetTree)
	{
		return;
	}
	WidgetTree->RootWidget = nullptr;

	UOverlay* Root = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass(), TEXT("CreatorRoot"));
	WidgetTree->RootWidget = Root;

	// Dim full-screen backdrop.
	UBorder* Dim = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("CreatorDim"));
	Dim->SetBrush(FSlateRoundedBoxBrush(FLinearColor(0.0f, 0.0f, 0.0f, 0.75f), 0.0f));
	if (UOverlaySlot* DS = Root->AddChildToOverlay(Dim))
	{
		DS->SetHorizontalAlignment(HAlign_Fill);
		DS->SetVerticalAlignment(VAlign_Fill);
	}

	UHorizontalBox* MainRow = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("CreatorMainRow"));
	if (UOverlaySlot* MS = Root->AddChildToOverlay(MainRow))
	{
		MS->SetHorizontalAlignment(HAlign_Fill);
		MS->SetVerticalAlignment(VAlign_Fill);
		MS->SetPadding(FMargin(60.0f, 50.0f));
	}

	// ---------------- Left: live preview ----------------
	{
		UOverlay* PreviewOverlay = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass(), TEXT("PreviewOverlay"));

		PreviewImage = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("PreviewImage"));
		PreviewImage->SetBrush(FSlateRoundedBoxBrush(FLinearColor(0.04f, 0.06f, 0.09f, 1.0f), 10.0f));
		if (UOverlaySlot* PS = PreviewOverlay->AddChildToOverlay(PreviewImage))
		{
			PS->SetHorizontalAlignment(HAlign_Fill);
			PS->SetVerticalAlignment(VAlign_Fill);
		}

		// Rotate controls at the bottom.
		UHorizontalBox* RotateRow = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("RotateRow"));
		RotateRow->AddChildToHorizontalBox(MakeButton(TEXT("RotL"), FText::FromString(TEXT("◀  ROTATE")), ActRotL, 0, 12));
		UZonefallCreatorButton* RotR = MakeButton(TEXT("RotR"), FText::FromString(TEXT("ROTATE  ▶")), ActRotR, 0, 12);
		if (UHorizontalBoxSlot* RR = RotateRow->AddChildToHorizontalBox(RotR))
		{
			RR->SetPadding(FMargin(10.0f, 0.0f, 0.0f, 0.0f));
		}
		if (UOverlaySlot* RS = PreviewOverlay->AddChildToOverlay(RotateRow))
		{
			RS->SetHorizontalAlignment(HAlign_Center);
			RS->SetVerticalAlignment(VAlign_Bottom);
			RS->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 18.0f));
		}

		UBorder* PreviewPanel = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("PreviewPanel"));
		PreviewPanel->SetBrush(FSlateRoundedBoxBrush(ZCC::GlassDeep, 12.0f, ZCC::Outline, 1.0f));
		PreviewPanel->SetPadding(FMargin(8.0f));
		PreviewPanel->SetContent(PreviewOverlay);

		if (UHorizontalBoxSlot* LSlot = MainRow->AddChildToHorizontalBox(PreviewPanel))
		{
			LSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
			LSlot->SetPadding(FMargin(0.0f, 0.0f, 18.0f, 0.0f));
		}
	}

	// ---------------- Right: form ----------------
	{
		UVerticalBox* RightCol = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("RightCol"));

		UTextBlock* Title = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("CreatorTitle"));
		Title->SetText(NSLOCTEXT("ZonefallCreator", "Title", "CREATE YOUR CHARACTER"));
		Title->SetFont(CcFont(22, true));
		Title->SetColorAndOpacity(FSlateColor(ZCC::TextHi));
		RightCol->AddChildToVerticalBox(Title);

		UScrollBox* Scroll = WidgetTree->ConstructWidget<UScrollBox>(UScrollBox::StaticClass(), TEXT("CreatorScroll"));
		UVerticalBox* Form = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("CreatorForm"));
		Scroll->AddChild(Form);

		// Identity.
		AddSection(Form, NSLOCTEXT("ZonefallCreator", "Identity", "IDENTITY"));
		NameBox = WidgetTree->ConstructWidget<UEditableTextBox>(UEditableTextBox::StaticClass(), TEXT("NameBox"));
		NameBox->SetHintText(NSLOCTEXT("ZonefallCreator", "NameHint", "Character name"));
		NameBox->SetText(FText::FromString(Working.CharacterName));
		NameBox->OnTextChanged.AddDynamic(this, &UZonefallCharacterCreatorWidget::HandleNameChanged);
		Form->AddChildToVerticalBox(NameBox);

		// Appearance selectors.
		AddSection(Form, NSLOCTEXT("ZonefallCreator", "Appearance", "APPEARANCE"));
		BodyValueText = AddSelectorRow(Form, NSLOCTEXT("ZonefallCreator", "Body", "Body type"), ActBodyPrev, ActBodyNext);
		FaceValueText = AddSelectorRow(Form, NSLOCTEXT("ZonefallCreator", "Face", "Face"), ActFacePrev, ActFaceNext);
		HairValueText = AddSelectorRow(Form, NSLOCTEXT("ZonefallCreator", "Hair", "Hair style"), ActHairPrev, ActHairNext);

		// Build sliders.
		AddSection(Form, NSLOCTEXT("ZonefallCreator", "Build", "BUILD"));
		HeightSlider = AddSliderRow(Form, NSLOCTEXT("ZonefallCreator", "Height", "Height"), 0.88f, 1.12f, Working.Height);
		HeightSlider->OnValueChanged.AddDynamic(this, &UZonefallCharacterCreatorWidget::HandleHeightChanged);
		BuildSlider = AddSliderRow(Form, NSLOCTEXT("ZonefallCreator", "BuildMass", "Build"), 0.85f, 1.20f, Working.Build);
		BuildSlider->OnValueChanged.AddDynamic(this, &UZonefallCharacterCreatorWidget::HandleBuildChanged);

		// Colours.
		AddSection(Form, NSLOCTEXT("ZonefallCreator", "Colours", "COLOURS"));
		AddSwatchRow(Form, NSLOCTEXT("ZonefallCreator", "Skin", "Skin tone"), ActSkin, SkinPalette);
		AddSwatchRow(Form, NSLOCTEXT("ZonefallCreator", "HairCol", "Hair colour"), ActHairColor, HairPalette);
		AddSwatchRow(Form, NSLOCTEXT("ZonefallCreator", "Primary", "Outfit primary"), ActPrimary, ClothPalette);
		AddSwatchRow(Form, NSLOCTEXT("ZonefallCreator", "Secondary", "Outfit secondary"), ActSecondary, ClothPalette);

		// Randomize / Reset.
		UHorizontalBox* RandRow = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
		RandRow->AddChildToHorizontalBox(MakeButton(TEXT("Randomize"), NSLOCTEXT("ZonefallCreator", "Randomize", "RANDOMIZE"), ActRandomize, 0, 12));
		UZonefallCreatorButton* ResetBtn = MakeButton(TEXT("Reset"), NSLOCTEXT("ZonefallCreator", "Reset", "RESET"), ActReset, 0, 12);
		if (UHorizontalBoxSlot* RBS = RandRow->AddChildToHorizontalBox(ResetBtn))
		{
			RBS->SetPadding(FMargin(10.0f, 0.0f, 0.0f, 0.0f));
		}
		if (UVerticalBoxSlot* RRS = Form->AddChildToVerticalBox(RandRow))
		{
			RRS->SetPadding(FMargin(0.0f, 18.0f, 0.0f, 0.0f));
		}

		if (UVerticalBoxSlot* ScrollSlot = RightCol->AddChildToVerticalBox(Scroll))
		{
			ScrollSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
			ScrollSlot->SetPadding(FMargin(0.0f, 10.0f));
		}

		// Bottom action bar (always visible).
		UHorizontalBox* BottomBar = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("CreatorBottomBar"));
		UZonefallCreatorButton* Back = MakeButton(TEXT("Back"), NSLOCTEXT("ZonefallCreator", "Back", "BACK"), ActBack, 0, 14);
		if (UHorizontalBoxSlot* BS = BottomBar->AddChildToHorizontalBox(Back))
		{
			BS->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
			BS->SetPadding(FMargin(0.0f, 0.0f, 10.0f, 0.0f));
		}
		UZonefallCreatorButton* Create = MakeButton(TEXT("Create"), NSLOCTEXT("ZonefallCreator", "Create", "CREATE & START"), ActCreate, 0, 14);
		{
			FButtonStyle Style = Create->GetStyle();
			Style.Normal = FSlateRoundedBoxBrush(AccentColor * FLinearColor(1, 1, 1, 0.85f), 8.0f);
			Style.Hovered = FSlateRoundedBoxBrush(AccentColor, 8.0f);
			Style.Pressed = FSlateRoundedBoxBrush(AccentColor * FLinearColor(0.85f, 0.85f, 0.85f, 1.0f), 8.0f);
			Create->SetStyle(Style);
		}
		if (UHorizontalBoxSlot* CS = BottomBar->AddChildToHorizontalBox(Create))
		{
			CS->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
		}
		RightCol->AddChildToVerticalBox(BottomBar);

		UBorder* RightPanel = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("RightPanel"));
		RightPanel->SetBrush(FSlateRoundedBoxBrush(ZCC::Glass, 12.0f, ZCC::Outline, 1.0f));
		RightPanel->SetPadding(FMargin(22.0f, 18.0f));
		RightPanel->SetContent(RightCol);

		USizeBox* RightSize = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("RightSize"));
		RightSize->SetWidthOverride(440.0f);
		RightSize->AddChild(RightPanel);

		if (UHorizontalBoxSlot* RSlot = MainRow->AddChildToHorizontalBox(RightSize))
		{
			RSlot->SetVerticalAlignment(VAlign_Fill);
		}
	}

	RefreshDynamicLabels();
	ApplyToPreview();
}

void UZonefallCharacterCreatorWidget::HandleAction(int32 ActionId, int32 Param)
{
	switch (ActionId)
	{
	case ActBodyPrev: Working.BodyType = (Working.BodyType - 1 + BodyOptionCount) % BodyOptionCount; break;
	case ActBodyNext: Working.BodyType = (Working.BodyType + 1) % BodyOptionCount; break;
	case ActFacePrev: Working.FaceType = (Working.FaceType - 1 + FaceCount) % FaceCount; break;
	case ActFaceNext: Working.FaceType = (Working.FaceType + 1) % FaceCount; break;
	case ActHairPrev: Working.HairStyle = (Working.HairStyle - 1 + HairCount) % HairCount; break;
	case ActHairNext: Working.HairStyle = (Working.HairStyle + 1) % HairCount; break;
	case ActSkin:      if (SkinPalette.IsValidIndex(Param))  { Working.SkinTone = SkinPalette[Param]; } break;
	case ActHairColor: if (HairPalette.IsValidIndex(Param))  { Working.HairColor = HairPalette[Param]; } break;
	case ActPrimary:   if (ClothPalette.IsValidIndex(Param)) { Working.PrimaryColor = ClothPalette[Param]; } break;
	case ActSecondary: if (ClothPalette.IsValidIndex(Param)) { Working.SecondaryColor = ClothPalette[Param]; } break;
	case ActRandomize: Randomize(); break;
	case ActReset:
		Working = FZonefallCharacterAppearance();
		if (NameBox) { NameBox->SetText(FText::FromString(Working.CharacterName)); }
		if (HeightSlider) { HeightSlider->SetValue(Working.Height); }
		if (BuildSlider) { BuildSlider->SetValue(Working.Build); }
		break;
	case ActRotL: if (PreviewCapture) { PreviewCapture->AddYaw(-25.0f); } return;
	case ActRotR: if (PreviewCapture) { PreviewCapture->AddYaw(25.0f); } return;
	case ActBack:
		if (Character)
		{
			Character->CloseCharacterCreatorUI();
		}
		if (UUIWorldMenuGameInstance* GI = GetWorld() ? Cast<UUIWorldMenuGameInstance>(GetWorld()->GetGameInstance()) : nullptr)
		{
			if (Character && Character->IsOnMenuMap())
			{
				GI->ShowMenuFromList(EUIWorldMenuScreen::MainMenu, false);
			}
			else
			{
				GI->LoadMainMenuLevel(true);
			}
		}
		return;
	case ActCreate:
		if (UUIWorldMenuGameInstance* GI = GetWorld() ? Cast<UUIWorldMenuGameInstance>(GetWorld()->GetGameInstance()) : nullptr)
		{
			GI->StartGameWithCreatedCharacter(Working);
		}
		return;
	default: break;
	}

	RefreshDynamicLabels();
	ApplyToPreview();
}

void UZonefallCharacterCreatorWidget::HandleNameChanged(const FText& Text)
{
	Working.CharacterName = Text.ToString();
}

void UZonefallCharacterCreatorWidget::HandleHeightChanged(float Value)
{
	Working.Height = Value;
	ApplyToPreview();
}

void UZonefallCharacterCreatorWidget::HandleBuildChanged(float Value)
{
	Working.Build = Value;
	ApplyToPreview();
}

void UZonefallCharacterCreatorWidget::Randomize()
{
	Working.BodyType = FMath::RandRange(0, BodyOptionCount - 1);
	Working.FaceType = FMath::RandRange(0, FaceCount - 1);
	Working.HairStyle = FMath::RandRange(0, HairCount - 1);
	Working.SkinTone = SkinPalette[FMath::RandRange(0, SkinPalette.Num() - 1)];
	Working.HairColor = HairPalette[FMath::RandRange(0, HairPalette.Num() - 1)];
	Working.PrimaryColor = ClothPalette[FMath::RandRange(0, ClothPalette.Num() - 1)];
	Working.SecondaryColor = ClothPalette[FMath::RandRange(0, ClothPalette.Num() - 1)];
	Working.Height = FMath::FRandRange(0.90f, 1.10f);
	Working.Build = FMath::FRandRange(0.88f, 1.16f);
	if (HeightSlider) { HeightSlider->SetValue(Working.Height); }
	if (BuildSlider) { BuildSlider->SetValue(Working.Build); }
}

void UZonefallCharacterCreatorWidget::ApplyToPreview()
{
	if (Character)
	{
		Character->ApplyAppearance(Working);
	}

	if (UUIWorldMenuGameInstance* GI = GetWorld() ? Cast<UUIWorldMenuGameInstance>(GetWorld()->GetGameInstance()) : nullptr)
	{
		GI->SetCurrentAppearance(Working);
	}
}

void UZonefallCharacterCreatorWidget::RefreshDynamicLabels()
{
	if (BodyValueText) { BodyValueText->SetText(FText::FromString(FString::Printf(TEXT("%d"), Working.BodyType + 1))); }
	if (FaceValueText) { FaceValueText->SetText(FText::FromString(FString::Printf(TEXT("%d"), Working.FaceType + 1))); }
	if (HairValueText) { HairValueText->SetText(FText::FromString(FString::Printf(TEXT("%d"), Working.HairStyle + 1))); }
}

void UZonefallCharacterCreatorWidget::RefreshPreviewBrush()
{
	if (bPreviewBrushSet || !PreviewImage || !PreviewCapture)
	{
		return;
	}
	UTextureRenderTarget2D* RT = PreviewCapture->GetRenderTarget();
	if (!RT)
	{
		return;
	}
	FSlateBrush Brush;
	Brush.SetResourceObject(RT);
	Brush.ImageSize = FVector2D(512.0f, 512.0f);
	Brush.DrawAs = ESlateBrushDrawType::Image;
	Brush.TintColor = FSlateColor(FLinearColor::White);
	PreviewImage->SetBrush(Brush);
	PreviewImage->SetColorAndOpacity(FLinearColor::White);
	bPreviewBrushSet = true;
}

void UZonefallCharacterCreatorWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	if (!PreviewCapture)
	{
		PreviewCapture = AZonefallCharacterPreviewCapture::Get(GetWorld());
	}
	RefreshPreviewBrush();
}
