#pragma once

#include "CoreMinimal.h"
#include "Components/TimelineComponent.h"
#include "GameFramework/Actor.h"
#include "interactive/Interactive.h"
#include "RoomDoor.generated.h"

UCLASS()
class SCP_API ARoomDoor : public AActor, public IInteractive
{
	GENERATED_BODY()

protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Room Door")
	USceneComponent* DoorPanel;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Room Door")
	UCurveFloat* OpeningCurve;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Room Door")
	float OpenMaxTranslation;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Room Door")
	float OpenTime;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Room Door")
	bool bIsOpen;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Room Door")
	UTimelineComponent* OpeningTimeline;

public:
	// Sets default values for this actor's properties
	ARoomDoor();

	void BeginPlay() override;

	void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	void OnUse(ASCPCharacter* Subject, UPrimitiveComponent* Comp) override;

	void OnHover(ASCPCharacter* Subject, UPrimitiveComponent* Comp, bool bIsStartHover) override;

private:
	UFUNCTION()
	void OnTimelineCallback(float Val);
};
