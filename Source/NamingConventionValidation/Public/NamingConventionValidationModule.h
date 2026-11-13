#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleInterface.h"
#include "Modules/ModuleManager.h"
#include "Stats/Stats.h"

class NAMINGCONVENTIONVALIDATION_API INamingConventionValidationModule : public IModuleInterface
{

public:
	static INamingConventionValidationModule& Get()
	{
		QUICK_SCOPE_CYCLE_COUNTER(STAT_INamingConventionValidationModule_Get);
		static auto& Singleton = FModuleManager::LoadModuleChecked<INamingConventionValidationModule>("NamingConventionValidation");
		return Singleton;
	}

	static bool IsAvailable()
	{
		QUICK_SCOPE_CYCLE_COUNTER(STAT_INamingConventionValidationModule_IsAvailable);
		return FModuleManager::Get().IsModuleLoaded("NamingConventionValidation");
	}
};
