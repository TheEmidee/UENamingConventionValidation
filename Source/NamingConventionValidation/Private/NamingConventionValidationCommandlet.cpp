#include "NamingConventionValidationCommandlet.h"

#include "NamingConventionValidationManager.h"

#include <AssetRegistryHelpers.h>
#include <AssetRegistryModule.h>
#include <IAssetRegistry.h>

// ReSharper disable once CppInconsistentNaming
DEFINE_LOG_CATEGORY_STATIC( LogNamingConventionValidation, Warning, All )

UNamingConventionValidationCommandlet::UNamingConventionValidationCommandlet()
{
    LogToConsole = false;
}

int32 UNamingConventionValidationCommandlet::Main( const FString & params )
{
    UE_LOG( LogNamingConventionValidation, Log, TEXT( "--------------------------------------------------------------------------------------------" ) );
    UE_LOG( LogNamingConventionValidation, Log, TEXT( "Running NamingConventionValidation Commandlet" ) );
    TArray< FString > tokens;
    TArray< FString > switches;
    TMap< FString, FString > params_map;
    ParseCommandLine( *params, tokens, switches, params_map );

    // validate data
    if ( !ValidateData() )
    {
        UE_LOG( LogNamingConventionValidation, Warning, TEXT( "Errors occurred while validating naming convention" ) );
        return 2; // return something other than 1 for error since the engine will return 1 if any other system (possibly unrelated) logged errors during execution.
    }

    UE_LOG( LogNamingConventionValidation, Log, TEXT( "Successfully finished running NamingConventionValidation Commandlet" ) );
    UE_LOG( LogNamingConventionValidation, Log, TEXT( "--------------------------------------------------------------------------------------------" ) );
    return 0;
}

//static
bool UNamingConventionValidationCommandlet::ValidateData()
{
    FAssetRegistryModule & asset_registry_module = FModuleManager::LoadModuleChecked< FAssetRegistryModule >( AssetRegistryConstants::ModuleName );

    TArray< FAssetData > asset_data_list;
    
    FARFilter filter;
    filter.bRecursivePaths = true;
    filter.PackagePaths.Add( "/Game" );
    asset_registry_module.Get().GetAssets( filter, asset_data_list );

    UNamingConventionValidationManager * naming_convention_validation_manager = UNamingConventionValidationManager::Get();
    check( naming_convention_validation_manager );

    // ReSharper disable once CppExpressionWithoutSideEffects
    naming_convention_validation_manager->ValidateAssets( asset_data_list );

    return true;
}
