#include "NamingConventionValidationModule.h"

#include "Modules/ModuleManager.h"
#include "NamingConventionValidationSettings.h"
#include "NamingConventionValidationSettingsCustomization.h"
#include "UObject/Object.h"

class FNamingConventionValidationModule : public INamingConventionValidationModule
{
public:
	void StartupModule() override;
	void ShutdownModule() override;

private:
	void OnPostEngineInit();
};

IMPLEMENT_MODULE(FNamingConventionValidationModule, NamingConventionValidation)

void FNamingConventionValidationModule::StartupModule()
{
	FCoreDelegates::OnPostEngineInit.AddRaw(this, &FNamingConventionValidationModule::OnPostEngineInit);
}

void FNamingConventionValidationModule::ShutdownModule()
{
}

void FNamingConventionValidationModule::OnPostEngineInit()
{
	FPropertyEditorModule& PropertyModule = FModuleManager::LoadModuleChecked<FPropertyEditorModule>("PropertyEditor");
	PropertyModule.RegisterCustomClassLayout(UNamingConventionValidationSettings::StaticClass()->GetFName(), FOnGetDetailCustomizationInstance::CreateStatic(&FNamingConventionValidationSettingsCustomization::MakeInstance));

	PropertyModule.NotifyCustomizationModuleChanged();
}