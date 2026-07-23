#include "NamingConventionValidationAddClassDescriptionWidget.h"

#include "PropertyCustomizationHelpers.h"
#include "Widgets/Input/SNumericEntryBox.h"
#include "Widgets/Layout/SWidgetSwitcher.h"

#define LOCTEXT_NAMESPACE "NamingConventionValidationClassDescriptionWidget"

void SNamingConventionValidationAddClassDescriptionWidget::Construct(const FArguments& InArgs)
{
	ClassDescription = InArgs._ClassDescription;
	bIsAdding = ClassDescription.ClassPath.ToString().IsEmpty();
	OwningWindow = InArgs._OwningWindow;
	OnClassDescriptionValidated = InArgs._OnClassDescriptionValidated;

	TSharedPtr<SWidgetSwitcher> ClassSelectorSwitcher;

	ChildSlot
	    [SNew(SOverlay) +
	        SOverlay::Slot()
	            [SNew(SGridPanel)
	                    .FillColumn(1, 1.0f) +
	                SGridPanel::Slot(0, 0)
	                    .Padding(2.0f)
	                    .VAlign(VAlign_Center)
	                    .HAlign(HAlign_Center)
	                        [SNew(SBox)
	                                .MinDesiredWidth(50.0f)
	                                    [SNew(STextBlock)
	                                            .Font(FAppStyle::GetFontStyle(TEXT("PropertyWindow.NormalFont")))
	                                            .Text(LOCTEXT("ClassPath", "Class Path:"))]] +
	                SGridPanel::Slot(1, 0)
	                    .Padding(2.0f)
	                    .VAlign(VAlign_Center)
	                    .HAlign(HAlign_Center)
	                        [SNew(SBox)
	                                .MinDesiredWidth(50.0f)
	                                    [SAssignNew(ClassSelectorSwitcher, SWidgetSwitcher) +
	                                        SWidgetSwitcher::Slot()
	                                            [SNew(SClassPropertyEntryBox)
	                                                    .AllowAbstract(true)
	                                                    .AllowNone(false)
	                                                    .MetaClass(UObject::StaticClass())
	                                                    .SelectedClass(this, &SNamingConventionValidationAddClassDescriptionWidget::OnGetClass)
	                                                    .OnSetClass_Lambda([this](const UClass* Class) {
		                                                    ClassDescription.ClassPath = FSoftClassPath(Class ? Class->GetPathName() : "");
	                                                    })] +
	                                        SWidgetSwitcher::Slot()[SNew(STextBlock)
	                                                .Text(FText::FromString(ClassDescription.ClassPath.ToString()))]]] +
	                SGridPanel::Slot(0, 1)
	                    .Padding(2.0f)
	                    .VAlign(VAlign_Center)
	                    .HAlign(HAlign_Center)
	                        [SNew(SBox)
	                                .MinDesiredWidth(50.0f)
	                                    [SNew(STextBlock)
	                                            .Font(FAppStyle::GetFontStyle(TEXT("PropertyWindow.NormalFont")))
	                                            .Text(LOCTEXT("Prefix", "Prefix:"))]] +

	                SGridPanel::Slot(1, 1)
	                    .Padding(2.0f)
	                    .VAlign(VAlign_Center)
	                    .HAlign(HAlign_Center)
	                        [SNew(SBox)
	                                .MinDesiredWidth(50.0f)
	                                    [SNew(SEditableTextBox)
	                                            .Font(FAppStyle::GetFontStyle(TEXT("PropertyWindow.NormalFont")))
	                                            .Text(FText::FromString(bIsAdding ? "" : ClassDescription.Prefix))
	                                            .OnTextChanged_Lambda([&](const FText& Text) {
		                                            ClassDescription.Prefix = Text.ToString();
	                                            })]] +

	                SGridPanel::Slot(0, 2)
	                    .Padding(2.0f)
	                    .VAlign(VAlign_Center)
	                    .HAlign(HAlign_Center)
	                        [SNew(SBox)
	                                .MinDesiredWidth(50.0f)
	                                    [SNew(STextBlock)
	                                            .Font(FAppStyle::GetFontStyle(TEXT("PropertyWindow.NormalFont")))
	                                            .Text(LOCTEXT("Suffix", "Suffix:"))]] +

	                SGridPanel::Slot(1, 2)
	                    .Padding(2.0f)
	                    .VAlign(VAlign_Center)
	                    .HAlign(HAlign_Center)
	                        [SNew(SBox)
	                                .MinDesiredWidth(50.0f)
	                                    [SNew(SEditableTextBox)
	                                            .Font(FAppStyle::GetFontStyle(TEXT("PropertyWindow.NormalFont")))
	                                            .Text(FText::FromString(bIsAdding ? "" : ClassDescription.Suffix))
	                                            .OnTextChanged_Lambda([&](const FText& Text) {
		                                            ClassDescription.Suffix = Text.ToString();
	                                            })]] +

	                SGridPanel::Slot(0, 3)
	                    .Padding(2.0f)
	                    .VAlign(VAlign_Center)
	                    .HAlign(HAlign_Center)
	                        [SNew(SBox)
	                                .MinDesiredWidth(50.0f)
	                                    [SNew(STextBlock)
	                                            .Font(FAppStyle::GetFontStyle(TEXT("PropertyWindow.NormalFont")))
	                                            .Text(LOCTEXT("Priority", "Priority:"))]] +

	                SGridPanel::Slot(1, 3)
	                    .Padding(2.0f)
	                    .VAlign(VAlign_Center)
	                    .HAlign(HAlign_Center)
	                        [SNew(SBox)
	                                .MinDesiredWidth(50.0f)
	                                    [SNew(SNumericEntryBox<int>)
	                                            .MinValue(0)
	                                            .Value(bIsAdding ? 0 : ClassDescription.Priority)
	                                            .OnValueChanged_Lambda([&](int Value) {
		                                            ClassDescription.Priority = Value;
	                                            })]] +

	                SGridPanel::Slot(0, 4)
	                    .Padding(2.0f)
	                    .ColumnSpan(2)
	                    .VAlign(VAlign_Center)
	                    .HAlign(HAlign_Center)
	                        [SNew(SBox)
	                                .MinDesiredWidth(50.0f)
	                                    [SNew(SButton)
	                                            .Text(bIsAdding ? LOCTEXT("AddButton", "Add") : LOCTEXT("EditButton", "Edit"))
	                                            .OnClicked_Lambda([this]() {
		                                            OnClassDescriptionValidated.ExecuteIfBound(ClassDescription);
		                                            OwningWindow.Pin()->RequestDestroyWindow();
		                                            return FReply::Handled();
	                                            })]]]];

	ClassSelectorSwitcher->SetActiveWidgetIndex(bIsAdding ? 0 : 1);
}

const UClass* SNamingConventionValidationAddClassDescriptionWidget::OnGetClass() const
{
	return ClassDescription.ClassPath.ResolveClass();
}

#undef LOCTEXT_NAMESPACE