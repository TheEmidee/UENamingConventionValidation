#include "NamingConventionValidation/Public/NamingConventionValidationSettings.h"

UNamingConventionValidationSettings::UNamingConventionValidationSettings()
{
    LogWarningWhenNoClassDescriptionForAsset = false;
    AllowValidationInDevelopersFolder = false;
    AllowValidationOnlyInGameFolder = true;
    DoesValidateOnSave = true;
    BlueprintsPrefix = "BP_";
}

bool UNamingConventionValidationSettings::IsPathExcludedFromValidation( const FString & path ) const
{
    if ( !path.StartsWith( "/Game/" ) && AllowValidationOnlyInGameFolder )
    {
        return true;
    }

    if ( path.StartsWith( "/Game/Developers/" ) && !AllowValidationInDevelopersFolder )
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