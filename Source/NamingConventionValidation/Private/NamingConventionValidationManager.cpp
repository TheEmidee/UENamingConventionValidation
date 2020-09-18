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

bool TryGetAssetDataRealClass( FName & asset_class, const FAssetData & asset_data )
{
    static const FName
        NativeParentClassKey( "NativeParentClass" ),
        NativeClassKey( "NativeClass" );

    if ( !asset_data.GetTagValue( NativeParentClassKey, asset_class ) )
    {
        if ( !asset_data.GetTagValue( NativeClassKey, asset_class ) )
        {
            if ( auto * asset = asset_data.GetAsset() )
            {
                const FSoftClassPath class_path( asset->GetClass() );
                asset_class = *class_path.ToString();
            }
            else
            {
                return false;
            }
        }
    }

    return true;
}

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
    FMessageLogInitializationOptions init_options;
    init_options.bShowFilters = true;

    auto * settings = GetMutableDefault< UNamingConventionValidationSettings >();

    auto & message_log_module = FModuleManager::LoadModuleChecked< FMessageLogModule >( "MessageLog" );
    message_log_module.RegisterLogListing( "NamingConventionValidation", LOCTEXT( "NamingConventionValidation", "Naming Convention Validation" ), init_options );

    for ( auto & class_description : settings->ClassDescriptions )
    {
        class_description.Class = class_description.ClassPath.LoadSynchronous();

        UE_CLOG( class_description.Class == nullptr, LogTemp, Warning, TEXT( "Impossible to get a valid UClass for the classpath %s" ), *class_description.ClassPath.ToString() );
    }

    settings->ClassDescriptions.Sort();

    for ( auto & class_path : settings->ExcludedClassPaths )
    {
        auto * excluded_class = class_path.LoadSynchronous();
        UE_CLOG( excluded_class == nullptr, LogTemp, Warning, TEXT( "Impossible to get a valid UClass for the excluded classpath %s" ), *class_path.ToString() );

        if ( excluded_class != nullptr )
        {
            settings->ExcludedClasses.Add( excluded_class );
        }
    }

    static const FDirectoryPath
        EngineDirectoryPath( { TEXT( "/Engine/" ) } );

    // Cannot use AddUnique since FDirectoryPath does not have operator==
    if ( !settings->ExcludedDirectories.ContainsByPredicate( []( const auto & item ) {
             return item.Path == EngineDirectoryPath.Path;
         } ) )
    {
        settings->ExcludedDirectories.Add( EngineDirectoryPath );
    }
}

UNamingConventionValidationManager::~UNamingConventionValidationManager()
{
}

ENamingConventionValidationResult UNamingConventionValidationManager::IsAssetNamedCorrectly( const FAssetData & asset_data, FText & error_message ) const
{
    const auto * settings = GetDefault< UNamingConventionValidationSettings >();
    if ( settings->IsPathExcludedFromValidation( asset_data.PackageName.ToString() ) )
    {
        error_message = LOCTEXT( "ExcludedFolder", "The asset is in an excluded directory" );
        return ENamingConventionValidationResult::Excluded;
    }

    FName asset_class;
    if ( !TryGetAssetDataRealClass( asset_class, asset_data ) )
    {
        error_message = LOCTEXT( "UnknownClass", "The asset is of a class which has not been set up in the settings" );
        return ENamingConventionValidationResult::Unknown;
    }

    return DoesAssetMatchNameConvention( asset_data, asset_class, error_message );
}

int32 UNamingConventionValidationManager::ValidateAssets( const TArray< FAssetData > & asset_data_list, bool /*skip_excluded_directories*/ /* = true */, bool show_if_no_failures /* = true */ ) const
{
    FScopedSlowTask slow_task( 1.0f, LOCTEXT( "NamingConventionValidatingDataTask", "Validating Naming Convention..." ) );
    slow_task.Visibility = show_if_no_failures ? ESlowTaskVisibility::ForceVisible : ESlowTaskVisibility::Invisible;

    if ( show_if_no_failures )
    {
        slow_task.MakeDialogDelayed( 0.1f );
    }

    FMessageLog data_validation_log( "NamingConventionValidation" );

    auto num_files_checked = 0;
    auto num_valid_files = 0;
    auto num_invalid_files = 0;
    auto num_files_skipped = 0;
    auto num_files_unable_to_validate = 0;

    const auto num_files_to_validate = asset_data_list.Num();

    for ( const auto & asset_data : asset_data_list )
    {
        slow_task.EnterProgressFrame( 1.0f / num_files_to_validate, FText::Format( LOCTEXT( "ValidatingNamingConventionFilename", "Validating Naming Convention {0}" ), FText::FromString( asset_data.GetFullName() ) ) );

        FText error_message;
        const auto result = IsAssetNamedCorrectly( asset_data, error_message );

        switch ( result )
        {
            case ENamingConventionValidationResult::Excluded:
            {
                data_validation_log.Info()
                    ->AddToken( FAssetNameToken::Create( asset_data.PackageName.ToString() ) )
                    ->AddToken( FTextToken::Create( LOCTEXT( "ExcludedNamingConventionResult", "has not been tested based on the configuration." ) ) )
                    ->AddToken( FTextToken::Create( error_message ) );

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
                    ->AddToken( FTextToken::Create( LOCTEXT( "InvalidNamingConventionResult", "does not match naming convention." ) ) )
                    ->AddToken( FTextToken::Create( error_message ) );

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

    const auto has_failed = num_invalid_files > 0;

    if ( has_failed || show_if_no_failures )
    {
        FFormatNamedArguments arguments;
        arguments.Add( TEXT( "Result" ), has_failed ? LOCTEXT( "Failed", "FAILED" ) : LOCTEXT( "Succeeded", "SUCCEEDED" ) );
        arguments.Add( TEXT( "NumChecked" ), num_files_checked );
        arguments.Add( TEXT( "NumValid" ), num_valid_files );
        arguments.Add( TEXT( "NumInvalid" ), num_invalid_files );
        arguments.Add( TEXT( "NumSkipped" ), num_files_skipped );
        arguments.Add( TEXT( "NumUnableToValidate" ), num_files_unable_to_validate );

        auto validation_log = has_failed ? data_validation_log.Error() : data_validation_log.Info();
        validation_log->AddToken( FTextToken::Create( FText::Format( LOCTEXT( "SuccessOrFailure", "NamingConvention Validation {Result}." ), arguments ) ) );
        validation_log->AddToken( FTextToken::Create( FText::Format( LOCTEXT( "ResultsSummary", "Files Checked: {NumChecked}, Passed: {NumValid}, Failed: {NumInvalid}, Skipped: {NumSkipped}, Unable to validate: {NumUnableToValidate}" ), arguments ) ) );

        data_validation_log.Open( EMessageSeverity::Info, true );
    }

    return num_invalid_files;
}

void UNamingConventionValidationManager::ValidateOnSave( const TArray< FAssetData > & asset_data_list ) const
{
    auto * settings = GetDefault< UNamingConventionValidationSettings >();
    if ( !settings->DoesValidateOnSave || GEditor->IsAutosaving() )
    {
        return;
    }

    FMessageLog data_validation_log( "NamingConventionValidation" );

    if ( ValidateAssets( asset_data_list, true, false ) > 0 )
    {
        const auto error_message_notification = FText::Format(
            LOCTEXT( "ValidationFailureNotification", "Naming Convention Validation failed when saving {0}, check Naming Convention Validation log" ),
            asset_data_list.Num() == 1 ? FText::FromName( asset_data_list[ 0 ].AssetName ) : LOCTEXT( "MultipleErrors", "multiple assets" ) );
        data_validation_log.Notify( error_message_notification, EMessageSeverity::Warning, /*bForce=*/true );
    }
}

void UNamingConventionValidationManager::ValidateSavedPackage( const FName package_name )
{
    auto * settings = GetDefault< UNamingConventionValidationSettings >();
    if ( !settings->DoesValidateOnSave || GEditor->IsAutosaving() )
    {
        return;
    }

    SavedPackagesToValidate.AddUnique( package_name );

    GEditor->GetTimerManager()->SetTimerForNextTick( this, &UNamingConventionValidationManager::ValidateAllSavedPackages );
}

// -- PROTECTED

void UNamingConventionValidationManager::ValidateAllSavedPackages()
{
    auto & asset_registry_module = FModuleManager::LoadModuleChecked< FAssetRegistryModule >( "AssetRegistry" );
    TArray< FAssetData > assets;

    for ( const auto package_name : SavedPackagesToValidate )
    {
        // We need to query the in-memory data as the disk cache may not be accurate
        asset_registry_module.Get().GetAssetsByPackageName( package_name, assets );
    }

    ValidateOnSave( assets );

    SavedPackagesToValidate.Empty();
}

// -- PRIVATE

ENamingConventionValidationResult UNamingConventionValidationManager::DoesAssetMatchNameConvention( const FAssetData & asset_data, const FName asset_class, FText & error_message ) const
{
    const auto * settings = GetDefault< UNamingConventionValidationSettings >();
    const auto asset_name = asset_data.AssetName.ToString();
    const FSoftClassPath asset_class_path( asset_class.ToString() );

    if ( const auto asset_real_class = asset_class_path.TryLoadClass< UObject >() )
    {
        for ( auto * excluded_class : settings->ExcludedClasses )
        {
            if ( asset_real_class->IsChildOf( excluded_class ) )
            {
                error_message = FText::Format( LOCTEXT( "ExcludedClass", "Assets of class '{0}' are excluded from naming convention validation" ), FText::FromString( excluded_class->GetDefaultObjectName().ToString() ) );
                return ENamingConventionValidationResult::Excluded;
            }
        }

        for ( const auto & class_description : settings->ClassDescriptions )
        {
            if ( asset_real_class->IsChildOf( class_description.Class ) )
            {
                if ( !class_description.Prefix.IsEmpty() )
                {
                    if ( !asset_name.StartsWith( class_description.Prefix ) )
                    {
                        error_message = FText::Format( LOCTEXT( "WrongPrefix", "Assets of class '{0}' must have a name which starts with {1}" ), FText::FromString( class_description.ClassPath.ToString() ), FText::FromString( class_description.Prefix ) );
                        return ENamingConventionValidationResult::Invalid;
                    }
                }

                if ( !class_description.Suffix.IsEmpty() )
                {
                    if ( !asset_name.EndsWith( class_description.Suffix ) )
                    {
                        error_message = FText::Format( LOCTEXT( "WrongSuffix", "Assets of class '{0}' must have a name which ends with {1}" ), FText::FromString( class_description.ClassPath.ToString() ), FText::FromString( class_description.Prefix ) );
                        return ENamingConventionValidationResult::Invalid;
                    }
                }

                return ENamingConventionValidationResult::Valid;
            }
        }
    }

    static const FName BlueprintClassName( "Blueprint" );
    if ( asset_data.AssetClass == BlueprintClassName )
    {
        if ( !asset_name.StartsWith( settings->BlueprintsPrefix ) )
        {
            error_message = FText::FromString( TEXT( "Generic blueprint assets must start with BP_" ) );
            return ENamingConventionValidationResult::Invalid;
        }

        return ENamingConventionValidationResult::Valid;
    }

    return ENamingConventionValidationResult::Unknown;
}

void UNamingConventionValidationManager::GetRenamedAssetSoftObjectPath( FSoftObjectPath & renamed_soft_object_path, const FAssetData & asset_data ) const
{
    const auto * settings = GetDefault< UNamingConventionValidationSettings >();
    const auto path = asset_data.ToSoftObjectPath();
    FName asset_class;

    // /Game/Levels/Props/Meshes/1M_Cube.1M_Cube
    TryGetAssetDataRealClass( asset_class, asset_data );

    auto renamed_path = FPaths::GetPath( path.GetLongPackageName() );
    auto renamed_name = path.GetAssetName();

    const auto asset_name = asset_data.AssetName.ToString();
    const FSoftClassPath asset_class_path( asset_class.ToString() );

    if ( const auto asset_real_class = asset_class_path.TryLoadClass< UObject >() )
    {
        for ( const auto & class_description : settings->ClassDescriptions )
        {
            if ( asset_real_class->IsChildOf( class_description.Class ) )
            {
                if ( !class_description.Prefix.IsEmpty() )
                {
                    renamed_name.InsertAt( 0, class_description.Prefix );
                }

                if ( !class_description.Suffix.IsEmpty() )
                {
                    renamed_name.Append( class_description.Suffix );
                }

                break;
            }
        }
    }

    if ( renamed_name == path.GetAssetName() )
    {
        static const FName BlueprintClassName( "Blueprint" );
        if ( asset_data.AssetClass == BlueprintClassName )
        {
            renamed_name.InsertAt( 0, settings->BlueprintsPrefix );
        }
    }

    renamed_path.Append( "/" );
    renamed_path.Append( renamed_name );
    renamed_path.Append( "." );
    renamed_path.Append( renamed_name );

    renamed_soft_object_path.SetPath( renamed_path );
}

#undef LOCTEXT_NAMESPACE
