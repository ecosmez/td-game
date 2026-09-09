#include "FloatingDamageTextWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/TextBlock.h"

UFloatingDamageTextWidget::UFloatingDamageTextWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	SetIsFocusable(false);
}

const TCHAR* UFloatingDamageTextWidget::GetWidgetBlueprintPath()
{
	return TEXT("/Game/TD/UI/WBP_FloatingDamage.WBP_FloatingDamage_C");
}

TSubclassOf<UFloatingDamageTextWidget> UFloatingDamageTextWidget::ResolveWidgetClass()
{
	return LoadClass<UFloatingDamageTextWidget>(nullptr, GetWidgetBlueprintPath());
}

void UFloatingDamageTextWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	if (!DamageLabel && WidgetTree)
	{
		DamageLabel = Cast<UTextBlock>(GetWidgetFromName(TEXT("FloatingDamageLabel")));
		if (!DamageLabel)
		{
			DamageLabel = Cast<UTextBlock>(WidgetTree->RootWidget);
		}
	}

	SetVisibility(ESlateVisibility::HitTestInvisible);
}

void UFloatingDamageTextWidget::SetDamageText(float Amount, const FLinearColor& Color)
{
	if (!DamageLabel)
	{
		return;
	}
	DamageLabel->SetText(FText::AsNumber(FMath::RoundToInt(Amount)));
	DamageLabel->SetColorAndOpacity(FSlateColor(Color));
}
