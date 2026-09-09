#include "AbilityBarWidget.h"

#include "ChampionFrameWidget.h"
#include "CrystalHealthBarWidget.h"
#include "MinimapWidget.h"
#include "MobaPlayerController.h"
#include "TowerStoreWidget.h"
#include "TDHudWidgetLibrary.h"
#include "TDUIInputLibrary.h"
#include "Blueprint/UserWidget.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "EngineUtils.h"
#include "GameFramework/Actor.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Styling/CoreStyle.h"
#include "Styling/SlateBrush.h"
#include "UObject/UObjectIterator.h"
#include "UObject/UnrealType.h"

namespace AbilityBarPrivate
{
	static FLinearColor SlotReadyBg(0.12f, 0.22f, 0.32f, 0.96f);
	static FLinearColor SlotReadyFrame(0.45f, 0.85f, 0.95f, 1.f);
	static FLinearColor SlotCdBg(0.06f, 0.08f, 0.12f, 0.96f);
	static FLinearColor SlotCdFrame(0.35f, 0.40f, 0.48f, 1.f);
	static FLinearColor SlotAimFrame(0.98f, 0.82f, 0.20f, 1.f);
	static FLinearColor SlotLockedBg(0.04f, 0.04f, 0.06f, 0.98f);
	static FLinearColor SlotLockedFrame(0.28f, 0.28f, 0.32f, 1.f);
	static FLinearColor CdOverlay(0.02f, 0.03f, 0.06f, 0.82f);
	static FLinearColor KeyColor(0.95f, 0.98f, 1.f, 1.f);
	static FLinearColor KeyColorDim(0.55f, 0.58f, 0.65f, 1.f);
	static FLinearColor CdTextColor(1.f, 0.90f, 0.30f, 1.f);
	static FLinearColor LockTextColor(0.72f, 0.74f, 0.80f, 1.f);
	static FLinearColor StorePlusBg(0.12f, 0.42f, 0.55f, 0.95f);
	static FLinearColor StorePlusBgOpen(0.55f, 0.28f, 0.18f, 0.95f);
	static FLinearColor StorePlusFrame(0.45f, 0.78f, 0.88f, 1.f);
}

UAbilityBarWidget::UAbilityBarWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	SetIsFocusable(false);
	// Ensure NativeTick runs even when the Widget BP has no Event Tick node.
	bHasScriptImplementedTick = true;
}

void UAbilityBarWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	EnsureBuilt();
}

void UAbilityBarWidget::NativeConstruct()
{
	Super::NativeConstruct();
	EnsureBuilt();
	// Root ignores empty space; bar chrome + store plus + ability slots receive clicks.
	ApplyHitTestPolicy();
	UE_LOG(LogTemp, Display, TEXT("AbilityBarWidget constructed. Built=%d Root=%s"),
		bBuilt ? 1 : 0,
		WidgetTree && WidgetTree->RootWidget ? *WidgetTree->RootWidget->GetName() : TEXT("None"));

	// Ensure companion HUD pieces exist for non-MOBA controllers as well.
	if (APlayerController* PC = GetOwningPlayer())
	{
		bool bMinimapAlready = false;
		for (TObjectIterator<UMinimapWidget> It; It; ++It)
		{
			if (It->GetOwningPlayer() == PC && It->IsInViewport())
			{
				bMinimapAlready = true;
				break;
			}
		}
		if (!bMinimapAlready)
		{
			if (UMinimapWidget* Mini = UTDHudWidgetLibrary::CreateTypedHudWidget<UMinimapWidget>(
				PC, UMinimapWidget::GetWidgetBlueprintPath()))
			{
				Mini->AddToViewport(20);
			}
		}

		bool bCrystalBarAlready = false;
		for (TObjectIterator<UCrystalHealthBarWidget> It; It; ++It)
		{
			if (It->GetOwningPlayer() == PC && It->IsInViewport())
			{
				bCrystalBarAlready = true;
				break;
			}
		}
		if (!bCrystalBarAlready)
		{
			if (UCrystalHealthBarWidget* CrystalBar = UTDHudWidgetLibrary::CreateTypedHudWidget<UCrystalHealthBarWidget>(
				PC, UCrystalHealthBarWidget::GetWidgetBlueprintPath()))
			{
				// Below ability bar Z=100; top-center so it does not overlap the bar.
				CrystalBar->AddToViewport(90);
			}
		}

		bool bChampionFrameAlready = false;
		for (TObjectIterator<UChampionFrameWidget> It; It; ++It)
		{
			if (It->GetOwningPlayer() == PC && It->IsInViewport())
			{
				bChampionFrameAlready = true;
				break;
			}
		}
		if (!bChampionFrameAlready)
		{
			if (UChampionFrameWidget* Frame = UTDHudWidgetLibrary::CreateTypedHudWidget<UChampionFrameWidget>(
				PC, UChampionFrameWidget::GetWidgetBlueprintPath()))
			{
				// Below ability bar Z=100; bottom-left unit frame (abilities dock over it).
				Frame->AddToViewport(92);
			}
		}
	}
}

void UAbilityBarWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	if (!bBuilt)
	{
		EnsureBuilt();
	}

	RefreshStorePlusVisual();

	if (APawn* Pawn = ResolveChampionPawn())
	{
		RefreshFromPawn(Pawn);
	}
}

const TCHAR* UAbilityBarWidget::GetWidgetBlueprintPath()
{
	return TEXT("/Game/TD/UI/WBP_AbilityBar.WBP_AbilityBar_C");
}

TSubclassOf<UAbilityBarWidget> UAbilityBarWidget::ResolveWidgetClass()
{
	return LoadClass<UAbilityBarWidget>(nullptr, GetWidgetBlueprintPath());
}

bool UAbilityBarWidget::ShouldBlockWorldClickInput(const UObject* WorldContextObject)
{
	return UTDUIInputLibrary::ShouldBlockChampionClickToMove(WorldContextObject, true);
}

void UAbilityBarWidget::EnsureBuilt()
{
	if (bBuilt && StorePlusButton && SlotQ.Button && SlotR.Button && SlotQ.CooldownClip)
	{
		return;
	}

	if (!WidgetTree)
	{
		return;
	}

	BindDesignerWidgets();
	CacheSlotSizeFromDesigner();

	bBuilt = SlotQ.Button != nullptr && SlotR.Button != nullptr
		&& SlotQ.CooldownClip != nullptr
		&& StorePlusButton != nullptr
		&& BarSizeBox != nullptr;
	if (bBuilt)
	{
		ApplyHitTestPolicy();
	}
}

void UAbilityBarWidget::BindDesignerWidgets()
{
	SlotRow = Cast<UHorizontalBox>(GetWidgetFromName(TEXT("AbilitySlotRow")));
	BarChrome = Cast<UBorder>(GetWidgetFromName(TEXT("AbilityBarChrome")));
	BarSizeBox = Cast<USizeBox>(GetWidgetFromName(TEXT("AbilityBarSize")));
	StorePlusSizeBox = Cast<USizeBox>(GetWidgetFromName(TEXT("StorePlusSize")));
	StorePlusFrame = Cast<UBorder>(GetWidgetFromName(TEXT("StorePlusFrame")));
	if (!StorePlusFrame)
	{
		StorePlusFrame = Cast<UBorder>(GetWidgetFromName(TEXT("PlusFrame")));
	}
	StorePlusButton = Cast<UButton>(GetWidgetFromName(TEXT("StorePlusButton")));
	if (!StorePlusButton)
	{
		StorePlusButton = Cast<UButton>(GetWidgetFromName(TEXT("PlusButton")));
	}
	StorePlusLabel = Cast<UTextBlock>(GetWidgetFromName(TEXT("StorePlusLabel")));
	if (!StorePlusLabel)
	{
		StorePlusLabel = Cast<UTextBlock>(GetWidgetFromName(TEXT("PlusLabel")));
	}

	SlotQ = BindSlot(TEXT('Q'), 1);
	SlotW = BindSlot(TEXT('W'), 2);
	SlotE = BindSlot(TEXT('E'), 3);
	SlotR = BindSlot(TEXT('R'), 4);

	BindStorePlusClick();
	BindSlotClicks(SlotQ);
	BindSlotClicks(SlotW);
	BindSlotClicks(SlotE);
	BindSlotClicks(SlotR);
}

void UAbilityBarWidget::BindStorePlusClick()
{
	if (StorePlusButton && !StorePlusButton->OnClicked.IsBound())
	{
		StorePlusButton->OnClicked.AddDynamic(this, &UAbilityBarWidget::OnStorePlusClicked);
	}
}

void UAbilityBarWidget::OnStorePlusClicked()
{
	ToggleStore();
}

void UAbilityBarWidget::ToggleStore()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	for (TObjectIterator<UTowerStoreWidget> It; It; ++It)
	{
		UTowerStoreWidget* Store = *It;
		if (!IsValid(Store) || Store->GetWorld() != World)
		{
			continue;
		}
		Store->ToggleStore();
		RefreshStorePlusVisual();
		return;
	}

	if (APlayerController* PC = GetOwningPlayer())
	{
		if (UUserWidget* Widget = UTDUIInputLibrary::CreateAndShowTowerStore(this, PC, 120))
		{
			if (UTowerStoreWidget* Store = Cast<UTowerStoreWidget>(Widget))
			{
				Store->SetStoreOpen(true);
			}
			RefreshStorePlusVisual();
		}
	}
}

void UAbilityBarWidget::RefreshStorePlusVisual()
{
	if (!StorePlusButton || !StorePlusLabel)
	{
		return;
	}

	bool bOpen = false;
	UWorld* World = GetWorld();
	if (World)
	{
		for (TObjectIterator<UTowerStoreWidget> It; It; ++It)
		{
			UTowerStoreWidget* Store = *It;
			if (IsValid(Store) && Store->GetWorld() == World && Store->IsInViewport())
			{
				bOpen = Store->IsStoreOpen();
				break;
			}
		}
	}

	StorePlusButton->SetBackgroundColor(bOpen ? AbilityBarPrivate::StorePlusBgOpen : AbilityBarPrivate::StorePlusBg);
	StorePlusLabel->SetText(FText::FromString(bOpen ? TEXT("-") : TEXT("+")));
	if (StorePlusFrame)
	{
		StorePlusFrame->SetBrushColor(bOpen
			? FLinearColor(0.9f, 0.55f, 0.35f, 1.f)
			: AbilityBarPrivate::StorePlusFrame);
	}
}

void UAbilityBarWidget::BindSlotClicks(FAbilityBarSlotWidgets& SlotUI)
{
	if (!SlotUI.Button || SlotUI.Button->OnClicked.IsBound())
	{
		return;
	}
	if (SlotUI.AbilityId == 1)
	{
		SlotUI.Button->OnClicked.AddDynamic(this, &UAbilityBarWidget::OnSlotQClicked);
	}
	else if (SlotUI.AbilityId == 2)
	{
		SlotUI.Button->OnClicked.AddDynamic(this, &UAbilityBarWidget::OnSlotWClicked);
	}
	else if (SlotUI.AbilityId == 3)
	{
		SlotUI.Button->OnClicked.AddDynamic(this, &UAbilityBarWidget::OnSlotEClicked);
	}
	else if (SlotUI.AbilityId == 4)
	{
		SlotUI.Button->OnClicked.AddDynamic(this, &UAbilityBarWidget::OnSlotRClicked);
	}
}

void UAbilityBarWidget::CacheSlotSizeFromDesigner()
{
	if (SlotQ.SizeBox && SlotQ.SizeBox->IsWidthOverride())
	{
		EffectiveSlotSize = SlotQ.SizeBox->GetWidthOverride();
	}
	else
	{
		EffectiveSlotSize = SlotSize;
	}
}

FAbilityBarSlotWidgets UAbilityBarWidget::BindSlot(TCHAR KeyChar, int32 AbilityId)
{
	FAbilityBarSlotWidgets Out;
	Out.KeyChar = KeyChar;
	Out.AbilityId = AbilityId;
	const FString KeyStr = FString(1, &KeyChar);
	Out.SizeBox = Cast<USizeBox>(GetWidgetFromName(*FString::Printf(TEXT("Size_%s"), *KeyStr)));
	Out.Frame = Cast<UBorder>(GetWidgetFromName(*FString::Printf(TEXT("Frame_%s"), *KeyStr)));
	Out.Button = Cast<UButton>(GetWidgetFromName(*FString::Printf(TEXT("Btn_%s"), *KeyStr)));
	Out.CooldownClip = Cast<USizeBox>(GetWidgetFromName(*FString::Printf(TEXT("CDClip_%s"), *KeyStr)));
	Out.CooldownFill = Cast<UBorder>(GetWidgetFromName(*FString::Printf(TEXT("CDFill_%s"), *KeyStr)));
	Out.KeyLabel = Cast<UTextBlock>(GetWidgetFromName(*FString::Printf(TEXT("Key_%s"), *KeyStr)));
	Out.CooldownText = Cast<UTextBlock>(GetWidgetFromName(*FString::Printf(TEXT("CDText_%s"), *KeyStr)));
	Out.LockText = Cast<UTextBlock>(GetWidgetFromName(*FString::Printf(TEXT("Lock_%s"), *KeyStr)));
	return Out;
}

void UAbilityBarWidget::ApplyHitTestPolicy()
{
	// Full-screen root does not eat world clicks; chrome + interactive plus do.
	SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	if (UWidget* Root = WidgetTree ? WidgetTree->RootWidget : nullptr)
	{
		Root->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	}
	if (BarChrome)
	{
		BarChrome->SetVisibility(ESlateVisibility::Visible);
	}
	if (SlotRow)
	{
		SlotRow->SetVisibility(ESlateVisibility::Visible);
	}
	if (StorePlusButton)
	{
		StorePlusButton->SetVisibility(ESlateVisibility::Visible);
	}
	auto ShowSlot = [](FAbilityBarSlotWidgets& SlotUI)
	{
		if (SlotUI.Button)
		{
			SlotUI.Button->SetVisibility(ESlateVisibility::Visible);
			SlotUI.Button->SetIsEnabled(true);
		}
	};
	ShowSlot(SlotQ);
	ShowSlot(SlotW);
	ShowSlot(SlotE);
	ShowSlot(SlotR);
}

void UAbilityBarWidget::OnAbilitySlotClicked(int32 AbilityId)
{
	TryBeginAbilityAim(AbilityId);
}

void UAbilityBarWidget::OnSlotQClicked()
{
	OnAbilitySlotClicked(1);
}

void UAbilityBarWidget::OnSlotWClicked()
{
	OnAbilitySlotClicked(2);
}

void UAbilityBarWidget::OnSlotEClicked()
{
	OnAbilitySlotClicked(3);
}

void UAbilityBarWidget::OnSlotRClicked()
{
	OnAbilitySlotClicked(4);
}

void UAbilityBarWidget::TryBeginAbilityAim(int32 AbilityId)
{
	APawn* Pawn = ResolveChampionPawn();
	if (!Pawn)
	{
		return;
	}

	UFunction* Fn = Pawn->FindFunction(FName(TEXT("BeginAbilityAim")));
	if (!Fn)
	{
		Fn = Pawn->FindFunction(FName(TEXT("beginAbilityAim")));
	}
	if (!Fn)
	{
		UE_LOG(LogTemp, Warning, TEXT("AbilityBar: BeginAbilityAim not found on %s"), *Pawn->GetName());
		return;
	}

	const int32 ParamsSize = Fn->ParmsSize;
	TArray<uint8> ParamBuffer;
	void* Params = nullptr;
	if (ParamsSize > 0)
	{
		ParamBuffer.SetNumZeroed(ParamsSize);
		Params = ParamBuffer.GetData();
		for (TFieldIterator<FProperty> It(Fn); It && It->HasAnyPropertyFlags(CPF_Parm); ++It)
		{
			if (It->HasAnyPropertyFlags(CPF_OutParm | CPF_ReturnParm))
			{
				continue;
			}
			if (FIntProperty* IntProp = CastField<FIntProperty>(*It))
			{
				IntProp->SetPropertyValue_InContainer(Params, AbilityId);
				break;
			}
			if (FByteProperty* ByteProp = CastField<FByteProperty>(*It))
			{
				ByteProp->SetPropertyValue_InContainer(Params, static_cast<uint8>(AbilityId));
				break;
			}
		}
	}

	Pawn->ProcessEvent(Fn, Params);
}

UChampionFrameWidget* UAbilityBarWidget::ResolveChampionFrame() const
{
	APlayerController* PC = GetOwningPlayer();
	UWorld* World = GetWorld();
	for (TObjectIterator<UChampionFrameWidget> It; It; ++It)
	{
		UChampionFrameWidget* Frame = *It;
		if (!IsValid(Frame) || !Frame->IsInViewport())
		{
			continue;
		}
		if (PC && Frame->GetOwningPlayer() != PC)
		{
			continue;
		}
		if (World && Frame->GetWorld() != World)
		{
			continue;
		}
		return Frame;
	}
	return nullptr;
}

APawn* UAbilityBarWidget::ResolveChampionPawn() const
{
	APlayerController* PC = GetOwningPlayer();
	if (!PC)
	{
		return GetOwningPlayerPawn();
	}

	// MOBA mode possesses the free camera; ability CDs live on ControlledChampion.
	if (const AMobaPlayerController* MobaPC = Cast<AMobaPlayerController>(PC))
	{
		if (APawn* Champion = MobaPC->GetControlledChampion())
		{
			return Champion;
		}
	}

	if (APawn* Possessed = PC->GetPawn())
	{
		// Prefer a pawn that actually exposes ability CD props.
		float UnusedCd = 0.f;
		if (ReadFloatProp(Possessed, FName(TEXT("CD_Q")), UnusedCd))
		{
			return Possessed;
		}
	}

	if (UWorld* World = GetWorld())
	{
		for (TActorIterator<APawn> It(World); It; ++It)
		{
			APawn* Candidate = *It;
			if (!IsValid(Candidate))
			{
				continue;
			}
			float UnusedCd = 0.f;
			if (ReadFloatProp(Candidate, FName(TEXT("CD_Q")), UnusedCd))
			{
				return Candidate;
			}
		}
	}

	return PC->GetPawn();
}

void UAbilityBarWidget::RefreshFromPawn(APawn* Pawn)
{
	if (!Pawn || !bBuilt)
	{
		return;
	}

	float CdQ = 0.f, CdW = 0.f, CdE = 0.f, CdR = 0.f;
	float MaxQ = 1.f, MaxW = 1.f, MaxE = 1.f, MaxR = 1.f;
	bool bDropping = false;
	int32 Level = 1;
	int32 Pending = 0;

	ReadFloatProp(Pawn, FName(TEXT("CD_Q")), CdQ);
	ReadFloatProp(Pawn, FName(TEXT("CD_W")), CdW);
	ReadFloatProp(Pawn, FName(TEXT("CD_E")), CdE);
	ReadFloatProp(Pawn, FName(TEXT("CD_R")), CdR);
	ReadFloatProp(Pawn, FName(TEXT("MaxCD_Q")), MaxQ);
	ReadFloatProp(Pawn, FName(TEXT("MaxCD_W")), MaxW);
	ReadFloatProp(Pawn, FName(TEXT("MaxCD_E")), MaxE);
	ReadFloatProp(Pawn, FName(TEXT("MaxCD_R")), MaxR);
	if (!ReadBoolProp(Pawn, FName(TEXT("bIsDropping")), bDropping))
	{
		ReadBoolProp(Pawn, FName(TEXT("IsDropping")), bDropping);
	}
	ReadIntProp(Pawn, FName(TEXT("ChampionLevel")), Level);
	ReadIntProp(Pawn, FName(TEXT("PendingAbility")), Pending);

	static TWeakObjectPtr<APawn> LastLoggedChampion;
	if (LastLoggedChampion.Get() != Pawn)
	{
		LastLoggedChampion = Pawn;
		UE_LOG(LogTemp, Display,
			TEXT("AbilityBar bound to champion=%s CD_Q=%.2f Level=%d Dropping=%d Pending=%d"),
			*Pawn->GetName(), CdQ, Level, bDropping ? 1 : 0, Pending);
	}

	ApplySlotState(SlotQ, 1, CdQ, MaxQ, bDropping, Level, Pending);
	ApplySlotState(SlotW, 2, CdW, MaxW, bDropping, Level, Pending);
	ApplySlotState(SlotE, 3, CdE, MaxE, bDropping, Level, Pending);
	ApplySlotState(SlotR, 4, CdR, MaxR, bDropping, Level, Pending);
}

void UAbilityBarWidget::ApplySlotState(FAbilityBarSlotWidgets& SlotUI, int32 AbilityId, float RemainingCD, float MaxCD,
	bool bDropping, int32 ChampionLevel, int32 PendingAbility)
{
	if (!SlotUI.Button || !SlotUI.CooldownText)
	{
		return;
	}

	const bool bUltLocked = (AbilityId == 4) && (ChampionLevel < UltimateUnlockLevel);
	const bool bOnCooldown = RemainingCD > 0.05f;
	const bool bUnavailable = bUltLocked || bDropping;
	const bool bAvailable = !bUnavailable && !bOnCooldown;
	const bool bAiming = PendingAbility == AbilityId;

	const float SafeMax = FMath::Max(MaxCD, 0.01f);
	const float CdPercent = bOnCooldown ? FMath::Clamp(RemainingCD / SafeMax, 0.f, 1.f) : 0.f;

		// Dark wipe covers remaining CD (shrinks top to bottom as it cools, LoL-style).
	if (SlotUI.CooldownClip && SlotUI.CooldownFill)
	{
		if (bOnCooldown && !bUltLocked)
		{
			SlotUI.CooldownClip->SetHeightOverride(EffectiveSlotSize * CdPercent);
			SlotUI.CooldownClip->SetVisibility(ESlateVisibility::HitTestInvisible);
			SlotUI.CooldownFill->SetBrushColor(AbilityBarPrivate::CdOverlay);
		}
		else
		{
			SlotUI.CooldownClip->SetHeightOverride(0.f);
			SlotUI.CooldownClip->SetVisibility(ESlateVisibility::Collapsed);
		}
	}

	if (bOnCooldown && !bUltLocked)
	{
		const FString CdStr = RemainingCD >= 10.f
			? FString::Printf(TEXT("%d"), FMath::CeilToInt(RemainingCD))
			: FString::Printf(TEXT("%.1f"), RemainingCD);
		SlotUI.CooldownText->SetText(FText::FromString(CdStr));
		SlotUI.CooldownText->SetVisibility(ESlateVisibility::HitTestInvisible);
		if (SlotUI.KeyLabel)
		{
			SlotUI.KeyLabel->SetVisibility(ESlateVisibility::Collapsed);
		}
	}
	else
	{
		SlotUI.CooldownText->SetText(FText::GetEmpty());
		SlotUI.CooldownText->SetVisibility(ESlateVisibility::Collapsed);
		if (SlotUI.KeyLabel)
		{
			SlotUI.KeyLabel->SetVisibility(ESlateVisibility::HitTestInvisible);
			SlotUI.KeyLabel->SetColorAndOpacity(FSlateColor(
				bUnavailable ? AbilityBarPrivate::KeyColorDim : AbilityBarPrivate::KeyColor));
		}
	}

	if (SlotUI.LockText)
	{
		if (bUltLocked)
		{
			SlotUI.LockText->SetText(FText::FromString(FString::Printf(TEXT("Lv%d"), UltimateUnlockLevel)));
			SlotUI.LockText->SetVisibility(ESlateVisibility::HitTestInvisible);
			if (SlotUI.KeyLabel)
			{
				SlotUI.KeyLabel->SetVisibility(ESlateVisibility::HitTestInvisible);
			}
		}
		else if (bDropping)
		{
			SlotUI.LockText->SetText(FText::FromString(TEXT("--")));
			SlotUI.LockText->SetVisibility(ESlateVisibility::HitTestInvisible);
		}
		else
		{
			SlotUI.LockText->SetVisibility(ESlateVisibility::Collapsed);
		}
	}

	if (SlotUI.Frame)
	{
		if (bAiming && bAvailable)
		{
			SlotUI.Frame->SetBrushColor(AbilityBarPrivate::SlotAimFrame);
		}
		else if (bUnavailable)
		{
			SlotUI.Frame->SetBrushColor(AbilityBarPrivate::SlotLockedFrame);
		}
		else if (bOnCooldown)
		{
			SlotUI.Frame->SetBrushColor(AbilityBarPrivate::SlotCdFrame);
		}
		else
		{
			SlotUI.Frame->SetBrushColor(AbilityBarPrivate::SlotReadyFrame);
		}
	}

	if (SlotUI.SizeBox)
	{
		if (bUltLocked || bDropping)
		{
			SlotUI.Button->SetBackgroundColor(AbilityBarPrivate::SlotLockedBg);
			SlotUI.SizeBox->SetRenderOpacity(0.42f);
		}
		else if (bOnCooldown)
		{
			SlotUI.Button->SetBackgroundColor(AbilityBarPrivate::SlotCdBg);
			SlotUI.SizeBox->SetRenderOpacity(0.92f);
		}
		else
		{
			SlotUI.Button->SetBackgroundColor(AbilityBarPrivate::SlotReadyBg);
			SlotUI.SizeBox->SetRenderOpacity(1.f);
		}
	}

	SlotUI.Button->SetIsEnabled(true);
}

bool UAbilityBarWidget::ReadBoolProp(const UObject* Obj, FName Name, bool& OutValue)
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

bool UAbilityBarWidget::ReadFloatProp(const UObject* Obj, FName Name, float& OutValue)
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

bool UAbilityBarWidget::ReadIntProp(const UObject* Obj, FName Name, int32& OutValue)
{
	if (!Obj)
	{
		return false;
	}
	if (const FIntProperty* Prop = FindFProperty<FIntProperty>(Obj->GetClass(), Name))
	{
		OutValue = Prop->GetPropertyValue_InContainer(Obj);
		return true;
	}
	if (const FByteProperty* ByteProp = FindFProperty<FByteProperty>(Obj->GetClass(), Name))
	{
		OutValue = ByteProp->GetPropertyValue_InContainer(Obj);
		return true;
	}
	return false;
}
