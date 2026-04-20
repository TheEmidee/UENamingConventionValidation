#include "NamingValidatorFromFolder.h"

#include "Misc/DataValidation.h"

UNamingValidatorFromFolder::UNamingValidatorFromFolder()
    : bValidateAssetsAreInSameFolder(false), bCheckForRegularAssetNamingValidation(true)
{
	IgnoredFolders = { "Shared", "Unused" };
}

bool UNamingValidatorFromFolder::CanValidateAsset_Implementation(const FAssetData& InAssetData, UObject* InObject, FDataValidationContext& InContext) const
{
	const auto PackagePath = InAssetData.PackagePath.ToString();

	if (!PackagePath.StartsWith(ParentFolderName))
	{
		return false;
	}

	for (const auto& ignored_folder : IgnoredFolders)
	{
		if (PackagePath.Contains(ignored_folder))
		{
			return false;
		}
	}

	const auto* AssetClass = InObject->GetClass();

	for (const auto* ignored_class : IgnoredClasses)
	{
		if (AssetClass->IsChildOf(ignored_class))
		{
			return false;
		}
	}

	return Super::CanValidateAsset_Implementation(InAssetData, InObject, InContext);
}

EDataValidationResult UNamingValidatorFromFolder::ValidateLoadedAsset_Implementation(const FAssetData& InAssetData, UObject* InAsset, FDataValidationContext& Context)
{
	const auto PackagePath = InAssetData.PackagePath.ToString();
	const auto RemainingPath = PackagePath.RightChop(ParentFolderName.Len());

	TArray<FString> Parts;
	const auto* Delimiter = TEXT("/");
	RemainingPath.ParseIntoArray(Parts, Delimiter);

	if (bValidateAssetsAreInSameFolder && Parts.Num() != 1)
	{
		Context.AddError(FText::FromString(FString::Printf(TEXT("Assets in the folder %s must all be in the same subfolder"), *ParentFolderName)));
		return EDataValidationResult::Invalid;
	}

	const auto AssetName = InAssetData.AssetName.ToString();

	TArray<FString> FilenameParts;
	const auto* FilenameDelimiter = TEXT("_");

	if (AssetName.ParseIntoArray(FilenameParts, FilenameDelimiter) == 0 || FilenameParts.Num() < 2)
	{
		Context.AddError(FText::FromString(FString::Printf(TEXT("Impossible to parse the filename. Asset name must conform to something like BP_XXX"))));
		return EDataValidationResult::Invalid;
	}

	if (!IdentifierToken.IsEmpty())
	{
		// If the identifier token contains underscores, we must rework the filename_parts to regroup the tokens to form the identifier
		TArray<FString> IdentifierTokenParts;
		IdentifierToken.ParseIntoArray(IdentifierTokenParts, TEXT("_"));
		const auto TokenCount = IdentifierTokenParts.Num();

		if (TokenCount > 1)
		{
			for (auto FilenameTokenIndex = 0; FilenameTokenIndex < FilenameParts.Num(); ++FilenameTokenIndex)
			{
				if (const auto Token = FilenameParts[FilenameTokenIndex]; Token == IdentifierTokenParts[0])
				{
					auto bAreAllTokensPresent = true;

					for (auto IdentifierTokenIndex = 1; IdentifierTokenIndex < IdentifierTokenParts.Num(); ++IdentifierTokenIndex)
					{
						if (FilenameParts[FilenameTokenIndex + IdentifierTokenIndex] != IdentifierTokenParts[IdentifierTokenIndex])
						{
							bAreAllTokensPresent = false;
							break;
						}
					}

					if (bAreAllTokensPresent)
					{
						FilenameParts[FilenameTokenIndex] = IdentifierToken;
						FilenameParts.RemoveAt(FilenameTokenIndex + 1, IdentifierTokenParts.Num() - 1);
					}
				}
			}
		}

		if (FilenameParts[1] != IdentifierToken)
		{
			Context.AddError(FText::FromString(FString::Printf(TEXT("The name of the asset must start with %s_%s"), *FilenameParts[0], *IdentifierToken)));
			return EDataValidationResult::Invalid;
		}
	}

	if (!bCheckForRegularAssetNamingValidation)
	{
		return EDataValidationResult::Valid;
	}

	return Super::ValidateLoadedAsset_Implementation(InAssetData, InAsset, Context);
}
