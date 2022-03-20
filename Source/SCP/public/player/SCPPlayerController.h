#pragma once

#include "CoreMinimal.h"
#include "data/Item.h"

#include "SCPPlayerController.generated.h"

/**
 * 
 */
UCLASS(Config=Game)
class ASCPPlayerController : public APlayerController
{
	GENERATED_BODY()

protected:
	UPROPERTY(Replicated, EditDefaultsOnly, BlueprintReadOnly, Category = "Player|Inventory")
	int32 InventorySize;

	UPROPERTY(Replicated, EditAnywhere, BlueprintReadOnly, Category = "Player|Inventory")
	TArray<UItem*> InventoryItems;

public:
	ASCPPlayerController();

	void PostInitProperties() override;

	void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;

	void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	bool ReplicateSubobjects(UActorChannel* Channel, FOutBunch* Bunch, FReplicationFlags* RepFlags) override;

	/** Inventory related Functions */

	UFUNCTION(BlueprintCallable)
	int32 GetInventorySize() { return InventorySize; }

	UFUNCTION(BlueprintCallable)
	UItem* GetInventoryItem(int32 Index);

	UFUNCTION(BlueprintCallable)
	int32 GetInventoryFirstEmptySlot();

	UFUNCTION(BlueprintCallable)
	bool TryPutItemToInventory(UItem* Item);

	UFUNCTION(Server, Reliable, BlueprintCallable)
	void R_DropItemServer(int32 Index);
};
