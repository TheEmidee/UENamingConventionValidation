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

	// First pass: find the most derived class, among the class descriptions, that the asset class inherits from.
	// All the matching classes are ancestors of AssetClass, so they form a single chain and the most derived one is unique.
	TArray<TPair<const FNamingConventionValidationClassDescription*, const UClass*>, TInlineAllocator<8>> MatchingDescriptions;
	const UClass* MostDerivedClass = nullptr;

	for (const auto& ClassDescription : Settings->ClassDescriptions)
	{
		TSoftClassPtr<UObject> SoftClassPtr(ClassDescription.ClassPath);
		const UClass* Class = SoftClassPtr.LoadSynchronous();
		if (Class == nullptr)
		{
			FMessageLog DataValidationLog("NamingConventionValidation");
			DataValidationLog
			    .Warning()
			    ->AddToken(FTextToken::Create(FText::FromString(FString::Printf(TEXT("invalid class description found : %s"), *ClassDescription.ToString()))));
			continue;
		}

		if (!AssetClass->IsChildOf(Class))
		{
			continue;
		}

		MatchingDescriptions.Emplace(&ClassDescription, Class);

		if (MostDerivedClass == nullptr || Class->IsChildOf(MostDerivedClass))
		{
			MostDerivedClass = Class;
		}
	}

	if (MostDerivedClass == nullptr)
	{
		return EDataValidationResult::NotValidated;
	}

	// Second pass: only check the descriptions of the most derived class.
	// Several descriptions can target the same class: the asset is valid if its name matches any of them.
	TArray<FText, TInlineAllocator<4>> Errors;

	for (const auto& [ClassDescription, Class] : MatchingDescriptions)
	{
		if (Class != MostDerivedClass)
		{
			continue;
		}

		const int32 ErrorCountBefore = Errors.Num();

		if (!ClassDescription->Prefix.IsEmpty() && !AssetName.StartsWith(ClassDescription->Prefix))
		{
			Errors.Emplace(FText::Format(LOCTEXT("WrongPrefix", "Assets of class '{0}' must have a name which starts with {1}"), FText::FromString(ClassDescription->ClassPath.ToString()), FText::FromString(ClassDescription->Prefix)));
		}

		if (!ClassDescription->Suffix.IsEmpty() && !AssetName.EndsWith(ClassDescription->Suffix))
		{
			Errors.Emplace(FText::Format(LOCTEXT("WrongSuffix", "Assets of class '{0}' must have a name which ends with {1}"), FText::FromString(ClassDescription->ClassPath.ToString()), FText::FromString(ClassDescription->Suffix)));
		}

		if (Errors.Num() == ErrorCountBefore)
		{
			return EDataValidationResult::Valid;
		}
	}

	for (const auto& Error : Errors)
	{
		InContext.AddError(Error);
	}

	return EDataValidationResult::Invalid;
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

	bool bCheckBlueprintNaming = true;

	if (const auto* AssetRealClass = AssetClassPath.TryLoadClass<UObject>())
	{
		if (IsClassExcluded(Context, AssetRealClass))
		{
			return EDataValidationResult::Invalid;
		}

		switch (DoesAssetMatchesClassDescriptions(Context, AssetRealClass, AssetName))
		{
			case EDataValidationResult::Invalid:
				return EDataValidationResult::Invalid;
			case EDataValidationResult::Valid:
			{
				bCheckBlueprintNaming = false;
			}
			break;
			case EDataValidationResult::NotValidated:
				break;
			default:
			{
				checkNoEntry();
			}
		}
	}

	if (bCheckBlueprintNaming)
	{
		static const FTopLevelAssetPath BlueprintClassName(FName(TEXT("/Script/Engine")), FName(TEXT("Blueprint")));

		if (InAssetData.AssetClassPath == BlueprintClassName || InAssetData.AssetClassPath == BlueprintGeneratedClassName)
		{
			if (!AssetName.StartsWith(Settings->BlueprintsPrefix))
			{
				Context.AddError(FText::FromString(TEXT("Generic blueprint assets must start with BP_")));
				return EDataValidationResult::Invalid;
			}
		}
	}

	return EDataValidationResult::Valid;
}

#undef LOCTEXT_NAMESPACE