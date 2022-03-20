#include "player/SCPPlayerController.h"

#include "Components/CapsuleComponent.h"
#include "Engine/ActorChannel.h"
#include "Net/UnrealNetwork.h"
#include "player/SCPCharacter.h"

ASCPPlayerController::ASCPPlayerController()
{
	bReplicates = true;

	InventorySize = 8;
}

void ASCPPlayerController::PostInitProperties()
{
	Super::PostInitProperties();

	InventoryItems.SetNum(InventorySize);
}

void ASCPPlayerController::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	const FName PropertyName = (PropertyChangedEvent.Property != nullptr) ? PropertyChangedEvent.Property->GetFName() : NAME_None;

	if (PropertyName == GET_MEMBER_NAME_CHECKED(ASCPPlayerController, InventorySize))
	{
		InventoryItems.SetNum(InventorySize);
	}
}

void ASCPPlayerController::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ASCPPlayerController, InventorySize);
	DOREPLIFETIME(ASCPPlayerController, InventoryItems);
}

bool ASCPPlayerController::ReplicateSubobjects(UActorChannel* Channel, FOutBunch* Bunch, FReplicationFlags* RepFlags)
{
	bool WroteSomething = Super::ReplicateSubobjects(Channel, Bunch, RepFlags);

	for (const auto& Item : InventoryItems)
	{
		WroteSomething |= Channel->ReplicateSubobject(Item, *Bunch, *RepFlags);
	}

	return WroteSomething;
}

UItem* ASCPPlayerController::GetInventoryItem(int32 Index)
{
	if (Index > InventoryItems.Num() - 1 || Index < 0)
	{
		return nullptr;
	}
	return InventoryItems[Index];
}

int32 ASCPPlayerController::GetInventoryFirstEmptySlot()
{
	for (int32 i = 0; i < InventorySize; i++)
	{
		if (InventoryItems[i] == nullptr)
		{
			return i;
		}
	}
	return -1;
}

bool ASCPPlayerController::TryPutItemToInventory(UItem* Item)
{
	const int32 Slot = GetInventoryFirstEmptySlot();
	if (Slot == -1)
	{
		return false;
	}
	InventoryItems[Slot] = Item;
	return true;
}

void ASCPPlayerController::R_DropItemServer_Implementation(int32 Index)
{
	if (Index > InventoryItems.Num() - 1 || Index < 0)
	{
		return;
	}
	if (UItem* Item = InventoryItems[Index]; Item != nullptr)
	{
		float Foot = 0.0f;
		if (const ACharacter* Char = GetPawn<ACharacter>())
		{
			Foot = Char->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
		}
		FVector Pos = GetPawn()->GetActorLocation();
		Pos.Z -= Foot;
		AActor* PickableActor = Item->CreatePickableActor(GetWorld(), FTransform(Pos));
		InventoryItems[Index] = nullptr;
	}
}
