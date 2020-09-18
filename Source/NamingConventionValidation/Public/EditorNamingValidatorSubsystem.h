#pragma once

#include <CoreMinimal.h>
#include <EditorSubsystem.h>

#include "EditorNamingValidatorSubsystem.generated.h"

enum class ENamingConventionValidationResult : uint8
{
    Invalid,
    Valid,
    Unknown,
    Excluded
};

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
    void ValidateSavedPackage( const FName package_name );

private:
    void RegisterBlueprintValidators();
    void AddValidator( UEditorNamingValidatorBase * validator );
    void CleanupValidators();
    void ValidateAllSavedPackages();
    void ValidateOnSave( const TArray< FAssetData > & asset_data_list ) const;
    ENamingConventionValidationResult IsAssetNamedCorrectly( const FAssetData & asset_data, FText & error_message ) const;
    ENamingConventionValidationResult DoesAssetMatchNameConvention( const FAssetData & asset_data, const FName asset_class, FText & error_message ) const;

    UPROPERTY( config )
    bool AllowBlueprintValidators;

    UPROPERTY( Transient )
    TMap< UClass *, UEditorNamingValidatorBase * > Validators;

    TArray< FName > SavedPackagesToValidate;
};
