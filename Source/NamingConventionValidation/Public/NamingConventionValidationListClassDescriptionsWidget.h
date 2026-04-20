#pragma once

#include "Widgets/SCompoundWidget.h"

class FClassDescriptionItem;

class SNamingConventionValidationListClassDescriptionsWidget : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SNamingConventionValidationListClassDescriptionsWidget)
	{}
	SLATE_END_ARGS();

	void Construct(const FArguments& InArgs);

private:
	/** The sort mode for the given column */
	EColumnSortMode::Type GetColumnSortMode(const FName ColumnId) const;

	/** Handler for when a column is clicked to change the sort mode */
	void OnColumnSortModeChanged(const EColumnSortPriority::Type SortPriority, const FName& ColumnId, const EColumnSortMode::Type InSortMode);

	void SortClassDescriptions();

	/** The state of the column header checkbox that toggles the check state of all items */
	ECheckBoxState GetToggleSelectedState() const;

	/** Handler for when column header checkbox that toggles the check state of all items is clicked */
	void OnToggleSelectedCheckBox(ECheckBoxState InNewState);

	/** Creates the widget for a list item */
	TSharedRef<ITableRow> MakeClassDescriptionListItemWidget(TSharedPtr<FClassDescriptionItem> Item, const TSharedRef<STableViewBase>& OwnerTable);

	/** Returns true if the user can click the remove button */
	bool IsRemoveEnabled() const;

	/** Handler for when the user clicks the button to remove the selected class descriptions */
	FReply OnRemovePressed();

	/** Handler for when the user clicks the button to remove the selected class descriptions */
	FReply OnAddDefaultsPressed();

	/** Handler for when the user clicks the button to add a new class description*/
	FReply OnAddNewDescription();

	void PopulateClassDescriptions();

	FName SortByColumn;
	EColumnSortMode::Type SortMode;

	/** The unused tag items */
	TArray<TSharedPtr<FClassDescriptionItem>> ClassDescriptionsItems;

	/** The list widget for the unused tag items */
	TSharedPtr<SListView<TSharedPtr<FClassDescriptionItem>>> ClassDescriptionsListView;
};