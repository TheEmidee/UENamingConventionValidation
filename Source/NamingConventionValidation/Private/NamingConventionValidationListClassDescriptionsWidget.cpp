#include "NamingConventionValidationListClassDescriptionsWidget.h"

#include "NamingConventionValidationAddClassDescriptionWidget.h"
#include "NamingConventionValidationSettings.h"

#define LOCTEXT_NAMESPACE "NamingConventionValidationClassDescriptionWidget"

namespace SNamingConventionValidationClassDescriptionWidgetDefs {
const FName ColumnID_Checkbox("Checkbox");
const FName ColumnID_ClassPath("ClassPath");
const FName ColumnID_Prefix("Prefix");
const FName ColumnID_Suffix("Suffix");
const FName ColumnID_Priority("Priority");
const FName ColumnID_Edit("Edit");
}

namespace {
void ShowAddDescriptionWidget(const FNamingConventionValidationClassDescription& ClassDescription, TFunction<void(const FNamingConventionValidationClassDescription&)> OnClassDescriptionAdded)
{
	const TSharedRef<SWindow> Window = SNew(SWindow)
	                                       .Title(LOCTEXT("AddBewDescription", "Add n  ew class description"))
	                                       .SizingRule(ESizingRule::Autosized)
	                                       .SupportsMaximize(false)
	                                       .SupportsMinimize(false);

	Window->SetContent(SNew(SBox)
	        .MinDesiredWidth(320.0f)
	            [SNew(SNamingConventionValidationAddClassDescriptionWidget)
	                    .ClassDescription(ClassDescription)
	                    .OwningWindow(Window)
	                    .OnClassDescriptionValidated_Lambda([OnClassDescriptionAdded](const FNamingConventionValidationClassDescription& ClassDescription) {
		                    OnClassDescriptionAdded(ClassDescription);
	                    })]);

	GEditor->EditorAddModalWindow(Window);
}
}

/**
 * Represents a class description item that is displayed as a checkbox inside the dialog
 */
class FClassDescriptionItem : public TSharedFromThis<FClassDescriptionItem>
{
public:
	FClassDescriptionItem(const FNamingConventionValidationClassDescription& InClassDescription)
	    : ClassDescription(InClassDescription)
	    , CheckState(ECheckBoxState::Checked)
	{
	}

	FNamingConventionValidationClassDescription ClassDescription;
	ECheckBoxState CheckState;
};

class SClassDescriptionListRow : public SMultiColumnTableRow<TSharedPtr<FClassDescriptionItem>>
{

public:
	DECLARE_DELEGATE(FOnClassDescriptionChanged)

	SLATE_BEGIN_ARGS(SClassDescriptionListRow) {}
	SLATE_ARGUMENT(TSharedPtr<FClassDescriptionItem>, Item)
	SLATE_ARGUMENT(TSharedPtr<SListView<TSharedPtr<FClassDescriptionItem>>>, List)
	SLATE_EVENT(FOnClassDescriptionChanged, OnClassDescriptionChanged)

	SLATE_END_ARGS()

	/** Construct function for this widget */
	void Construct(const FArguments& InArgs, const TSharedRef<STableViewBase>& InOwnerTableView)
	{
		Item = InArgs._Item;
		check(Item.IsValid());

		List = InArgs._List;
		OnClassDescriptionChanged = InArgs._OnClassDescriptionChanged;

		SMultiColumnTableRow<TSharedPtr<FClassDescriptionItem>>::Construct(FSuperRowType::FArguments(), InOwnerTableView);
	}

	/** Overridden from SMultiColumnTableRow.  Generates a widget for this column of the list row. */
	virtual TSharedRef<SWidget> GenerateWidgetForColumn(const FName& ColumnName) override
	{
		check(Item.IsValid());

		TSharedPtr<SWidget> ItemContentWidget;
		const FMargin RowPadding(3, 3, 3, 3);

		if (ColumnName == SNamingConventionValidationClassDescriptionWidgetDefs::ColumnID_Checkbox)
		{
			ItemContentWidget = SNew(SHorizontalBox) +
			    SHorizontalBox::Slot()
			        .Padding(FMargin(10, 3, 6, 3))
			            [SNew(SCheckBox)
			                    .IsChecked(this, &SClassDescriptionListRow::OnGetDisplayCheckState)
			                    .OnCheckStateChanged(this, &SClassDescriptionListRow::OnDisplayCheckStateChanged)];
		}
		else if (ColumnName == SNamingConventionValidationClassDescriptionWidgetDefs::ColumnID_ClassPath)
		{
			ItemContentWidget = SNew(SHorizontalBox) +
			    SHorizontalBox::Slot()
			        .Padding(RowPadding)
			            [SNew(STextBlock)
			                    .Text(FText::FromString(Item->ClassDescription.ClassPath.ToString()))];
		}
		else if (ColumnName == SNamingConventionValidationClassDescriptionWidgetDefs::ColumnID_Prefix)
		{
			ItemContentWidget = SNew(SHorizontalBox) +
			    SHorizontalBox::Slot()
			        .Padding(RowPadding)
			            [SNew(STextBlock)
			                    .Text(FText::FromString(Item->ClassDescription.Prefix))];
		}
		else if (ColumnName == SNamingConventionValidationClassDescriptionWidgetDefs::ColumnID_Suffix)
		{
			ItemContentWidget = SNew(SHorizontalBox) +
			    SHorizontalBox::Slot()
			        .Padding(RowPadding)
			            [SNew(STextBlock)
			                    .Text(FText::FromString(Item->ClassDescription.Suffix))];
		}
		else if (ColumnName == SNamingConventionValidationClassDescriptionWidgetDefs::ColumnID_Priority)
		{
			ItemContentWidget = SNew(SHorizontalBox) +
			    SHorizontalBox::Slot()
			        .Padding(RowPadding)
			            [SNew(STextBlock)
			                    .Text(FText::FromString(FString::FromInt(Item->ClassDescription.Priority)))];
		}
		else if (ColumnName == SNamingConventionValidationClassDescriptionWidgetDefs::ColumnID_Edit)
		{
			ItemContentWidget = SNew(SHorizontalBox) +
			    SHorizontalBox::Slot()
			        .Padding(RowPadding)
			            [SNew(SButton)
			                    .Text(LOCTEXT("NamingConventionValidationEditDescription", "Edit"))
			                    .HAlign(HAlign_Center)
			                    .OnClicked_Lambda([this]() {
				                    ShowAddDescriptionWidget(Item->ClassDescription, [this](const FNamingConventionValidationClassDescription& ClassDescription) {
					                    if (auto* Settings = GetMutableDefault<UNamingConventionValidationSettings>())
					                    {
						                    if (auto* ExistingClassDescription = Settings->ClassDescriptions.FindByPredicate([ClassDescription](const FNamingConventionValidationClassDescription& ExistingClassDescription) {
							                        return ExistingClassDescription.ClassPath == ClassDescription.ClassPath;
						                        }))
						                    {
							                    *ExistingClassDescription = ClassDescription;
							                    OnClassDescriptionChanged.ExecuteIfBound();
						                    }
					                    }
				                    });
				                    return FReply::Handled();
			                    })];
		}

		return ItemContentWidget.ToSharedRef();
	}

	ECheckBoxState OnGetDisplayCheckState() const
	{
		return Item->CheckState;
	}

	void OnDisplayCheckStateChanged(ECheckBoxState InNewState)
	{
		Item->CheckState = InNewState;

		TSharedPtr<SListView<TSharedPtr<FClassDescriptionItem>>> UnusedTagsListView = List.Pin();
		if (UnusedTagsListView.IsValid())
		{
			TArray<TSharedPtr<FClassDescriptionItem>> SelectedItems = UnusedTagsListView->GetSelectedItems();
			if (SelectedItems.Contains(Item))
			{
				for (const TSharedPtr<FClassDescriptionItem>& SelectedItem : SelectedItems)
				{
					SelectedItem->CheckState = InNewState;
				}
			}
		}
	}

private:
	TSharedPtr<FClassDescriptionItem> Item;
	TWeakPtr<SListView<TSharedPtr<FClassDescriptionItem>>> List;
	FOnClassDescriptionChanged OnClassDescriptionChanged;
};

void SNamingConventionValidationListClassDescriptionsWidget::Construct(const FArguments& InArgs)
{
	TSharedRef<SHeaderRow> HeaderRowWidget = SNew(SHeaderRow);

	HeaderRowWidget->AddColumn(
	    SHeaderRow::Column(SNamingConventionValidationClassDescriptionWidgetDefs::ColumnID_Checkbox)
	        [SNew(SBox)
	                .Padding(FMargin(6, 3, 6, 3))
	                .HAlign(HAlign_Center)
	                    [SNew(SCheckBox)
	                            .IsChecked(this, &SNamingConventionValidationListClassDescriptionsWidget::GetToggleSelectedState)
	                            .OnCheckStateChanged(this, &SNamingConventionValidationListClassDescriptionsWidget::OnToggleSelectedCheckBox)]]
	            .FixedWidth(38.0f));

	HeaderRowWidget->AddColumn(
	    SHeaderRow::Column(SNamingConventionValidationClassDescriptionWidgetDefs::ColumnID_ClassPath)
	        .DefaultLabel(LOCTEXT("NamingConventionValidationColumnLabel", "ClassPath"))
	        .SortMode(this, &SNamingConventionValidationListClassDescriptionsWidget::GetColumnSortMode, SNamingConventionValidationClassDescriptionWidgetDefs::ColumnID_ClassPath)
	        .OnSort(this, &SNamingConventionValidationListClassDescriptionsWidget::OnColumnSortModeChanged)
	        .FillWidth(1.0f));

	HeaderRowWidget->AddColumn(
	    SHeaderRow::Column(SNamingConventionValidationClassDescriptionWidgetDefs::ColumnID_Prefix)
	        .DefaultLabel(LOCTEXT("NamingConventionValidationColumnLabel", "Prefix"))
	        .SortMode(this, &SNamingConventionValidationListClassDescriptionsWidget::GetColumnSortMode, SNamingConventionValidationClassDescriptionWidgetDefs::ColumnID_Prefix)
	        .OnSort(this, &SNamingConventionValidationListClassDescriptionsWidget::OnColumnSortModeChanged)
	        .FixedWidth(80.0f));

	HeaderRowWidget->AddColumn(
	    SHeaderRow::Column(SNamingConventionValidationClassDescriptionWidgetDefs::ColumnID_Suffix)
	        .DefaultLabel(LOCTEXT("NamingConventionValidationColumnLabel", "Suffix"))
	        .SortMode(this, &SNamingConventionValidationListClassDescriptionsWidget::GetColumnSortMode, SNamingConventionValidationClassDescriptionWidgetDefs::ColumnID_Suffix)
	        .OnSort(this, &SNamingConventionValidationListClassDescriptionsWidget::OnColumnSortModeChanged)
	        .FixedWidth(80.0f));

	HeaderRowWidget->AddColumn(
	    SHeaderRow::Column(SNamingConventionValidationClassDescriptionWidgetDefs::ColumnID_Priority)
	        .DefaultLabel(LOCTEXT("NamingConventionValidationColumnLabel", "Priority"))
	        .SortMode(this, &SNamingConventionValidationListClassDescriptionsWidget::GetColumnSortMode, SNamingConventionValidationClassDescriptionWidgetDefs::ColumnID_Priority)
	        .OnSort(this, &SNamingConventionValidationListClassDescriptionsWidget::OnColumnSortModeChanged)
	        .FixedWidth(80.0f));

	HeaderRowWidget->AddColumn(
	    SHeaderRow::Column(SNamingConventionValidationClassDescriptionWidgetDefs::ColumnID_Edit)
	        .DefaultLabel(LOCTEXT("NamingConventionValidationColumnLabel", "Edit"))
	        .FixedWidth(80.0f));

	ChildSlot
	    [SNew(SOverlay) +
	        SOverlay::Slot()[SNew(SVerticalBox) +
	            SVerticalBox::Slot()
	                [SAssignNew(ClassDescriptionsListView, SListView<TSharedPtr<FClassDescriptionItem>>)
	                        .ListItemsSource(&ClassDescriptionsItems)
	                        .OnGenerateRow(this, &SNamingConventionValidationListClassDescriptionsWidget::MakeClassDescriptionListItemWidget)
	                        .HeaderRow(HeaderRowWidget)
	                        .SelectionMode(ESelectionMode::Multi)] +
	            SVerticalBox::Slot().Padding(15).AutoHeight().HAlign(HAlign_Right)
	                [SNew(SHorizontalBox) +
	                    SHorizontalBox::Slot()
	                        .AutoWidth()
	                            [SNew(SButton)
	                                    .Text(LOCTEXT("AddDefaultsButton", "Add Defaults"))
	                                    .OnClicked(this, &SNamingConventionValidationListClassDescriptionsWidget::OnAddDefaultsPressed)] +
	                    SHorizontalBox::Slot()
	                        .AutoWidth()
	                            [SNew(SButton)
	                                    .Text(LOCTEXT("AddNewButton", "Add New"))
	                                    .OnClicked(this, &SNamingConventionValidationListClassDescriptionsWidget::OnAddNewDescription)] +
	                    SHorizontalBox::Slot()
	                        .AutoWidth()
	                            [SNew(SButton)
	                                    .Text(LOCTEXT("RemoveButton", "Remove Selected Classes"))
	                                    .OnClicked(this, &SNamingConventionValidationListClassDescriptionsWidget::OnRemovePressed)
	                                    .IsEnabled(this, &SNamingConventionValidationListClassDescriptionsWidget::IsRemoveEnabled)]]]];

	PopulateClassDescriptions();
}

EColumnSortMode::Type SNamingConventionValidationListClassDescriptionsWidget::GetColumnSortMode(const FName ColumnId) const
{
	if (SortByColumn != ColumnId)
	{
		return EColumnSortMode::None;
	}

	return SortMode;
}

void SNamingConventionValidationListClassDescriptionsWidget::OnColumnSortModeChanged(const EColumnSortPriority::Type SortPriority, const FName& ColumnId, const EColumnSortMode::Type InSortMode)
{
}

ECheckBoxState SNamingConventionValidationListClassDescriptionsWidget::GetToggleSelectedState() const
{
	if (ClassDescriptionsItems.Num() == 0)
	{
		return ECheckBoxState::Checked;
	}

	ECheckBoxState CommonCheckState = ClassDescriptionsItems[0]->CheckState;

	for (int32 ItemIdx = 1; ItemIdx < ClassDescriptionsItems.Num(); ++ItemIdx)
	{
		if (ClassDescriptionsItems[ItemIdx]->CheckState != CommonCheckState)
		{
			CommonCheckState = ECheckBoxState::Undetermined;
			break;
		}
	}

	return CommonCheckState;
}

void SNamingConventionValidationListClassDescriptionsWidget::OnToggleSelectedCheckBox(ECheckBoxState InNewState)
{
	for (const auto& Item : ClassDescriptionsItems)
	{
		Item->CheckState = InNewState;
	}

	ClassDescriptionsListView->RequestListRefresh();
}

TSharedRef<ITableRow> SNamingConventionValidationListClassDescriptionsWidget::MakeClassDescriptionListItemWidget(TSharedPtr<FClassDescriptionItem> Item, const TSharedRef<STableViewBase>& OwnerTable)
{
	return SNew(SClassDescriptionListRow, OwnerTable)
	    .Item(Item)
	    .List(ClassDescriptionsListView)
	    .OnClassDescriptionChanged_Lambda([this]() {
		    if (auto* Settings = GetMutableDefault<UNamingConventionValidationSettings>())
		    {
			    Settings->TryUpdateDefaultConfigFile();
			    PopulateClassDescriptions();
		    }
	    });
}

bool SNamingConventionValidationListClassDescriptionsWidget::IsRemoveEnabled() const
{
	return ClassDescriptionsItems.FindByPredicate([](const auto& Item) {
		return Item->CheckState == ECheckBoxState::Checked;
	}) != nullptr;
}

FReply SNamingConventionValidationListClassDescriptionsWidget::OnRemovePressed()
{
	if (auto* Settings = GetMutableDefault<UNamingConventionValidationSettings>())
	{
		const int PreviousCount = Settings->ClassDescriptions.Num();
		for (const auto& Item : ClassDescriptionsItems)
		{
			if (Item->CheckState == ECheckBoxState::Checked)
			{
				Settings->ClassDescriptions.RemoveAll([Item](const FNamingConventionValidationClassDescription& ClassDescription) {
					return ClassDescription.ClassPath == Item->ClassDescription.ClassPath;
				});
			}
		}
		const int NewCount = Settings->ClassDescriptions.Num();

		PopulateClassDescriptions();

		Settings->TryUpdateDefaultConfigFile();

		FMessageDialog::Open(
		    EAppMsgType::Ok,
		    FText::Format(LOCTEXT("ClassDescriptionsRemoved_Text", "{0} class descriptions were removed in total."),
		        FText::AsNumber(PreviousCount - NewCount)),
		    LOCTEXT("ClassDescriptionsRemoved_Title", "Class Descriptions Removal Complete"));
	}

	return FReply::Handled();
}

FReply SNamingConventionValidationListClassDescriptionsWidget::OnAddDefaultsPressed()
{
	if (FMessageDialog::Open(EAppMsgType::YesNo, LOCTEXT("AddDefaults", "Do you want to replace all the class descriptions with default ones?"), LOCTEXT("AddDefaults", "Confirmation")) == EAppReturnType::Yes)
	{
		if (auto* Settings = GetMutableDefault<UNamingConventionValidationSettings>())
		{
			Settings->ClassDescriptions.Reset();
			Settings->ClassDescriptions.Emplace(FSoftClassPath(TEXT("/Script/Engine.AnimInstance")), TEXT("ABP_"), TEXT(""), 0);
			Settings->TryUpdateDefaultConfigFile();
		}
		PopulateClassDescriptions();
	}
	return FReply::Handled();
}

FReply SNamingConventionValidationListClassDescriptionsWidget::OnAddNewDescription()
{
	ShowAddDescriptionWidget({}, [this](const FNamingConventionValidationClassDescription& ClassDescription) {
		if (auto* Settings = GetMutableDefault<UNamingConventionValidationSettings>())
		{
			if (Settings->ClassDescriptions.FindByPredicate([ClassDescription](const FNamingConventionValidationClassDescription& ExistingClassDescription) {
				    return ExistingClassDescription.ClassPath == ClassDescription.ClassPath;
			    }) == nullptr)
			{
				Settings->ClassDescriptions.Emplace(ClassDescription);
				Settings->TryUpdateDefaultConfigFile();
				PopulateClassDescriptions();
			}
		}
	});
	return FReply::Handled();
}

void SNamingConventionValidationListClassDescriptionsWidget::PopulateClassDescriptions()
{
	if (auto* Settings = GetDefault<UNamingConventionValidationSettings>())
	{
		ClassDescriptionsItems.Empty(Settings->ClassDescriptions.Num());
		for (const auto& ClassDescription : Settings->ClassDescriptions)
		{
			ClassDescriptionsItems.Emplace(MakeShared<FClassDescriptionItem>(ClassDescription));
		}
	}

	ClassDescriptionsListView->RequestListRefresh();
}

#undef LOCTEXT_NAMESPACE