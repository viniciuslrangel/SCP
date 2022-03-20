#pragma  once

#include "EditorStyleSet.h"

class FSCPEditorCommands : public TCommands<FSCPEditorCommands>
{
public:
	FSCPEditorCommands() : TCommands<FSCPEditorCommands>(
		FName(TEXT("SCPEditor_Commands")),
		FText::FromString(TEXT("SCP Editor Commands")), NAME_None,
		FEditorStyle::GetStyleSetName()
	)
	{
	}

	virtual void RegisterCommands() override;

	TSharedPtr<FUICommandInfo> OpenMainWindow;

};
