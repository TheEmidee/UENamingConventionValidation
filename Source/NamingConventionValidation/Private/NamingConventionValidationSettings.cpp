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

bool UNamingConventionValidationSettings::IsPathExcludedFromValidation(const FString& Path) const
{
	if (!Path.StartsWith("/Game/") && bAllowValidationOnlyInGameFolder)
	{
		auto can_process_folder = NonGameFoldersDirectoriesToProcess.FindByPredicate([&Path](const auto& directory) {
			return Path.StartsWith(directory.Path);
		}) != nullptr;

		if (!can_process_folder)
		{
			can_process_folder = NonGameFoldersDirectoriesToProcessContainingToken.FindByPredicate([&Path](const auto& token) {
				return Path.Contains(token);
			}) != nullptr;
		}

		if (!can_process_folder)
		{
			return true;
		}
	}

	if (Path.StartsWith("/Game/Developers/") && !bAllowValidationInDevelopersFolder)
	{
		return true;
	}

	for (const auto& excluded_path : ExcludedDirectories)
	{
		if (Path.StartsWith(excluded_path.Path))
		{
			return true;
		}
	}

	return false;
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