#include "NamingConventionValidation/Public/NamingConventionValidationSettings.h"

UNamingConventionValidationSettings::UNamingConventionValidationSettings()
{
    NamingConventionValidationManagerClassName = FSoftClassPath( TEXT( "/Script/NamingConventionValidation.NamingConventionValidationManager" ) );
    
    DoesValidateOnSave = true;
    BlueprintsPrefix = "BP_";
}

bool UNamingConventionValidationSettings::IsPathExcludedFromValidation( const FString & path ) const
{
    for ( const auto & excluded_path : ExcludedDirectories )
    {
        if ( path.Contains( excluded_path.Path ) )
        {
            return true;
        }
    }

    return false;
}