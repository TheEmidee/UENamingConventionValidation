#pragma once

#include "NamingConventionValidationTypes.h"

#include <CoreMinimal.h>
#include <UObject/NoExportTypes.h>

#include "EditorNamingValidatorBase.generated.h"

UCLASS()
class NAMINGCONVENTIONVALIDATION_API UEditorNamingValidatorBase : public UObject
{
    GENERATED_BODY()

public:
    UEditorNamingValidatorBase();

    UFUNCTION( BlueprintNativeEvent, BlueprintPure, Category = "Asset Naming Validation" )
    bool CanValidateAssetNaming( const UClass * asset_class, const FAssetData & asset_data ) const;

    UFUNCTION( BlueprintNativeEvent, Category = "Asset Naming Validation" )
    ENamingConventionValidationResult ValidateAssetNaming( FText & error_message, const UClass * asset_class, const FAssetData & asset_data );

    virtual bool IsEnabled() const;

protected:
    UPROPERTY( EditAnywhere, Category = "Asset Validation", meta = ( BlueprintProtected = true ), DisplayName = "IsEnabled" )
    uint8 ItIsEnabled : 1;
};
