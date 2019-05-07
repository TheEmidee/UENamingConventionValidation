#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "AssetData.h"
#include "Engine/EngineTypes.h"
#include "NamingConventionValidationManager.generated.h"

class FLogWindowManager;
class SLogWindow;
class SWindow;
class UUnitTest;

enum class ENamingConventionValidationResult : uint8
{
    Invalid,
    Valid,
    Unknown
};

UCLASS(config=Editor)
class NAMINGCONVENTIONVALIDATION_API UNamingConventionValidationManager : public UObject
{
    GENERATED_UCLASS_BODY()

public:
    static UNamingConventionValidationManager * Get();

    virtual void Initialize();
    virtual ~UNamingConventionValidationManager();

    virtual ENamingConventionValidationResult IsObjectNamedCorrectly( UObject * object ) const;
    virtual ENamingConventionValidationResult IsAssetNamedCorrectly( const FAssetData & asset_data ) const;
    virtual int32 ValidateAssets( const TArray< FAssetData > & asset_data_list, bool skip_excluded_directories = true, bool show_if_no_failures = true ) const;
    virtual void ValidateOnSave( const TArray< FAssetData > & asset_data_list ) const;
    virtual void ValidateSavedPackage( FName package_name );

protected:

    virtual bool IsPathExcludedFromValidation( const FString & path ) const;
    void ValidateAllSavedPackages();

    UPROPERTY(config)
    TArray<FDirectoryPath> ExcludedDirectories;

    UPROPERTY(config)
    bool bValidateOnSave;

    TArray<FName> SavedPackagesToValidate;

private:

    UPROPERTY(config)
    FSoftClassPath NamingConventionValidationManagerClassName;
};
