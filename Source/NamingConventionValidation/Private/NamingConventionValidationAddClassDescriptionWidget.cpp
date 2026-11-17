#include "NamingConventionValidationAddClassDescriptionWidget.h"

#include "PropertyCustomizationHelpers.h"
#include "Styling/AppStyle.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SEditableTextBox.h"
#include "Widgets/Input/SNumericEntryBox.h"
#include "Widgets/Layout/SGridPanel.h"
#include "Widgets/Layout/SWidgetSwitcher.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/SWindow.h"
#include "Widgets/Text/STextBlock.h"

#define LOCTEXT_NAMESPACE "NamingConventionValidationClassDescriptionWidget"

void SNamingConventionValidationAddClassDescriptionWidget::Construct(const FArguments& InArgs)
{
	ClassDescription = InArgs._ClassDescription;
	bIsAdding = ClassDescription.ClassPath.ToString().IsEmpty();
	OwningWindow = InArgs._OwningWindow;
	OnClassDescriptionValidated = InArgs._OnClassDescriptionValidated;

	TSharedPtr<SWidgetSwitcher> ClassSelectorSwitcher;

	const FSlateFontInfo Font = FAppStyle::GetFontStyle(TEXT("PropertyWindow.NormalFont"));

	// Column 0 is auto-sized: it is as wide as the widest label, plus the right margin of LabelPadding.
	// Column 1 fills the remaining width, and its widgets stretch to fill it.
	const FMargin LabelPadding(2.0f, 2.0f, 8.0f, 2.0f);
	const FMargin ValuePadding(2.0f);

	auto MakeLabel = [&Font](const FText& Text) {
		return SNew(STextBlock)
		    .Font(Font)
		    .Text(Text);
	};

	ChildSlot
	    [SNew(SGridPanel)
	            .FillColumn(1, 1.0f) +
	        SGridPanel::Slot(0, 0)
	            .Padding(LabelPadding)
	            .VAlign(VAlign_Center)
	            .HAlign(HAlign_Left)
	                [MakeLabel(LOCTEXT("ClassPath", "Class Path:"))] +
	        SGridPanel::Slot(1, 0)
	            .Padding(ValuePadding)
	            .VAlign(VAlign_Center)
	            .HAlign(HAlign_Fill)
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
	                    SWidgetSwitcher::Slot()
	                        [SNew(STextBlock)
	                                .Font(Font)
	                                .Text(FText::FromString(ClassDescription.ClassPath.ToString()))]] +

	        SGridPanel::Slot(0, 1)
	            .Padding(LabelPadding)
	            .VAlign(VAlign_Center)
	            .HAlign(HAlign_Left)
	                [MakeLabel(LOCTEXT("Prefix", "Prefix:"))] +
	        SGridPanel::Slot(1, 1)
	            .Padding(ValuePadding)
	            .VAlign(VAlign_Center)
	            .HAlign(HAlign_Fill)
	                [SNew(SEditableTextBox)
	                        .Font(Font)
	                        .Text(FText::FromString(bIsAdding ? "" : ClassDescription.Prefix))
	                        .OnTextChanged_Lambda([this](const FText& Text) {
		                        ClassDescription.Prefix = Text.ToString();
	                        })] +

	        SGridPanel::Slot(0, 2)
	            .Padding(LabelPadding)
	            .VAlign(VAlign_Center)
	            .HAlign(HAlign_Left)
	                [MakeLabel(LOCTEXT("Suffix", "Suffix:"))] +
	        SGridPanel::Slot(1, 2)
	            .Padding(ValuePadding)
	            .VAlign(VAlign_Center)
	            .HAlign(HAlign_Fill)
	                [SNew(SEditableTextBox)
	                        .Font(Font)
	                        .Text(FText::FromString(bIsAdding ? "" : ClassDescription.Suffix))
	                        .OnTextChanged_Lambda([this](const FText& Text) {
		                        ClassDescription.Suffix = Text.ToString();
	                        })] +

	        SGridPanel::Slot(0, 3)
	            .Padding(LabelPadding)
	            .VAlign(VAlign_Center)
	            .HAlign(HAlign_Left)
	                [MakeLabel(LOCTEXT("Priority", "Priority:"))] +
	        SGridPanel::Slot(1, 3)
	            .Padding(ValuePadding)
	            .VAlign(VAlign_Center)
	            .HAlign(HAlign_Fill)
	                [SNew(SNumericEntryBox<int>)
	                        .MinValue(0)
	                        .Value(bIsAdding ? 0 : ClassDescription.Priority)
	                        .OnValueChanged_Lambda([this](int Value) {
		                        ClassDescription.Priority = Value;
	                        })] +

	        SGridPanel::Slot(0, 4)
	            .Padding(ValuePadding)
	            .ColumnSpan(2)
	            .VAlign(VAlign_Center)
	            .HAlign(HAlign_Center)
	                [SNew(SHorizontalBox) +
	                    SHorizontalBox::Slot()
	                        .AutoWidth()
	                        .Padding(2.0f)
	                            [SNew(SButton)
	                                    .Text(bIsAdding ? LOCTEXT("AddButton", "Add") : LOCTEXT("EditButton", "Edit"))
	                                    .OnClicked_Lambda([this]() {
		                                    OnClassDescriptionValidated.ExecuteIfBound(ClassDescription);
		                                    CloseOwningWindow();
		                                    return FReply::Handled();
	                                    })] +
	                    SHorizontalBox::Slot()
	                        .AutoWidth()
	                        .Padding(2.0f)
	                            [SNew(SButton)
	                                    .Text(LOCTEXT("CancelButton", "Cancel"))
	                                    .OnClicked_Lambda([this]() {
		                                    CloseOwningWindow();
		                                    return FReply::Handled();
	                                    })]]];

	ClassSelectorSwitcher->SetActiveWidgetIndex(bIsAdding ? 0 : 1);
}

const UClass* SNamingConventionValidationAddClassDescriptionWidget::OnGetClass() const
{
	return ClassDescription.ClassPath.ResolveClass();
}

void SNamingConventionValidationAddClassDescriptionWidget::CloseOwningWindow() const
{
	if (const TSharedPtr<SWindow> Window = OwningWindow.Pin())
	{
		Window->RequestDestroyWindow();
	}
}

#undef LOCTEXT_NAMESPACE
