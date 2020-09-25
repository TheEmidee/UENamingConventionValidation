#pragma once

#include "NamingConventionValidationTypes.generated.h"

UENUM()
enum class ENamingConventionValidationResult : uint8
{
    Invalid,
    Valid,
    Unknown,
    Excluded
};