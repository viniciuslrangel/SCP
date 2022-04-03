#include "interactive/PickableItem.h"

#include "player/SCPCharacter.h"
#include "player/SCPPlayerController.h"

// Sets default values
APickableItem::APickableItem()
{
	bNetLoadOnClient = false;
	bReplicates = true;
	SetReplicatingMovement(true);

	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));

	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	Mesh->SetupAttachment(RootComponent);
	Mesh->SetIsReplicated(true);
	Mesh->SetCollisionResponseToAllChannels(ECR_Overlap);
	Mesh->SetCollisionResponseToChannel(ECC_Camera, ECR_Block);
	Mesh->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
}

void APickableItem::OnUse(ASCPCharacter* Subject, UPrimitiveComponent* Comp)
{
	ASCPPlayerController* Controller = Subject->GetController<ASCPPlayerController>();
	bool bOk = Controller->TryPutItemToInventory(RelatedItem);
	if (bOk)
	{
		Destroy();
	}
}

// Called when the game starts or when spawned
void APickableItem::BeginPlay()
{
	Super::BeginPlay();
}
