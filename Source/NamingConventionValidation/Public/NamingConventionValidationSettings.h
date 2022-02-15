#pragma once

#include <CoreMinimal.h>
#include <Engine/DeveloperSettings.h>
#include <Engine/EngineTypes.h>

#include "NamingConventionValidationSettings.generated.h"

USTRUCT()
struct FNamingConventionValidationClassDescription
{
    GENERATED_USTRUCT_BODY()

    FNamingConventionValidationClassDescription() :
        Class( nullptr ),
        Priority( 0 )
    {}

    bool operator<( const FNamingConventionValidationClassDescription & other ) const
    {
        return Priority > other.Priority;
    }

    UPROPERTY( config, EditAnywhere )
    TSoftClassPtr< UObject > ClassPath;

    UPROPERTY( transient )
    UClass * Class;

    UPROPERTY( config, EditAnywhere )
    FString Prefix;

    UPROPERTY( config, EditAnywhere )
    FString Suffix;

    UPROPERTY( config, EditAnywhere )
    int Priority;
};

UCLASS( config = Editor )
class NAMINGCONVENTIONVALIDATION_API UNamingConventionValidationSettings final : public UDeveloperSettings
{
    GENERATED_BODY()

public:
    UNamingConventionValidationSettings();

    bool IsPathExcludedFromValidation( const FString & path ) const;

    UPROPERTY( config, EditAnywhere, meta = ( LongPackageName, ConfigRestartRequired = true ) )
    TArray< FDirectoryPath > ExcludedDirectories;

    UPROPERTY( config, EditAnywhere )
    uint8 bLogWarningWhenNoClassDescriptionForAsset : 1;

    UPROPERTY( config, EditAnywhere )
    uint8 bAllowValidationInDevelopersFolder : 1;

    UPROPERTY( config, EditAnywhere )
    uint8 bAllowValidationOnlyInGameFolder : 1;

    // Add folders located outside of /Game that you still want to process when bAllowValidationOnlyInGameFolder is checked
    UPROPERTY( config, EditAnywhere, meta = ( LongPackageName, ConfigRestartRequired = true, editCondition = "bAllowValidationOnlyInGameFolder" ) )
    TArray< FDirectoryPath > NonGameFoldersDirectoriesToProcess;

    // Add folders located outside of /Game that you still want to process when bAllowValidationOnlyInGameFolder is checked, and which contain one of those tokens in their path
    UPROPERTY( config, EditAnywhere, meta = ( LongPackageName, ConfigRestartRequired = true, editCondition = "bAllowValidationOnlyInGameFolder" ) )
    TArray< FString > NonGameFoldersDirectoriesToProcessContainingToken;

    UPROPERTY( config, EditAnywhere )
    uint8 bDoesValidateOnSave : 1;

    UPROPERTY( config, EditAnywhere, meta = ( ConfigRestartRequired = true ) )
    TArray< FNamingConventionValidationClassDescription > ClassDescriptions;

    UPROPERTY( config, EditAnywhere )
    TArray< TSoftClassPtr< UObject > > ExcludedClassPaths;

    UPROPERTY( transient )
    TArray< UClass * > ExcludedClasses;

    UPROPERTY( config, EditAnywhere )
    FString BlueprintsPrefix;
};