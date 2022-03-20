#pragma once

class FSCPEditorModule : public IModuleInterface
{
	friend class FOriginalAssetImporter;
	
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

	void AddMenuExtension(FToolBarBuilder& builder) const;
	
	void BindCommands();

	TSharedRef<SDockTab> OpenTab(const FSpawnTabArgs&);

	FReply SelectAssetFolder();

public:
	TSharedPtr<FExtender> Extender;
	TSharedPtr<const FExtensionBase> MenuBarExtension;

private:
	TSharedPtr<FUICommandList> CommandList;
};
