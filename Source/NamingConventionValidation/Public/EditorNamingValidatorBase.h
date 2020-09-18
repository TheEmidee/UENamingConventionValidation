#pragma once

#include "EditorNamingValidatorSubsystem.h"

#include <CoreMinimal.h>
#include <UObject/NoExportTypes.h>

#include "EditorNamingValidatorBase.generated.h"

UCLASS()
class NAMINGCONVENTIONVALIDATION_API UEditorNamingValidatorBase : public UObject
{
    GENERATED_BODY()

public:
    UFUNCTION( BlueprintNativeEvent, BlueprintPure, Category = "Asset Naming Validation" )
    bool CanValidateAssetNaming( const UClass * asset_class, const FString & asset_name ) const;

    UFUNCTION( BlueprintNativeEvent, Category = "Asset Naming Validation" )
    ENamingConventionValidationResult ValidateAssetNaming( FText & error_message, const UClass * asset_class, const FString & asset_name );

    virtual bool IsEnabled() const;

protected:
    UPROPERTY( EditAnywhere, Category = "Asset Validation", meta = ( BlueprintProtected = "true" ), DisplayName = "IsEnabled" )
    bool ItIsEnabled;
};
