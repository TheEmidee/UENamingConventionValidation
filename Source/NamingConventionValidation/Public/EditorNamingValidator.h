#pragma once

#include "CoreMinimal.h"
#include "EditorValidatorBase.h"

#include "EditorNamingValidator.generated.h"

/**
 * Validator which checks assets naming convention
 */
UCLASS()
class NAMINGCONVENTIONVALIDATION_API UEditorNamingValidator : public UEditorValidatorBase
{
	GENERATED_BODY()

public:
	bool CanValidateAsset_Implementation(const FAssetData& InAssetData, UObject* InObject, FDataValidationContext& InContext) const override;
	EDataValidationResult ValidateLoadedAsset_Implementation(const FAssetData& InAssetData, UObject* InAsset, FDataValidationContext& Context) override;

private:
	bool IsClassExcluded(FDataValidationContext& Context, const UClass* AssetClass) const;
	EDataValidationResult DoesAssetMatchesClassDescriptions(FDataValidationContext& InContext, const UClass* AssetClass, const FString& AssetName) const;
};
