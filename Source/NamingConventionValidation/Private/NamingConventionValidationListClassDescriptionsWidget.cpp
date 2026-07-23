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
	SortByColumn = ColumnId;
	SortMode = InSortMode;

	SortClassDescriptions();
}

void SNamingConventionValidationListClassDescriptionsWidget::SortClassDescriptions()
{
	if (SortByColumn == SNamingConventionValidationClassDescriptionWidgetDefs::ColumnID_ClassPath)
	{
		if (SortMode == EColumnSortMode::Ascending)
		{
			ClassDescriptionsItems.Sort([](const TSharedPtr<FClassDescriptionItem>& A, const TSharedPtr<FClassDescriptionItem>& B) { return A->ClassDescription.ClassPath.ToString().Compare(B->ClassDescription.ClassPath.ToString()) < 0; });
		}
		else if (SortMode == EColumnSortMode::Descending)
		{
			ClassDescriptionsItems.Sort([](const TSharedPtr<FClassDescriptionItem>& A, const TSharedPtr<FClassDescriptionItem>& B) { return A->ClassDescription.ClassPath.ToString().Compare(B->ClassDescription.ClassPath.ToString()) >= 0; });
		}
	}
	else if (SortByColumn == SNamingConventionValidationClassDescriptionWidgetDefs::ColumnID_Prefix)
	{
		if (SortMode == EColumnSortMode::Ascending)
		{
			ClassDescriptionsItems.Sort([](const TSharedPtr<FClassDescriptionItem>& A, const TSharedPtr<FClassDescriptionItem>& B) { return A->ClassDescription.Prefix.Compare(B->ClassDescription.Prefix) < 0; });
		}
		else if (SortMode == EColumnSortMode::Descending)
		{
			ClassDescriptionsItems.Sort([](const TSharedPtr<FClassDescriptionItem>& A, const TSharedPtr<FClassDescriptionItem>& B) { return A->ClassDescription.Prefix.Compare(B->ClassDescription.Prefix) >= 0; });
		}
	}
	else if (SortByColumn == SNamingConventionValidationClassDescriptionWidgetDefs::ColumnID_Suffix)
	{
		if (SortMode == EColumnSortMode::Ascending)
		{
			ClassDescriptionsItems.Sort([](const TSharedPtr<FClassDescriptionItem>& A, const TSharedPtr<FClassDescriptionItem>& B) { return A->ClassDescription.Suffix.Compare(B->ClassDescription.Suffix) < 0; });
		}
		else if (SortMode == EColumnSortMode::Descending)
		{
			ClassDescriptionsItems.Sort([](const TSharedPtr<FClassDescriptionItem>& A, const TSharedPtr<FClassDescriptionItem>& B) { return A->ClassDescription.Suffix.Compare(B->ClassDescription.Suffix) >= 0; });
		}
	}
	else if (SortByColumn == SNamingConventionValidationClassDescriptionWidgetDefs::ColumnID_Priority)
	{
		if (SortMode == EColumnSortMode::Ascending)
		{
			ClassDescriptionsItems.Sort([](const TSharedPtr<FClassDescriptionItem>& A, const TSharedPtr<FClassDescriptionItem>& B) { return A->ClassDescription.Priority >= B->ClassDescription.Priority; });
		}
		else if (SortMode == EColumnSortMode::Descending)
		{
			ClassDescriptionsItems.Sort([](const TSharedPtr<FClassDescriptionItem>& A, const TSharedPtr<FClassDescriptionItem>& B) { return A->ClassDescription.Priority <= B->ClassDescription.Priority; });
		}
	}

	ClassDescriptionsListView->RequestListRefresh();
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
			    Settings->ClassDescriptions.Sort();
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

		Settings->ClassDescriptions.Sort();
		Settings->TryUpdateDefaultConfigFile();

		PopulateClassDescriptions();

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
			Settings->ClassDescriptions.Emplace(FSoftClassPath(TEXT("/Script/Chooser.ChooserTable")), TEXT("CHT_"), TEXT(""), 0);
			Settings->ClassDescriptions.Emplace(FSoftClassPath(TEXT("/Script/PoseSearch.PoseSearchDatabase")), TEXT("PSD_"), TEXT(""), 0);
			Settings->ClassDescriptions.Emplace(FSoftClassPath(TEXT("/Script/PoseSearch.PoseSearchSchema")), TEXT("PSS_"), TEXT(""), 0);
			Settings->ClassDescriptions.Emplace(FSoftClassPath(TEXT("/Script/PoseSearch.PoseSearchNormalizationSet")), TEXT("PSNS_"), TEXT(""), 0);
			Settings->ClassDescriptions.Emplace(FSoftClassPath(TEXT("/Script/AIModule.BTDecorator")), TEXT("BTD_"), TEXT(""), 0);
			Settings->ClassDescriptions.Emplace(FSoftClassPath(TEXT("/Script/AIModule.BTService")), TEXT("BTS_"), TEXT(""), 0);
			Settings->ClassDescriptions.Emplace(FSoftClassPath(TEXT("/Script/AIModule.BTTaskNode")), TEXT("BTT_"), TEXT(""), 0);
			Settings->ClassDescriptions.Emplace(FSoftClassPath(TEXT("/Script/CommonUI.CommonBorderStyle")), TEXT("CS_"), TEXT("_BORDER"), 0);
			Settings->ClassDescriptions.Emplace(FSoftClassPath(TEXT("/Script/Engine.MaterialFunctionMaterialLayerBlend")), TEXT("MLB_"), TEXT(""), 150);
			Settings->ClassDescriptions.Emplace(FSoftClassPath(TEXT("/Script/Engine.Blueprint")), TEXT("BP_"), TEXT(""), 0);
			Settings->ClassDescriptions.Emplace(FSoftClassPath(TEXT("/Script/Engine.AimOffsetBlendSpace")), TEXT("AO2D_"), TEXT(""), 100);
			Settings->ClassDescriptions.Emplace(FSoftClassPath(TEXT("/Script/CommonUI.CommonButtonStyle")), TEXT("CS_"), TEXT("_BTN"), 0);
			Settings->ClassDescriptions.Emplace(FSoftClassPath(TEXT("/Script/CommonInput.CommonInputBaseControllerData")), TEXT("CICD_"), TEXT(""), 0);
			Settings->ClassDescriptions.Emplace(FSoftClassPath(TEXT("/Script/CommonUI.CommonTextScrollStyle")), TEXT("CTSS_"), TEXT(""), 0);
			Settings->ClassDescriptions.Emplace(FSoftClassPath(TEXT("/Script/Engine.MaterialFunctionMaterialLayerInstance")), TEXT("MLI_"), TEXT(""), 110);
			Settings->ClassDescriptions.Emplace(FSoftClassPath(TEXT("/Script/Engine.CanvasRenderTarget2D")), TEXT("RT_"), TEXT(""), 100);
			Settings->ClassDescriptions.Emplace(FSoftClassPath(TEXT("/Script/Engine.AimOffsetBlendSpace1D")), TEXT("AO1D_"), TEXT(""), 100);
			Settings->ClassDescriptions.Emplace(FSoftClassPath(TEXT("/Script/AIModule.BlackboardData")), TEXT("BBD_"), TEXT(""), 100);
			Settings->ClassDescriptions.Emplace(FSoftClassPath(TEXT("/Script/Engine.AnimInstance")), TEXT("ABP_"), TEXT(""), 0);
			Settings->ClassDescriptions.Emplace(FSoftClassPath(TEXT("/Script/Engine.AnimLayerInterface")), TEXT("ALI_"), TEXT(""), 0);
			Settings->ClassDescriptions.Emplace(FSoftClassPath(TEXT("/Script/Engine.AnimMontage")), TEXT("AM_"), TEXT(""), 0);
			Settings->ClassDescriptions.Emplace(FSoftClassPath(TEXT("/Script/Engine.MaterialFunctionMaterialLayer")), TEXT("ML_"), TEXT(""), 100);
			Settings->ClassDescriptions.Emplace(FSoftClassPath(TEXT("/Script/Engine.AnimSequence")), TEXT("A_"), TEXT(""), 0);
			Settings->ClassDescriptions.Emplace(FSoftClassPath(TEXT("/Script/AIModule.BehaviorTree")), TEXT("BT_"), TEXT(""), 0);
			Settings->ClassDescriptions.Emplace(FSoftClassPath(TEXT("/Script/Engine.BlendSpace")), TEXT("BS_"), TEXT(""), 0);
			Settings->ClassDescriptions.Emplace(FSoftClassPath(TEXT("/Script/CommonUI.CommonTextStyle")), TEXT("CTS_"), TEXT(""), 0);
			Settings->ClassDescriptions.Emplace(FSoftClassPath(TEXT("/Script/Blutility.EditorUtilityWidgetBlueprint")), TEXT("EUW_"), TEXT(""), 10);
			Settings->ClassDescriptions.Emplace(FSoftClassPath(TEXT("/Script/ContextualAnimation.ContextualAnimSceneAsset")), TEXT("CTXAS_"), TEXT(""), 0);
			Settings->ClassDescriptions.Emplace(FSoftClassPath(TEXT("/Script/Engine.CurveFloat")), TEXT("CF_"), TEXT(""), 0);
			Settings->ClassDescriptions.Emplace(FSoftClassPath(TEXT("/Script/Engine.CurveLinearColor")), TEXT("CC_"), TEXT(""), 0);
			Settings->ClassDescriptions.Emplace(FSoftClassPath(TEXT("/Script/Engine.CurveLinearColorAtlas")), TEXT("CA_"), TEXT(""), 0);
			Settings->ClassDescriptions.Emplace(FSoftClassPath(TEXT("/Script/Engine.CurveTable")), TEXT("CT_"), TEXT(""), 0);
			Settings->ClassDescriptions.Emplace(FSoftClassPath(TEXT("/Script/GameFeatures.GameFeatureData")), TEXT("GFD_"), TEXT(""), 200);
			Settings->ClassDescriptions.Emplace(FSoftClassPath(TEXT("/Script/Engine.CurveVector")), TEXT("CV_"), TEXT(""), 0);
			Settings->ClassDescriptions.Emplace(FSoftClassPath(TEXT("/Script/Engine.DataAsset")), TEXT("DA_"), TEXT(""), 0);
			Settings->ClassDescriptions.Emplace(FSoftClassPath(TEXT("/Script/Engine.DataLayerAsset")), TEXT("DL_"), TEXT(""), 0);
			Settings->ClassDescriptions.Emplace(FSoftClassPath(TEXT("/Script/Engine.DataTable")), TEXT("DT_"), TEXT(""), 0);
			Settings->ClassDescriptions.Emplace(FSoftClassPath(TEXT("/Script/Blutility.EditorUtilityBlueprint")), TEXT("BP_"), TEXT(""), 0);
			Settings->ClassDescriptions.Emplace(FSoftClassPath(TEXT("/Script/AIModule.EnvQuery")), TEXT("EQ_"), TEXT(""), 100);
			Settings->ClassDescriptions.Emplace(FSoftClassPath(TEXT("/Script/Blutility.EditorUtilityWidget")), TEXT("EUW_"), TEXT(""), 0);
			Settings->ClassDescriptions.Emplace(FSoftClassPath(TEXT("/Script/AIModule.EnvQueryContext")), TEXT("EQC_"), TEXT(""), 0);
			Settings->ClassDescriptions.Emplace(FSoftClassPath(TEXT("/Script/Engine.ForceFeedbackAttenuation")), TEXT("FFA_"), TEXT(""), 0);
			Settings->ClassDescriptions.Emplace(FSoftClassPath(TEXT("/Script/Engine.ForceFeedbackEffect")), TEXT("FFE_"), TEXT(""), 0);
			Settings->ClassDescriptions.Emplace(FSoftClassPath(TEXT("/Script/GameplayAbilities.GameplayAbility")), TEXT("GA_"), TEXT(""), 0);
			Settings->ClassDescriptions.Emplace(FSoftClassPath(TEXT("/Script/GameplayAbilities.GameplayCueNotify_Actor")), TEXT("GC_"), TEXT(""), 0);
			Settings->ClassDescriptions.Emplace(FSoftClassPath(TEXT("/Script/GameplayAbilities.GameplayCueNotify_Static")), TEXT("GC_"), TEXT(""), 0);
			Settings->ClassDescriptions.Emplace(FSoftClassPath(TEXT("/Script/GameplayAbilities.GameplayEffect")), TEXT("GE_"), TEXT(""), 0);
			Settings->ClassDescriptions.Emplace(FSoftClassPath(TEXT("/Script/Engine.PrimaryDataAsset")), TEXT("PDA_"), TEXT(""), 100);
			Settings->ClassDescriptions.Emplace(FSoftClassPath(TEXT("/Script/Engine.HapticFeedbackEffect_Curve")), TEXT("HFEC_"), TEXT(""), 0);
			Settings->ClassDescriptions.Emplace(FSoftClassPath(TEXT("/Script/CoreUObject.Interface")), TEXT("BPI_"), TEXT(""), 100);
			Settings->ClassDescriptions.Emplace(FSoftClassPath(TEXT("/Script/EnhancedInput.InputAction")), TEXT("IA_"), TEXT(""), 0);
			Settings->ClassDescriptions.Emplace(FSoftClassPath(TEXT("/Script/EnhancedInput.InputMappingContext")), TEXT("IMC_"), TEXT(""), 0);
			Settings->ClassDescriptions.Emplace(FSoftClassPath(TEXT("/Script/Engine.LevelScriptActor")), TEXT("L_"), TEXT(""), 0);
			Settings->ClassDescriptions.Emplace(FSoftClassPath(TEXT("/Script/LevelSequence.LevelSequence")), TEXT("LS_"), TEXT(""), 0);
			Settings->ClassDescriptions.Emplace(FSoftClassPath(TEXT("/Script/Engine.Material")), TEXT("M_"), TEXT(""), 0);
			Settings->ClassDescriptions.Emplace(FSoftClassPath(TEXT("/Script/Engine.MaterialFunctionInterface")), TEXT("MF_"), TEXT(""), 0);
			Settings->ClassDescriptions.Emplace(FSoftClassPath(TEXT("/Script/Engine.MaterialInstance")), TEXT("MI_"), TEXT(""), 0);
			Settings->ClassDescriptions.Emplace(FSoftClassPath(TEXT("/Script/Engine.MaterialParameterCollection")), TEXT("MPC_"), TEXT(""), 0);
			Settings->ClassDescriptions.Emplace(FSoftClassPath(TEXT("/Script/ModelViewViewModel.MVVMViewModelBase")), TEXT("VM_"), TEXT(""), 0);
			Settings->ClassDescriptions.Emplace(FSoftClassPath(TEXT("/Script/Niagara.NiagaraEffectType")), TEXT("NET_"), TEXT(""), 0);
			Settings->ClassDescriptions.Emplace(FSoftClassPath(TEXT("/Script/Niagara.NiagaraEmitter")), TEXT("NE_"), TEXT(""), 0);
			Settings->ClassDescriptions.Emplace(FSoftClassPath(TEXT("/Script/Niagara.NiagaraScript")), TEXT("NSC_"), TEXT(""), 0);
			Settings->ClassDescriptions.Emplace(FSoftClassPath(TEXT("/Script/Niagara.NiagaraSystem")), TEXT("NS_"), TEXT(""), 0);
			Settings->ClassDescriptions.Emplace(FSoftClassPath(TEXT("/Script/Engine.ParticleSystem")), TEXT("PS_"), TEXT(""), 0);
			Settings->ClassDescriptions.Emplace(FSoftClassPath(TEXT("/Script/PCG.PCGBlueprintElement")), TEXT("PCGBPE_"), TEXT(""), 0);
			Settings->ClassDescriptions.Emplace(FSoftClassPath(TEXT("/Script/Engine.PhysicsAsset")), TEXT("PA_"), TEXT(""), 0);
			Settings->ClassDescriptions.Emplace(FSoftClassPath(TEXT("/Script/Blutility.PlacedEditorUtilityBase")), TEXT("BPT_"), TEXT(""), 0);
			Settings->ClassDescriptions.Emplace(FSoftClassPath(TEXT("/Script/EnhancedInput.PlayerMappableInputConfig")), TEXT("PMIC_"), TEXT(""), 0);
			Settings->ClassDescriptions.Emplace(FSoftClassPath(TEXT("/Script/Engine.SkeletalMesh")), TEXT("SK_"), TEXT(""), 0);
			Settings->ClassDescriptions.Emplace(FSoftClassPath(TEXT("/Script/Engine.Skeleton")), TEXT("SKEL_"), TEXT(""), 0);
			Settings->ClassDescriptions.Emplace(FSoftClassPath(TEXT("/Script/SmartObjectsModule.SmartObjectDefinition")), TEXT("SO_"), TEXT(""), 0);
			Settings->ClassDescriptions.Emplace(FSoftClassPath(TEXT("/Script/Engine.SoundClass")), TEXT("SCL_"), TEXT(""), 0);
			Settings->ClassDescriptions.Emplace(FSoftClassPath(TEXT("/Script/Engine.SoundCue")), TEXT("SC_"), TEXT(""), 0);
			Settings->ClassDescriptions.Emplace(FSoftClassPath(TEXT("/Script/Engine.SoundMix")), TEXT("SMix_"), TEXT(""), 0);
			Settings->ClassDescriptions.Emplace(FSoftClassPath(TEXT("/Script/Engine.SoundWave")), TEXT("SW_"), TEXT(""), 0);
			Settings->ClassDescriptions.Emplace(FSoftClassPath(TEXT("/Script/StateTreeModule.StateTree")), TEXT("ST_"), TEXT(""), 0);
			Settings->ClassDescriptions.Emplace(FSoftClassPath(TEXT("/Script/StateTreeModule.StateTreeConditionBlueprintBase")), TEXT("STC_"), TEXT(""), 0);
			Settings->ClassDescriptions.Emplace(FSoftClassPath(TEXT("/Script/StateTreeModule.StateTreeEvaluatorBlueprintBase")), TEXT("STE_"), TEXT(""), 0);
			Settings->ClassDescriptions.Emplace(FSoftClassPath(TEXT("/Script/StateTreeModule.StateTreeTaskBlueprintBase")), TEXT("STT_"), TEXT(""), 0);
			Settings->ClassDescriptions.Emplace(FSoftClassPath(TEXT("/Script/Engine.StaticMesh")), TEXT("SM_"), TEXT(""), 0);
			Settings->ClassDescriptions.Emplace(FSoftClassPath(TEXT("/Script/Engine.Texture")), TEXT("T_"), TEXT(""), 0);
			Settings->ClassDescriptions.Emplace(FSoftClassPath(TEXT("/Script/Engine.Texture2D")), TEXT("T_"), TEXT(""), 0);
			Settings->ClassDescriptions.Emplace(FSoftClassPath(TEXT("/Script/Engine.UserDefinedEnum")), TEXT("BPE_"), TEXT(""), 0);
			Settings->ClassDescriptions.Emplace(FSoftClassPath(TEXT("/Script/Engine.UserDefinedStruct")), TEXT("BPS_"), TEXT(""), 0);
			Settings->ClassDescriptions.Emplace(FSoftClassPath(TEXT("/Script/UMG.UserWidget")), TEXT("WBP_"), TEXT(""), 0);
			Settings->ClassDescriptions.Emplace(FSoftClassPath(TEXT("/Script/Engine.World")), TEXT("L_"), TEXT(""), 0);
			Settings->ClassDescriptions.Emplace(FSoftClassPath(TEXT("/Script/Soundscape.SoundScapeColor")), TEXT("SSC_"), TEXT(""), 0);
			Settings->ClassDescriptions.Emplace(FSoftClassPath(TEXT("/Script/Soundscape.SoundScapePalette")), TEXT("SSP_"), TEXT(""), 0);
			Settings->ClassDescriptions.Sort();
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
				Settings->ClassDescriptions.Sort();
				Settings->TryUpdateDefaultConfigFile();
				PopulateClassDescriptions();
			}
			else
			{
				FMessageDialog::Open(
				    EAppMsgType::Ok,
				    FText::Format(LOCTEXT("ClassDescriptionsAdd_Error", "A class description for {0} already exists!."), FText::FromString(ClassDescription.ClassPath.ToString())),
				    LOCTEXT("ClassDescriptionsRemoved_Title", "Add class description error"));
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