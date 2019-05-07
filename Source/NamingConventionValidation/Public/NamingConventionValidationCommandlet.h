#pragma once

#include "Commandlets/Commandlet.h"
#include "UObject/Interface.h"
#include "NamingConventionValidationCommandlet.generated.h"

UCLASS(CustomConstructor)
class NAMINGCONVENTIONVALIDATION_API UNamingConventionValidationCommandlet : public UCommandlet
{
    GENERATED_UCLASS_BODY()

public:

    UNamingConventionValidationCommandlet( const FObjectInitializer & object_initializer );

    // Begin UCommandlet Interface
    virtual int32 Main(const FString & params) override;
    // End UCommandlet Interface

    static bool ValidateData();
};
