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

UCLASS()
class NAMINGCONVENTIONVALIDATION_API UNamingConventionValidationManager final : public UObject
{
    GENERATED_UCLASS_BODY()

public:

    static UNamingConventionValidationManager * Get();

    void Initialize();
    virtual ~UNamingConventionValidationManager();

    ENamingConventionValidationResult IsAssetNamedCorrectly( const FAssetData & asset_data, FText & error_message ) const;
    int32 ValidateAssets( const TArray< FAssetData > & asset_data_list, bool skip_excluded_directories = true, bool show_if_no_failures = true ) const;
    void ValidateOnSave( const TArray< FAssetData > & asset_data_list ) const;
    void ValidateSavedPackage( const FName package_name );

protected:
    void ValidateAllSavedPackages();

    TArray< FName > SavedPackagesToValidate;

private:
    ENamingConventionValidationResult DoesAssetMatchNameConvention( const FAssetData & asset_data, const FName asset_class, FText & error_message ) const;
    void GetRenamedAssetSoftObjectPath( FSoftObjectPath & renamed_soft_object_path, const FAssetData & asset_data ) const;
};
