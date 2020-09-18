#include "EditorNamingValidatorSubsystem.h"

#include "AssetRegistryModule.h"
#include "EditorNamingValidatorBase.h"
#include "EditorUtilityBlueprint.h"

UEditorNamingValidatorSubsystem::UEditorNamingValidatorSubsystem()
{
    AllowBlueprintValidators = true;
}

void UEditorNamingValidatorSubsystem::Initialize( FSubsystemCollectionBase & /*collection*/ )
{
    const auto & asset_registry_module = FModuleManager::LoadModuleChecked< FAssetRegistryModule >( TEXT( "AssetRegistry" ) );

    if ( !asset_registry_module.Get().IsLoadingAssets() )
    {
        RegisterBlueprintValidators();
    }
    else
    {
        if ( !asset_registry_module.Get().OnFilesLoaded().IsBoundToObject( this ) )
        {
            asset_registry_module.Get().OnFilesLoaded().AddUObject( this, &UEditorNamingValidatorSubsystem::RegisterBlueprintValidators );
        }
    }

    TArray< UClass * > validator_classes;
    GetDerivedClasses( UEditorNamingValidatorBase::StaticClass(), validator_classes );
    for ( auto * validator_class : validator_classes )
    {
        if ( !validator_class->HasAllClassFlags( CLASS_Abstract ) )
        {
            if ( const auto * class_package = validator_class->GetOuterUPackage() )
            {
                const auto module_name = FPackageName::GetShortFName( class_package->GetFName() );
                if ( FModuleManager::Get().IsModuleLoaded( module_name ) )
                {
                    auto * validator = NewObject< UEditorNamingValidatorBase >( GetTransientPackage(), validator_class );
                    AddValidator( validator );
                }
            }
        }
    }
}

void UEditorNamingValidatorSubsystem::Deinitialize()
{
    CleanupValidators();

    Super::Deinitialize();
}

void UEditorNamingValidatorSubsystem::RegisterBlueprintValidators()
{
    if ( !AllowBlueprintValidators )
    {
        return;
    }

    // Locate all validators (include unloaded)
    const auto & asset_registry_module = FModuleManager::LoadModuleChecked< FAssetRegistryModule >( TEXT( "AssetRegistry" ) );
    TArray< FAssetData > all_blueprint_asset_data;
    asset_registry_module.Get().GetAssetsByClass( UEditorUtilityBlueprint::StaticClass()->GetFName(), all_blueprint_asset_data, true );

    for ( auto & asset_data : all_blueprint_asset_data )
    {
        UClass * parent_class = nullptr;
        FString parent_class_name;

        if ( !asset_data.GetTagValue( FBlueprintTags::NativeParentClassPath, parent_class_name ) )
        {
            asset_data.GetTagValue( FBlueprintTags::ParentClassPath, parent_class_name );
        }

        if ( !parent_class_name.IsEmpty() )
        {
            UObject * outer = nullptr;
            ResolveName( outer, parent_class_name, false, false );
            parent_class = FindObject< UClass >( ANY_PACKAGE, *parent_class_name );
            if ( !parent_class->IsChildOf( UEditorNamingValidatorBase::StaticClass() ) )
            {
                continue;
            }
        }

        // If this object isn't currently loaded, load it
        auto * validator_object = asset_data.ToSoftObjectPath().ResolveObject();
        if ( validator_object == nullptr )
        {
            validator_object = asset_data.ToSoftObjectPath().TryLoad();
        }
        if ( validator_object )
        {
            auto * validator_blueprint = Cast< UEditorUtilityBlueprint >( validator_object );
            auto * validator = NewObject< UEditorNamingValidatorBase >( GetTransientPackage(), validator_blueprint->GeneratedClass );
            AddValidator( validator );
        }
    }
}

void UEditorNamingValidatorSubsystem::AddValidator( UEditorNamingValidatorBase * validator )
{
    if ( validator )
    {
        Validators.Add( validator->GetClass(), validator );
    }
}

void UEditorNamingValidatorSubsystem::CleanupValidators()
{
    Validators.Empty();
}
