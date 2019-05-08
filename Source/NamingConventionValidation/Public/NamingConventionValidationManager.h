#pragma once

#include "AssetData.h"
#include "CoreMinimal.h"
#include "Engine/EngineTypes.h"
#include "UObject/Object.h"

#include "NamingConventionValidationManager.generated.h"

class FLogWindowManager;
class SLogWindow;
class SWindow;
class UUnitTest;

enum class ENamingConventionValidationResult : uint8
{
    Invalid,
    Valid,
    Unknown,
    Excluded
};

USTRUCT()
struct FNamingConventionValidationClassDescription
{
    GENERATED_USTRUCT_BODY()

    UPROPERTY( config )
    FSoftClassPath ClassPath;

    UPROPERTY( transient )
    UClass * Class;

    UPROPERTY( config )
    FString Prefix;

    UPROPERTY( config )
    FString Suffix;
};

UCLASS( config = Editor )
class NAMINGCONVENTIONVALIDATION_API UNamingConventionValidationManager : public UObject
{
    GENERATED_UCLASS_BODY()

public:
    static UNamingConventionValidationManager * Get();

    virtual void Initialize();
    virtual ~UNamingConventionValidationManager();

    virtual ENamingConventionValidationResult IsAssetNamedCorrectly( const FAssetData & asset_data, FText & error_message ) const;
    virtual int32 ValidateAssets( const TArray< FAssetData > & asset_data_list, bool skip_excluded_directories = true, bool show_if_no_failures = true ) const;
    virtual void ValidateOnSave( const TArray< FAssetData > & asset_data_list ) const;
    virtual void ValidateSavedPackage( FName package_name );

protected:
    virtual bool IsPathExcludedFromValidation( const FString & path ) const;
    void ValidateAllSavedPackages();

    UPROPERTY( config )
    TArray< FDirectoryPath > ExcludedDirectories;

    UPROPERTY( config )
    bool bValidateOnSave;

    UPROPERTY( config )
    TArray< FNamingConventionValidationClassDescription > ClassDescriptions;

    UPROPERTY( config )
    TArray< FSoftClassPath > ExcludedClassPaths;

    UPROPERTY( transient )
    TArray< UClass * > ExcludedClasses;

    UPROPERTY( config )
    FString BlueprintsPrefix;

    TArray< FName > SavedPackagesToValidate;

private:
    ENamingConventionValidationResult DoesAssetMatchNameConvention( const FString & asset_name, const FName asset_class, FText & error_message ) const;

    UPROPERTY( config )
    FSoftClassPath NamingConventionValidationManagerClassName;
};
