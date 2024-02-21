#include "NamingConventionValidationCommandlet.h"

#include "NamingConventionValidationLog.h"
#include "EditorNamingValidatorSubsystem.h"

#include <Editor.h>
#include <AssetRegistry/AssetRegistryHelpers.h>
#include <AssetRegistry/AssetRegistryModule.h>
#include <AssetRegistry/IAssetRegistry.h>

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

    TArray< FString > paths;
    if ( const auto * path = params_map.Find( TEXT( "Paths" ) ) ) 
    {
        path->ParseIntoArray( paths, TEXT( "+" ) );
    }
    else 
    {
        paths.Add( TEXT( "/Game" ) );
    }

    // validate data
    if ( !ValidateData( paths ) )
    {
        UE_LOG( LogNamingConventionValidation, Warning, TEXT( "Errors occurred while validating naming convention" ) );
        return 2; // return something other than 1 for error since the engine will return 1 if any other system (possibly unrelated) logged errors during execution.
    }

    UE_LOG( LogNamingConventionValidation, Log, TEXT( "Successfully finished running NamingConventionValidation Commandlet" ) );
    UE_LOG( LogNamingConventionValidation, Log, TEXT( "--------------------------------------------------------------------------------------------" ) );
    return 0;
}

//static
bool UNamingConventionValidationCommandlet::ValidateData( TArrayView< FString > paths )
{
    const auto & asset_registry_module = FModuleManager::LoadModuleChecked< FAssetRegistryModule >( AssetRegistryConstants::ModuleName );
    asset_registry_module.Get().ScanPathsSynchronous( TArray< FString >( paths ), true );

    TArray< FAssetData > asset_data_list;

    FARFilter filter;
    filter.bRecursivePaths = true;
    filter.PackagePaths.Append( paths );
    asset_registry_module.Get().GetAssets( filter, asset_data_list );

    const auto * editor_validator_subsystem = GEditor->GetEditorSubsystem< UEditorNamingValidatorSubsystem >();
    check( editor_validator_subsystem );

    // ReSharper disable once CppExpressionWithoutSideEffects
    editor_validator_subsystem->ValidateAssets( asset_data_list );

    return true;
}
