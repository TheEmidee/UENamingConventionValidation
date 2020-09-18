#include "NamingConventionValidationManager.h"

#include "NamingConventionValidation/Public/NamingConventionValidationSettings.h"

#include <AssetRegistryModule.h>
#include <AssetToolsModule.h>
#include <Developer/MessageLog/Public/MessageLogModule.h>
#include <Editor.h>
#include <IAssetTools.h>
#include <Logging/MessageLog.h>
#include <Misc/ScopedSlowTask.h>
#include <Modules/ModuleManager.h>

#define LOCTEXT_NAMESPACE "NamingConventionValidationManager"



UNamingConventionValidationManager * NamingConventionValidationManager = nullptr;

UNamingConventionValidationManager::UNamingConventionValidationManager( const FObjectInitializer & object_initializer ) :
    Super( object_initializer )
{
}

UNamingConventionValidationManager * UNamingConventionValidationManager::Get()
{
    if ( NamingConventionValidationManager == nullptr )
    {
        NamingConventionValidationManager = NewObject< UNamingConventionValidationManager >( GetTransientPackage() );
        checkf( NamingConventionValidationManager != nullptr, TEXT( "Naming Convention validation config value NamingConventionValidationManagerClassName is not a subclass of UNamingConventionValidationManager." ) );

        NamingConventionValidationManager->AddToRoot();
        NamingConventionValidationManager->Initialize();
    }

    return NamingConventionValidationManager;
}

void UNamingConventionValidationManager::Initialize()
{
    
}

UNamingConventionValidationManager::~UNamingConventionValidationManager()
{
}

ENamingConventionValidationResult UNamingConventionValidationManager::IsAssetNamedCorrectly( const FAssetData & asset_data, FText & error_message ) const
{
    return ENamingConventionValidationResult::Invalid;
}

int32 UNamingConventionValidationManager::ValidateAssets( const TArray< FAssetData > & asset_data_list, bool /*skip_excluded_directories*/ /* = true */, bool show_if_no_failures /* = true */ ) const
{
    return -1;
}

void UNamingConventionValidationManager::ValidateOnSave( const TArray< FAssetData > & asset_data_list ) const
{
    
}

void UNamingConventionValidationManager::ValidateSavedPackage( const FName package_name )
{
    
}

// -- PROTECTED

void UNamingConventionValidationManager::ValidateAllSavedPackages()
{
    
}

// -- PRIVATE

ENamingConventionValidationResult UNamingConventionValidationManager::DoesAssetMatchNameConvention( const FAssetData & asset_data, const FName asset_class, FText & error_message ) const
{
    return ENamingConventionValidationResult::Unknown;
}

#undef LOCTEXT_NAMESPACE
