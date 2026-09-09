#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Templates/SubclassOf.h"
#include "CaptureChannelWidget.generated.h"

class AActor;
class UProgressBar;
class UWidgetComponent;

/** Screen-space channel fill bar used by capture bases and resource crystals. */
UCLASS(Blueprintable)
class TD_API UCaptureChannelWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UCaptureChannelWidget(const FObjectInitializer& ObjectInitializer);

	virtual void NativeOnInitialized() override;
	virtual void NativeConstruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
	virtual TSharedRef<SWidget> RebuildWidget() override;

	UFUNCTION(BlueprintCallable, Category = "Capture")
	void SetProgress(float In01);

	UFUNCTION(BlueprintCallable, Category = "Capture")
	void SetFillColor(FLinearColor Color);

	/** Pixel size from the designer SizeBox named BarSize. Fallback 80x10. */
	FVector2D GetDesignedDrawSize() const;
	static FVector2D GetDefaultDesignedDrawSize();
	static void ApplyDrawSizeToComponent(UWidgetComponent* Comp, UCaptureChannelWidget* Widget);

	/** Designer widget at /Game/TD/UI/WBP_CaptureChannel. */
	static const TCHAR* GetWidgetBlueprintPath();
	static TSubclassOf<UCaptureChannelWidget> ResolveWidgetClass();

protected:
	void EnsureBuilt();
	void ApplyFillStyle();
	void SyncFromHostActor();
	AActor* ResolveHostActor();

	UPROPERTY()
	TObjectPtr<UProgressBar> Bar = nullptr;

	TWeakObjectPtr<AActor> HostActor;

	FLinearColor FillColor = FLinearColor(0.22f, 0.78f, 0.95f, 1.f);

	bool bBuilt = false;
	bool bDesignerBar = false;
};
