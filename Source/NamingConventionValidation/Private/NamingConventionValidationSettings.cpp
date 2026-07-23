#include "NamingConventionValidation/Public/NamingConventionValidationSettings.h"

FString FNamingConventionValidationClassDescription::ToString() const
{
	return FString::Printf(TEXT("ClassPath : %s - Prefix : %s - Suffix : %s - Priority : %i"),
	    *ClassPath.ToString(),
	    *Prefix,
	    *Suffix,
	    Priority);
}

UNamingConventionValidationSettings::UNamingConventionValidationSettings()
{
	bLogWarningWhenNoClassDescriptionForAsset = false;
	bAllowValidationInDevelopersFolder = false;
	bAllowValidationOnlyInGameFolder = true;
	bDoesValidateOnSave = true;
	BlueprintsPrefix = "BP_";
}

#if WITH_EDITOR
void UNamingConventionValidationSettings::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	static const FDirectoryPath
	    EngineDirectoryPath({ TEXT("/Engine/") });

	// Cannot use AddUnique since FDirectoryPath does not have operator==
	if (!ExcludedDirectories.ContainsByPredicate([](const auto& item) {
		    return item.Path == EngineDirectoryPath.Path;
	    }))
	{
		ExcludedDirectories.Add(EngineDirectoryPath);
	}
}
#endif