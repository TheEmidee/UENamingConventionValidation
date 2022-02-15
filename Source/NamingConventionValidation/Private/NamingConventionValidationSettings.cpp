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
        return true;
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