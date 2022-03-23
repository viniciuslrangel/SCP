#include "SCPEditorModule.h"

#include "Config.h"
#include "SCPEditor.h"
#include "DesktopPlatformModule.h"
#include "FSCPEditorStyle.h"
#include "IDesktopPlatform.h"
#include "LevelEditor.h"
#include "OriginalAssetImporter.h"
#include "MainFrame/Public/Interfaces/IMainFrameModule.h"
#include "Modules/ModuleManager.h"

#include "SCPCommands.h"
#include "SlateUtility.h"
#include "Dialogs/DlgPickPath.h"

#include "Widgets/Layout/SScrollBox.h"

IMPLEMENT_MODULE(FSCPEditorModule, SCPEditor);

DEFINE_LOG_CATEGORY(LogSCPEditor);

FName TabName(TEXT("SCPEditorTab"));

void FSCPEditorModule::StartupModule()
{
	FSCPEditorStyle::Initialize();

	CommandList = MakeShareable(new FUICommandList);

	BindCommands();

	Extender = MakeShareable(new FExtender());
	MenuBarExtension = Extender->AddToolBarExtension(
		"Content",
		EExtensionHook::Before,
		CommandList,
		FToolBarExtensionDelegate::CreateRaw(this, &FSCPEditorModule::AddMenuExtension)
	);

	FLevelEditorModule& LevelEditorModule = FModuleManager::LoadModuleChecked<FLevelEditorModule>("LevelEditor");
	LevelEditorModule.GetToolBarExtensibilityManager()->AddExtender(Extender);

	FGlobalTabmanager::Get()->RegisterNomadTabSpawner(
		TabName,
		FOnSpawnTab::CreateRaw(this, &FSCPEditorModule::OpenTab)
	);
}

void FSCPEditorModule::ShutdownModule()
{
	Extender->RemoveExtension(MenuBarExtension.ToSharedRef());
	MenuBarExtension.Reset();
	Extender.Reset();
}

void FSCPEditorModule::AddMenuExtension(FToolBarBuilder& builder) const
{
	const FSlateIcon IconBrush = FSlateIcon(
		FSCPEditorStyle::GetStyleSetName(),
		"Toolbar.OpenIcon.White.small"
	);

	builder.AddToolBarButton(
		FSCPEditorCommands::Get().OpenMainWindow,
		NAME_None,
		FText::GetEmpty(),
		FText::FromString(TEXT("SPC Actions")),
		IconBrush,
		NAME_None
	);
}

void FSCPEditorModule::BindCommands()
{
	FSCPEditorCommands::Register();

	CommandList->MapAction(
		FSCPEditorCommands::Get().OpenMainWindow,
		FExecuteAction::CreateLambda([]()
		{
			FGlobalTabmanager::Get()->TryInvokeTab(TabName);
		}),
		FCanExecuteAction()
	);
}

TSharedRef<SDockTab> FSCPEditorModule::OpenTab(const FSpawnTabArgs& args)
{
	struct Entry
	{
		FText Desc;
		FText Label;
		FOnClicked OnClick;
	};

	TArray<TSharedRef<SWidget>> Items = {
		SNew(SSeparator),
		SNew(SHeader)[SNew(STextBlock).Text(FText::FromString("Import original assets"))],
		FSlateUtility::AddBorderLabel(
			FText::FromString(TEXT("Asset map path:")),
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().FillWidth(true).VAlign(VAlign_Center)
			[
				SNew(SEditableText)
				.HintText(FText::FromString(TEXT("Asset path")))
				.OnTextCommitted_Lambda([](const FText& Text, ETextCommit::Type CommitType)
				                   {
					                   UConfig::Get().OriginalGamePath = Text.ToString();
					                   UConfig::Get().SaveConfig();
				                   })
				.Text_Lambda([]() { return FText::FromString(UConfig::Get().OriginalGamePath); })
			]
			+ SHorizontalBox::Slot().AutoWidth()
			[
				SNew(SButton)
					.Text(FText::FromString(TEXT("Select")))
					.OnClicked(FOnClicked::CreateRaw(this, &FSCPEditorModule::SelectAssetFolder))
			]
		),
		FSlateUtility::AddLabel(
			FText::FromString(TEXT("Import mesh file (StaticMesh/X)")),
			SNew(SButton)
					.Text(FText::FromString(TEXT("Import")))
					.OnClicked(FOnClicked::CreateRaw(
				             &FOriginalAssetImporter::Get(),
				             &FOriginalAssetImporter::WrapFileDialog,
				             FOriginalAssetImporter::FWrapFileOptions{
					             "Open MESH asset file",
					             "RMESH File (*.rmesh)|*.rmesh",
				             },
				             &FOriginalAssetImporter::ImportMeshInteractive
			             ))
		),
		FSlateUtility::AddLabel(
			FText::FromString(TEXT("Import cbmap2 file (Level)")),
			SNew(SButton)
					.Text(FText::FromString(TEXT("Import")))
					.OnClicked(FOnClicked::CreateRaw(
				             &FOriginalAssetImporter::Get(),
				             &FOriginalAssetImporter::WrapFileDialog,
				             FOriginalAssetImporter::FWrapFileOptions{
					             "Open cbmap2 asset file",
					             "CBMAP2 File (*.cbmap2)|*.cbmap2"
				             },
				             &FOriginalAssetImporter::ImportLevelInteractive
			             ))
		)
	};

	TSharedPtr<SScrollBox> Scroll;
	TSharedRef<SDockTab> Tab = SNew(SDockTab)
		.Label(FText::FromString(TEXT("SCP Toolbar")))
		.TabRole(NomadTab)
		.ContentPadding(10.0f)
	[
		SAssignNew(Scroll, SScrollBox)
	];

	for (TSharedRef<SWidget> E : Items)
	{
		Scroll->AddSlot()
		      .Padding(0.0f, 2.5f)[E];
	}

	return Tab;
}

FReply FSCPEditorModule::SelectAssetFolder()
{
	static FString LastPath;
	static FSCPEditorModule& Module = FModuleManager::LoadModuleChecked<FSCPEditorModule>(TEXT("SCPEditor"));

	const void* ParentWindow = nullptr;
	if (const TSharedPtr<SWindow> Root = FGlobalTabmanager::Get()->GetRootWindow())
	{
		ParentWindow = Root->GetNativeWindow()->GetOSWindowHandle();
	}

	IDesktopPlatform* DesktopPlatform = FDesktopPlatformModule::Get();
	const bool bOpened = DesktopPlatform->OpenDirectoryDialog(
		ParentWindow,
		TEXT("Select asset path"),
		LastPath,
		LastPath
	);

	if (!bOpened)
	{
		return FReply::Handled();
	}

	UConfig::Get().OriginalGamePath = LastPath;
	UConfig::Get().SaveConfig();

	return FReply::Handled();
}
