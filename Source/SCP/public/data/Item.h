#pragma once

#include "Item.generated.h"

USTRUCT(BlueprintType)
struct FPickOptions
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	UStaticMesh* Mesh;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	FTransform Transform;

	FPickOptions() : Mesh(nullptr), Transform(FTransform())
	{
	}
};

UCLASS(Blueprintable, BlueprintType, Abstract)
class SCP_API UItem : public UObject
{
	GENERATED_BODY()

protected:
	UPROPERTY(Replicated, EditDefaultsOnly, BlueprintReadOnly)
	FPickOptions PickOptions;

	UPROPERTY(Replicated, EditDefaultsOnly, BlueprintReadOnly)
	FText Name;

	UPROPERTY(Replicated, EditDefaultsOnly, BlueprintReadOnly)
	UTexture2D* Icon;

public:
	UItem();

	bool IsSupportedForNetworking() const override { return true; }

	void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UFUNCTION(BlueprintCallable, meta = (WorldContext = "WorldContextObject"))
	AActor* CreatePickableActor(const UObject* WorldContextObject, const FTransform Location = FTransform());
};
