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
	// This is called on server side
	virtual void OnUse(ASCPCharacter* Subject, UPrimitiveComponent* Comp) = 0;

	// This is called on client side
	virtual void OnHover(ASCPCharacter* Subject, UPrimitiveComponent* Comp, bool bIsStartHover);
};
