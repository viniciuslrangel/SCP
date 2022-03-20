#include "interactive/Interactive.h"

#include "SCP.h"

void IInteractive::OnHover(ASCPCharacter* Subject, UPrimitiveComponent* Comp, bool bIsStartHover)
{
	if (bIsStartHover)
	{
		Comp->SetRenderCustomDepth(true);
		Comp->SetCustomDepthStencilValue(CUSTOM_STENCIL_OUTLINE_CODE);
	}
	else
	{
		Comp->SetRenderCustomDepth(false);
	}
}
