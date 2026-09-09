#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Templates/SubclassOf.h"
#include "FloatingDamageTextWidget.generated.h"

class UTextBlock;

/**
 * Single floating combat-text number (LoL ARAM style) shown above a damaged actor.
 * Purely a label; MobaPlayerController owns lifetime, screen position, and fade-out.
 */
UCLASS(Blueprintable)
class TD_API UFloatingDamageTextWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFloatingDamageTextWidget(const FObjectInitializer& ObjectInitializer);

	virtual void NativeOnInitialized() override;

	/** Designer widget at /Game/TD/UI/WBP_FloatingDamage. */
	static const TCHAR* GetWidgetBlueprintPath();
	static TSubclassOf<UFloatingDamageTextWidget> ResolveWidgetClass();

	void SetDamageText(float Amount, const FLinearColor& Color = FLinearColor(1.0f, 0.86f, 0.24f, 1.0f));

protected:
	UPROPERTY()
	TObjectPtr<UTextBlock> DamageLabel = nullptr;
};
