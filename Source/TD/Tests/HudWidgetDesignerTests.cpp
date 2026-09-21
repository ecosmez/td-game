#if WITH_DEV_AUTOMATION_TESTS

#include "../AbilityBarWidget.h"
#include "../CameraOrbitGizmoWidget.h"
#include "../CaptureChannelWidget.h"
#include "../ChampionFrameWidget.h"
#include "../CrystalHealthBarWidget.h"
#include "../FloatingDamageTextWidget.h"
#include "../MinimapWidget.h"
#include "../TowerStoreWidget.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/CanvasPanel.h"
#include "Components/Image.h"
#include "Components/ProgressBar.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Engine/Texture2D.h"
#include "Layout/Geometry.h"
#include "Misc/AutomationTest.h"
#include "Styling/SlateBrush.h"

namespace HudWidgetDesignerTestPrivate
{
	template <typename TWidget>
	TWidget* MakeCxxHud()
	{
		return NewObject<TWidget>();
	}

	template <typename TWidget>
	bool InitCxxHud(FAutomationTestBase& Test, TWidget* Widget, const TCHAR* Label)
	{
		Test.TestNotNull(Label, Widget);
		if (!Widget)
		{
			return false;
		}
		Test.TestTrue(TEXT("widget initializes"), Widget->Initialize());
		Widget->TakeWidget();
		return true;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FHudWidgetsDoNotInventDesignerTreesTest,
	"TD.UI.Hud.CxxClassDoesNotInventDesignerTree",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FHudWidgetsDoNotInventDesignerTreesTest::RunTest(const FString& Parameters)
{
	using namespace HudWidgetDesignerTestPrivate;

	if (UCrystalHealthBarWidget* Crystal = MakeCxxHud<UCrystalHealthBarWidget>())
	{
		if (!InitCxxHud(*this, Crystal, TEXT("crystal HUD")))
		{
			return false;
		}
		TestNull(TEXT("crystal C++ class does not invent TopHudStack"),
			Crystal->GetWidgetFromName(TEXT("TopHudStack")));
		TestNull(TEXT("crystal C++ class does not invent BaseHealthBar"),
			Crystal->GetWidgetFromName(TEXT("BaseHealthBar")));
	}

	if (UAbilityBarWidget* Bar = MakeCxxHud<UAbilityBarWidget>())
	{
		if (!InitCxxHud(*this, Bar, TEXT("ability bar")))
		{
			return false;
		}
		TestNull(TEXT("ability C++ class does not invent AbilitySlotRow"),
			Bar->GetWidgetFromName(TEXT("AbilitySlotRow")));
		TestNull(TEXT("ability C++ class does not invent Btn_Q"),
			Bar->GetWidgetFromName(TEXT("Btn_Q")));
	}

	if (UChampionFrameWidget* Frame = MakeCxxHud<UChampionFrameWidget>())
	{
		if (!InitCxxHud(*this, Frame, TEXT("champion frame")))
		{
			return false;
		}
		TestNull(TEXT("champion C++ class does not invent ChampionFrameChrome"),
			Frame->GetWidgetFromName(TEXT("ChampionFrameChrome")));
	}

	if (UMinimapWidget* Mini = MakeCxxHud<UMinimapWidget>())
	{
		if (!InitCxxHud(*this, Mini, TEXT("minimap")))
		{
			return false;
		}
		TestNull(TEXT("minimap C++ class does not invent MinimapFrame"),
			Mini->GetWidgetFromName(TEXT("MinimapFrame")));
	}

	if (UTowerStoreWidget* Store = MakeCxxHud<UTowerStoreWidget>())
	{
		if (!InitCxxHud(*this, Store, TEXT("tower store")))
		{
			return false;
		}
		TestNull(TEXT("store C++ class does not invent StorePanel"),
			Store->GetWidgetFromName(TEXT("StorePanel")));
	}

	if (UCameraOrbitGizmoWidget* Gizmo = MakeCxxHud<UCameraOrbitGizmoWidget>())
	{
		if (!InitCxxHud(*this, Gizmo, TEXT("orbit gizmo")))
		{
			return false;
		}
		TestNull(TEXT("orbit C++ class does not invent OrbitRing"),
			Gizmo->GetWidgetFromName(TEXT("OrbitRing")));
	}

	if (UFloatingDamageTextWidget* Damage = MakeCxxHud<UFloatingDamageTextWidget>())
	{
		if (!InitCxxHud(*this, Damage, TEXT("floating damage")))
		{
			return false;
		}
		TestNull(TEXT("damage C++ class does not invent FloatingDamageLabel"),
			Damage->GetWidgetFromName(TEXT("FloatingDamageLabel")));
	}

	if (UCaptureChannelWidget* Channel = MakeCxxHud<UCaptureChannelWidget>())
	{
		if (!InitCxxHud(*this, Channel, TEXT("capture channel")))
		{
			return false;
		}
		TestNull(TEXT("channel C++ class does not invent ChannelBar"),
			Channel->GetWidgetFromName(TEXT("ChannelBar")));
	}

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FHudWidgetsResolveDesignerBlueprintPathsTest,
	"TD.UI.Hud.ResolvesDesignerWidgetBlueprints",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FHudWidgetsResolveDesignerBlueprintPathsTest::RunTest(const FString& Parameters)
{
	TestEqual(TEXT("ability bar path"),
		FString(UAbilityBarWidget::GetWidgetBlueprintPath()),
		FString(TEXT("/Game/TD/UI/WBP_AbilityBar.WBP_AbilityBar_C")));
	TestEqual(TEXT("tower store path"),
		FString(UTowerStoreWidget::GetWidgetBlueprintPath()),
		FString(TEXT("/Game/TD/UI/WBP_TowerStore.WBP_TowerStore_C")));
	TestEqual(TEXT("crystal health path"),
		FString(UCrystalHealthBarWidget::GetWidgetBlueprintPath()),
		FString(TEXT("/Game/TD/UI/WBP_CrystalHealthBar.WBP_CrystalHealthBar_C")));
	TestEqual(TEXT("capture channel path"),
		FString(UCaptureChannelWidget::GetWidgetBlueprintPath()),
		FString(TEXT("/Game/TD/UI/WBP_CaptureChannel.WBP_CaptureChannel_C")));
	TestEqual(TEXT("champion frame path"),
		FString(UChampionFrameWidget::GetWidgetBlueprintPath()),
		FString(TEXT("/Game/TD/UI/WBP_ChampionFrame.WBP_ChampionFrame_C")));
	TestEqual(TEXT("minimap path"),
		FString(UMinimapWidget::GetWidgetBlueprintPath()),
		FString(TEXT("/Game/TD/UI/WBP_Minimap.WBP_Minimap_C")));
	TestEqual(TEXT("orbit gizmo path"),
		FString(UCameraOrbitGizmoWidget::GetWidgetBlueprintPath()),
		FString(TEXT("/Game/TD/UI/WBP_CameraOrbitGizmo.WBP_CameraOrbitGizmo_C")));
	TestEqual(TEXT("floating damage path"),
		FString(UFloatingDamageTextWidget::GetWidgetBlueprintPath()),
		FString(TEXT("/Game/TD/UI/WBP_FloatingDamage.WBP_FloatingDamage_C")));

	auto ExpectDesignerClass = [this](UClass* Loaded, UClass* Native, const TCHAR* Label)
	{
		TestNotNull(Label, Loaded);
		if (Loaded && Native)
		{
			TestTrue(FString::Printf(TEXT("%s is a designer WBP, not the native C++ class"), Label),
				Loaded != Native);
			TestTrue(FString::Printf(TEXT("%s subclasses the C++ HUD"), Label),
				Loaded->IsChildOf(Native));
		}
	};

	ExpectDesignerClass(UAbilityBarWidget::ResolveWidgetClass().Get(),
		UAbilityBarWidget::StaticClass(), TEXT("ability bar"));
	ExpectDesignerClass(UTowerStoreWidget::ResolveWidgetClass().Get(),
		UTowerStoreWidget::StaticClass(), TEXT("tower store"));
	ExpectDesignerClass(UCrystalHealthBarWidget::ResolveWidgetClass().Get(),
		UCrystalHealthBarWidget::StaticClass(), TEXT("crystal health"));
	ExpectDesignerClass(UCaptureChannelWidget::ResolveWidgetClass().Get(),
		UCaptureChannelWidget::StaticClass(), TEXT("capture channel"));
	ExpectDesignerClass(UChampionFrameWidget::ResolveWidgetClass().Get(),
		UChampionFrameWidget::StaticClass(), TEXT("champion frame"));
	ExpectDesignerClass(UMinimapWidget::ResolveWidgetClass().Get(),
		UMinimapWidget::StaticClass(), TEXT("minimap"));
	ExpectDesignerClass(UCameraOrbitGizmoWidget::ResolveWidgetClass().Get(),
		UCameraOrbitGizmoWidget::StaticClass(), TEXT("orbit gizmo"));
	ExpectDesignerClass(UFloatingDamageTextWidget::ResolveWidgetClass().Get(),
		UFloatingDamageTextWidget::StaticClass(), TEXT("floating damage"));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FCaptureChannelBindsDesignerProgressBarTest,
	"TD.UI.CaptureChannel.BindsDesignerProgressBar",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCaptureChannelBindsDesignerProgressBarTest::RunTest(const FString& Parameters)
{
	UCaptureChannelWidget* Widget = NewObject<UCaptureChannelWidget>();
	TestNotNull(TEXT("channel widget can be created"), Widget);
	if (!Widget || !Widget->WidgetTree)
	{
		return false;
	}

	USizeBox* Size = Widget->WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("BarSize"));
	Size->SetWidthOverride(80.f);
	Size->SetHeightOverride(10.f);
	UProgressBar* Bar = Widget->WidgetTree->ConstructWidget<UProgressBar>(UProgressBar::StaticClass(), TEXT("ChannelBar"));
	Size->SetContent(Bar);
	Widget->WidgetTree->RootWidget = Size;

	TestTrue(TEXT("widget initializes"), Widget->Initialize());
	Widget->TakeWidget();

	UProgressBar* Bound = Cast<UProgressBar>(Widget->GetWidgetFromName(TEXT("ChannelBar")));
	TestEqual(TEXT("binds the designer ChannelBar"), Bound, Bar);
	if (!Bound)
	{
		return false;
	}

	Widget->SetProgress(0.4f);
	TestEqual(TEXT("progress is applied"), Bound->GetPercent(), 0.4f);
	Widget->SetProgress(1.7f);
	TestEqual(TEXT("progress clamps to 1"), Bound->GetPercent(), 1.f);
	Widget->SetProgress(-0.2f);
	TestEqual(TEXT("progress clamps to 0"), Bound->GetPercent(), 0.f);

	const FVector2D Designed = Widget->GetDesignedDrawSize();
	TestTrue(TEXT("designed size has a usable width"), Designed.X > 1.f);
	TestTrue(TEXT("designed size has a usable height"), Designed.Y > 1.f);

	const FLinearColor ExpectedEnemyFill(0.95f, 0.18f, 0.16f, 1.f);
	Widget->SetFillColor(ExpectedEnemyFill);
	TestEqual(TEXT("fill color is applied"), Bound->GetFillColorAndOpacity(), ExpectedEnemyFill);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FMinimapPreservesDesignerMarkerColorsOnTickTest,
	"TD.UI.Minimap.PreservesDesignerMarkerColorsOnTick",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FMinimapPreservesDesignerMarkerColorsOnTickTest::RunTest(const FString& Parameters)
{
	UMinimapWidget* Widget = NewObject<UMinimapWidget>();
	TestNotNull(TEXT("minimap can be created"), Widget);
	if (!Widget || !Widget->WidgetTree)
	{
		return false;
	}

	UCanvasPanel* Root = Widget->WidgetTree->ConstructWidget<UCanvasPanel>(
		UCanvasPanel::StaticClass(), TEXT("MinimapRoot"));
	Widget->WidgetTree->RootWidget = Root;

	USizeBox* Size = Widget->WidgetTree->ConstructWidget<USizeBox>(
		USizeBox::StaticClass(), TEXT("MinimapSizeBox"));
	Root->AddChild(Size);
	UBorder* Frame = Widget->WidgetTree->ConstructWidget<UBorder>(
		UBorder::StaticClass(), TEXT("MinimapFrame"));
	Size->SetContent(Frame);
	UCanvasPanel* Canvas = Widget->WidgetTree->ConstructWidget<UCanvasPanel>(
		UCanvasPanel::StaticClass(), TEXT("MinimapCanvas"));
	Frame->SetContent(Canvas);
	UImage* MapImage = Widget->WidgetTree->ConstructWidget<UImage>(
		UImage::StaticClass(), TEXT("MinimapImage"));
	Canvas->AddChild(MapImage);
	UBorder* Champion = Widget->WidgetTree->ConstructWidget<UBorder>(
		UBorder::StaticClass(), TEXT("ChampionMarker"));
	Canvas->AddChild(Champion);
	UBorder* Crystal = Widget->WidgetTree->ConstructWidget<UBorder>(
		UBorder::StaticClass(), TEXT("CrystalMarker"));
	Canvas->AddChild(Crystal);

	TestTrue(TEXT("widget initializes"), Widget->Initialize());
	Widget->TakeWidget();

	const FLinearColor Sentinel(0.12f, 0.84f, 0.33f, 1.f);
	Frame->SetBrushColor(Sentinel);
	Champion->SetBrushColor(Sentinel);
	Crystal->SetBrushColor(Sentinel);

	Widget->ChampionMarkerColor = FLinearColor(1.f, 0.f, 0.f, 1.f);
	Widget->CrystalMarkerColor = FLinearColor(1.f, 0.f, 0.f, 1.f);
	Widget->FrameColor = FLinearColor(1.f, 0.f, 0.f, 1.f);

	Widget->NativeTick(FGeometry(), 0.f);

	TestEqual(TEXT("minimap frame keeps the designer tint"),
		Frame->GetBrushColor(), Sentinel);
	TestEqual(TEXT("champion marker keeps the designer tint"),
		Champion->GetBrushColor(), Sentinel);
	TestEqual(TEXT("crystal marker keeps the designer tint"),
		Crystal->GetBrushColor(), Sentinel);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FMinimapPreservesDesignerMapBrushOnTickTest,
	"TD.UI.Minimap.PreservesDesignerMapBrushOnTick",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FMinimapPreservesDesignerMapBrushOnTickTest::RunTest(const FString& Parameters)
{
	UMinimapWidget* Widget = NewObject<UMinimapWidget>();
	TestNotNull(TEXT("minimap can be created"), Widget);
	if (!Widget || !Widget->WidgetTree)
	{
		return false;
	}

	UCanvasPanel* Root = Widget->WidgetTree->ConstructWidget<UCanvasPanel>(
		UCanvasPanel::StaticClass(), TEXT("MinimapRoot"));
	Widget->WidgetTree->RootWidget = Root;
	USizeBox* Size = Widget->WidgetTree->ConstructWidget<USizeBox>(
		USizeBox::StaticClass(), TEXT("MinimapSizeBox"));
	Root->AddChild(Size);
	UBorder* Frame = Widget->WidgetTree->ConstructWidget<UBorder>(
		UBorder::StaticClass(), TEXT("MinimapFrame"));
	Size->SetContent(Frame);
	UCanvasPanel* Canvas = Widget->WidgetTree->ConstructWidget<UCanvasPanel>(
		UCanvasPanel::StaticClass(), TEXT("MinimapCanvas"));
	Frame->SetContent(Canvas);
	UImage* MapImage = Widget->WidgetTree->ConstructWidget<UImage>(
		UImage::StaticClass(), TEXT("MinimapImage"));
	Canvas->AddChild(MapImage);

	TestFalse(TEXT("minimap keeps the designer texture instead of a live scene capture"),
		Widget->bUseLiveSceneCapture);

	UTexture2D* Sentinel = UTexture2D::CreateTransient(8, 8, PF_B8G8R8A8);
	TestNotNull(TEXT("sentinel map texture"), Sentinel);
	if (!Sentinel)
	{
		return false;
	}

	FSlateBrush Designed;
	Designed.SetResourceObject(Sentinel);
	Designed.DrawAs = ESlateBrushDrawType::Image;
	MapImage->SetBrush(Designed);

	TestTrue(TEXT("widget initializes"), Widget->Initialize());
	Widget->TakeWidget();
	Widget->NativeTick(FGeometry(), 0.f);

	TestEqual(TEXT("minimap image keeps the designer texture"),
		MapImage->GetBrush().GetResourceObject(), static_cast<UObject*>(Sentinel));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FMinimapCreatesMapImageWhenDesignerOmitsItTest,
	"TD.UI.Minimap.CreatesMapImageWhenDesignerOmitsIt",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FMinimapCreatesMapImageWhenDesignerOmitsItTest::RunTest(const FString& Parameters)
{
	UMinimapWidget* Widget = NewObject<UMinimapWidget>();
	TestNotNull(TEXT("minimap can be created"), Widget);
	if (!Widget || !Widget->WidgetTree)
	{
		return false;
	}

	UCanvasPanel* Root = Widget->WidgetTree->ConstructWidget<UCanvasPanel>(
		UCanvasPanel::StaticClass(), TEXT("MinimapRoot"));
	Widget->WidgetTree->RootWidget = Root;
	USizeBox* Size = Widget->WidgetTree->ConstructWidget<USizeBox>(
		USizeBox::StaticClass(), TEXT("MinimapSizeBox"));
	Root->AddChild(Size);
	UBorder* Frame = Widget->WidgetTree->ConstructWidget<UBorder>(
		UBorder::StaticClass(), TEXT("MinimapFrame"));
	Size->SetContent(Frame);
	UCanvasPanel* Canvas = Widget->WidgetTree->ConstructWidget<UCanvasPanel>(
		UCanvasPanel::StaticClass(), TEXT("MinimapCanvas"));
	Frame->SetContent(Canvas);

	TestNull(TEXT("designer tree has no MinimapImage yet"),
		Widget->GetWidgetFromName(TEXT("MinimapImage")));

	TestTrue(TEXT("widget initializes"), Widget->Initialize());
	Widget->TakeWidget();

	UImage* MapImage = Cast<UImage>(Widget->GetWidgetFromName(TEXT("MinimapImage")));
	TestNotNull(TEXT("C++ creates MinimapImage so the designer map has a target"), MapImage);
	if (MapImage)
	{
		TestEqual(TEXT("created map image is visible"),
			MapImage->GetVisibility(), ESlateVisibility::Visible);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FChampionFramePreservesDesignerNameOnTickTest,
	"TD.UI.ChampionFrame.PreservesDesignerNameOnTick",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FChampionFramePreservesDesignerNameOnTickTest::RunTest(const FString& Parameters)
{
	UChampionFrameWidget* Widget = NewObject<UChampionFrameWidget>();
	TestNotNull(TEXT("champion frame can be created"), Widget);
	if (!Widget || !Widget->WidgetTree)
	{
		return false;
	}

	UCanvasPanel* Root = Widget->WidgetTree->ConstructWidget<UCanvasPanel>(
		UCanvasPanel::StaticClass(), TEXT("ChampionFrameRoot"));
	Widget->WidgetTree->RootWidget = Root;

	auto AddNamed = [Widget, Root](UClass* Class, FName Name) -> UWidget*
	{
		UWidget* Child = Widget->WidgetTree->ConstructWidget<UWidget>(Class, Name);
		Root->AddChild(Child);
		return Child;
	};

	AddNamed(UBorder::StaticClass(), TEXT("ChampionFrameChrome"));
	AddNamed(UBorder::StaticClass(), TEXT("ChampionAvatarFrame"));
	AddNamed(UProgressBar::StaticClass(), TEXT("ChampionHpBar"));
	AddNamed(UTextBlock::StaticClass(), TEXT("ChampionHpValue"));
	UTextBlock* Name = Cast<UTextBlock>(
		AddNamed(UTextBlock::StaticClass(), TEXT("ChampionName")));

	TestNotNull(TEXT("champion name exists"), Name);
	if (!Name)
	{
		return false;
	}

	TestTrue(TEXT("widget initializes"), Widget->Initialize());
	Widget->TakeWidget();

	const FText NameSentinel = FText::FromString(TEXT("MY HERO"));
	Name->SetText(NameSentinel);
	Widget->NativeTick(FGeometry(), 0.f);

	TestEqual(TEXT("champion name keeps the designer text"),
		Name->GetText().ToString(), NameSentinel.ToString());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FChampionFramePreservesDesignerAvatarOnTickTest,
	"TD.UI.ChampionFrame.PreservesDesignerAvatarOnTick",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FChampionFramePreservesDesignerAvatarOnTickTest::RunTest(const FString& Parameters)
{
	UChampionFrameWidget* Widget = NewObject<UChampionFrameWidget>();
	TestNotNull(TEXT("champion frame can be created"), Widget);
	if (!Widget || !Widget->WidgetTree)
	{
		return false;
	}

	UCanvasPanel* Root = Widget->WidgetTree->ConstructWidget<UCanvasPanel>(
		UCanvasPanel::StaticClass(), TEXT("ChampionFrameRoot"));
	Widget->WidgetTree->RootWidget = Root;

	auto AddNamed = [Widget, Root](UClass* Class, FName Name) -> UWidget*
	{
		UWidget* Child = Widget->WidgetTree->ConstructWidget<UWidget>(Class, Name);
		Root->AddChild(Child);
		return Child;
	};

	AddNamed(UBorder::StaticClass(), TEXT("ChampionFrameChrome"));
	AddNamed(UBorder::StaticClass(), TEXT("ChampionAvatarFrame"));
	UProgressBar* HealthBar = Cast<UProgressBar>(
		AddNamed(UProgressBar::StaticClass(), TEXT("ChampionHpBar")));
	UTextBlock* HealthValue = Cast<UTextBlock>(
		AddNamed(UTextBlock::StaticClass(), TEXT("ChampionHpValue")));
	UImage* Avatar = Cast<UImage>(
		AddNamed(UImage::StaticClass(), TEXT("ChampionAvatarImage")));
	UTextBlock* Letter = Cast<UTextBlock>(
		AddNamed(UTextBlock::StaticClass(), TEXT("ChampionAvatarLetter")));

	TestNotNull(TEXT("avatar image exists"), Avatar);
	TestNotNull(TEXT("avatar letter exists"), Letter);
	TestNotNull(TEXT("health bar exists"), HealthBar);
	TestNotNull(TEXT("health value exists"), HealthValue);
	if (!Avatar || !Letter || !HealthBar || !HealthValue)
	{
		return false;
	}

	UTexture2D* Sentinel = UTexture2D::CreateTransient(8, 8, PF_B8G8R8A8);
	TestNotNull(TEXT("sentinel avatar texture"), Sentinel);
	if (!Sentinel)
	{
		return false;
	}

	FSlateBrush Designed;
	Designed.SetResourceObject(Sentinel);
	Designed.DrawAs = ESlateBrushDrawType::Image;
	Avatar->SetBrush(Designed);
	Avatar->SetVisibility(ESlateVisibility::Visible);
	Letter->SetVisibility(ESlateVisibility::HitTestInvisible);
	HealthBar->SetFillColorAndOpacity(FLinearColor(0.12f, 0.84f, 0.33f, 1.f));
	HealthValue->SetText(FText::FromString(TEXT("99 / 99")));

	TestTrue(TEXT("widget initializes"), Widget->Initialize());
	Widget->TakeWidget();
	Widget->NativeTick(FGeometry(), 0.f);

	TestEqual(TEXT("avatar keeps the designer texture"),
		Avatar->GetBrush().GetResourceObject(), static_cast<UObject*>(Sentinel));
	TestEqual(TEXT("avatar keeps the designer visibility"),
		Avatar->GetVisibility(), ESlateVisibility::Visible);
	TestEqual(TEXT("letter keeps the designer visibility"),
		Letter->GetVisibility(), ESlateVisibility::HitTestInvisible);
	TestEqual(TEXT("health fill keeps the designer color"),
		HealthBar->GetFillColorAndOpacity(), FLinearColor(0.12f, 0.84f, 0.33f, 1.f));
	TestEqual(TEXT("health value keeps the designer text when no pawn is possessed"),
		HealthValue->GetText().ToString(), FString(TEXT("99 / 99")));
	return true;
}

#endif
