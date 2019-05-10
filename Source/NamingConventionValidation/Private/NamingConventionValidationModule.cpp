#include "NamingConventionValidationModule.h"

#include "NamingConventionValidationCommandlet.h"
#include "NamingConventionValidationManager.h"

#include <AssetRegistryModule.h>
#include <AssetToolsModule.h>
#include <ContentBrowserDelegates.h>
#include <ContentBrowserModule.h>
#include <EditorStyleSet.h>
#include <Framework/Application/SlateApplication.h>
#include <Framework/MultiBox/MultiBoxBuilder.h>
#include <Framework/MultiBox/MultiBoxExtender.h>
#include <LevelEditor.h>
#include <Misc/MessageDialog.h>
#include <Modules/ModuleManager.h>
#include <UObject/Object.h>
#include <UObject/SoftObjectPath.h>

#define LOCTEXT_NAMESPACE "NamingConventionValidationModule"

void LOCAL_FindAssetDependencies( const FAssetRegistryModule & asset_registry_module, const FAssetData & asset_data, TSet< FAssetData > & dependent_assets )
{
    if ( asset_data.IsValid() )
    {
        if ( UObject * object = asset_data.GetAsset() )
        {
            const FName selected_package_name = object->GetOutermost()->GetFName();
            FString package_string = selected_package_name.ToString();
            FString object_string = FString::Printf( TEXT( "%s.%s" ), *package_string, *FPackageName::GetLongPackageAssetName( package_string ) );

            if ( !dependent_assets.Contains( asset_data ) )
            {
                dependent_assets.Add( asset_data );

                TArray< FName > dependencies;
                asset_registry_module.Get().GetDependencies( selected_package_name, dependencies, EAssetRegistryDependencyType::Packages );

                for ( const FName dependency : dependencies )
                {
                    const FString dependency_package_string = dependency.ToString();
                    FString dependency_object_string = FString::Printf( TEXT( "%s.%s" ), *dependency_package_string, *FPackageName::GetLongPackageAssetName( dependency_package_string ) );

                    // recurse on each dependency
                    FName object_path( *dependency_object_string );
                    FAssetData dependent_asset = asset_registry_module.Get().GetAssetByObjectPath( object_path );

                    LOCAL_FindAssetDependencies( asset_registry_module, dependent_asset, dependent_assets );
                }
            }
        }
    }
}

void LOCAL_OnPackageSaved( const FString & package_file_name, UObject * object )
{
    if ( auto * manager = UNamingConventionValidationManager::Get() )
    {
        if ( object != nullptr )
        {
            manager->ValidateSavedPackage( object->GetFName() );
        }
    }
}

void LOCAL_ValidateAssets( TArray< FAssetData > selected_assets )
{
    if ( auto * naming_convention_validation_manager = UNamingConventionValidationManager::Get() )
    {
        naming_convention_validation_manager->ValidateAssets( selected_assets );
    }
}

void LOCAL_RenameAssets( TArray< FAssetData > selected_assets, bool include_dependencies )
{
    if ( auto * naming_convention_validation_manager = UNamingConventionValidationManager::Get() )
    {
        TSet< FAssetData > dependent_assets;
        if ( include_dependencies )
        {
            const FAssetRegistryModule & asset_registry_module = FModuleManager::LoadModuleChecked< FAssetRegistryModule >( "AssetRegistry" );

            for ( const FAssetData & asset_data : selected_assets )
            {
                LOCAL_FindAssetDependencies( asset_registry_module, asset_data, dependent_assets );
            }

            selected_assets = dependent_assets.Array();
        }

        naming_convention_validation_manager->RenameAssets( selected_assets );
    }
}

void LOCAL_ValidateFolders( TArray< FString > selected_folders )
{
    FAssetRegistryModule & asset_registry_module = FModuleManager::Get().LoadModuleChecked< FAssetRegistryModule >( TEXT( "AssetRegistry" ) );

    FARFilter filter;
    filter.bRecursivePaths = true;

    for ( const auto & folder : selected_folders )
    {
        filter.PackagePaths.Emplace( *folder );
    }

    TArray< FAssetData > asset_list;
    asset_registry_module.Get().GetAssets( filter, asset_list );

    LOCAL_ValidateAssets( asset_list );
}

void LOCAL_RenameInFolders( TArray< FString > selected_folders )
{
    FAssetRegistryModule & asset_registry_module = FModuleManager::Get().LoadModuleChecked< FAssetRegistryModule >( TEXT( "AssetRegistry" ) );

    FARFilter filter;
    filter.bRecursivePaths = true;

    for ( const auto & folder : selected_folders )
    {
        filter.PackagePaths.Emplace( *folder );
    }

    TArray< FAssetData > asset_list;
    asset_registry_module.Get().GetAssets( filter, asset_list );

    LOCAL_RenameAssets( asset_list, false );
}

void LOCAL_CreateDataValidationContentBrowserAssetMenu( FMenuBuilder & menu_builder, TArray< FAssetData > selected_assets )
{
    menu_builder.AddMenuSeparator();
    menu_builder.AddMenuEntry(
        LOCTEXT( "NamingConventionValidateAssetsTabTitle", "Validate Assets Naming Convention" ),
        LOCTEXT( "NamingConventionValidateAssetsTooltipText", "Runs naming convention validation on these assets." ),
        FSlateIcon(),
        FUIAction( FExecuteAction::CreateStatic( LOCAL_ValidateAssets, selected_assets ) ) );
    menu_builder.AddMenuEntry(
        LOCTEXT( "NamingConventionRenameAssetsTabTitle", "Rename Assets following Naming Convention" ),
        LOCTEXT( "NamingConventionRenameAssetsTooltipText", "Runs a renaming following the naming convention on these assets." ),
        FSlateIcon(),
        FUIAction( FExecuteAction::CreateStatic( LOCAL_RenameAssets, selected_assets, false ) ) );
    menu_builder.AddMenuEntry(
        LOCTEXT( "NamingConventionRenameAssetsInFolderTabTitle", "Rename Assets following Naming Convention, and all their dependencies" ),
        LOCTEXT( "NamingConventionRenameAssetsInFolderTooltipText", "Runs a renaming following the naming convention on these assets and their dependencies." ),
        FSlateIcon(),
        FUIAction( FExecuteAction::CreateStatic( LOCAL_RenameAssets, selected_assets, true ) ) );
}

TSharedRef< FExtender > LOCAL_OnExtendContentBrowserAssetSelectionMenu( const TArray< FAssetData > & selected_assets )
{
    TSharedRef< FExtender > extender( new FExtender() );

    extender->AddMenuExtension(
        "AssetContextAdvancedActions",
        EExtensionHook::After,
        nullptr,
        FMenuExtensionDelegate::CreateStatic( LOCAL_CreateDataValidationContentBrowserAssetMenu, selected_assets ) );

    return extender;
}

void LOCAL_CreateDataValidationContentBrowserPathMenu( FMenuBuilder & menu_builder, TArray< FString > selected_paths )
{
    menu_builder.AddMenuEntry(
        LOCTEXT( "NamingConventionValidateAssetsPathTabTitle", "Validate Assets Naming Convention in Folder" ),
        LOCTEXT( "NamingConventionValidateAssetsPathTooltipText", "Runs naming convention validation on the assets in the selected folder." ),
        FSlateIcon(),
        FUIAction( FExecuteAction::CreateStatic( LOCAL_ValidateFolders, selected_paths ) ) );
    menu_builder.AddMenuEntry(
        LOCTEXT( "NamingConventionRenameAssetsPathTabTitle", "Renaming assets following Assets Naming Convention in Folder" ),
        LOCTEXT( "NamingConventionRenameAssetsPathTooltipText", "Runs naming convention validation on the assets in the selected folder." ),
        FSlateIcon(),
        FUIAction( FExecuteAction::CreateStatic( LOCAL_RenameInFolders, selected_paths ) ) );
}

TSharedRef< FExtender > LOCAL_OnExtendContentBrowserPathSelectionMenu( const TArray< FString > & selected_paths )
{
    TSharedRef< FExtender > Extender( new FExtender() );

    Extender->AddMenuExtension(
        "PathContextBulkOperations",
        EExtensionHook::After,
        nullptr,
        FMenuExtensionDelegate::CreateStatic( LOCAL_CreateDataValidationContentBrowserPathMenu, selected_paths ) );

    return Extender;
}

FText LOCAL_MenuValidateDataGetTitle()
{
    FAssetRegistryModule & asset_registry_module = FModuleManager::LoadModuleChecked< FAssetRegistryModule >( "AssetRegistry" );
    if ( asset_registry_module.Get().IsLoadingAssets() )
    {
        return LOCTEXT( "NamingConventionValidationTitleDA", "Validate Naming Convention [Discovering Assets]" );
    }
    return LOCTEXT( "NamingConventionValidationTitle", "Naming Convention..." );
}

void LOCAL_MenuValidateData()
{
    // make sure the asset registry is finished building
    FAssetRegistryModule & asset_registry_module = FModuleManager::LoadModuleChecked< FAssetRegistryModule >( "AssetRegistry" );
    if ( asset_registry_module.Get().IsLoadingAssets() )
    {
        FMessageDialog::Open( EAppMsgType::Ok, LOCTEXT( "AssetsStillScanningError", "Cannot run naming convention validation while still discovering assets." ) );
        return;
    }

    const auto success = UNamingConventionValidationCommandlet::ValidateData();

    if ( !success )
    {
        FMessageDialog::Open( EAppMsgType::Ok, LOCTEXT( "NamingConventionValidationError", "An error was encountered during naming convention validation. See the log for details." ) );
        return;
    }
}

void LOCAL_NamingConventionValidationMenuCreationDelegate( FMenuBuilder & menu_builder )
{
    menu_builder.BeginSection( "NamingConventionValidation", LOCTEXT( "NamingConventionValidation", "NamingConventionValidation" ) );
    menu_builder.AddMenuEntry(
        TAttribute< FText >::Create( &LOCAL_MenuValidateDataGetTitle ),
        LOCTEXT( "NamingConventionValidationTooltip", "Validates all naming convention in content directory." ),
        FSlateIcon( FEditorStyle::GetStyleSetName(), "DeveloperTools.MenuIcon" ),
        FUIAction( FExecuteAction::CreateStatic( &LOCAL_MenuValidateData ) ) );
    menu_builder.EndSection();
}

class FNamingConventionValidationModule : public INamingConventionValidationModule
{
    virtual void StartupModule() override;
    virtual void ShutdownModule() override;

private:
    TSharedPtr< FExtender > MenuExtender;
    FDelegateHandle ContentBrowserAssetExtenderDelegateHandle;
    FDelegateHandle ContentBrowserPathExtenderDelegateHandle;
    FDelegateHandle OnPackageSavedDelegateHandle;
};

IMPLEMENT_MODULE( FNamingConventionValidationModule, NamingConventionValidation )

void FNamingConventionValidationModule::StartupModule()
{
    if ( !IsRunningCommandlet() && !IsRunningGame() && FSlateApplication::IsInitialized() )
    {
        FContentBrowserModule & content_browser_module = FModuleManager::LoadModuleChecked< FContentBrowserModule >( TEXT( "ContentBrowser" ) );
        TArray< FContentBrowserMenuExtender_SelectedAssets > & CBAssetMenuExtenderDelegates = content_browser_module.GetAllAssetViewContextMenuExtenders();

        CBAssetMenuExtenderDelegates.Add( FContentBrowserMenuExtender_SelectedAssets::CreateStatic( LOCAL_OnExtendContentBrowserAssetSelectionMenu ) );
        ContentBrowserAssetExtenderDelegateHandle = CBAssetMenuExtenderDelegates.Last().GetHandle();

        TArray< FContentBrowserMenuExtender_SelectedPaths > & CBFolderMenuExtenderDelegates = content_browser_module.GetAllPathViewContextMenuExtenders();

        CBFolderMenuExtenderDelegates.Add( FContentBrowserMenuExtender_SelectedPaths::CreateStatic( LOCAL_OnExtendContentBrowserPathSelectionMenu ) );
        ContentBrowserPathExtenderDelegateHandle = CBFolderMenuExtenderDelegates.Last().GetHandle();

        MenuExtender = MakeShareable( new FExtender );
        MenuExtender->AddMenuExtension( "FileLoadAndSave", EExtensionHook::After, nullptr, FMenuExtensionDelegate::CreateStatic( LOCAL_NamingConventionValidationMenuCreationDelegate ) );

        FLevelEditorModule & LevelEditorModule = FModuleManager::LoadModuleChecked< FLevelEditorModule >( "LevelEditor" );
        LevelEditorModule.GetMenuExtensibilityManager()->AddExtender( MenuExtender );

        OnPackageSavedDelegateHandle = UPackage::PackageSavedEvent.AddStatic( LOCAL_OnPackageSaved );
    }
}

void FNamingConventionValidationModule::ShutdownModule()
{
    if ( !IsRunningCommandlet() && !IsRunningGame() && !IsRunningDedicatedServer() )
    {
        if ( auto * content_browser_module = FModuleManager::GetModulePtr< FContentBrowserModule >( TEXT( "ContentBrowser" ) ) )
        {
            TArray< FContentBrowserMenuExtender_SelectedAssets > & content_browser_menu_extender_delegates = content_browser_module->GetAllAssetViewContextMenuExtenders();
            content_browser_menu_extender_delegates.RemoveAll( [this]( const FContentBrowserMenuExtender_SelectedAssets & Delegate ) {
                return Delegate.GetHandle() == ContentBrowserAssetExtenderDelegateHandle;
            } );
            content_browser_menu_extender_delegates.RemoveAll( [this]( const FContentBrowserMenuExtender_SelectedAssets & Delegate ) {
                return Delegate.GetHandle() == ContentBrowserPathExtenderDelegateHandle;
            } );
        }

        if ( auto * level_editor_module = FModuleManager::GetModulePtr< FLevelEditorModule >( "LevelEditor" ) )
        {
            level_editor_module->GetMenuExtensibilityManager()->RemoveExtender( MenuExtender );
        }
        MenuExtender = nullptr;

        UPackage::PackageSavedEvent.Remove( OnPackageSavedDelegateHandle );
    }
}

#undef LOCTEXT_NAMESPACE