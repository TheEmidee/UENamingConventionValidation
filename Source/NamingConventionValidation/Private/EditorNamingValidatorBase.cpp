#include "EditorNamingValidatorBase.h"

UEditorNamingValidatorBase::UEditorNamingValidatorBase()
{
    ItIsEnabled = true;
}

bool UEditorNamingValidatorBase::CanValidateAssetNaming_Implementation( const UClass * asset_class, const FString & asset_name ) const
{
    return false;
}

ENamingConventionValidationResult UEditorNamingValidatorBase::ValidateAssetNaming_Implementation( FText & error_message, const UClass * asset_class, const FString & asset_name )
{
    return ENamingConventionValidationResult::Unknown;
}

bool UEditorNamingValidatorBase::IsEnabled() const
{
    return ItIsEnabled;
}
