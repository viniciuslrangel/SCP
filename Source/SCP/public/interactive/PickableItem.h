#pragma once

#include "CoreMinimal.h"
#include "Interactive.h"
#include "GameFramework/Actor.h"
#include "PickableItem.generated.h"

class ASCPCharacter;

UCLASS()
class SCP_API APickableItem : public AActor, public IInteractive
{
	friend class UItem;
	GENERATED_BODY()

protected:

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	UStaticMeshComponent* Mesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	UItem *RelatedItem;

public:
	APickableItem();

	void OnUse(ASCPCharacter* Subject) override;

protected:
	virtual void BeginPlay() override;
};
