#include "NamingValidatorFromFolder.h"

#include "EditorNamingValidatorSubsystem.h"

UNamingValidatorFromFolder::UNamingValidatorFromFolder() :
    bValidateAssetsAreInSameFolder( false ),
    bCheckForRegularAssetNamingValidation( true )
{
    IgnoredFolders = { "Shared", "Unused" };
}

bool UNamingValidatorFromFolder::CanValidateAssetNaming_Implementation( const UClass * asset_class, const FAssetData & asset_data ) const
{
    const auto package_path = asset_data.PackagePath.ToString();

    if ( !package_path.StartsWith( ParentFolderName ) )
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

    for ( const auto * ignored_class : IgnoredClasses ) 
    {
        if ( asset_class->IsChildOf( ignored_class ) ) 
        {
            return false;
        }
    }
    

    return true;

}

ENamingConventionValidationResult UNamingValidatorFromFolder::ValidateAssetNaming_Implementation( FText & error_message, const UClass * asset_class, const FAssetData & asset_data ) 
{
    const auto package_path = asset_data.PackagePath.ToString();
    const auto remaining_path = package_path.RightChop( ParentFolderName.Len() );

    TArray< FString > parts;
    const auto * delimiter = TEXT( "/" );
    remaining_path.ParseIntoArray( parts, delimiter );

    if ( bValidateAssetsAreInSameFolder && parts.Num() != 1 )
    {
        error_message = FText::FromString( FString::Printf( TEXT( "Assets in the folder %s must all be in the same subfolder" ), *ParentFolderName ) );
        return ENamingConventionValidationResult::Invalid;
    }

    const auto asset_name = asset_data.AssetName.ToString();

    TArray< FString > filename_parts;
    const auto * filename_delimiter = TEXT( "_" );

    if ( asset_name.ParseIntoArray( filename_parts, filename_delimiter ) == 0 || filename_parts.Num() < 2 )
    {
        error_message = FText::FromString( FString::Printf( TEXT( "Impossible to parse the filename. Asset name must conform to something like BP_XXX" ) ) );
        return ENamingConventionValidationResult::Invalid;
    }

    if ( !IdentifierToken.IsEmpty() )
    {
        // If the identifier token contains underscores, we must rework the filename_parts to regroup the tokens to form the identifier
        TArray< FString > identifier_token_parts;
        IdentifierToken.ParseIntoArray( identifier_token_parts, TEXT( "_") );
        const auto token_count = identifier_token_parts.Num();

        if ( token_count > 1 ) 
        {
            for ( auto filename_token_index = 0; filename_token_index < filename_parts.Num(); ++filename_token_index ) 
            {
                const auto token = filename_parts[ filename_token_index ];
                if ( token == identifier_token_parts[ 0 ] ) 
                {
                    auto are_all_tokens_present = true;

                    for ( auto identifier_token_index = 1; identifier_token_index < identifier_token_parts.Num(); ++identifier_token_index ) 
                    {
                        if ( filename_parts[ filename_token_index + identifier_token_index ] != identifier_token_parts[ identifier_token_index ] ) 
                        {
                            are_all_tokens_present = false;
                            break;
                        }
                    }

                    if ( are_all_tokens_present ) 
                    {
                        filename_parts[ filename_token_index ] = IdentifierToken;
                        filename_parts.RemoveAt( filename_token_index + 1, identifier_token_parts.Num() - 1 );
                    }
                }
            }
        }

        if ( filename_parts[ 1 ] != IdentifierToken ) 
        {
            error_message = FText::FromString( FString::Printf( TEXT( "The name of the asset must start with %s_%s" ), *filename_parts[ 0 ], *IdentifierToken ) );
            return ENamingConventionValidationResult::Invalid;
        }
    }

    if ( !bCheckForRegularAssetNamingValidation )
    {
        return ENamingConventionValidationResult::Valid;
    }

    return GEditor->GetEditorSubsystem< UEditorNamingValidatorSubsystem >()->IsAssetNamedCorrectly( error_message, asset_data, false );

}
