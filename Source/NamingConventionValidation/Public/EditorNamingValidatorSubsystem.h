#pragma once

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

private:
    void RegisterBlueprintValidators();
    void AddValidator( UEditorNamingValidatorBase * validator );
    void CleanupValidators();

    UPROPERTY( config )
    bool AllowBlueprintValidators;

    UPROPERTY( Transient )
    TMap< UClass *, UEditorNamingValidatorBase * > Validators;
};
