#include "core/SCPGameMode.h"
#include "player/SCPPlayerController.h"
#include "UObject/ConstructorHelpers.h"

ASCPGameMode::ASCPGameMode()
	: Super()
{
	// set default pawn class to our Blueprinted character
	static ConstructorHelpers::FClassFinder<APawn> PlayerPawnClassFinder(TEXT("/Game/SCP/Gameplay/Player/BP_SCPCharacter"));
	check(PlayerPawnClassFinder.Succeeded());
	DefaultPawnClass = PlayerPawnClassFinder.Class;

	static ConstructorHelpers::FClassFinder<APlayerController> PlayerControllerClassFinder(TEXT("/Game/SCP/Gameplay/Player/BP_SCPPlayerController"));
	check(PlayerControllerClassFinder.Succeeded());
	PlayerControllerClass = PlayerControllerClassFinder.Class;
}
