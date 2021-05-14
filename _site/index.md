This plug-in allows you to make sure all assets of your project are correcly named, based on your own rules. You can define a prefix and / or a suffix based on the asset type.

### Settings

You can access the settings from the Project Settings window of the editor.

![Settings Menu]({{ 'assets/img/SettingsMenu.png' | relative_url }})

From there, you have access to the general settings of the plugin

![PlugIn Settings]({{ 'assets/img/Settings1.png' | relative_url }})

and you can define the naming convention of your assets based on their class:

![Class naming convention]({{ 'assets/img/Settings2.png' | relative_url }})

The settings are saved in the `DefaultEditor.ini` config file of your project.

### Validation in the editor

The plug-in adds a new entry to the contextual menu which is opened when you right click on an asset or a folder in the content browser. 

![Contextual Menu]({{ 'assets/img/ContextualMenu.png' | relative_url }})

It then fills the message log with all the errors it could find.

![Message Log]({{ 'assets/img/MessageLog.png' | relative_url }})

The validation is also executed automatically whenever you create a new asset, or save an existing one (if you enable the option in the settings.)

### Blueprint validators

By creating a blueprint which inherits from `UEditorNamingValidatorBase` you can easily add custom validation. Also, while the plug-in allows you to validate assets individually based on their type, using a class gives you more power. For example, you can make sure that all assets within a given folder have that folder name in their name, whatever the asset type. This allows you to make sure that for example all the assets in a Weapons folder all have the weapon identifier in their name. You could have for example `Weapons/MachineGun/BP_MachineGun` and `Weapons/MachineGun/M_MachineGun` be accepted, but not `Weapons/MachineGun/A_Weapon_Fire`.

Here's such a validator:

```
#pragma once

#include "NamingValidatorBase.h"

#include <CoreMinimal.h>

#include "NamingValidatorInFolder.generated.h"

UCLASS( Abstract, Blueprintable, NotPlaceable )
class OURGAMEEDITOR_API UNamingValidatorInFolder : public UNamingValidatorBase
{
    GENERATED_BODY()

public:
    UNamingValidatorInFolder();

    bool CanValidateAssetNaming_Implementation( const UClass * asset_class, const FAssetData & asset_data ) const override;
    ENamingConventionValidationResult ValidateAssetNaming_Implementation( FText & error_message, const UClass * asset_class, const FAssetData & asset_data ) override;

protected:
    UPROPERTY( EditDefaultsOnly )
    FString ParentFolderName;

    UPROPERTY( EditDefaultsOnly )
    FString AssetTypeName;

    UPROPERTY( EditDefaultsOnly )
    uint8 ItAllowsSubFolders : 1;

    UPROPERTY( EditDefaultsOnly )
    uint8 ItMustCheckForParentFolderNameInAssetName : 1;

    UPROPERTY( EditDefaultsOnly )
    uint8 ItMustCheckForRegularAssetNamingValidation : 1;

    UPROPERTY( EditDefaultsOnly )
    TArray< FString > IgnoredFolders;

private:
    FString GetContentParentFolderPath() const;
};
```

```
#include "NamingValidatorInFolder.h"

#include "EditorNamingValidatorSubsystem.h"

#include <Editor.h>

UNamingValidatorInFolder::UNamingValidatorInFolder()
{
    ItAllowsSubFolders = false;
    ItMustCheckForParentFolderNameInAssetName = true;
    ItMustCheckForRegularAssetNamingValidation = true;
    IgnoredFolders = { "Shared", "Unused" };
}

bool UNamingValidatorInFolder::CanValidateAssetNaming_Implementation( const UClass * /* asset_class */, const FAssetData & asset_data ) const
{
    const auto package_path = asset_data.PackagePath.ToString();

    if ( !package_path.StartsWith( GetContentParentFolderPath() ) )
    {
        return false;
    }

    for ( const auto & ignored_folder : IgnoredFolders )
    {
        if ( package_path.Contains( ignored_folder ) )
        {
            return false;
        }
    }

    return true;
}

ENamingConventionValidationResult UNamingValidatorInFolder::ValidateAssetNaming_Implementation( FText & error_message, const UClass * /* asset_class */, const FAssetData & asset_data )
{
    const auto package_path = asset_data.PackagePath.ToString();
    const auto remaining_path = package_path.RightChop( GetContentParentFolderPath().Len() );

    TArray< FString > parts;
    const auto * delimiter = TEXT( "/" );
    if ( remaining_path.ParseIntoArray( parts, delimiter ) == 0 )
    {
        error_message = FText::FromString( FString::Printf( TEXT( "Impossible to parse the path" ) ) );
        return ENamingConventionValidationResult::Invalid;
    }

    if ( !ItAllowsSubFolders && parts.Num() != 1 )
    {
        error_message = FText::FromString( FString::Printf( TEXT( "Assets in the folder %s must all be in the same subfolder" ), *ParentFolderName ) );
        return ENamingConventionValidationResult::Invalid;
    }

    const auto child_folder_name = parts[ 0 ];
    const auto child_file_name = asset_data.AssetName.ToString();

    TArray< FString > filename_parts;
    const auto * filename_delimiter = TEXT( "_" );

    if ( child_file_name.ParseIntoArray( filename_parts, filename_delimiter ) == 0 || filename_parts.Num() < 2 )
    {
        error_message = FText::FromString( FString::Printf( TEXT( "Impossible to parse the filename. Asset name must conform to something like BP_XXX" ) ) );
        return ENamingConventionValidationResult::Invalid;
    }

    if ( ItMustCheckForParentFolderNameInAssetName )
    {
        if ( filename_parts[ 1 ] != AssetTypeName )
        {
            error_message = FText::FromString( FString::Printf( TEXT( "Assets in the folder %s must have the %s name right after the asset type prefix" ), *ParentFolderName, *AssetTypeName ) );
            return ENamingConventionValidationResult::Invalid;
        }

        if ( filename_parts[ 2 ] != child_folder_name )
        {
            error_message = FText::FromString( FString::Printf( TEXT( "Assets in the folder %s must have their name after the asset type prefix and the asset category (Ex: BP_Enemy_Walker)" ), *ParentFolderName ) );
            return ENamingConventionValidationResult::Invalid;
        }
    }
    else if ( filename_parts[ 1 ] != child_folder_name )
    {
        error_message = FText::FromString( FString::Printf( TEXT( "Assets in the folder %s must have their name right after the asset type prefix (Ex: BP_RocketLauncher)" ), *ParentFolderName ) );
        return ENamingConventionValidationResult::Invalid;
    }

    if ( !ItMustCheckForRegularAssetNamingValidation )
    {
        return ENamingConventionValidationResult::Valid;
    }

    return GEditor->GetEditorSubsystem< UEditorNamingValidatorSubsystem >()->IsAssetNamedCorrectly( error_message, asset_data, false );
}

FString UNamingValidatorInFolder::GetContentParentFolderPath() const
{
    return FString::Printf( TEXT( "/Game/OurGame/%s" ), *ParentFolderName );
}
```

You can then create blueprints from that class. They will be registered automatically by the plugin when the editor starts, and will be used when an asset is validated:

![Naming validation of weapons]({{ 'assets/img/BP_NamingValidator_Weapons.png' | relative_url }})
![Naming validation of enemies]({{ 'assets/img/BP_NamingValidator_Enemies.png' | relative_url }})

### Commandlet

The plug-in comes with a commandlet which allows you to validate the naming convention outside of the editor. It's most useful in a continuous integration environment, where you would want to validate the data before it's merged into the main branch.

Here's an example of calling the commandlet:

`%UE4PATH%\Binaries\Win64\UE4Editor-Cmd.exe <Your Project Name> -run=NamingConventionValidation -log -log=NamingConventionValidation.log -unattended -nopause -nullrhi`

You can then parse the log file to list all the errors the commandlet found.