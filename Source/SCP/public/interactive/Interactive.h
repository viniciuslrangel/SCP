#pragma once

#include "CoreMinimal.h"
#include "player/SCPCharacter.h"
#include "UObject/Interface.h"
#include "Interactive.generated.h"

// This class does not need to be modified.
UINTERFACE()
class UInteractive : public UInterface
{
	GENERATED_BODY()
};

/**
 * 
 */
class SCP_API IInteractive
{
	GENERATED_BODY()

public:
	virtual void OnUse(ASCPCharacter* Subject) = 0;

	virtual void OnHover(ASCPCharacter* Subject, UPrimitiveComponent* Comp, bool bIsStartHover);
};
