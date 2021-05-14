#include "NamingValidatorInFolder.h"

#include "EditorNamingValidatorSubsystem.h"

#include <Editor.h>

UNamingValidatorInFolder::UNamingValidatorInFolder()
{
    ItAllowsSubFolders = false;
    ItMustCheckForParentFolderNameInAssetName = true;
    ItMustCheckForRegularAssetNamingValidation = true;
    IgnoredFolders = { "Shared", "Unused" };
}

bool UNamingValidatorInFolder::CanValidateAssetNaming_Implementation( const UClass * /* asset_class */, const FAssetData & asset_data ) const
{
    const auto package_path = asset_data.PackagePath.ToString();

    if ( !package_path.StartsWith( GetContentParentFolderPath() ) )
    {
        return false;
    }

    for ( const auto & ignored_folder : IgnoredFolders )
    {
        if ( package_path.Contains( ignored_folder ) )
        {
            return false;
        }
    }

    return true;
}

ENamingConventionValidationResult UNamingValidatorInFolder::ValidateAssetNaming_Implementation( FText & error_message, const UClass * /* asset_class */, const FAssetData & asset_data )
{
    const auto package_path = asset_data.PackagePath.ToString();
    const auto remaining_path = package_path.RightChop( GetContentParentFolderPath().Len() );

    TArray< FString > parts;
    const auto * delimiter = TEXT( "/" );
    if ( remaining_path.ParseIntoArray( parts, delimiter ) == 0 )
    {
        error_message = FText::FromString( FString::Printf( TEXT( "Impossible to parse the path" ) ) );
        return ENamingConventionValidationResult::Invalid;
    }

    if ( !ItAllowsSubFolders && parts.Num() != 1 )
    {
        error_message = FText::FromString( FString::Printf( TEXT( "Assets in the folder %s must all be in the same subfolder" ), *ParentFolderName ) );
        return ENamingConventionValidationResult::Invalid;
    }

    const auto child_folder_name = parts[ 0 ];
    const auto child_file_name = asset_data.AssetName.ToString();

    TArray< FString > filename_parts;
    const auto * filename_delimiter = TEXT( "_" );

    if ( child_file_name.ParseIntoArray( filename_parts, filename_delimiter ) == 0 || filename_parts.Num() < 2 )
    {
        error_message = FText::FromString( FString::Printf( TEXT( "Impossible to parse the filename. Asset name must conform to something like BP_XXX" ) ) );
        return ENamingConventionValidationResult::Invalid;
    }

    if ( ItMustCheckForParentFolderNameInAssetName )
    {
        if ( filename_parts[ 1 ] != AssetTypeName )
        {
            error_message = FText::FromString( FString::Printf( TEXT( "Assets in the folder %s must have the %s name right after the asset type prefix" ), *ParentFolderName, *AssetTypeName ) );
            return ENamingConventionValidationResult::Invalid;
        }

        if ( filename_parts[ 2 ] != child_folder_name )
        {
            error_message = FText::FromString( FString::Printf( TEXT( "Assets in the folder %s must have their name after the asset type prefix and the asset category (Ex: BP_Enemy_Walker)" ), *ParentFolderName ) );
            return ENamingConventionValidationResult::Invalid;
        }
    }
    else if ( filename_parts[ 1 ] != child_folder_name )
    {
        error_message = FText::FromString( FString::Printf( TEXT( "Assets in the folder %s must have their name right after the asset type prefix (Ex: BP_RocketLauncher)" ), *ParentFolderName ) );
        return ENamingConventionValidationResult::Invalid;
    }

    if ( !ItMustCheckForRegularAssetNamingValidation )
    {
        return ENamingConventionValidationResult::Valid;
    }

    return GEditor->GetEditorSubsystem< UEditorNamingValidatorSubsystem >()->IsAssetNamedCorrectly( error_message, asset_data, false );
}

FString UNamingValidatorInFolder::GetContentParentFolderPath() const
{
    return FString::Printf( TEXT( "/Game/OurGame/%s" ), *ParentFolderName );
}
