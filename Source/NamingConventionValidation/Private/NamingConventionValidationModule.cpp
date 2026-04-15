#include "NamingConventionValidationModule.h"

#include "Modules/ModuleManager.h"
#include "UObject/Object.h"

class FNamingConventionValidationModule : public INamingConventionValidationModule
{
public:
	void StartupModule() override;
	void ShutdownModule() override;
};

IMPLEMENT_MODULE(FNamingConventionValidationModule, NamingConventionValidation)

void FNamingConventionValidationModule::StartupModule()
{
}

void FNamingConventionValidationModule::ShutdownModule()
{
}