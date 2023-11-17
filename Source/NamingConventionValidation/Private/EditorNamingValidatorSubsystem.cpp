#include "EditorNamingValidatorSubsystem.h"

#include "NamingConventionValidationSettings.h"

#include <AssetRegistry/AssetRegistryModule.h>
#include <Editor.h>
#include <EditorNamingValidatorBase.h>
#include <EditorUtilityBlueprint.h>
#include <Logging/MessageLog.h>
#include <MessageLogInitializationOptions.h>
#include <MessageLogModule.h>
#include <Misc/ScopedSlowTask.h>
#include <UObject/UObjectHash.h>

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
            if ( const auto * asset = asset_data.GetAsset() )
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

UEditorNamingValidatorSubsystem::UEditorNamingValidatorSubsystem()
{
    AllowBlueprintValidators = true;
}

void UEditorNamingValidatorSubsystem::Initialize( FSubsystemCollectionBase & /*collection*/ )
{
    const auto & asset_registry_module = FModuleManager::LoadModuleChecked< FAssetRegistryModule >( TEXT( "AssetRegistry" ) );

    if ( !asset_registry_module.Get().IsLoadingAssets() )
    {
        RegisterBlueprintValidators();
    }
    else
    {
        if ( !asset_registry_module.Get().OnFilesLoaded().IsBoundToObject( this ) )
        {
            asset_registry_module.Get().OnFilesLoaded().AddUObject( this, &UEditorNamingValidatorSubsystem::RegisterBlueprintValidators );
        }
    }

    TArray< UClass * > validator_classes;
    GetDerivedClasses( UEditorNamingValidatorBase::StaticClass(), validator_classes );
    for ( const auto * validator_class : validator_classes )
    {
        if ( !validator_class->HasAllClassFlags( CLASS_Abstract ) )
        {
            if ( const auto * class_package = validator_class->GetOuterUPackage() )
            {
                const auto module_name = FPackageName::GetShortFName( class_package->GetFName() );
                if ( FModuleManager::Get().IsModuleLoaded( module_name ) )
                {
                    auto * validator = NewObject< UEditorNamingValidatorBase >( GetTransientPackage(), validator_class );
                    AddValidator( validator );
                }
            }
        }
    }

    FMessageLogInitializationOptions init_options;
    init_options.bShowFilters = true;

    auto & message_log_module = FModuleManager::LoadModuleChecked< FMessageLogModule >( "MessageLog" );
    message_log_module.RegisterLogListing( "NamingConventionValidation", LOCTEXT( "NamingConventionValidation", "Naming Convention Validation" ), init_options );

    auto * settings = GetMutableDefault< UNamingConventionValidationSettings >();
    settings->PostProcessSettings();
}

void UEditorNamingValidatorSubsystem::Deinitialize()
{
    CleanupValidators();

    Super::Deinitialize();
}

int32 UEditorNamingValidatorSubsystem::ValidateAssets( const TArray< FAssetData > & asset_data_list, bool /*skip_excluded_directories*/, const bool show_if_no_failures ) const
{
    const auto * settings = GetDefault< UNamingConventionValidationSettings >();

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
        const auto result = IsAssetNamedCorrectly( error_message, asset_data );

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
                if ( show_if_no_failures && settings->bLogWarningWhenNoClassDescriptionForAsset )
                {
                    FFormatNamedArguments arguments;
                    arguments.Add( TEXT( "ClassName" ), FText::FromString( asset_data.AssetClassPath.ToString() ) );

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

void UEditorNamingValidatorSubsystem::ValidateSavedPackage( const FName package_name )
{
    auto * settings = GetDefault< UNamingConventionValidationSettings >();
    if ( !settings->bDoesValidateOnSave || GEditor->IsAutosaving() )
    {
        return;
    }

    SavedPackagesToValidate.AddUnique( package_name );

    GEditor->GetTimerManager()->SetTimerForNextTick( this, &UEditorNamingValidatorSubsystem::ValidateAllSavedPackages );
}

void UEditorNamingValidatorSubsystem::AddValidator( UEditorNamingValidatorBase * validator )
{
    if ( validator )
    {
        Validators.Add( validator->GetClass(), validator );
    }
}

ENamingConventionValidationResult UEditorNamingValidatorSubsystem::IsAssetNamedCorrectly( FText & error_message, const FAssetData & asset_data, bool can_use_editor_validators ) const
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

    return DoesAssetMatchNameConvention( error_message, asset_data, asset_class, can_use_editor_validators );
}

void UEditorNamingValidatorSubsystem::RegisterBlueprintValidators()
{
    if ( !AllowBlueprintValidators )
    {
        return;
    }

    // Locate all validators (include unloaded)
    const auto & asset_registry_module = FModuleManager::LoadModuleChecked< FAssetRegistryModule >( TEXT( "AssetRegistry" ) );
    TArray< FAssetData > all_blueprint_asset_data;
    asset_registry_module.Get().GetAssetsByClass( UEditorUtilityBlueprint::StaticClass()->GetClassPathName(), all_blueprint_asset_data, true );

    for ( auto & asset_data : all_blueprint_asset_data )
    {
        FString parent_class_name;

        if ( !asset_data.GetTagValue( FBlueprintTags::NativeParentClassPath, parent_class_name ) )
        {
            asset_data.GetTagValue( FBlueprintTags::ParentClassPath, parent_class_name );
        }

        if ( !parent_class_name.IsEmpty() )
        {
            const UClass* parent_class = nullptr;
            UObject* Outer = nullptr;

            ResolveName(Outer, parent_class_name, false, false);
            parent_class = FindObject<UClass>(Outer, *parent_class_name);

            if ( parent_class == nullptr
                || !parent_class->IsChildOf( UEditorNamingValidatorBase::StaticClass() ) )
            {
                continue;
            }
        }

        // If this object isn't currently loaded, load it
        auto * validator_object = asset_data.ToSoftObjectPath().ResolveObject();
        if ( validator_object == nullptr )
        {
            validator_object = asset_data.ToSoftObjectPath().TryLoad();
        }
        if ( validator_object )
        {
            const auto * validator_blueprint = Cast< UEditorUtilityBlueprint >( validator_object );
            auto * validator = NewObject< UEditorNamingValidatorBase >( GetTransientPackage(), validator_blueprint->GeneratedClass );
            AddValidator( validator );
        }
    }
}

void UEditorNamingValidatorSubsystem::CleanupValidators()
{
    Validators.Empty();
}

void UEditorNamingValidatorSubsystem::ValidateAllSavedPackages()
{
    const auto & asset_registry_module = FModuleManager::LoadModuleChecked< FAssetRegistryModule >( "AssetRegistry" );
    TArray< FAssetData > assets;

    for ( const auto package_name : SavedPackagesToValidate )
    {
        // We need to query the in-memory data as the disk cache may not be accurate
        asset_registry_module.Get().GetAssetsByPackageName( package_name, assets );
    }

    ValidateOnSave( assets );

    SavedPackagesToValidate.Empty();
}

void UEditorNamingValidatorSubsystem::ValidateOnSave( const TArray< FAssetData > & asset_data_list ) const
{
    const auto * settings = GetDefault< UNamingConventionValidationSettings >();
    if ( !settings->bDoesValidateOnSave || GEditor->IsAutosaving() )
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

ENamingConventionValidationResult UEditorNamingValidatorSubsystem::DoesAssetMatchNameConvention( FText & error_message, const FAssetData & asset_data, const FName asset_class, bool can_use_editor_validators ) const
{
    const auto * settings = GetDefault< UNamingConventionValidationSettings >();
    static const FTopLevelAssetPath BlueprintGeneratedClassName( FName( TEXT( "/" ) ), FName( TEXT( "BlueprintGeneratedClass" ) ) );

    auto asset_name = asset_data.AssetName.ToString();

    // Starting UE4.27 (?) some blueprints now have BlueprintGeneratedClass as their AssetClass, and their name ends with a _C.
    if ( asset_data.AssetClassPath == BlueprintGeneratedClassName )
    {
        asset_name.RemoveFromEnd( TEXT( "_C" ), ESearchCase::CaseSensitive );
    }

    const FSoftClassPath asset_class_path( asset_class.ToString() );

    if ( const auto * asset_real_class = asset_class_path.TryLoadClass< UObject >() )
    {
        if ( IsClassExcluded( error_message, asset_real_class ) )
        {
            return ENamingConventionValidationResult::Excluded;
        }

        auto result = ENamingConventionValidationResult::Unknown;

        if ( can_use_editor_validators )
        {
            result = DoesAssetMatchesValidators( error_message, asset_real_class, asset_data );
            if ( result != ENamingConventionValidationResult::Unknown )
            {
                return result;
            }
        }

        result = DoesAssetMatchesClassDescriptions( error_message, asset_real_class, asset_name );
        if ( result != ENamingConventionValidationResult::Unknown )
        {
            return result;
        }
    }

    static const FTopLevelAssetPath BlueprintClassName( FName( TEXT( "/Script/Engine" ) ), FName( TEXT( "Blueprint" ) ) );

    if ( asset_data.AssetClassPath == BlueprintClassName || asset_data.AssetClassPath == BlueprintGeneratedClassName )
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

bool UEditorNamingValidatorSubsystem::IsClassExcluded( FText & error_message, const UClass * asset_class ) const
{
    const auto * settings = GetDefault< UNamingConventionValidationSettings >();

    for ( const auto * excluded_class : settings->ExcludedClasses )
    {
        if ( asset_class->IsChildOf( excluded_class ) )
        {
            error_message = FText::Format( LOCTEXT( "ExcludedClass", "Assets of class '{0}' are excluded from naming convention validation" ), FText::FromString( excluded_class->GetDefaultObjectName().ToString() ) );
            return true;
        }
    }

    return false;
}

ENamingConventionValidationResult UEditorNamingValidatorSubsystem::DoesAssetMatchesClassDescriptions( FText & error_message, const UClass * asset_class, const FString & asset_name ) const
{
    const auto * settings = GetDefault< UNamingConventionValidationSettings >();
    const UClass* MostPreciseClass = UObject::StaticClass();
    ENamingConventionValidationResult Result = ENamingConventionValidationResult::Unknown;

    for ( const auto & class_description : settings->ClassDescriptions )
    {
        if ( class_description.Class == nullptr ) 
        {
            FMessageLog data_validation_log( "NamingConventionValidation" );
            data_validation_log
                .Warning()
                ->AddToken( FTextToken::Create( FText::FromString( FString::Printf( TEXT( "invalid class description found : %s" ), *class_description.ToString() ) ) ) );
            continue;
        }

        const bool bClassFilterMatches = asset_class->IsChildOf( class_description.Class );
        const bool bClassIsMoreOrSamePrecise = class_description.Class->IsChildOf(MostPreciseClass);
        const bool bClassIsSamePrecise = bClassIsMoreOrSamePrecise && class_description.Class == MostPreciseClass;
        const bool bClassIsMorePrecise = bClassIsMoreOrSamePrecise && class_description.Class != MostPreciseClass;
        // had an error on this precision level before. but there could be another filter that passes
        const bool bSamePrecisionCanBeValid = bClassIsSamePrecise && Result != ENamingConventionValidationResult::Valid;

        const bool bCheckAffixes = bClassFilterMatches && (bClassIsMorePrecise || bSamePrecisionCanBeValid);
        if ( bCheckAffixes )
        {
            MostPreciseClass = class_description.Class;

            error_message = FText::GetEmpty();
            Result = ENamingConventionValidationResult::Valid;

            if ( !class_description.Prefix.IsEmpty() )
            {
                if ( !asset_name.StartsWith( class_description.Prefix ) )
                {
                    error_message = FText::Format( LOCTEXT( "WrongPrefix", "Assets of class '{0}' must have a name which starts with {1}" ), FText::FromString( class_description.ClassPath.ToString() ), FText::FromString( class_description.Prefix ) );
                    Result = ENamingConventionValidationResult::Invalid;
                }
            }

            if ( !class_description.Suffix.IsEmpty() )
            {
                if ( !asset_name.EndsWith( class_description.Suffix ) )
                {
                    error_message = FText::Format( LOCTEXT( "WrongSuffix", "Assets of class '{0}' must have a name which ends with {1}" ), FText::FromString( class_description.ClassPath.ToString() ), FText::FromString( class_description.Suffix ) );
                    Result = ENamingConventionValidationResult::Invalid;
                }
            }
        }
    }

    return Result;
}

ENamingConventionValidationResult UEditorNamingValidatorSubsystem::DoesAssetMatchesValidators( FText & error_message, const UClass * asset_class, const FAssetData & asset_data ) const
{
    for ( const auto & validator_pair : Validators )
    {
        if ( validator_pair.Value != nullptr && validator_pair.Value->IsEnabled() && validator_pair.Value->CanValidateAssetNaming( asset_class, asset_data ) )
        {
            const auto result = validator_pair.Value->ValidateAssetNaming( error_message, asset_class, asset_data );

            if ( result != ENamingConventionValidationResult::Valid )
            {
                return result;
            }
        }
    }

    return ENamingConventionValidationResult::Unknown;
}

#undef LOCTEXT_NAMESPACE
