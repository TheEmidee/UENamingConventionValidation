#pragma once

#include "NamingValidatorBase.h"

#include <CoreMinimal.h>

#include "NamingValidatorInFolder.generated.h"

UCLASS( Abstract, Blueprintable, NotPlaceable )
class OURGAMEEDITOR_API UNamingValidatorInFolder : public UNamingValidatorBase
{
    GENERATED_BODY()

public:
    UNamingValidatorInFolder();

    bool CanValidateAssetNaming_Implementation( const UClass * asset_class, const FAssetData & asset_data ) const override;
    ENamingConventionValidationResult ValidateAssetNaming_Implementation( FText & error_message, const UClass * asset_class, const FAssetData & asset_data ) override;

protected:
    UPROPERTY( EditDefaultsOnly )
    FString ParentFolderName;

    UPROPERTY( EditDefaultsOnly )
    FString AssetTypeName;

    UPROPERTY( EditDefaultsOnly )
    uint8 ItAllowsSubFolders : 1;

    UPROPERTY( EditDefaultsOnly )
    uint8 ItMustCheckForParentFolderNameInAssetName : 1;

    UPROPERTY( EditDefaultsOnly )
    uint8 ItMustCheckForRegularAssetNamingValidation : 1;

    UPROPERTY( EditDefaultsOnly )
    TArray< FString > IgnoredFolders;

private:
    FString GetContentParentFolderPath() const;
};
