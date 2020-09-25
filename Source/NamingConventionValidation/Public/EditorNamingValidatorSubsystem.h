#pragma once

#include "NamingConventionValidationTypes.h"

#include <CoreMinimal.h>
#include <EditorSubsystem.h>

#include "EditorNamingValidatorSubsystem.generated.h"

class UEditorNamingValidatorBase;

UCLASS( Config = Editor )
class NAMINGCONVENTIONVALIDATION_API UEditorNamingValidatorSubsystem final : public UEditorSubsystem
{
    GENERATED_BODY()

public:
    UEditorNamingValidatorSubsystem();

    void Initialize( FSubsystemCollectionBase & collection ) override;
    void Deinitialize() override;

    int32 ValidateAssets( const TArray< FAssetData > & asset_data_list, bool skip_excluded_directories = true, bool show_if_no_failures = true ) const;
    void ValidateSavedPackage( FName package_name );
    void AddValidator( UEditorNamingValidatorBase * validator );
    ENamingConventionValidationResult IsAssetNamedCorrectly( FText & error_message, const FAssetData & asset_data, bool can_use_editor_validators = true ) const;

private:
    void RegisterBlueprintValidators();
    void CleanupValidators();
    void ValidateAllSavedPackages();
    void ValidateOnSave( const TArray< FAssetData > & asset_data_list ) const;
    ENamingConventionValidationResult DoesAssetMatchNameConvention( FText & error_message, const FAssetData & asset_data, FName asset_class, bool can_use_editor_validators = true ) const;
    bool IsClassExcluded( FText & error_message, const UClass * asset_class ) const;
    ENamingConventionValidationResult DoesAssetMatchesClassDescriptions( FText & error_message, const UClass * asset_class, const FString & asset_name ) const;
    ENamingConventionValidationResult DoesAssetMatchesValidators( FText & error_message, const UClass * asset_class, const FAssetData & asset_data ) const;

    UPROPERTY( config )
    uint8 AllowBlueprintValidators : 1;

    UPROPERTY( Transient )
    TMap< UClass *, UEditorNamingValidatorBase * > Validators;

    TArray< FName > SavedPackagesToValidate;
};
