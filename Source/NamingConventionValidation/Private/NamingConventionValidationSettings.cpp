#include "NamingConventionValidation/Public/NamingConventionValidationSettings.h"

UNamingConventionValidationSettings::UNamingConventionValidationSettings()
{
    bLogWarningWhenNoClassDescriptionForAsset = false;
    bAllowValidationInDevelopersFolder = false;
    bAllowValidationOnlyInGameFolder = true;
    bDoesValidateOnSave = true;
    BlueprintsPrefix = "BP_";
}

bool UNamingConventionValidationSettings::IsPathExcludedFromValidation( const FString & path ) const
{
    if ( !path.StartsWith( "/Game/" ) && bAllowValidationOnlyInGameFolder )
    {
        auto can_process_folder = NonGameFoldersDirectoriesToProcess.FindByPredicate( [ &path ]( const auto & directory ) {
            return path.StartsWith( directory.Path );
        } ) != nullptr;

        if ( !can_process_folder )
        {
            can_process_folder = NonGameFoldersDirectoriesToProcessContainingToken.FindByPredicate( [ &path ]( const auto & token ) {
                return path.Contains( token );
            } ) != nullptr;
        }

        if ( !can_process_folder )
        {
            return true;
        }
    }

    if ( path.StartsWith( "/Game/Developers/" ) && !bAllowValidationInDevelopersFolder )
    {
        return true;
    }

    for ( const auto & excluded_path : ExcludedDirectories )
    {
        if ( path.StartsWith( excluded_path.Path ) )
        {
            return true;
        }
    }

    return false;
}