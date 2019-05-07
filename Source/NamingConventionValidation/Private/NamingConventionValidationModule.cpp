#include "NamingConventionValidationModule.h"

#include "NamingConventionValidationCommandlet.h"
#include "NamingConventionValidationManager.h"

#include <AssetRegistryModule.h>
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
    if ( auto * naming_conventioon_validation_manager = UNamingConventionValidationManager::Get() )
    {
        naming_conventioon_validation_manager->ValidateAssets( selected_assets, false );
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

void LOCAL_CreateDataValidationContentBrowserAssetMenu( FMenuBuilder & menu_builder, TArray< FAssetData > selected_assets )
{
    menu_builder.AddMenuSeparator();
    menu_builder.AddMenuEntry(
        LOCTEXT( "NamingConventionValidateAssetsTabTitle", "Validate Assets Naming Convention" ),
        LOCTEXT( "NamingConventionValidateAssetsTooltipText", "Runs naming convention validation on these assets." ),
        FSlateIcon(),
        FUIAction( FExecuteAction::CreateStatic( LOCAL_ValidateAssets, selected_assets ) ) );
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