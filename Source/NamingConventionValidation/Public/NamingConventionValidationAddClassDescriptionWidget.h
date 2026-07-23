#pragma once

#include "NamingConventionValidationSettings.h"
#include "Widgets/SCompoundWidget.h"

class FClassDescriptionItem;

class SNamingConventionValidationAddClassDescriptionWidget : public SCompoundWidget
{
public:
	DECLARE_DELEGATE_OneParam(FOnClassDescriptionValidated, const FNamingConventionValidationClassDescription& /*ClassDescription*/);

	SLATE_BEGIN_ARGS(SNamingConventionValidationAddClassDescriptionWidget)
	{}
	SLATE_ARGUMENT(FNamingConventionValidationClassDescription, ClassDescription)
	SLATE_ARGUMENT(TSharedPtr<SWindow>, OwningWindow)
	SLATE_EVENT(FOnClassDescriptionValidated, OnClassDescriptionValidated)
	SLATE_END_ARGS();

	void Construct(const FArguments& InArgs);

private:
	const UClass* OnGetClass() const;
	FNamingConventionValidationClassDescription ClassDescription;
	bool bIsAdding = false;
	TWeakPtr<SWindow> OwningWindow;
	FOnClassDescriptionValidated OnClassDescriptionValidated;
};