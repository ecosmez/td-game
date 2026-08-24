#include "TowerStoreWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Image.h"
#include "Components/SceneCaptureComponent2D.h"
#include "Components/ScrollBox.h"
#include "Components/SizeBox.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Engine/StaticMesh.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/KismetRenderingLibrary.h"
#include "Materials/MaterialInterface.h"
#include "Styling/SlateBrush.h"
#include "UObject/UObjectIterator.h"
#include "UObject/UnrealType.h"

#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#endif

namespace TowerStorePrivate
{
	static const FLinearColor PanelBg(0.04f, 0.06f, 0.09f, 0.92f);
	static const FLinearColor PanelFrame(0.35f, 0.55f, 0.72f, 1.f);
	static const FLinearColor CardAfford(0.10f, 0.18f, 0.16f, 0.96f);
	static const FLinearColor CardUnafford(0.18f, 0.08f, 0.08f, 0.96f);
	static const FLinearColor CardHover(0.16f, 0.28f, 0.34f, 0.98f);
	static const FLinearColor TabActive(0.16f, 0.48f, 0.66f, 1.f);
	static const FLinearColor TabInactive(0.10f, 0.16f, 0.22f, 0.96f);
	static const FLinearColor TextMain(0.92f, 0.95f, 1.f, 1.f);
	static const FLinearColor TextDim(0.78f, 0.86f, 0.92f, 1.f);
	static const FLinearColor TextCost(1.f, 0.86f, 0.35f, 1.f);
	// Bare mesh image — no border/frame. Transparent clear RT so only the hologram shows.
	// Alpha on clear color is 0 so no solid background plate.
	static const FLinearColor PreviewClear(0.f, 0.f, 0.f, 0.f);

	static void SetTextStyle(UTextBlock* Text, const FLinearColor& Color, float Size, bool bBold = false)
	{
		if (!Text)
		{
			return;
		}
		Text->SetColorAndOpacity(FSlateColor(Color));
		FSlateFontInfo Font = Text->GetFont();
		Font.Size = Size;
		if (bBold)
		{
			Font.TypefaceFontName = TEXT("Bold");
		}
		Text->SetFont(Font);
		Text->SetShadowOffset(FVector2D(1.f, 1.f));
		Text->SetShadowColorAndOpacity(FLinearColor(0.f, 0.f, 0.f, 0.8f));
	}

	static bool ReadFloatProp(const UObject* Obj, FName Name, float& OutValue)
	{
		if (!Obj)
		{
			return false;
		}
		if (const FFloatProperty* Prop = FindFProperty<FFloatProperty>(Obj->GetClass(), Name))
		{
			OutValue = Prop->GetPropertyValue_InContainer(Obj);
			return true;
		}
		if (const FDoubleProperty* DProp = FindFProperty<FDoubleProperty>(Obj->GetClass(), Name))
		{
			OutValue = static_cast<float>(DProp->GetPropertyValue_InContainer(Obj));
			return true;
		}
		return false;
	}

	static bool ReadBoolProp(const UObject* Obj, FName Name, bool& OutValue)
	{
		if (!Obj)
		{
			return false;
		}
		if (const FBoolProperty* Prop = FindFProperty<FBoolProperty>(Obj->GetClass(), Name))
		{
			OutValue = Prop->GetPropertyValue_InContainer(Obj);
			return true;
		}
		return false;
	}

}

UTowerStoreWidget::UTowerStoreWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	SetIsFocusable(false);
	BuildDefaultCatalog();
}

void UTowerStoreCardClickBinder::HandleClicked()
{
	if (Store)
	{
		Store->OnCardClicked(CardIndex);
	}
}

void UTowerStoreCardClickBinder::HandleHovered()
{
	if (Store)
	{
		Store->OnCardHovered(CardIndex);
	}
}

void UTowerStoreCardClickBinder::HandleUnhovered()
{
	if (Store)
	{
		Store->OnCardUnhovered(CardIndex);
	}
}

void UTowerStoreCategoryClickBinder::HandleClicked()
{
	if (Store)
	{
		Store->SetCategoryFilter(bShowAll, Category);
	}
}

void UTowerStoreWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	EnsureBuilt();
}

void UTowerStoreWidget::NativeConstruct()
{
	Super::NativeConstruct();
	EnsureBuilt();
	ApplyHitTestPolicy();
	SetStoreOpen(bStartOpen);
	RefreshResourceLabel();
}

void UTowerStoreWidget::NativeDestruct()
{
	DestroyHoverPreview();
	Super::NativeDestruct();
}

void UTowerStoreWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	if (!bBuilt)
	{
		EnsureBuilt();
	}

	RefreshResourceLabel();

	if (bStoreOpen && HoveredCardIndex != INDEX_NONE)
	{
		PreviewYaw = FMath::Fmod(PreviewYaw + InDeltaTime * PreviewSpinSpeed, 360.f);
		UpdateHoverPreview(InDeltaTime);
	}
}

void UTowerStoreWidget::BuildDefaultCatalog()
{
	Catalog.Reset();

	auto Add = [this](const TCHAR* Name, const TCHAR* Role, ETowerStoreCategory Category,
		const TCHAR* SelectFn, const TCHAR* ClassPath,
		const TCHAR* MeshPath, const TCHAR* ColorMI, int32 Cost, float Build, float AS, float Dmg, float Range,
		const TCHAR* Note)
	{
		FTowerStoreEntryDef E;
		E.DisplayName = Name;
		E.Role = Role;
		E.Category = Category;
		E.SelectFunctionName = FName(SelectFn);
		E.TowerClassPath = ClassPath;
		E.MeshPath = MeshPath;
		E.ColorMaterialPath = ColorMI;
		E.Cost = Cost;
		E.BuildTime = Build;
		E.AttackSpeed = AS;
		E.Damage = Dmg;
		E.Range = Range;
		E.ExtraNote = Note;
		Catalog.Add(E);
	};

	const TCHAR* MeshBase = TEXT("/Game/TD/Assets/Towers/TowerBase.TowerBase");

	Add(TEXT("Mine"), TEXT("Mine"), ETowerStoreCategory::Attack, TEXT("SelectMineTower"),
		TEXT("/Game/TD/Towers/BP_Tower_Mine.BP_Tower_Mine_C"),
		MeshBase, TEXT("/Game/TD/Materials/TowerColors/MI_Tower_Trap.MI_Tower_Trap"),
		40, 1.5f, 0.f, 45.f, 200.f, TEXT("Path • AoE, one-shot or pulse"));

	Add(TEXT("Wall"), TEXT("Wall"), ETowerStoreCategory::Defense, TEXT("SelectWallTower"),
		TEXT("/Game/TD/Towers/BP_Tower_Wall.BP_Tower_Wall_C"),
		MeshBase, TEXT("/Game/TD/Materials/TowerColors/MI_Tower_Wall.MI_Tower_Wall"),
		35, 2.0f, 0.f, 0.f, 0.f, TEXT("Path block / redirect"));

	Add(TEXT("Arrow"), TEXT("Combat"), ETowerStoreCategory::Attack, TEXT("SelectArrowTower"),
		TEXT("/Game/TD/Towers/BP_Tower_Arrow.BP_Tower_Arrow_C"),
		MeshBase, TEXT("/Game/TD/Materials/TowerColors/MI_Tower_Arrow.MI_Tower_Arrow"),
		60, 2.5f, 3.0f, 6.f, 1600.f, TEXT("High ROF volley"));

	Add(TEXT("Economy"), TEXT("Economy"), ETowerStoreCategory::Support, TEXT("SelectEconomyTower"),
		TEXT("/Game/TD/Towers/BP_Tower_Economy.BP_Tower_Economy_C"),
		MeshBase, TEXT("/Game/TD/Materials/TowerColors/MI_Tower_Economy.MI_Tower_Economy"),
		90, 3.5f, 0.f, 0.f, 0.f, TEXT("Crystal • +gold over time"));

	Add(TEXT("Buff"), TEXT("Support"), ETowerStoreCategory::Support, TEXT("SelectBuffTower"),
		TEXT("/Game/TD/Towers/BP_Tower_Buff.BP_Tower_Buff_C"),
		MeshBase, TEXT("/Game/TD/Materials/TowerColors/MI_Tower_Buff.MI_Tower_Buff"),
		80, 3.0f, 0.f, 0.f, 900.f, TEXT("+20% dmg / +15% AS"));

	Add(TEXT("Cannon"), TEXT("Combat"), ETowerStoreCategory::Attack, TEXT("SelectCannonTower"),
		TEXT("/Game/TD/Towers/BP_Tower_Cannon.BP_Tower_Cannon_C"),
		MeshBase, TEXT("/Game/TD/Materials/TowerColors/MI_Tower_Cannon.MI_Tower_Cannon"),
		100, 4.0f, 0.6f, 35.f, 1400.f, TEXT("Ground AoE shell"));

	Add(TEXT("Sniper"), TEXT("Combat"), ETowerStoreCategory::Attack, TEXT("SelectSniperTower"),
		TEXT("/Game/TD/Towers/BP_Tower_Sniper.BP_Tower_Sniper_C"),
		MeshBase, TEXT("/Game/TD/Materials/TowerColors/MI_Tower_Sniper.MI_Tower_Sniper"),
		140, 4.5f, 0.45f, 80.f, 2800.f, TEXT("Long-range single"));

	Add(TEXT("Magic"), TEXT("Combat"), ETowerStoreCategory::Attack, TEXT("SelectMagicTower"),
		TEXT("/Game/TD/Towers/BP_Tower_Magic.BP_Tower_Magic_C"),
		MeshBase, TEXT("/Game/TD/Materials/TowerColors/MI_Tower_Magic.MI_Tower_Magic"),
		200, 5.0f, 1.2f, 55.f, 2000.f, TEXT("Multi-shot beam"));

	Add(TEXT("Slow Field"), TEXT("Trap"), ETowerStoreCategory::Defense, TEXT("SelectSlowFieldTrap"),
		TEXT("/Game/TD/Traps/BP_Trap_Base.BP_Trap_Base_C"),
		MeshBase, TEXT("/Game/TD/Materials/TowerColors/MI_Tower_Buff.MI_Tower_Buff"),
		50, 2.0f, 0.f, 0.f, 350.f, TEXT("Pulses slow • permanent"));

	Add(TEXT("Slow Snare"), TEXT("Trap"), ETowerStoreCategory::Defense, TEXT("SelectSlowSnareTrap"),
		TEXT("/Game/TD/Traps/BP_Trap_Base.BP_Trap_Base_C"),
		MeshBase, TEXT("/Game/TD/Materials/TowerColors/MI_Tower_Buff.MI_Tower_Buff"),
		30, 1.5f, 0.f, 0.f, 250.f, TEXT("One-shot slow • consumed"));

	Add(TEXT("Root Field"), TEXT("Trap"), ETowerStoreCategory::Defense, TEXT("SelectRootFieldTrap"),
		TEXT("/Game/TD/Traps/BP_Trap_Base.BP_Trap_Base_C"),
		MeshBase, TEXT("/Game/TD/Materials/TowerColors/MI_Tower_Wall.MI_Tower_Wall"),
		70, 2.5f, 0.f, 0.f, 300.f, TEXT("Pulses root • permanent"));

	Add(TEXT("Root Snare"), TEXT("Trap"), ETowerStoreCategory::Defense, TEXT("SelectRootSnareTrap"),
		TEXT("/Game/TD/Traps/BP_Trap_Base.BP_Trap_Base_C"),
		MeshBase, TEXT("/Game/TD/Materials/TowerColors/MI_Tower_Wall.MI_Tower_Wall"),
		45, 1.5f, 0.f, 0.f, 220.f, TEXT("One-shot root • consumed"));

	Add(TEXT("Freeze Field"), TEXT("Trap"), ETowerStoreCategory::Defense, TEXT("SelectFreezeFieldTrap"),
		TEXT("/Game/TD/Traps/BP_Trap_Base.BP_Trap_Base_C"),
		MeshBase, TEXT("/Game/TD/Materials/TowerColors/MI_Tower_Sniper.MI_Tower_Sniper"),
		90, 3.0f, 0.f, 0.f, 280.f, TEXT("Pulses freeze • permanent"));

	Add(TEXT("Freeze Snare"), TEXT("Trap"), ETowerStoreCategory::Defense, TEXT("SelectFreezeSnareTrap"),
		TEXT("/Game/TD/Traps/BP_Trap_Base.BP_Trap_Base_C"),
		MeshBase, TEXT("/Game/TD/Materials/TowerColors/MI_Tower_Sniper.MI_Tower_Sniper"),
		55, 1.5f, 0.f, 0.f, 200.f, TEXT("One-shot freeze • consumed"));
}

void UTowerStoreWidget::EnsureBuilt()
{
	if (bBuilt)
	{
		return;
	}
	if (!WidgetTree)
	{
		return;
	}
	if (!WidgetTree->RootWidget || !StorePanel)
	{
		BuildDefaultUI();
	}
	bBuilt = StorePanel != nullptr;
}

void UTowerStoreWidget::BuildDefaultUI()
{
	if (!WidgetTree)
	{
		return;
	}

	RootCanvas = Cast<UCanvasPanel>(WidgetTree->RootWidget);
	if (!RootCanvas)
	{
		RootCanvas = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("TowerStoreRoot"));
		WidgetTree->RootWidget = RootCanvas;
	}
	else
	{
		RootCanvas->ClearChildren();
	}

	// -------------------------------------------------------------------------
	// 1) Horizontal store strip — solid chrome, bottom-center.
	// -------------------------------------------------------------------------
	StorePanel = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("StorePanel"));
	StorePanel->SetPadding(FMargin(12.f, 8.f));
	StorePanel->SetBrushColor(TowerStorePrivate::PanelBg);
	if (UCanvasPanelSlot* PanelSlot = RootCanvas->AddChildToCanvas(StorePanel))
	{
		// Layout applied in ApplyDockLayout (Offset.Top negative = lift from bottom).
		StorePanelSlot = PanelSlot;
	}

	UVerticalBox* PanelV = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("PanelVBox"));
	StorePanel->SetContent(PanelV);

	// Header (resource / title / close)
	UHorizontalBox* Header = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("StoreHeader"));
	if (UVerticalBoxSlot* HeaderSlot = PanelV->AddChildToVerticalBox(Header))
	{
		HeaderSlot->SetPadding(FMargin(0.f, 0.f, 0.f, 6.f));
	}

	TitleText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("StoreTitle"));
	TitleText->SetText(FText::FromString(TEXT("TOWER STORE")));
	TowerStorePrivate::SetTextStyle(TitleText, TowerStorePrivate::TextMain, 13.f, true);
	if (UHorizontalBoxSlot* TitleSlot = Header->AddChildToHorizontalBox(TitleText))
	{
		TitleSlot->SetVerticalAlignment(VAlign_Center);
		TitleSlot->SetPadding(FMargin(0.f, 0.f, 12.f, 0.f));
	}

	ResourceText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("ResourceText"));
	ResourceText->SetText(FText::FromString(TEXT("Resource: —")));
	TowerStorePrivate::SetTextStyle(ResourceText, TowerStorePrivate::TextCost, 12.f, false);
	if (UHorizontalBoxSlot* ResSlot = Header->AddChildToHorizontalBox(ResourceText))
	{
		ResSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
		ResSlot->SetVerticalAlignment(VAlign_Center);
	}

	BuildCategoryTabs(PanelV);

	// Horizontal tower strip only — no hover inside this chrome.
	USizeBox* StripSize = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("StripSize"));
	StripSize->SetHeightOverride(StoreStripHeight);
	StripSize->SetWidthOverride(StoreMaxWidth);
	if (UVerticalBoxSlot* StripSlot = PanelV->AddChildToVerticalBox(StripSize))
	{
		StripSlot->SetHorizontalAlignment(HAlign_Fill);
	}

	CardScroll = WidgetTree->ConstructWidget<UScrollBox>(UScrollBox::StaticClass(), TEXT("CardScroll"));
	CardScroll->SetOrientation(Orient_Horizontal);
	CardScroll->SetScrollBarVisibility(ESlateVisibility::Collapsed);
	StripSize->SetContent(CardScroll);

	CardRow = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("CardRow"));
	CardScroll->AddChild(CardRow);

	// -------------------------------------------------------------------------
	// 2) Floating hover projection — OUTSIDE store chrome. Transparent, draws
	//    over the world / other UI (mesh + stats "floating").
	// -------------------------------------------------------------------------
	HoverPanel = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("HoverFloat"));
	HoverPanel->SetHeightOverride(HoverPanelHeight);
	HoverPanel->SetWidthOverride(StoreMaxWidth);
	HoverPanel->SetVisibility(ESlateVisibility::Collapsed);
	if (UCanvasPanelSlot* HoverSlot = RootCanvas->AddChildToCanvas(HoverPanel))
	{
		// Layout applied in ApplyDockLayout — floats above strip, no store chrome.
		HoverPanelSlot = HoverSlot;
	}

	// Transparent horizontal float: [ mesh ]  name / cost / stats  — no panel / no chrome.
	UHorizontalBox* HoverRow = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("HoverRow"));
	HoverRow->SetVisibility(ESlateVisibility::HitTestInvisible);
	HoverPanel->SetContent(HoverRow);

	USizeBox* MeshBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("HoverMeshBox"));
	MeshBox->SetWidthOverride(170.f);
	MeshBox->SetHeightOverride(170.f);
	MeshBox->SetVisibility(ESlateVisibility::HitTestInvisible);
	if (UHorizontalBoxSlot* MeshSlot = HoverRow->AddChildToHorizontalBox(MeshBox))
	{
		MeshSlot->SetPadding(FMargin(0.f, 0.f, 16.f, 0.f));
		MeshSlot->SetVerticalAlignment(VAlign_Center);
	}

	// Bare mesh image — no border, no frame, transparent RT background bleeds through.
	HoverMeshImage = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("HoverMeshImage"));
	{
		FSlateBrush Brush;
		Brush.DrawAs = ESlateBrushDrawType::Image;
		Brush.TintColor = FSlateColor(FLinearColor(1.f, 1.f, 1.f, 1.f));
		Brush.ImageSize = FVector2D(170.f, 170.f);
		// Leave brush color fully white; opacity is only the hologram itself.
		HoverMeshImage->SetBrush(Brush);
		HoverMeshImage->SetColorAndOpacity(FLinearColor(1.f, 1.f, 1.f, HoverProjectionOpacity));
		HoverMeshImage->SetVisibility(ESlateVisibility::HitTestInvisible);
	}
	MeshBox->SetContent(HoverMeshImage);

	UVerticalBox* HoverInfo = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("HoverInfo"));
	HoverInfo->SetVisibility(ESlateVisibility::HitTestInvisible);
	if (UHorizontalBoxSlot* InfoSlot = HoverRow->AddChildToHorizontalBox(HoverInfo))
	{
		InfoSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
		InfoSlot->SetVerticalAlignment(VAlign_Center);
	}

	UHorizontalBox* HoverTitleRow = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("HoverTitleRow"));
	if (UVerticalBoxSlot* HTSlot = HoverInfo->AddChildToVerticalBox(HoverTitleRow))
	{
		HTSlot->SetPadding(FMargin(0.f, 0.f, 0.f, 6.f));
	}

	HoverNameText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("HoverName"));
	HoverNameText->SetText(FText::FromString(TEXT("—")));
	TowerStorePrivate::SetTextStyle(HoverNameText, TowerStorePrivate::TextMain, 20.f, true);
	HoverNameText->SetVisibility(ESlateVisibility::HitTestInvisible);
	if (UHorizontalBoxSlot* HNSlot = HoverTitleRow->AddChildToHorizontalBox(HoverNameText))
	{
		HNSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	}

	HoverCostText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("HoverCost"));
	HoverCostText->SetText(FText::GetEmpty());
	TowerStorePrivate::SetTextStyle(HoverCostText, TowerStorePrivate::TextCost, 18.f, true);
	HoverCostText->SetVisibility(ESlateVisibility::HitTestInvisible);
	HoverTitleRow->AddChildToHorizontalBox(HoverCostText);

	HoverStatsText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("HoverStats"));
	HoverStatsText->SetText(FText::GetEmpty());
	TowerStorePrivate::SetTextStyle(HoverStatsText, TowerStorePrivate::TextDim, 13.f, false);
	HoverStatsText->SetAutoWrapText(true);
	HoverStatsText->SetVisibility(ESlateVisibility::HitTestInvisible);
	HoverInfo->AddChildToVerticalBox(HoverStatsText);

	BuildCards();
	ApplyDockLayout();
	ApplyHitTestPolicy();
}

float UTowerStoreWidget::GetStoreStripBottomPad() const
{
	// Distance from screen bottom to the *bottom edge* of the store strip.
	// Ability bar now sits over the champion frame (bottom-left); store is bottom-center.
	return StoreScreenBottomPad;
}

float UTowerStoreWidget::GetHoverFloatBottomPad() const
{
	// Floating hover sits above the full store body + float gap.
	const float StoreBody = StoreHeaderHeight + StoreTabsHeight + StoreStripHeight + 24.f;
	return GetStoreStripBottomPad() + StoreBody + HoverFloatGap;
}

void UTowerStoreWidget::ApplyDockLayout()
{
	// SConstraintCanvas (non-stretch anchors): LocalY = AnchorY + Offset.Top - AlignY*Size
	// With bottom anchor + AlignY=1, Offset.Top is how far UP from the bottom edge.
	// Offset.Bottom is NOT used for positioning when AutoSize is true — only Offset.Top.
	if (StorePanelSlot)
	{
		StorePanelSlot->SetAutoSize(true);
		StorePanelSlot->SetAnchors(FAnchors(0.5f, 1.f, 0.5f, 1.f));
		StorePanelSlot->SetAlignment(FVector2D(0.5f, 1.f));
		StorePanelSlot->SetOffsets(FMargin(0.f, -GetStoreStripBottomPad(), 0.f, 0.f));
		StorePanelSlot->SetZOrder(55);
	}
	if (HoverPanelSlot)
	{
		HoverPanelSlot->SetAutoSize(true);
		HoverPanelSlot->SetAnchors(FAnchors(0.5f, 1.f, 0.5f, 1.f));
		HoverPanelSlot->SetAlignment(FVector2D(0.5f, 1.f));
		HoverPanelSlot->SetOffsets(FMargin(0.f, -GetHoverFloatBottomPad(), 0.f, 0.f));
		HoverPanelSlot->SetZOrder(80);
	}
}

void UTowerStoreWidget::BuildCategoryTabs(UVerticalBox* Parent)
{
	CategoryTabs.Reset();
	CategoryClickBinders.Reset();
	if (!Parent || !WidgetTree)
	{
		return;
	}

	USizeBox* TabsSize = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("CategoryTabsSize"));
	TabsSize->SetHeightOverride(StoreTabsHeight);
	if (UVerticalBoxSlot* TabsSlot = Parent->AddChildToVerticalBox(TabsSize))
	{
		TabsSlot->SetHorizontalAlignment(HAlign_Left);
		TabsSlot->SetPadding(FMargin(0.f, 0.f, 0.f, 6.f));
	}

	UHorizontalBox* TabsRow = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("CategoryTabsRow"));
	TabsSize->SetContent(TabsRow);

	auto AddTab = [this, TabsRow](const TCHAR* Name, bool bShowAll, ETowerStoreCategory Category)
	{
		FTowerStoreCategoryTabUI Tab;
		Tab.bShowAll = bShowAll;
		Tab.Category = Category;

		USizeBox* TabSize = WidgetTree->ConstructWidget<USizeBox>(
			USizeBox::StaticClass(), *FString::Printf(TEXT("CategoryTabSize_%s"), Name));
		TabSize->SetWidthOverride(88.f);
		TabSize->SetHeightOverride(StoreTabsHeight - 6.f);
		if (UHorizontalBoxSlot* TabSlot = TabsRow->AddChildToHorizontalBox(TabSize))
		{
			TabSlot->SetPadding(FMargin(0.f, 0.f, 6.f, 0.f));
		}

		Tab.Button = WidgetTree->ConstructWidget<UButton>(
			UButton::StaticClass(), *FString::Printf(TEXT("CategoryTab_%s"), Name));
		TabSize->SetContent(Tab.Button);

		UTowerStoreCategoryClickBinder* Binder = NewObject<UTowerStoreCategoryClickBinder>(this);
		Binder->Store = this;
		Binder->bShowAll = bShowAll;
		Binder->Category = Category;
		Tab.Button->OnClicked.AddDynamic(Binder, &UTowerStoreCategoryClickBinder::HandleClicked);
		CategoryClickBinders.Add(Binder);

		Tab.Label = WidgetTree->ConstructWidget<UTextBlock>(
			UTextBlock::StaticClass(), *FString::Printf(TEXT("CategoryTabLabel_%s"), Name));
		Tab.Label->SetText(FText::FromString(Name));
		Tab.Label->SetJustification(ETextJustify::Center);
		Tab.Label->SetVisibility(ESlateVisibility::HitTestInvisible);
		TowerStorePrivate::SetTextStyle(Tab.Label, TowerStorePrivate::TextMain, 11.f, true);
		Tab.Button->SetContent(Tab.Label);

		CategoryTabs.Add(Tab);
	};

	AddTab(TEXT("All"), true, ETowerStoreCategory::Attack);
	AddTab(TEXT("Attack"), false, ETowerStoreCategory::Attack);
	AddTab(TEXT("Defense"), false, ETowerStoreCategory::Defense);
	AddTab(TEXT("Support"), false, ETowerStoreCategory::Support);
	RefreshCategoryTabVisuals();
}

void UTowerStoreWidget::RefreshCategoryTabVisuals()
{
	for (FTowerStoreCategoryTabUI& Tab : CategoryTabs)
	{
		const bool bActive = Tab.bShowAll == bShowAllCategories
			&& (Tab.bShowAll || Tab.Category == ActiveCategory);
		if (Tab.Button)
		{
			Tab.Button->SetBackgroundColor(bActive
				? TowerStorePrivate::TabActive
				: TowerStorePrivate::TabInactive);
		}
	}
}

void UTowerStoreWidget::SetCategoryFilter(bool bShowAll, ETowerStoreCategory Category)
{
	if (bShowAllCategories == bShowAll && (bShowAll || ActiveCategory == Category))
	{
		return;
	}

	ShowHoverPanel(false);
	DestroyHoverTowerActor();
	HoveredCardIndex = INDEX_NONE;
	CaptureTimer = 0.f;
	bShowAllCategories = bShowAll;
	ActiveCategory = Category;

	BuildCards();
	RefreshCategoryTabVisuals();
	RefreshResourceLabel();
}

void UTowerStoreWidget::BuildCards()
{
	Cards.Reset();
	CardClickBinders.Reset();
	if (!CardRow || !WidgetTree)
	{
		return;
	}
	CardRow->ClearChildren();

	for (const FTowerStoreEntryDef& Def : Catalog)
	{
		if (!DoesCategoryMatchFilter(Def.Category, bShowAllCategories, ActiveCategory))
		{
			continue;
		}
		const int32 VisibleIndex = Cards.Num();
		Cards.Add(BuildCard(Def, VisibleIndex));
	}
}

FTowerStoreCardUI UTowerStoreWidget::BuildCard(const FTowerStoreEntryDef& Def, int32 Index)
{
	FTowerStoreCardUI Card;
	Card.Def = Def;
	Card.CardIndex = Index;

	if (!CardRow || !WidgetTree)
	{
		return Card;
	}

	USizeBox* TileSize = WidgetTree->ConstructWidget<USizeBox>(
		USizeBox::StaticClass(), *FString::Printf(TEXT("TileSize_%d"), Index));
	TileSize->SetWidthOverride(84.f);
	TileSize->SetHeightOverride(StoreStripHeight - 8.f);
	if (UHorizontalBoxSlot* TileSlot = CardRow->AddChildToHorizontalBox(TileSize))
	{
		TileSlot->SetPadding(FMargin(0.f, 0.f, 6.f, 0.f));
		TileSlot->SetVerticalAlignment(VAlign_Center);
	}

	Card.CardFrame = WidgetTree->ConstructWidget<UBorder>(
		UBorder::StaticClass(), *FString::Printf(TEXT("CardFrame_%d"), Index));
	Card.CardFrame->SetPadding(FMargin(4.f));
	Card.CardFrame->SetBrushColor(TowerStorePrivate::CardAfford);
	TileSize->SetContent(Card.CardFrame);

	Card.CardButton = WidgetTree->ConstructWidget<UButton>(
		UButton::StaticClass(), *FString::Printf(TEXT("CardBtn_%d"), Index));
	Card.CardButton->SetBackgroundColor(FLinearColor(0.06f, 0.09f, 0.12f, 0.55f));
	{
		UTowerStoreCardClickBinder* Binder = NewObject<UTowerStoreCardClickBinder>(this);
		Binder->Store = this;
		Binder->CardIndex = Index;
		Card.CardButton->OnClicked.AddDynamic(Binder, &UTowerStoreCardClickBinder::HandleClicked);
		Card.CardButton->OnHovered.AddDynamic(Binder, &UTowerStoreCardClickBinder::HandleHovered);
		Card.CardButton->OnUnhovered.AddDynamic(Binder, &UTowerStoreCardClickBinder::HandleUnhovered);
		CardClickBinders.Add(Binder);
	}
	Card.CardFrame->SetContent(Card.CardButton);

	UVerticalBox* Info = WidgetTree->ConstructWidget<UVerticalBox>(
		UVerticalBox::StaticClass(), *FString::Printf(TEXT("CardInfo_%d"), Index));
	Card.CardButton->SetContent(Info);

	Card.NameText = WidgetTree->ConstructWidget<UTextBlock>(
		UTextBlock::StaticClass(), *FString::Printf(TEXT("Name_%d"), Index));
	Card.NameText->SetText(FText::FromString(Def.DisplayName));
	Card.NameText->SetJustification(ETextJustify::Center);
	TowerStorePrivate::SetTextStyle(Card.NameText, TowerStorePrivate::TextMain, 11.f, true);
	Card.NameText->SetVisibility(ESlateVisibility::HitTestInvisible);
	if (UVerticalBoxSlot* NameSlot = Info->AddChildToVerticalBox(Card.NameText))
	{
		NameSlot->SetHorizontalAlignment(HAlign_Center);
		NameSlot->SetPadding(FMargin(0.f, 8.f, 0.f, 2.f));
	}

	UTextBlock* RoleText = WidgetTree->ConstructWidget<UTextBlock>(
		UTextBlock::StaticClass(), *FString::Printf(TEXT("Role_%d"), Index));
	RoleText->SetText(FText::FromString(Def.Role));
	RoleText->SetJustification(ETextJustify::Center);
	TowerStorePrivate::SetTextStyle(RoleText, TowerStorePrivate::TextDim, 9.f);
	RoleText->SetVisibility(ESlateVisibility::HitTestInvisible);
	if (UVerticalBoxSlot* RoleSlot = Info->AddChildToVerticalBox(RoleText))
	{
		RoleSlot->SetHorizontalAlignment(HAlign_Center);
	}

	Card.CostText = WidgetTree->ConstructWidget<UTextBlock>(
		UTextBlock::StaticClass(), *FString::Printf(TEXT("Cost_%d"), Index));
	Card.CostText->SetText(FText::FromString(FString::Printf(TEXT("%d"), Def.Cost)));
	Card.CostText->SetJustification(ETextJustify::Center);
	TowerStorePrivate::SetTextStyle(Card.CostText, TowerStorePrivate::TextCost, 13.f, true);
	Card.CostText->SetVisibility(ESlateVisibility::HitTestInvisible);
	if (UVerticalBoxSlot* CostSlot = Info->AddChildToVerticalBox(Card.CostText))
	{
		CostSlot->SetHorizontalAlignment(HAlign_Center);
		CostSlot->SetPadding(FMargin(0.f, 6.f, 0.f, 0.f));
	}

	return Card;
}

FString UTowerStoreWidget::FormatStats(const FTowerStoreEntryDef& Def) const
{
	TArray<FString> Parts;
	Parts.Add(FString::Printf(TEXT("Build %.1fs"), Def.BuildTime));
	if (Def.Damage > 0.f)
	{
		Parts.Add(FString::Printf(TEXT("DMG %.0f"), Def.Damage));
	}
	if (Def.AttackSpeed > 0.f)
	{
		Parts.Add(FString::Printf(TEXT("AS %.2f/s"), Def.AttackSpeed));
	}
	if (Def.Range > 0.f)
	{
		Parts.Add(FString::Printf(TEXT("Range %.0f"), Def.Range));
	}
	return FString::Join(Parts, TEXT("  •  "));
}

FString UTowerStoreWidget::FormatDetailedStats(const FTowerStoreEntryDef& Def) const
{
	TArray<FString> Lines;
	Lines.Add(FString::Printf(TEXT("Role: %s"), *Def.Role));
	Lines.Add(FString::Printf(TEXT("Build time: %.1fs"), Def.BuildTime));
	if (Def.Damage > 0.f)
	{
		Lines.Add(FString::Printf(TEXT("Damage: %.0f"), Def.Damage));
	}
	if (Def.AttackSpeed > 0.f)
	{
		Lines.Add(FString::Printf(TEXT("Attack speed: %.2f / s"), Def.AttackSpeed));
	}
	if (Def.Range > 0.f)
	{
		Lines.Add(FString::Printf(TEXT("Range: %.0f"), Def.Range));
	}
	if (!Def.ExtraNote.IsEmpty())
	{
		Lines.Add(Def.ExtraNote);
	}
	Lines.Add(TEXT("Click to purchase / place"));
	return FString::Join(Lines, TEXT("\n"));
}

void UTowerStoreWidget::ApplyHitTestPolicy()
{
	SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	if (RootCanvas)
	{
		RootCanvas->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	}
	if (StorePanel)
	{
		StorePanel->SetVisibility(bStoreOpen ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	}
	// Hover never captures clicks — mesh/stats float over world + UI.
	if (HoverPanel && HoveredCardIndex == INDEX_NONE)
	{
		HoverPanel->SetVisibility(ESlateVisibility::Collapsed);
	}
}

void UTowerStoreWidget::SetStoreOpen(bool bOpen)
{
	bStoreOpen = bOpen;
	ApplyDockLayout();
	if (StorePanel)
	{
		StorePanel->SetVisibility(bStoreOpen ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	}

	// Ability bar is added at viewport Z=100 (ShowAbilityHUD). Store must sit above it.
	// UE 5.8: re-AddToViewport updates the existing slot ZOrder.
	if (IsInViewport())
	{
		AddToViewport(bStoreOpen ? 120 : 110);
	}

	if (!bStoreOpen)
	{
		SetHoveredCard(INDEX_NONE);
	}
	else
	{
		EnsureHoverPreview();
	}

	ApplyHitTestPolicy();
}

void UTowerStoreWidget::ToggleStore()
{
	SetStoreOpen(!bStoreOpen);
}

void UTowerStoreWidget::OnCardClicked(int32 CardIndex)
{
	if (!Cards.IsValidIndex(CardIndex))
	{
		return;
	}

	AActor* BM = FindBuildManager();
	const float Resource = ReadResource(BM);
	const FTowerStoreEntryDef& Def = Cards[CardIndex].Def;
	if (Resource + 0.01f < static_cast<float>(Def.Cost))
	{
		UE_LOG(LogTemp, Display, TEXT("TowerStore: cannot afford %s (%d)"), *Def.DisplayName, Def.Cost);
		return;
	}

	if (CallBuildManagerSelect(Def.SelectFunctionName))
	{
		// Some Select*Tower Blueprint functions only store the selection. Ensure the
		// placement mode starts now so the ghost follows the cursor before a pad click.
		bool bTowerDragLive = false;
		const bool bReadDragState = TowerStorePrivate::ReadBoolProp(
			BM, FName(TEXT("TowerDragLive")), bTowerDragLive);
		if ((!bReadDragState || !bTowerDragLive) && BM)
		{
			if (UFunction* BeginDrag = BM->FindFunction(FName(TEXT("BeginTowerDrag"))))
			{
				BM->ProcessEvent(BeginDrag, nullptr);
			}
		}
		UE_LOG(LogTemp, Display, TEXT("TowerStore: selected %s via %s"),
			*Def.DisplayName, *Def.SelectFunctionName.ToString());
		SetStoreOpen(false);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("TowerStore: failed to call %s on BuildManager"),
			*Def.SelectFunctionName.ToString());
	}
}

void UTowerStoreWidget::OnCardHovered(int32 CardIndex)
{
	if (!bStoreOpen)
	{
		return;
	}
	SetHoveredCard(CardIndex);
}

void UTowerStoreWidget::OnCardUnhovered(int32 CardIndex)
{
	// Only clear if we're leaving the currently hovered tile (prevents flicker on rapid moves).
	if (HoveredCardIndex == CardIndex)
	{
		SetHoveredCard(INDEX_NONE);
	}
}

void UTowerStoreWidget::SetHoveredCard(int32 CardIndex)
{
	// Restore previous frame color
	if (Cards.IsValidIndex(HoveredCardIndex) && Cards[HoveredCardIndex].CardFrame)
	{
		const float Resource = ReadResource(FindBuildManager());
		const bool bCan = Resource + 0.01f >= static_cast<float>(Cards[HoveredCardIndex].Def.Cost);
		Cards[HoveredCardIndex].CardFrame->SetBrushColor(
			bCan ? TowerStorePrivate::CardAfford : TowerStorePrivate::CardUnafford);
	}

	HoveredCardIndex = CardIndex;

	if (!Cards.IsValidIndex(HoveredCardIndex))
	{
		ShowHoverPanel(false);
		return;
	}

	if (Cards[HoveredCardIndex].CardFrame)
	{
		Cards[HoveredCardIndex].CardFrame->SetBrushColor(TowerStorePrivate::CardHover);
	}

	const FTowerStoreEntryDef& Def = Cards[HoveredCardIndex].Def;
	if (HoverNameText)
	{
		HoverNameText->SetText(FText::FromString(Def.DisplayName.ToUpper()));
	}
	if (HoverCostText)
	{
		HoverCostText->SetText(FText::FromString(FString::Printf(TEXT("Cost %d"), Def.Cost)));
	}
	if (HoverStatsText)
	{
		HoverStatsText->SetText(FText::FromString(FormatDetailedStats(Def)));
	}

	EnsureHoverPreview();
	ApplyHoverMesh(Def);
	CaptureHoverPreview();
	ShowHoverPanel(true);
}

void UTowerStoreWidget::ShowHoverPanel(bool bShow)
{
	if (!HoverPanel)
	{
		return;
	}
	// HitTestInvisible: floats over game/UI without blocking clicks.
	HoverPanel->SetVisibility(bShow ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	if (bShow)
	{
		ApplyDockLayout();
	}
}

AActor* UTowerStoreWidget::FindBuildManager() const
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return nullptr;
	}

	for (TObjectIterator<AActor> It; It; ++It)
	{
		AActor* Actor = *It;
		if (!IsValid(Actor) || Actor->GetWorld() != World)
		{
			continue;
		}
		if (Actor->GetClass()->GetName().Contains(TEXT("BuildManager")))
		{
			return Actor;
		}
	}
	return nullptr;
}

float UTowerStoreWidget::ReadResource(AActor* BuildManager) const
{
	if (!BuildManager)
	{
		return 0.f;
	}
	float Resource = 0.f;
	if (TowerStorePrivate::ReadFloatProp(BuildManager, FName(TEXT("Resource")), Resource))
	{
		return Resource;
	}
	return 0.f;
}

bool UTowerStoreWidget::CallBuildManagerSelect(const FName& FunctionName)
{
	AActor* BM = FindBuildManager();
	if (!BM || FunctionName.IsNone())
	{
		return false;
	}
	if (UFunction* Fn = BM->FindFunction(FunctionName))
	{
		BM->ProcessEvent(Fn, nullptr);
		return true;
	}
	return false;
}

void UTowerStoreWidget::RefreshResourceLabel()
{
	const float Resource = ReadResource(FindBuildManager());
	if (ResourceText)
	{
		ResourceText->SetText(FText::FromString(FString::Printf(TEXT("Resource: %.0f"), Resource)));
	}
	RefreshCardAffordability(Resource);
}

void UTowerStoreWidget::RefreshCardAffordability(float Resource)
{
	for (int32 i = 0; i < Cards.Num(); ++i)
	{
		FTowerStoreCardUI& Card = Cards[i];
		const bool bCanAfford = Resource + 0.01f >= static_cast<float>(Card.Def.Cost);
		if (Card.CardFrame && i != HoveredCardIndex)
		{
			Card.CardFrame->SetBrushColor(bCanAfford ? TowerStorePrivate::CardAfford : TowerStorePrivate::CardUnafford);
		}
		if (Card.CardButton)
		{
			// Keep enabled so hover still works when unaffordable; click gates spend.
			Card.CardButton->SetIsEnabled(true);
			Card.CardButton->SetRenderOpacity(bCanAfford ? 1.f : 0.55f);
		}
	}
}

UClass* UTowerStoreWidget::ResolveTowerClass(const FTowerStoreEntryDef& Def) const
{
	if (Def.TowerClassPath.IsEmpty())
	{
		return nullptr;
	}

	const FSoftClassPath SoftPath(Def.TowerClassPath);
	if (UClass* Loaded = SoftPath.TryLoadClass<AActor>())
	{
		return Loaded;
	}

	if (UClass* AsClass = LoadObject<UClass>(nullptr, *Def.TowerClassPath))
	{
		return AsClass;
	}

	return LoadClass<AActor>(nullptr, *Def.TowerClassPath);
}

UStaticMesh* UTowerStoreWidget::ResolveMeshForEntry(const FTowerStoreEntryDef& Def) const
{
	// Prefer TowerMesh property template on the tower CDO (BP_Tower hierarchy).
	if (UClass* TowerClass = ResolveTowerClass(Def))
	{
		if (AActor* CDO = Cast<AActor>(TowerClass->GetDefaultObject()))
		{
			// Named blueprint component variable (most reliable for BP_Tower children).
			if (const FObjectProperty* MeshProp = FindFProperty<FObjectProperty>(TowerClass, TEXT("TowerMesh")))
			{
				if (UStaticMeshComponent* SMC = Cast<UStaticMeshComponent>(
					MeshProp->GetObjectPropertyValue_InContainer(CDO)))
				{
					if (UStaticMesh* Mesh = SMC->GetStaticMesh())
					{
						return Mesh;
					}
				}
			}

			TArray<UStaticMeshComponent*> Meshes;
			CDO->GetComponents<UStaticMeshComponent>(Meshes);
			for (UStaticMeshComponent* SMC : Meshes)
			{
				if (!SMC || !SMC->GetStaticMesh())
				{
					continue;
				}
				const FString CompName = SMC->GetName();
				if (CompName.Contains(TEXT("Holo")))
				{
					continue;
				}
				if (CompName.Contains(TEXT("Tower")) || CompName.Contains(TEXT("Mesh")))
				{
					return SMC->GetStaticMesh();
				}
			}
			for (UStaticMeshComponent* SMC : Meshes)
			{
				if (SMC && SMC->GetStaticMesh())
				{
					return SMC->GetStaticMesh();
				}
			}
		}
	}

	if (!Def.MeshPath.IsEmpty())
	{
		if (UStaticMesh* Mesh = LoadObject<UStaticMesh>(nullptr, *Def.MeshPath))
		{
			return Mesh;
		}
	}

	return LoadObject<UStaticMesh>(nullptr, *FallbackMeshPath);
}

UMaterialInterface* UTowerStoreWidget::ResolveHoloMaterial() const
{
	return LoadObject<UMaterialInterface>(nullptr, *HoloMaterialPath);
}

void UTowerStoreWidget::ApplyTranslucentPreviewMaterial(UStaticMeshComponent* MeshComp, const FTowerStoreEntryDef& Def) const
{
	if (!MeshComp || !bUseHoloPreviewMaterial)
	{
		return;
	}

	if (UMaterialInterface* Holo = ResolveHoloMaterial())
	{
		const int32 Num = FMath::Max(1, MeshComp->GetNumMaterials());
		for (int32 i = 0; i < Num; ++i)
		{
			MeshComp->SetMaterial(i, Holo);
		}
		return;
	}

	if (!Def.ColorMaterialPath.IsEmpty())
	{
		if (UMaterialInterface* ColorMI = LoadObject<UMaterialInterface>(nullptr, *Def.ColorMaterialPath))
		{
			MeshComp->SetMaterial(0, ColorMI);
		}
	}
}

void UTowerStoreWidget::ConfigurePreviewComponents(AActor* TowerActor)
{
	if (!TowerActor || !HoverCapture)
	{
		return;
	}

	HoverCapture->ClearShowOnlyComponents();
	HoverCapture->ShowOnlyActors.Reset();
	HoverCapture->ShowOnlyActorComponents(TowerActor);

	TArray<UStaticMeshComponent*> Meshes;
	TowerActor->GetComponents<UStaticMeshComponent>(Meshes);
	for (UStaticMeshComponent* SMC : Meshes)
	{
		if (!SMC)
		{
			continue;
		}

		const FString CompName = SMC->GetName();
		const bool bHolo = CompName.Contains(TEXT("Holo"));
		SMC->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		SMC->SetCastShadow(false);
		SMC->bVisibleInSceneCaptureOnly = true;
		if (bHolo)
		{
			SMC->SetVisibility(false, true);
			continue;
		}

		// Built look: solid tower mesh (BeginPlay already applied type color).
		SMC->SetVisibility(true, true);
		if (bUseHoloPreviewMaterial)
		{
			// Def not available here when only reconfiguring; handled after spawn with catalog def.
		}
	}
}

void UTowerStoreWidget::FrameHoverCamera()
{
	if (!HoverCapture)
	{
		return;
	}

	FVector Focus = PreviewOrigin;
	float Radius = 80.f;

	if (HoverTowerActor && IsValid(HoverTowerActor))
	{
		FVector Origin;
		FVector Extent;
		HoverTowerActor->GetActorBounds(false, Origin, Extent);
		Focus = Origin;
		Radius = FMath::Max3(Extent.X, Extent.Y, Extent.Z);
		Radius = FMath::Max(Radius, 40.f);
	}
	else if (HoverPreviewMesh && HoverPreviewMesh->GetStaticMesh())
	{
		const FBoxSphereBounds Bounds = HoverPreviewMesh->Bounds;
		Focus = Bounds.Origin;
		Radius = FMath::Max(Bounds.SphereRadius, 40.f);
	}

	const FVector CamOffset(-Radius * 2.9f, Radius * 1.55f, Radius * 1.15f);
	const FVector CamLoc = Focus + CamOffset;
	HoverCapture->SetWorldLocation(CamLoc);
	HoverCapture->SetWorldRotation((Focus - CamLoc).Rotation());
	HoverCapture->FOVAngle = 30.f;
}

void UTowerStoreWidget::EnsureHoverPreview()
{
	if (bHoverPreviewReady && HoverPreviewActor && IsValid(HoverPreviewActor) && HoverCapture)
	{
		return;
	}
	if (!GetWorld())
	{
		return;
	}

	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	Params.ObjectFlags |= RF_Transient;
	Params.Owner = GetOwningPlayer();
	Params.Name = TEXT("TowerStoreHoverStage");

	AActor* Actor = GetWorld()->SpawnActor<AActor>(AActor::StaticClass(), PreviewOrigin, FRotator::ZeroRotator, Params);
	if (!Actor)
	{
		return;
	}
	Actor->SetActorEnableCollision(false);
	Actor->SetActorTickEnabled(false);

	USceneComponent* Root = NewObject<USceneComponent>(Actor, TEXT("Root"));
	Root->SetMobility(EComponentMobility::Movable);
	Actor->SetRootComponent(Root);
	Root->RegisterComponent();
	Root->SetWorldLocation(PreviewOrigin);

	// Fallback single-mesh renderer (used only if tower class spawn fails).
	HoverPreviewMesh = NewObject<UStaticMeshComponent>(Actor, TEXT("FallbackPreviewMesh"));
	HoverPreviewMesh->SetMobility(EComponentMobility::Movable);
	HoverPreviewMesh->SetupAttachment(Root);
	HoverPreviewMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	HoverPreviewMesh->SetCastShadow(false);
	HoverPreviewMesh->bVisibleInSceneCaptureOnly = true;
	HoverPreviewMesh->SetVisibility(false);
	HoverPreviewMesh->RegisterComponent();

	HoverCapture = NewObject<USceneCaptureComponent2D>(Actor, TEXT("Capture"));
	HoverCapture->SetupAttachment(Root);
	HoverCapture->RegisterComponent();
	HoverCapture->ProjectionType = ECameraProjectionMode::Perspective;
	HoverCapture->FOVAngle = 30.f;
	HoverCapture->CaptureSource = ESceneCaptureSource::SCS_FinalColorLDR;
	HoverCapture->bCaptureEveryFrame = false;
	HoverCapture->bCaptureOnMovement = false;
	HoverCapture->bAlwaysPersistRenderingState = true;
	HoverCapture->PrimitiveRenderMode = ESceneCapturePrimitiveRenderMode::PRM_UseShowOnlyList;

	FEngineShowFlags& Flags = HoverCapture->ShowFlags;
	Flags.SetAtmosphere(false);
	Flags.SetFog(false);
	Flags.SetBloom(false);
	Flags.SetMotionBlur(false);
	Flags.SetDepthOfField(false);
	Flags.SetTemporalAA(false);
	Flags.SetScreenSpaceReflections(false);
	Flags.SetAmbientOcclusion(false);
	Flags.SetGlobalIllumination(false);
	Flags.SetPostProcessing(true);
	Flags.SetTranslucency(true);
	Flags.SetLighting(true);
	Flags.SetParticles(false);
	Flags.SetSkeletalMeshes(false);

	const int32 Size = FMath::Clamp(PreviewRenderSize, 64, 512);
	HoverRenderTarget = UKismetRenderingLibrary::CreateRenderTarget2D(
		this, Size, Size, RTF_RGBA8, TowerStorePrivate::PreviewClear, false);
	HoverCapture->TextureTarget = HoverRenderTarget;

	if (HoverMeshImage && HoverRenderTarget)
	{
		FSlateBrush Brush = HoverMeshImage->GetBrush();
		Brush.SetResourceObject(HoverRenderTarget);
		Brush.DrawAs = ESlateBrushDrawType::Image;
		Brush.ImageSize = FVector2D(static_cast<float>(Size), static_cast<float>(Size));
		Brush.TintColor = FSlateColor(FLinearColor(1.f, 1.f, 1.f, 1.f));
		HoverMeshImage->SetBrush(Brush);
		HoverMeshImage->SetColorAndOpacity(FLinearColor(1.f, 1.f, 1.f, HoverProjectionOpacity));
	}

	HoverPreviewActor = Actor;
	bHoverPreviewReady = true;
	FrameHoverCamera();
}

void UTowerStoreWidget::DestroyHoverTowerActor()
{
	if (HoverTowerActor && IsValid(HoverTowerActor))
	{
		HoverTowerActor->Destroy();
	}
	HoverTowerActor = nullptr;
}

void UTowerStoreWidget::ApplyHoverMesh(const FTowerStoreEntryDef& Def)
{
	EnsureHoverPreview();
	if (!HoverCapture || !GetWorld())
	{
		return;
	}

	DestroyHoverTowerActor();
	PreviewYaw = 25.f;

	if (HoverPreviewMesh)
	{
		HoverPreviewMesh->SetVisibility(false);
	}

	// Spawn the real tower Blueprint so we get TowerMesh + ApplyTowerColor from BeginPlay.
	if (UClass* TowerClass = ResolveTowerClass(Def))
	{
		FActorSpawnParameters Params;
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		Params.ObjectFlags |= RF_Transient;
		Params.Owner = GetOwningPlayer();

		AActor* Tower = GetWorld()->SpawnActor<AActor>(
			TowerClass, PreviewOrigin, FRotator(0.f, PreviewYaw, 0.f), Params);
		if (Tower)
		{
			// BeginPlay already ran: authored mesh transform + type color applied.
			Tower->SetActorEnableCollision(false);
			Tower->SetActorTickEnabled(false);
			Tower->SetActorHiddenInGame(false);
			Tower->SetActorLocation(PreviewOrigin);
			Tower->SetActorRotation(FRotator(0.f, PreviewYaw, 0.f));
			Tower->SetActorScale3D(FVector(1.f));

			// Mark ghost-like so construction logic does nothing if something re-enables tick.
			if (FBoolProperty* GhostProp = FindFProperty<FBoolProperty>(Tower->GetClass(), TEXT("isGhost")))
			{
				GhostProp->SetPropertyValue_InContainer(Tower, true);
			}
			else if (FBoolProperty* GhostPropB = FindFProperty<FBoolProperty>(Tower->GetClass(), TEXT("IsGhost")))
			{
				GhostPropB->SetPropertyValue_InContainer(Tower, true);
			}
			if (FBoolProperty* BuiltProp = FindFProperty<FBoolProperty>(Tower->GetClass(), TEXT("isBuilt")))
			{
				BuiltProp->SetPropertyValue_InContainer(Tower, true);
			}
			else if (FBoolProperty* BuiltPropB = FindFProperty<FBoolProperty>(Tower->GetClass(), TEXT("IsBuilt")))
			{
				BuiltPropB->SetPropertyValue_InContainer(Tower, true);
			}
			if (FBoolProperty* ConsProp = FindFProperty<FBoolProperty>(Tower->GetClass(), TEXT("isConstructing")))
			{
				ConsProp->SetPropertyValue_InContainer(Tower, false);
			}
			else if (FBoolProperty* ConsPropB = FindFProperty<FBoolProperty>(Tower->GetClass(), TEXT("IsConstructing")))
			{
				ConsPropB->SetPropertyValue_InContainer(Tower, false);
			}

			HoverTowerActor = Tower;
			ConfigurePreviewComponents(Tower);

			if (bUseHoloPreviewMaterial)
			{
				TArray<UStaticMeshComponent*> Meshes;
				Tower->GetComponents<UStaticMeshComponent>(Meshes);
				for (UStaticMeshComponent* SMC : Meshes)
				{
					if (SMC && !SMC->GetName().Contains(TEXT("Holo")))
					{
						ApplyTranslucentPreviewMaterial(SMC, Def);
					}
				}
			}

			FrameHoverCamera();
			CaptureHoverPreview();
			return;
		}
	}

	// Fallback: single shared mesh on the stage actor.
	if (HoverPreviewMesh)
	{
		if (UStaticMesh* Mesh = ResolveMeshForEntry(Def))
		{
			HoverPreviewMesh->SetStaticMesh(Mesh);
			const FBoxSphereBounds Bounds = Mesh->GetBounds();
			const float Radius = FMath::Max(Bounds.SphereRadius, 20.f);
			const float Scale = 95.f / Radius;
			HoverPreviewMesh->SetRelativeScale3D(FVector(Scale));
			HoverPreviewMesh->SetRelativeLocation(FVector(0.f, 0.f, -Bounds.Origin.Z * Scale));
			HoverPreviewMesh->SetVisibility(true);
		}
		if (!Def.ColorMaterialPath.IsEmpty())
		{
			if (UMaterialInterface* ColorMI = LoadObject<UMaterialInterface>(nullptr, *Def.ColorMaterialPath))
			{
				HoverPreviewMesh->SetMaterial(0, ColorMI);
			}
		}
		ApplyTranslucentPreviewMaterial(HoverPreviewMesh, Def);
		HoverCapture->ClearShowOnlyComponents();
		HoverCapture->ShowOnlyActors.Reset();
		HoverCapture->ShowOnlyComponent(HoverPreviewMesh);
		FrameHoverCamera();
		CaptureHoverPreview();
	}
}

void UTowerStoreWidget::CaptureHoverPreview()
{
	if (!HoverCapture)
	{
		return;
	}

	if (HoverTowerActor && IsValid(HoverTowerActor))
	{
		HoverTowerActor->SetActorRotation(FRotator(0.f, PreviewYaw, 0.f));
	}
	else if (HoverPreviewMesh)
	{
		HoverPreviewMesh->SetRelativeRotation(FRotator(0.f, PreviewYaw, 0.f));
	}

	HoverCapture->CaptureScene();
}

void UTowerStoreWidget::UpdateHoverPreview(float DeltaTime)
{
	if (!bHoverPreviewReady)
	{
		EnsureHoverPreview();
		return;
	}

	CaptureTimer += DeltaTime;
	if (CaptureTimer < CaptureInterval)
	{
		return;
	}
	CaptureTimer = 0.f;
	CaptureHoverPreview();
}

void UTowerStoreWidget::DestroyHoverPreview()
{
	DestroyHoverTowerActor();

	if (HoverPreviewActor && IsValid(HoverPreviewActor))
	{
		HoverPreviewActor->Destroy();
	}
	HoverPreviewActor = nullptr;
	HoverPreviewMesh = nullptr;
	HoverCapture = nullptr;
	HoverRenderTarget = nullptr;
	bHoverPreviewReady = false;
	HoveredCardIndex = INDEX_NONE;
}

bool UTowerStoreWidget::DoesCategoryMatchFilter(
	ETowerStoreCategory EntryCategory,
	bool bShowAll,
	ETowerStoreCategory ActiveCategory)
{
	return bShowAll || EntryCategory == ActiveCategory;
}

#if WITH_DEV_AUTOMATION_TESTS
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FTowerStoreCategoryFilterTest,
	"TD.UI.TowerStore.CategoryFilter",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FTowerStoreCategoryFilterTest::RunTest(const FString& Parameters)
{
	TestTrue(TEXT("All includes Attack"), UTowerStoreWidget::DoesCategoryMatchFilter(
		ETowerStoreCategory::Attack, true, ETowerStoreCategory::Defense));
	TestTrue(TEXT("Attack includes Attack"), UTowerStoreWidget::DoesCategoryMatchFilter(
		ETowerStoreCategory::Attack, false, ETowerStoreCategory::Attack));
	TestFalse(TEXT("Attack excludes Defense"), UTowerStoreWidget::DoesCategoryMatchFilter(
		ETowerStoreCategory::Defense, false, ETowerStoreCategory::Attack));
	TestFalse(TEXT("Attack excludes Support"), UTowerStoreWidget::DoesCategoryMatchFilter(
		ETowerStoreCategory::Support, false, ETowerStoreCategory::Attack));

	const ETowerStoreCategory Categories[] = {
		ETowerStoreCategory::Attack,
		ETowerStoreCategory::Defense,
		ETowerStoreCategory::Support
	};
	auto CountVisible = [&Categories](bool bShowAll, ETowerStoreCategory ActiveCategory)
	{
		int32 Count = 0;
		for (ETowerStoreCategory Category : Categories)
		{
			Count += UTowerStoreWidget::DoesCategoryMatchFilter(Category, bShowAll, ActiveCategory) ? 1 : 0;
		}
		return Count;
	};
	TestEqual(TEXT("All shows every category"), CountVisible(true, ETowerStoreCategory::Attack), 3);
	TestEqual(TEXT("Attack shows one category"), CountVisible(false, ETowerStoreCategory::Attack), 1);
	TestEqual(TEXT("Defense shows one category"), CountVisible(false, ETowerStoreCategory::Defense), 1);
	TestEqual(TEXT("Support shows one category"), CountVisible(false, ETowerStoreCategory::Support), 1);
	return true;
}
#endif
