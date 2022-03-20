#include "data/Item.h"

#include "SCP.h"
#include "interactive/PickableItem.h"
#include "Kismet/GameplayStatics.h"
#include "Net/UnrealNetwork.h"

UItem::UItem()
{
	
}

void UItem::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	UObject::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UItem, PickOptions);
	DOREPLIFETIME(UItem, Name);
	DOREPLIFETIME(UItem, Icon);
}

AActor* UItem::CreatePickableActor(const UObject* WorldContextObject, const FTransform Location)
{
	UWorld *World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::LogAndReturnNull);
	APickableItem *Item = World->SpawnActorDeferred<APickableItem>(APickableItem::StaticClass(), Location, nullptr, nullptr, ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
	Item->Mesh->SetRelativeTransform(PickOptions.Transform);
	Item->Mesh->SetStaticMesh(PickOptions.Mesh);
	Item->RelatedItem = this;
	UGameplayStatics::FinishSpawningActor(Item, Location);
	return Item;
}
