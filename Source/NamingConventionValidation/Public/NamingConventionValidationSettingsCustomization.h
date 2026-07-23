#pragma once

#include "IDetailCustomization.h"

class SGameplayTagWidget;
class IDetailLayoutBuilder;

//////////////////////////////////////////////////////////////////////////
// FGameplayTagsSettingsCustomization

class FNamingConventionValidationSettingsCustomization : public IDetailCustomization
{
public:
	FNamingConventionValidationSettingsCustomization();
	virtual ~FNamingConventionValidationSettingsCustomization() override;

	/** Makes a new instance of this detail layout class for a specific detail view requesting it */
	static TSharedRef<IDetailCustomization> MakeInstance();

	// IDetailCustomization interface
	virtual void CustomizeDetails(IDetailLayoutBuilder& DetailLayout) override;
	// End of IDetailCustomization interface
};
