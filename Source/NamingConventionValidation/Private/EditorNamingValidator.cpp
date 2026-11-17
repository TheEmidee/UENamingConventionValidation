#include "EditorNamingValidator.h"

#include "Misc/DataValidation.h"
#include "NamingConventionValidationSettings.h"

#define LOCTEXT_NAMESPACE "NamingConventionValidation"

namespace {
bool TryGetAssetDataRealClass(FName& AssetClass, const FAssetData& InAssetData)
{
	static const FName
	    NativeParentClassKey("NativeParentClass"),
	    NativeClassKey("NativeClass");

	if (!InAssetData.GetTagValue(NativeParentClassKey, AssetClass))
	{
		if (!InAssetData.GetTagValue(NativeClassKey, AssetClass))
		{
			if (const auto* Asset = InAssetData.GetAsset())
			{
				const FSoftClassPath ClassPath(Asset->GetClass());
				AssetClass = *ClassPath.ToString();
			}
			else
			{
				return false;
			}
		}
	}

	return true;
}

bool IsPathExcludedFromValidation(const FString& Path)
{
	const auto* Settings = GetDefault<UNamingConventionValidationSettings>();

	if (!Path.StartsWith("/Game/") && Settings->bAllowValidationOnlyInGameFolder)
	{
		auto bCanProcessFolder = Settings->NonGameFoldersDirectoriesToProcess.FindByPredicate([&Path](const auto& Directory) {
			return Path.StartsWith(Directory.Path);
		}) != nullptr;

		if (!bCanProcessFolder)
		{
			bCanProcessFolder = Settings->NonGameFoldersDirectoriesToProcessContainingToken.FindByPredicate([&Path](const auto& Token) {
				return Path.Contains(Token);
			}) != nullptr;
		}

		if (!bCanProcessFolder)
		{
			return true;
		}
	}

	if (Path.StartsWith("/Game/Developers/") && !Settings->bAllowValidationInDevelopersFolder)
	{
		return true;
	}

	for (const auto& ExcludedPath : Settings->ExcludedDirectories)
	{
		if (Path.StartsWith(ExcludedPath.Path))
		{
			return true;
		}
	}

	return false;
}

bool IsClassExcluded(FDataValidationContext& Context, const UClass* AssetClass)
{
	const auto* Settings = GetDefault<UNamingConventionValidationSettings>();

	for (const auto& ExcludedClass : Settings->ExcludedClassPaths)
	{
		TSoftClassPtr<UObject> SoftClassPtr(ExcludedClass);

		if (auto* Class = SoftClassPtr.LoadSynchronous())
		{
			if (AssetClass->IsChildOf(Class))
			{
				Context.AddError(FText::Format(LOCTEXT("ExcludedClass", "Assets of class '{0}' are excluded from naming convention validation"), FText::FromString(ExcludedClass.ToString())));
				return true;
			}
		}
	}

	return false;
}

EDataValidationResult DoesAssetMatchesClassDescriptions(FDataValidationContext& InContext, const UClass* AssetClass, const FString& AssetName)
{
	const auto* Settings = GetDefault<UNamingConventionValidationSettings>();
	const UClass* MostPreciseClass = UObject::StaticClass();
	EDataValidationResult Result = EDataValidationResult::NotValidated;

	for (const auto& ClassDescription : Settings->ClassDescriptions)
	{
		TSoftClassPtr<UObject> SoftClassPtr(ClassDescription.ClassPath);
		UClass* Class = SoftClassPtr.LoadSynchronous();
		if (Class == nullptr)
		{
			FMessageLog DataValidationLog("NamingConventionValidation");
			DataValidationLog
			    .Warning()
			    ->AddToken(FTextToken::Create(FText::FromString(FString::Printf(TEXT("invalid class description found : %s"), *ClassDescription.ToString()))));
			continue;
		}

		const bool bClassFilterMatches = AssetClass->IsChildOf(Class);
		const bool bClassIsMorePreciseOrTheSame = Class->IsChildOf(MostPreciseClass);
		const bool bClassIsSame = bClassIsMorePreciseOrTheSame && Class == MostPreciseClass;
		const bool bClassIsMorePrecise = bClassIsMorePreciseOrTheSame && Class != MostPreciseClass;
		// had an error on this precision level before. but there could be another filter that passes
		const bool bSamePrecisionCanBeValid = bClassIsSame && Result != EDataValidationResult::Valid;

		const bool bCheckAffixes = bClassFilterMatches && (bClassIsMorePrecise || bSamePrecisionCanBeValid);
		if (bCheckAffixes)
		{
			MostPreciseClass = Class;

			Result = EDataValidationResult::Valid;

			if (!ClassDescription.Prefix.IsEmpty())
			{
				if (!AssetName.StartsWith(ClassDescription.Prefix))
				{
					InContext.AddError(FText::Format(LOCTEXT("WrongPrefix", "Assets of class '{0}' must have a name which starts with {1}"), FText::FromString(ClassDescription.ClassPath.ToString()), FText::FromString(ClassDescription.Prefix)));
					Result = EDataValidationResult::Invalid;
				}
			}

			if (!ClassDescription.Suffix.IsEmpty())
			{
				if (!AssetName.EndsWith(ClassDescription.Suffix))
				{
					InContext.AddError(FText::Format(LOCTEXT("WrongSuffix", "Assets of class '{0}' must have a name which ends with {1}"), FText::FromString(ClassDescription.ClassPath.ToString()), FText::FromString(ClassDescription.Suffix)));
					Result = EDataValidationResult::Invalid;
				}
			}
		}
	}

	return Result;
}
}

bool UEditorNamingValidator::CanValidateAsset_Implementation(const FAssetData& InAssetData, UObject* InObject, FDataValidationContext& InContext) const
{
	return true;
}

EDataValidationResult UEditorNamingValidator::ValidateLoadedAsset_Implementation(const FAssetData& InAssetData, UObject* InAsset, FDataValidationContext& Context)
{
	const auto* Settings = GetDefault<UNamingConventionValidationSettings>();
	if (IsPathExcludedFromValidation(InAssetData.PackageName.ToString()))
	{
		return EDataValidationResult::Valid;
	}

	FName AssetClass;
	if (!TryGetAssetDataRealClass(AssetClass, InAssetData))
	{
		return EDataValidationResult::Invalid;
	}

	static const FTopLevelAssetPath BlueprintGeneratedClassName(FName(TEXT("/")), FName(TEXT("BlueprintGeneratedClass")));

	auto AssetName = InAssetData.AssetName.ToString();

	// Starting UE4.27 (?) some blueprints now have BlueprintGeneratedClass as their AssetClass, and their name ends with a _C.
	if (InAssetData.AssetClassPath == BlueprintGeneratedClassName)
	{
		AssetName.RemoveFromEnd(TEXT("_C"), ESearchCase::CaseSensitive);
	}

	const FSoftClassPath AssetClassPath(AssetClass.ToString());

	if (const auto* AssetRealClass = AssetClassPath.TryLoadClass<UObject>())
	{
		if (IsClassExcluded(Context, AssetRealClass))
		{
			return EDataValidationResult::Invalid;
		}

		const auto Result = DoesAssetMatchesClassDescriptions(Context, AssetRealClass, AssetName);
		if (Result == EDataValidationResult::Invalid)
		{
			return Result;
		}
	}

	static const FTopLevelAssetPath BlueprintClassName(FName(TEXT("/Script/Engine")), FName(TEXT("Blueprint")));

	if (InAssetData.AssetClassPath == BlueprintClassName || InAssetData.AssetClassPath == BlueprintGeneratedClassName)
	{
		if (!AssetName.StartsWith(Settings->BlueprintsPrefix))
		{
			Context.AddError(FText::FromString(TEXT("Generic blueprint assets must start with BP_")));
			return EDataValidationResult::Invalid;
		}
	}

	return EDataValidationResult::Valid;
}

#undef LOCTEXT_NAMESPACE