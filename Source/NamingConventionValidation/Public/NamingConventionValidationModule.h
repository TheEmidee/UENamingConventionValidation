#pragma once

#include "CoreMinimal.h"
#include "Stats/Stats.h"
#include "AssetData.h"
#include "Modules/ModuleInterface.h"
#include "Modules/ModuleManager.h"

class NAMINGCONVENTIONVALIDATION_API INamingConventionValidationModule : public IModuleInterface
{

public:

    static inline INamingConventionValidationModule & Get()
    {
        QUICK_SCOPE_CYCLE_COUNTER( STAT_INamingConventionValidationModule_Get );
        static INamingConventionValidationModule & singleton = FModuleManager::LoadModuleChecked< INamingConventionValidationModule >( "NamingConventionValidation" );
        return singleton;
    }

    static inline bool IsAvailable()
    {
        QUICK_SCOPE_CYCLE_COUNTER(STAT_INamingConventionValidationModule_IsAvailable);
        return FModuleManager::Get().IsModuleLoaded( "NamingConventionValidation" );
    }

    virtual void ValidateAssets( const TArray< FAssetData > & selected_assets ) const = 0;
};
