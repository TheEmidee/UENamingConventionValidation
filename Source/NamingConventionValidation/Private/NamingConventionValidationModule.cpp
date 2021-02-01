#include "NamingConventionValidationModule.h"

#include "EditorNamingValidatorSubsystem.h"
#include "NamingConventionValidationCommandlet.h"

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

#define LOCTEXT_NAMESPACE "NamingConventionValidationModule"

void FindAssetDependencies( const FAssetRegistryModule & asset_registry_module, const FAssetData & asset_data, TSet< FAssetData > & dependent_assets )
{
    if ( asset_data.IsValid() )
    {
        if ( const auto object = asset_data.GetAsset() )
        {
            const auto selected_package_name = object->GetOutermost()->GetFName();
            const auto package_string = selected_package_name.ToString();

            if ( !dependent_assets.Contains( asset_data ) )
            {
                dependent_assets.Add( asset_data );

                TArray< FName > dependencies;
                asset_registry_module.Get().GetDependencies( selected_package_name, dependencies, UE::AssetRegistry::EDependencyCategory::Package );

                for ( const auto dependency : dependencies )
                {
                    const auto dependency_package_string = dependency.ToString();
                    auto dependency_object_string = FString::Printf( TEXT( "%s.%s" ), *dependency_package_string, *FPackageName::GetLongPackageAssetName( dependency_package_string ) );

                    // recurse on each dependency
                    const FName object_path( *dependency_object_string );
                    auto dependent_asset = asset_registry_module.Get().GetAssetByObjectPath( object_path );

                    FindAssetDependencies( asset_registry_module, dependent_asset, dependent_assets );
                }
            }
        }
    }
}

void OnPackageSaved( const FString & /*package_file_name*/, UObject * object )
{
    if ( auto * editor_validation_subsystem = GEditor->GetEditorSubsystem< UEditorNamingValidatorSubsystem >() )
    {
         editor_validation_subsystem->ValidateSavedPackage( object->GetFName() );
    }
}

void ValidateAssets( const TArray< FAssetData > selected_assets )
{
    if ( auto * editor_validation_subsystem = GEditor->GetEditorSubsystem< UEditorNamingValidatorSubsystem >() )
    {
         editor_validation_subsystem->ValidateAssets( selected_assets );
    }
}

void ValidateFolders( const TArray< FString > selected_folders )
{
    auto & asset_registry_module = FModuleManager::Get().LoadModuleChecked< FAssetRegistryModule >( TEXT( "AssetRegistry" ) );

    FARFilter filter;
    filter.bRecursivePaths = true;

    for ( const auto & folder : selected_folders )
    {
        filter.PackagePaths.Emplace( *folder );
    }

    TArray< FAssetData > asset_list;
    asset_registry_module.Get().GetAssets( filter, asset_list );

    ValidateAssets( asset_list );
}

void CreateDataValidationContentBrowserAssetMenu( FMenuBuilder & menu_builder, const TArray< FAssetData > selected_assets )
{
    menu_builder.AddMenuSeparator();
    menu_builder.AddMenuEntry(
        LOCTEXT( "NamingConventionValidateAssetsTabTitle", "Validate Assets Naming Convention" ),
        LOCTEXT( "NamingConventionValidateAssetsTooltipText", "Run naming convention validation on these assets." ),
        FSlateIcon(),
        FUIAction( FExecuteAction::CreateStatic( ValidateAssets, selected_assets ) ) );
}

TSharedRef< FExtender > OnExtendContentBrowserAssetSelectionMenu( const TArray< FAssetData > & selected_assets )
{
    TSharedRef< FExtender > extender( new FExtender() );

    extender->AddMenuExtension(
        "AssetContextAdvancedActions",
        EExtensionHook::After,
        nullptr,
        FMenuExtensionDelegate::CreateStatic( CreateDataValidationContentBrowserAssetMenu, selected_assets ) );

    return extender;
}

void CreateDataValidationContentBrowserPathMenu( FMenuBuilder & menu_builder, const TArray< FString > selected_paths )
{
    menu_builder.AddMenuEntry(
        LOCTEXT( "NamingConventionValidateAssetsPathTabTitle", "Validate Assets Naming Convention in Folder" ),
        LOCTEXT( "NamingConventionValidateAssetsPathTooltipText", "Runs naming convention validation on the assets in the selected folder." ),
        FSlateIcon(),
        FUIAction( FExecuteAction::CreateStatic( ValidateFolders, selected_paths ) ) );
}

TSharedRef< FExtender > OnExtendContentBrowserPathSelectionMenu( const TArray< FString > & selected_paths )
{
    TSharedRef< FExtender > extender( new FExtender() );

    extender->AddMenuExtension(
        "PathContextBulkOperations",
        EExtensionHook::After,
        nullptr,
        FMenuExtensionDelegate::CreateStatic( CreateDataValidationContentBrowserPathMenu, selected_paths ) );

    return extender;
}

FText MenuValidateDataGetTitle()
{
    auto & asset_registry_module = FModuleManager::LoadModuleChecked< FAssetRegistryModule >( "AssetRegistry" );
    if ( asset_registry_module.Get().IsLoadingAssets() )
    {
        return LOCTEXT( "NamingConventionValidationTitleDA", "Validate Naming Convention [Discovering Assets]" );
    }
    return LOCTEXT( "NamingConventionValidationTitle", "Naming Convention..." );
}

void MenuValidateData()
{
    // make sure the asset registry is finished building
    auto & asset_registry_module = FModuleManager::LoadModuleChecked< FAssetRegistryModule >( "AssetRegistry" );
    if ( asset_registry_module.Get().IsLoadingAssets() )
    {
        FMessageDialog::Open( EAppMsgType::Ok, LOCTEXT( "AssetsStillScanningError", "Cannot run naming convention validation while still discovering assets." ) );
        return;
    }

    const auto success = UNamingConventionValidationCommandlet::ValidateData();

    if ( !success )
    {
        FMessageDialog::Open( EAppMsgType::Ok, LOCTEXT( "NamingConventionValidationError", "An error was encountered during naming convention validation. See the log for details." ) );
    }
}

void NamingConventionValidationMenuCreationDelegate( FMenuBuilder & menu_builder )
{
    menu_builder.BeginSection( "NamingConventionValidation", LOCTEXT( "NamingConventionValidation", "NamingConventionValidation" ) );
    menu_builder.AddMenuEntry(
        TAttribute< FText >::Create( &MenuValidateDataGetTitle ),
        LOCTEXT( "NamingConventionValidationTooltip", "Validates all naming convention in content directory." ),
        FSlateIcon( FEditorStyle::GetStyleSetName(), "DeveloperTools.MenuIcon" ),
        FUIAction( FExecuteAction::CreateStatic( &MenuValidateData ) ) );
    menu_builder.EndSection();
}

class FNamingConventionValidationModule : public INamingConventionValidationModule
{
public:
    void StartupModule() override;
    void ShutdownModule() override;

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
        auto & content_browser_module = FModuleManager::LoadModuleChecked< FContentBrowserModule >( TEXT( "ContentBrowser" ) );
        auto & cb_asset_menu_extender_delegates = content_browser_module.GetAllAssetViewContextMenuExtenders();

        cb_asset_menu_extender_delegates.Add( FContentBrowserMenuExtender_SelectedAssets::CreateStatic( OnExtendContentBrowserAssetSelectionMenu ) );
        ContentBrowserAssetExtenderDelegateHandle = cb_asset_menu_extender_delegates.Last().GetHandle();

        auto & cb_folder_menu_extender_delegates = content_browser_module.GetAllPathViewContextMenuExtenders();

        cb_folder_menu_extender_delegates.Add( FContentBrowserMenuExtender_SelectedPaths::CreateStatic( OnExtendContentBrowserPathSelectionMenu ) );
        ContentBrowserPathExtenderDelegateHandle = cb_folder_menu_extender_delegates.Last().GetHandle();

        MenuExtender = MakeShareable( new FExtender );
        MenuExtender->AddMenuExtension( "FileLoadAndSave", EExtensionHook::After, nullptr, FMenuExtensionDelegate::CreateStatic( NamingConventionValidationMenuCreationDelegate ) );

        auto & level_editor_module = FModuleManager::LoadModuleChecked< FLevelEditorModule >( "LevelEditor" );
        level_editor_module.GetMenuExtensibilityManager()->AddExtender( MenuExtender );

        OnPackageSavedDelegateHandle = UPackage::PackageSavedEvent.AddStatic( OnPackageSaved );
    }
}

void FNamingConventionValidationModule::ShutdownModule()
{
    if ( !IsRunningCommandlet() && !IsRunningGame() && !IsRunningDedicatedServer() )
    {
        if ( auto * content_browser_module = FModuleManager::GetModulePtr< FContentBrowserModule >( TEXT( "ContentBrowser" ) ) )
        {
            auto & content_browser_menu_extender_delegates = content_browser_module->GetAllAssetViewContextMenuExtenders();
            content_browser_menu_extender_delegates.RemoveAll( [ &extender_delegate = ContentBrowserAssetExtenderDelegateHandle ]( const FContentBrowserMenuExtender_SelectedAssets & delegate ) {
                return delegate.GetHandle() == extender_delegate;
            } );
            content_browser_menu_extender_delegates.RemoveAll( [ &extender_delegate = ContentBrowserAssetExtenderDelegateHandle ]( const FContentBrowserMenuExtender_SelectedAssets & delegate ) {
                return delegate.GetHandle() == extender_delegate;
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