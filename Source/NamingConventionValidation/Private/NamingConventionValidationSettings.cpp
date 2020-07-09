#include "NamingConventionValidation/Public/NamingConventionValidationSettings.h"

UNamingConventionValidationSettings::UNamingConventionValidationSettings()
{
    AllowRenamingInDevelopersFolder = false;
    AllowRenamingOnlyInGameFolder = true;
    DoesValidateOnSave = true;
    BlueprintsPrefix = "BP_";
}

bool UNamingConventionValidationSettings::IsPathExcludedFromValidation( const FString & path ) const
{
    if ( !path.StartsWith( "/Game/" ) && AllowRenamingOnlyInGameFolder )
    {
        return true;
    }

    if ( path.StartsWith( "/Game/Developers/" ) && !AllowRenamingInDevelopersFolder )
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