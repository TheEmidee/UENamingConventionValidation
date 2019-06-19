#pragma once

#include <Commandlets/Commandlet.h>
#include <UObject/Interface.h>

#include "NamingConventionValidationCommandlet.generated.h"

UCLASS( CustomConstructor )
class NAMINGCONVENTIONVALIDATION_API UNamingConventionValidationCommandlet : public UCommandlet
{
    GENERATED_BODY()

public:

    UNamingConventionValidationCommandlet();

    // Begin UCommandlet Interface
    int32 Main( const FString & params ) override;
    // End UCommandlet Interface

    static bool ValidateData();
};
