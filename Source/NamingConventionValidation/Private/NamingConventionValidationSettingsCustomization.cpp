#include "NamingConventionValidationSettingsCustomization.h"

#include "DetailCategoryBuilder.h"
#include "DetailLayoutBuilder.h"
#include "DetailWidgetRow.h"
#include "NamingConventionValidationListClassDescriptionsWidget.h"
#include "NamingConventionValidationSettings.h"

#define LOCTEXT_NAMESPACE "FGameplayTagsSettingsCustomization"

FNamingConventionValidationSettingsCustomization::FNamingConventionValidationSettingsCustomization()
{
}

FNamingConventionValidationSettingsCustomization::~FNamingConventionValidationSettingsCustomization()
{
}

TSharedRef<IDetailCustomization> FNamingConventionValidationSettingsCustomization::MakeInstance()
{
	return MakeShareable(new FNamingConventionValidationSettingsCustomization());
}

void FNamingConventionValidationSettingsCustomization::CustomizeDetails(IDetailLayoutBuilder& DetailLayout)
{
	IDetailCategoryBuilder& Category = DetailLayout.EditCategory("Naming Convention Validation");

	TArray<TSharedRef<IPropertyHandle>> Properties;
	Category.GetDefaultProperties(Properties, true, true);

	TSharedPtr<IPropertyHandle> OpenClassDescriptionProperty = DetailLayout.GetProperty(GET_MEMBER_NAME_CHECKED(UNamingConventionValidationSettings, OpenClassDescription));
	OpenClassDescriptionProperty->MarkHiddenByCustomization();

	for (TSharedPtr<IPropertyHandle> Property : Properties)
	{
		if (Property->GetProperty() == OpenClassDescriptionProperty->GetProperty())
		{
			Category.AddCustomRow(OpenClassDescriptionProperty->GetPropertyDisplayName(), /*bForAdvanced*/ false)
			    .NameContent()
			        [OpenClassDescriptionProperty->CreatePropertyNameWidget()]
			    .ValueContent()
			        [SNew(SButton)
			                .VAlign(VAlign_Center)
			                .HAlign(HAlign_Center)
			                .OnClicked_Lambda([this]() {
				                const TSharedRef<SWindow> Window = SNew(SWindow)
				                                                       .Title(LOCTEXT("ManageNamingConvention", "Manage Naming Convention"))
				                                                       .SizingRule(ESizingRule::UserSized)
				                                                       .MinWidth(800.0f)
				                                                       .MinHeight(500.0f)
				                                                       .SupportsMaximize(false)
				                                                       .SupportsMinimize(false)
				                                                       .Content()
				                                                           [SNew(SBox)
				                                                                   .MinDesiredWidth(320.0f)
				                                                                       [SNew(SNamingConventionValidationListClassDescriptionsWidget)]];

				                GEditor->EditorAddModalWindow(Window);
				                return FReply::Handled();
			                })
			                    [SNew(SHorizontalBox) +
			                        SHorizontalBox::Slot()
			                            .AutoWidth()
			                            .Padding(FMargin(0, 0, 4, 0))
			                                [SNew(SImage)
			                                        .Image(FAppStyle::GetBrush("Icons.Settings"))
			                                        .ColorAndOpacity(FSlateColor::UseForeground())] +
			                        SHorizontalBox::Slot().AutoWidth()
			                            [SNew(STextBlock)
			                                    .Text(LOCTEXT("ManageClassDescriptions", "Manage naming convention"))]]];
		}
		else
		{
			Category.AddProperty(Property);
		}
	}
}