#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"

#include "SCPCharacter.generated.h"

class UInputComponent;
class USkeletalMeshComponent;
class USceneComponent;
class UCameraComponent;
class UMotionControllerComponent;
class UAnimMontage;
class USoundBase;

UCLASS(Config=Game)
class ASCPCharacter : public ACharacter
{
	friend class ASCPPlayerController;

	GENERATED_BODY()

protected:
	/** Base turn rate, in deg/sec. Other scaling may affect final turn rate. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="SCPPlayer|Camera")
	float BaseTurnRate;

	/** Base look up/down rate, in deg/sec. Other scaling may affect final rate. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="SCPPlayer|Camera")
	float BaseLookUpRate;

	/** First person camera */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "SCPPlayer|Camera")
	UCameraComponent* FirstPersonCameraComponent;

	/** Pawn mesh: 1st person view (arms; seen only by self) */
	UPROPERTY(VisibleDefaultsOnly, Category="SCPPlayer|Mesh")
	USkeletalMeshComponent* MeshFP;

	UPROPERTY(EditDefaultsOnly, Category="SCPPlayer")
	TSubclassOf<UUserWidget> PlayerHUDWidgetClass;

	UPROPERTY(VisibleAnywhere, Category="SCPPlayer")
	UUserWidget* PlayerHUDWidget;

	/** Inventory Related Stuff */

	UPROPERTY(EditDefaultsOnly, Category="SCPPlayer|Inventory")
	TSubclassOf<UUserWidget> InventoryWidgetClass;

	UPROPERTY(VisibleAnywhere, Category="SCPPlayer|Inventory")
	UUserWidget* InventoryWidget;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="SCPPlayer|Inventory")
	bool bIsInventoryOpen;

	/** Interact Stuff */

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="SCPPlayer")
	float MaxGrabDistance;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="SCPPlayer")
	UPrimitiveComponent* CurrentLookingComponent;

	/** Blink Related Stuff */

	UPROPERTY(Replicated, EditAnywhere, BlueprintReadWrite, Category="SCPPlayer|Blink")
	float FullBlinkTime;

	UPROPERTY(Replicated, EditAnywhere, BlueprintReadWrite, Category="SCPPlayer|Blink")
	float BlinkTimeRate;

	UPROPERTY(Replicated, EditAnywhere, BlueprintReadWrite, Category="SCPPlayer|Blink")
	float RemainingBlinkTime;

	UPROPERTY(Replicated, VisibleAnywhere, BlueprintReadOnly, Category="SCPPlayer|Blink")
	bool bIsBlinking;

	UPROPERTY(Replicated, VisibleAnywhere, BlueprintReadOnly, Category="SCPPlayer|Blink")
	bool bIsForcingBlink;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="SCPPlayer|Blink")
	float BlinkDuration;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="SCPPlayer|Blink")
	UMaterialInterface* BaseBlinkMaterial;

	UPROPERTY()
	UMaterialInstanceDynamic* BlinkMaterial;

	/** Walk Stuff */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="SCPPlayer|Movement")
	float WalkSpeed;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="SCPPlayer|Movement")
	float SprintSpeed;

	UPROPERTY(Replicated, EditAnywhere, BlueprintReadWrite, Category="SCPPlayer|Movement")
	float MaxSprintTime;

	UPROPERTY(Replicated, EditAnywhere, BlueprintReadWrite, Category="SCPPlayer|Movement")
	float CurrentSprintTime;

	UPROPERTY(Replicated, EditAnywhere, BlueprintReadWrite, Category="SCPPlayer|Movement")
	float SprintRegenRate;

	UPROPERTY(Replicated, VisibleAnywhere, BlueprintReadOnly, Category="SCPPlayer|Movement")
	bool bIsSprinting;

public:
	ASCPCharacter();

	USkeletalMeshComponent* GetMeshFP() const { return MeshFP; }

	UCameraComponent* GetFirstPersonCameraComponent() const { return FirstPersonCameraComponent; }

protected:
	//region Super
	void BeginPlay() override;

	void SetupPlayerInputComponent(UInputComponent* InputComponent) override;

	void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	void PossessedBy(AController* NewController) override;

	void UnPossessed() override;

	void Tick(float DeltaSeconds) override;
	//endregion

	//region INPUTS
	void MoveForward(float Val);

	void MoveRight(float Val);

	void TurnAtRate(float Rate);

	void LookUpAtRate(float Rate);

	void Interact();

	void ToggleInventory();

	void ForceBlink(bool bPressed);

	void StartSprint(bool bSprinting);
	//endregion

private:
	void UpdateLookingAt();

	void UpdateBlinkTime(float DeltaSeconds);

	void UpdateSprint(float DeltaSeconds);

	UFUNCTION(Client, Reliable)
	void R_TearUpUIClient();

	UFUNCTION(Client, Reliable)
	void R_TearDownUIClient();

	UFUNCTION(Server, Reliable)
	void R_InteractServer(UPrimitiveComponent* TargetComp);

	UFUNCTION(Server, Reliable)
	void R_SetForceBlinkServer(bool bForce);

	UFUNCTION(Client, Reliable)
	void R_BlinkClient(bool bClosed = false);

	UFUNCTION(Server, Reliable)
	void R_SetSprintingServer(bool bSprinting);
};
