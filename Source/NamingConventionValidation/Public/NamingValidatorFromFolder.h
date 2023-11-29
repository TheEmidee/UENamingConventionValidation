#pragma once

#include "EditorNamingValidatorBase.h"

#include <CoreMinimal.h>

#include "NamingValidatorFromFolder.generated.h"

UCLASS( Abstract, Blueprintable, NotPlaceable )
class NAMINGCONVENTIONVALIDATION_API UNamingValidatorFromFolder : public UEditorNamingValidatorBase
{
    GENERATED_BODY()

public:
    UNamingValidatorFromFolder();

    bool CanValidateAssetNaming_Implementation( const UClass * asset_class, const FAssetData & asset_data ) const override;
    ENamingConventionValidationResult ValidateAssetNaming_Implementation( FText & error_message, const UClass * asset_class, const FAssetData & asset_data ) override;

protected:
    // The root folder that this rule will be applied to (Eg /Game)
    UPROPERTY( EditDefaultsOnly )
    FString ParentFolderName;

    // The token to look for in the asset name. For example if the identifier token is Foo, the assets must be named like BP_Foo_Character
    UPROPERTY( EditDefaultsOnly )
    FString IdentifierToken;

    // Set to true to validate that all the assets that are being validated are in the ParentFolderName and not in a sub-folder
    UPROPERTY( EditDefaultsOnly )
    uint8 bValidateAssetsAreInSameFolder : 1;

    // Set to true to also validate the regular naming convention (such as does the static mesh asset starts with SM_)
    UPROPERTY( EditDefaultsOnly )
    uint8 bCheckForRegularAssetNamingValidation : 1;

    UPROPERTY( EditDefaultsOnly )
    TArray< FString > IgnoredFolders;

    UPROPERTY( EditDefaultsOnly )
    TArray< UClass * > IgnoredClasses;	
};
