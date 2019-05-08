// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.

#include "NamingConventionValidationManager.h"

#include "AssetRegistryModule.h"
#include "CoreGlobals.h"
#include "Developer/MessageLog/Public/MessageLogModule.h"
#include "Editor.h"
#include "Logging/MessageLog.h"
#include "Misc/ScopedSlowTask.h"
#include "Modules/ModuleManager.h"

#define LOCTEXT_NAMESPACE "NamingConventionValidationManager"

UNamingConventionValidationManager * GNamingConventionValidationManager = nullptr;

UNamingConventionValidationManager::UNamingConventionValidationManager( const FObjectInitializer & object_initializer ) :
    Super( object_initializer )
{
    NamingConventionValidationManagerClassName = FSoftClassPath( TEXT( "/Script/NamingConventionValidation.NamingConventionValidationManager" ) );
    bValidateOnSave = true;
}

UNamingConventionValidationManager * UNamingConventionValidationManager::Get()
{
    if ( GNamingConventionValidationManager == nullptr )
    {
        FSoftClassPath naming_convention_validation_manager_class_name = ( UNamingConventionValidationManager::StaticClass()->GetDefaultObject< UNamingConventionValidationManager >() )->NamingConventionValidationManagerClassName;

        UClass * singleton_class = naming_convention_validation_manager_class_name.TryLoadClass< UObject >();
        checkf( singleton_class != nullptr, TEXT( "Naming Convention Validation config value NamingConventionValidationManagerClassName is not a valid class name." ) );

        GNamingConventionValidationManager = NewObject< UNamingConventionValidationManager >( GetTransientPackage(), singleton_class, NAME_None );
        checkf( GNamingConventionValidationManager != nullptr, TEXT( "Naming Convention validation config value NamingConventionValidationManagerClassName is not a subclass of UNamingConventionValidationManager." ) )

            GNamingConventionValidationManager->AddToRoot();
        GNamingConventionValidationManager->Initialize();
    }

    return GNamingConventionValidationManager;
}

void UNamingConventionValidationManager::Initialize()
{
    FMessageLogInitializationOptions init_options;
    init_options.bShowFilters = true;

    FMessageLogModule & message_log_module = FModuleManager::LoadModuleChecked< FMessageLogModule >( "MessageLog" );
    message_log_module.RegisterLogListing( "NamingConventionValidation", LOCTEXT( "NamingConventionValidation", "Naming Convention Validation" ), init_options );
}

UNamingConventionValidationManager::~UNamingConventionValidationManager()
{
}

ENamingConventionValidationResult UNamingConventionValidationManager::IsAssetNamedCorrectly( const FAssetData & asset_data ) const
{
    auto result = ENamingConventionValidationResult::Unknown;
    static const FName blueprint_class_name( "Blueprint" );

    auto asset_class = asset_data.AssetClass;

    if ( asset_data.AssetClass == blueprint_class_name )
    {
        static const FName key( "NativeParentClass" );

        if ( asset_data.GetTagValue( key, asset_class ) )
        {
            auto asset_class_str = asset_class.ToString();
            asset_class_str.RemoveFromStart( "Class'/Script/" );
            asset_class_str.RemoveFromEnd( "'" );
            asset_class = *asset_class_str;
        }
    }

    for ( const auto & class_description : ClassDescriptions )
    {
        if ( class_description.ClassName == asset_class )
        {
            const auto asset_name = asset_data.AssetName.ToString();

            if ( !class_description.Prefix.IsEmpty() )
            {
                if ( !asset_name.StartsWith( class_description.Prefix ) )
                {
                    result = ENamingConventionValidationResult::Invalid;
                    break;
                }
            }

            if ( !class_description.Suffix.IsEmpty() )
            {
                if ( !asset_name.EndsWith( class_description.Suffix ) )
                {
                    result = ENamingConventionValidationResult::Invalid;
                    break;
                }
            }

            result = ENamingConventionValidationResult::Valid;
            break;
        }
    }

    return result;
}

int32 UNamingConventionValidationManager::ValidateAssets( const TArray< FAssetData > & asset_data_list, bool skip_excluded_directories /* = true */, bool show_if_no_failures /* = true */ ) const
{
    FScopedSlowTask slow_task( 1.0f, LOCTEXT( "NamingConventionValidatingDataTask", "Validating Naming Convention..." ) );
    slow_task.Visibility = show_if_no_failures ? ESlowTaskVisibility::ForceVisible : ESlowTaskVisibility::Invisible;

    if ( show_if_no_failures )
    {
        slow_task.MakeDialogDelayed( .1f );
    }

    FMessageLog data_validation_log( "NamingConventionValidation " );

    int32 num_added = 0;
    int32 num_files_checked = 0;
    int32 num_valid_files = 0;
    int32 num_invalid_files = 0;
    int32 num_files_skipped = 0;
    int32 num_files_unable_to_validate = 0;

    const auto num_files_to_validate = asset_data_list.Num();

    for ( const FAssetData & asset_data : asset_data_list )
    {
        slow_task.EnterProgressFrame( 1.0f / num_files_to_validate, FText::Format( LOCTEXT( "ValidatingNamingConventionFilename", "Validating Naming Convention {0}" ), FText::FromString( asset_data.GetFullName() ) ) );

        // Check exclusion path
        if ( skip_excluded_directories && IsPathExcludedFromValidation( asset_data.PackageName.ToString() ) )
        {
            ++num_files_skipped;
            continue;
        }

        const auto result = IsAssetNamedCorrectly( asset_data );
        ++num_files_checked;

        switch ( result )
        {
            case ENamingConventionValidationResult::Valid:
            {
                ++num_valid_files;
            }
            break;
            case ENamingConventionValidationResult::Invalid:
            {
                data_validation_log.Error()
                    ->AddToken( FAssetNameToken::Create( asset_data.PackageName.ToString() ) )
                    ->AddToken( FTextToken::Create( LOCTEXT( "InvalidNamingConventionResult", "does not match naming convention." ) ) );
                ++num_invalid_files;
            }
            break;
            case ENamingConventionValidationResult::Unknown:
            {
                if ( show_if_no_failures )
                {
                    data_validation_log.Warning()
                        ->AddToken( FAssetNameToken::Create( asset_data.PackageName.ToString() ) )
                        ->AddToken( FTextToken::Create( LOCTEXT( "UnknownNamingConventionResult", "has no known naming convention." ) ) );
                }
                ++num_files_unable_to_validate;
            }
            break;
        }
    }

    const auto has_failed = ( num_invalid_files > 0 );

    if ( has_failed || show_if_no_failures )
    {
        FFormatNamedArguments arguments;
        arguments.Add( TEXT( "Result" ), has_failed ? LOCTEXT( "Failed", "FAILED" ) : LOCTEXT( "Succeeded", "SUCCEEDED" ) );
        arguments.Add( TEXT( "NumChecked" ), num_files_checked );
        arguments.Add( TEXT( "NumValid" ), num_valid_files );
        arguments.Add( TEXT( "NumInvalid" ), num_invalid_files );
        arguments.Add( TEXT( "NumSkipped" ), num_files_skipped );
        arguments.Add( TEXT( "NumUnableToValidate" ), num_files_unable_to_validate );

        TSharedRef< FTokenizedMessage > validation_log = has_failed ? data_validation_log.Error() : data_validation_log.Info();
        validation_log->AddToken( FTextToken::Create( FText::Format( LOCTEXT( "SuccessOrFailure", "NamingConvention Validation {Result}." ), arguments ) ) );
        validation_log->AddToken( FTextToken::Create( FText::Format( LOCTEXT( "ResultsSummary", "Files Checked: {NumChecked}, Passed: {NumValid}, Failed: {NumInvalid}, Skipped: {NumSkipped}, Unable to validate: {NumUnableToValidate}" ), arguments ) ) );

        data_validation_log.Open( EMessageSeverity::Info, true );
    }

    return num_invalid_files;
}

void UNamingConventionValidationManager::ValidateOnSave( const TArray< FAssetData > & asset_data_list ) const
{
    if ( !bValidateOnSave || GEditor->IsAutosaving() )
    {
        return;
    }

    FMessageLog data_validation_log( "NamingConventionValidation" );

    if ( ValidateAssets( asset_data_list, true, false ) > 0 )
    {
        const FText error_message_notification = FText::Format(
            LOCTEXT( "ValidationFailureNotification", "Naming Convention Validation failed when saving {0}, check Naming Convention Validation log" ),
            asset_data_list.Num() == 1 ? FText::FromName( asset_data_list[ 0 ].AssetName ) : LOCTEXT( "MultipleErrors", "multiple assets" ) );
        data_validation_log.Notify( error_message_notification, EMessageSeverity::Warning, /*bForce=*/true );
    }
}

void UNamingConventionValidationManager::ValidateSavedPackage( FName package_name )
{
    if ( !bValidateOnSave || GEditor->IsAutosaving() )
    {
        return;
    }

    SavedPackagesToValidate.AddUnique( package_name );

    GEditor->GetTimerManager()->SetTimerForNextTick( this, &UNamingConventionValidationManager::ValidateAllSavedPackages );
}

// -- PROTECTED

bool UNamingConventionValidationManager::IsPathExcludedFromValidation( const FString & path ) const
{
    for ( const FDirectoryPath & ExcludedPath : ExcludedDirectories )
    {
        if ( path.Contains( ExcludedPath.Path ) )
        {
            return true;
        }
    }

    return false;
}

void UNamingConventionValidationManager::ValidateAllSavedPackages()
{
    FAssetRegistryModule & asset_registry_module = FModuleManager::LoadModuleChecked< FAssetRegistryModule >( "AssetRegistry" );
    TArray< FAssetData > assets;

    for ( FName package_name : SavedPackagesToValidate )
    {
        // We need to query the in-memory data as the disk cache may not be accurate
        asset_registry_module.Get().GetAssetsByPackageName( package_name, assets );
    }

    ValidateOnSave( assets );

    SavedPackagesToValidate.Empty();
}

#undef LOCTEXT_NAMESPACE
