#include "NamingConventionValidation/Public/NamingConventionValidationSettings.h"
#include "NamingConventionValidationLog.h"

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

void UNamingConventionValidationSettings::PostProcessSettings()
{
    for ( auto & class_description : ClassDescriptions )
    {
        class_description.Class = class_description.ClassPath.LoadSynchronous();

        UE_CLOG( class_description.Class == nullptr, LogNamingConventionValidation, Warning, TEXT( "Impossible to get a valid UClass for the classpath %s" ), *class_description.ClassPath.ToString() );
    }

    ClassDescriptions.Sort();

    for ( auto & class_path : ExcludedClassPaths )
    {
        auto * excluded_class = class_path.LoadSynchronous();
        UE_CLOG( excluded_class == nullptr, LogNamingConventionValidation, Warning, TEXT( "Impossible to get a valid UClass for the excluded classpath %s" ), *class_path.ToString() );

        if ( excluded_class != nullptr )
        {
            ExcludedClasses.Add( excluded_class );
        }
    }

    static const FDirectoryPath
        EngineDirectoryPath( { TEXT( "/Engine/" ) } );

    // Cannot use AddUnique since FDirectoryPath does not have operator==
    if ( !ExcludedDirectories.ContainsByPredicate( []( const auto & item ) {
             return item.Path == EngineDirectoryPath.Path;
         } ) )
    {
        ExcludedDirectories.Add( EngineDirectoryPath );
    }
}

#if WITH_EDITOR
void UNamingConventionValidationSettings::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
    Super::PostEditChangeProperty(PropertyChangedEvent);
    PostProcessSettings();
}
#endif
