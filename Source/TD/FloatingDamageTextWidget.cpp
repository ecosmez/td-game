#include "FloatingDamageTextWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/TextBlock.h"

UFloatingDamageTextWidget::UFloatingDamageTextWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	SetIsFocusable(false);
}

void UFloatingDamageTextWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	if (DamageLabel || !WidgetTree)
	{
		return;
	}

	DamageLabel = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("FloatingDamageLabel"));
	DamageLabel->SetJustification(ETextJustify::Center);
	DamageLabel->SetShadowOffset(FVector2D(1.5f, 1.5f));
	DamageLabel->SetShadowColorAndOpacity(FLinearColor(0.0f, 0.0f, 0.0f, 0.9f));
	{
		FSlateFontInfo Font = DamageLabel->GetFont();
		Font.Size = 22.0f;
		Font.TypefaceFontName = TEXT("Bold");
		DamageLabel->SetFont(Font);
	}
	WidgetTree->RootWidget = DamageLabel;

	// Never intercept the world right-click that drives champion move/attack.
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
