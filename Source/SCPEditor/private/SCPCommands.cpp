#include "SCPCommands.h"

void FSCPEditorCommands::RegisterCommands()
{
#define LOCTEXT_NAMESPACE ""
	UI_COMMAND(OpenMainWindow, "OpenSPUEditorWindow", "SPU Actions Window", EUserInterfaceActionType::Button, FInputChord());
#undef LOCTEXT_NAMESPACE
}
