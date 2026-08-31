#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "CaptureChannelWidget.generated.h"

class UProgressBar;

/** World-space channel fill bar shown on a capture base. */
UCLASS()
class TD_API UCaptureChannelWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UCaptureChannelWidget(const FObjectInitializer& ObjectInitializer);

	virtual void NativeOnInitialized() override;
	virtual void NativeConstruct() override;

	void SetProgress(float In01);

protected:
	void EnsureBuilt();

	UPROPERTY()
	TObjectPtr<UProgressBar> Bar = nullptr;

	bool bBuilt = false;
};
