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
    BlueprintsPrefix = "BP_";
}

UNamingConventionValidationManager * UNamingConventionValidationManager::Get()
{
    if ( GNamingConventionValidationManager == nullptr )
    {
        FSoftClassPath naming_convention_validation_manager_class_name = ( UNamingConventionValidationManager::StaticClass()->GetDefaultObject< UNamingConventionValidationManager >() )->NamingConventionValidationManagerClassName;

        UClass * singleton_class = naming_convention_validation_manager_class_name.TryLoadClass< UObject >();
        checkf( singleton_class != nullptr, TEXT( "Naming Convention Validation config value NamingConventionValidationManagerClassName is not a valid class name." ) );

        GNamingConventionValidationManager = NewObject< UNamingConventionValidationManager >( GetTransientPackage(), singleton_class, NAME_None );
        checkf( GNamingConventionValidationManager != nullptr, TEXT( "Naming Convention validation config value NamingConventionValidationManagerClassName is not a subclass of UNamingConventionValidationManager." ) );

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

    for ( auto & class_description : ClassDescriptions )
    {
        class_description.Class = class_description.ClassPath.TryLoadClass< UObject >();

        ensureAlwaysMsgf( class_description.Class != nullptr, TEXT( "Impossible to get a valid UClass for the classpath %s" ), *class_description.ClassPath.ToString() );
    }

    for ( auto & class_path : ExcludedClassPaths )
    {
        auto * excluded_class = class_path.TryLoadClass< UObject >();
        ensureAlwaysMsgf( excluded_class != nullptr, TEXT( "Impossible to get a valid UClass for the excluded classpath %s" ), *class_path.ToString() );

        if ( excluded_class != nullptr )
        {
            ExcludedClasses.Add( excluded_class );
        }
    }
}

UNamingConventionValidationManager::~UNamingConventionValidationManager()
{
}

ENamingConventionValidationResult UNamingConventionValidationManager::IsAssetNamedCorrectly( const FAssetData & asset_data ) const
{
    static const FName
        native_parent_class_key( "NativeParentClass" ),
        native_class_key( "NativeClass" );

    const auto asset_name = asset_data.AssetName.ToString();

    FName asset_class;

    if ( !asset_data.GetTagValue( native_parent_class_key, asset_class ) )
    {
        if ( !asset_data.GetTagValue( native_class_key, asset_class ) )
        {
            asset_class = *( "/Script/Engine." + asset_data.AssetClass.ToString() );
        }
    }

    const auto result = DoesAssetMatchNameConvention( asset_name, asset_class );

    if ( result == ENamingConventionValidationResult::Unknown )
    {
        static const FName blueprint_class_name( "Blueprint" );
        if ( asset_data.AssetClass == blueprint_class_name )
        {
            return asset_name.StartsWith( BlueprintsPrefix )
                       ? ENamingConventionValidationResult::Valid
                       : ENamingConventionValidationResult::Invalid;
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

    FMessageLog data_validation_log( "NamingConventionValidation" );

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

        switch ( result )
        {
            case ENamingConventionValidationResult::Excluded:
            {
                ++num_files_skipped;
            }
            break;
            case ENamingConventionValidationResult::Valid:
            {
                ++num_valid_files;
                ++num_files_checked;
            }
            break;
            case ENamingConventionValidationResult::Invalid:
            {
                data_validation_log.Error()
                    ->AddToken( FAssetNameToken::Create( asset_data.PackageName.ToString() ) )
                    ->AddToken( FTextToken::Create( LOCTEXT( "InvalidNamingConventionResult", "does not match naming convention." ) ) );
                ++num_invalid_files;
                ++num_files_checked;
            }
            break;
            case ENamingConventionValidationResult::Unknown:
            {
                if ( show_if_no_failures )
                {
                    FFormatNamedArguments arguments;
                    arguments.Add( TEXT( "ClassName" ), FText::FromString( asset_data.AssetClass.ToString() ) );

                    data_validation_log.Warning()
                        ->AddToken( FAssetNameToken::Create( asset_data.PackageName.ToString() ) )
                        ->AddToken( FTextToken::Create( LOCTEXT( "UnknownNamingConventionResult", "has no known naming convention." ) ) )
                        ->AddToken( FTextToken::Create( FText::Format( LOCTEXT( "UnknownClass", " Class = {ClassName}" ), arguments ) ) );
                }
                ++num_files_checked;
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

// -- PRIVATE

ENamingConventionValidationResult UNamingConventionValidationManager::DoesAssetMatchNameConvention( const FString & asset_name, const FName asset_class ) const
{
    FSoftClassPath asset_class_path( asset_class.ToString() );
    if ( UClass * asset_real_class = asset_class_path.TryLoadClass< UObject >() )
    {
        for ( auto * excluded_class : ExcludedClasses )
        {
            if ( asset_real_class->IsChildOf( excluded_class ) )
            {
                return ENamingConventionValidationResult::Excluded;
            }
        }

        bool found_type = false;

        for ( const auto & class_description : ClassDescriptions )
        {
            if ( asset_real_class->IsChildOf( class_description.Class ) )
            {
                found_type = true;

                if ( !class_description.Prefix.IsEmpty() )
                {
                    if ( !asset_name.StartsWith( class_description.Prefix ) )
                    {
                        continue;
                    }
                }

                if ( !class_description.Suffix.IsEmpty() )
                {
                    if ( !asset_name.EndsWith( class_description.Suffix ) )
                    {
                        continue;
                    }
                }

                return ENamingConventionValidationResult::Valid;
            }
        }

        // If a type was found but Valid was not returned, the naming convention is broken
        // Don't return Invalid as soon as a type is not validated because another more derived type might match
        if ( found_type )
        {
            return ENamingConventionValidationResult::Invalid;
        }
    }

    return ENamingConventionValidationResult::Unknown;
}

#undef LOCTEXT_NAMESPACE
