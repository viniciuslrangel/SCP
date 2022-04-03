#include "player/SCPCharacter.h"

#include "SCP.h"
#include "Animation/AnimInstance.h"
#include "Camera/CameraComponent.h"
#include "Components/InputComponent.h"
#include "GameFramework/InputSettings.h"
#include "Kismet/GameplayStatics.h"
#include "MotionControllerComponent.h"
#include "Blueprint/UserWidget.h"
#include "Blueprint/WidgetBlueprintLibrary.h"
#include "Components/CapsuleComponent.h"
#include "Engine/PostProcessVolume.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "interactive/Interactive.h"
#include "Net/UnrealNetwork.h"

DECLARE_DELEGATE_OneParam(FSingleBoolDelegate, bool);

ASCPCharacter::ASCPCharacter()
{
	// Set size for collision capsule
	GetCapsuleComponent()->InitCapsuleSize(55.f, 96.0f);

	// set our turn rates for input
	BaseTurnRate = 45.f;
	BaseLookUpRate = 45.f;

	// Create a CameraComponent	
	FirstPersonCameraComponent = CreateDefaultSubobject<UCameraComponent>(TEXT("FirstPersonCamera"));
	FirstPersonCameraComponent->SetupAttachment(GetCapsuleComponent());
	FirstPersonCameraComponent->SetRelativeLocation(FVector(30.44f, 1.75f, 64.0f)); // Position the camera
	FirstPersonCameraComponent->bUsePawnControlRotation = true;

	// Create a mesh component that will be used when being viewed from a '1st person' view (when controlling this pawn)
	MeshFP = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("CharacterMesh1P"));
	MeshFP->SetOnlyOwnerSee(true);
	MeshFP->SetupAttachment(FirstPersonCameraComponent);
	MeshFP->bCastDynamicShadow = false;
	MeshFP->CastShadow = false;
	MeshFP->SetRelativeRotation(FRotator(1.9f, -19.19f, 5.2f));
	MeshFP->SetRelativeLocation(FVector(-0.5f, -4.4f, -155.7f));

	static ConstructorHelpers::FClassFinder<UUserWidget> PlayerHUDWidgetClassFinder(TEXT("/Game/SCP/Gameplay/Player/HUD/WBP_PlayerHUD"));
	check(PlayerHUDWidgetClassFinder.Succeeded());
	PlayerHUDWidgetClass = PlayerHUDWidgetClassFinder.Class;

	static ConstructorHelpers::FClassFinder<UUserWidget> InventoryWidgetClassFinder(TEXT("/Game/SCP/Gameplay/Player/Inventory/WBP_PlayerInventory"));
	check(InventoryWidgetClassFinder.Succeeded());
	InventoryWidgetClass = InventoryWidgetClassFinder.Class;

	InventoryWidget = nullptr;
	bIsInventoryOpen = false;
	MaxGrabDistance = 300.0f;
	CurrentLookingComponent = nullptr;

	FullBlinkTime = 12.0f;
	BlinkTimeRate = 1.0f;
	RemainingBlinkTime = 0;
	bIsBlinking = false;
	bIsForcingBlink = false;
	BlinkDuration = 0.5;

	BaseBlinkMaterial = nullptr;
	BlinkMaterial = nullptr;

	WalkSpeed = 400.0f;
	SprintSpeed = 700.0f;
	GetCharacterMovement()->MaxWalkSpeed = WalkSpeed;

	MaxSprintTime = 8.0f;
	CurrentSprintTime = 0.0f;
	SprintRegenRate = 1.0f;
	bIsSprinting = false;
}

void ASCPCharacter::BeginPlay()
{
	// Call the base class  
	Super::BeginPlay();

	if (IsLocallyViewed())
	{
		// Handle Blink material
		check(BaseBlinkMaterial);
		TArray<AActor*> PostProcessingActors;
		UGameplayStatics::GetAllActorsWithTag(GetWorld(), TEXT("GLOBAL_POSTPROCESSING"), PostProcessingActors);
		check(PostProcessingActors.Num() == 1);
		if(PostProcessingActors.Num() == 1)
		{
			AActor* Actor = PostProcessingActors[0];
			APostProcessVolume* Volume = Cast<APostProcessVolume>(Actor);
			check(Volume);
			// Volume->AddOrUpdateBlendable();
			BlinkMaterial = UMaterialInstanceDynamic::Create(BaseBlinkMaterial, this);
			Volume->AddOrUpdateBlendable(BlinkMaterial);
			R_BlinkClient(false);
		}
	}
}

void ASCPCharacter::SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent)
{
	// set up gameplay key bindings
	check(PlayerInputComponent);

	// Bind jump events
	PlayerInputComponent->BindAction("Jump", IE_Pressed, this, &ACharacter::Jump);
	PlayerInputComponent->BindAction("Jump", IE_Released, this, &ACharacter::StopJumping);

	// Bind movement events
	PlayerInputComponent->BindAxis("MoveForward", this, &ASCPCharacter::MoveForward);
	PlayerInputComponent->BindAxis("MoveRight", this, &ASCPCharacter::MoveRight);

	PlayerInputComponent->BindAxis("Turn", this, &APawn::AddControllerYawInput);
	PlayerInputComponent->BindAxis("TurnRate", this, &ASCPCharacter::TurnAtRate);
	PlayerInputComponent->BindAxis("LookUp", this, &APawn::AddControllerPitchInput);
	PlayerInputComponent->BindAxis("LookUpRate", this, &ASCPCharacter::LookUpAtRate);

	PlayerInputComponent->BindAction("Interact", IE_Pressed, this, &ASCPCharacter::Interact);
	PlayerInputComponent->BindAction("ToggleInventory", IE_Pressed, this, &ASCPCharacter::ToggleInventory);

	PlayerInputComponent->BindAction<FSingleBoolDelegate>("Blink", IE_Pressed, this, &ASCPCharacter::ForceBlink, true);
	PlayerInputComponent->BindAction<FSingleBoolDelegate>("Blink", IE_Released, this, &ASCPCharacter::ForceBlink, false);

	PlayerInputComponent->BindAction<FSingleBoolDelegate>("Sprint", IE_Pressed, this, &ASCPCharacter::StartSprint, true);
	PlayerInputComponent->BindAction<FSingleBoolDelegate>("Sprint", IE_Released, this, &ASCPCharacter::StartSprint, false);

	// PlayerInputComponent->BindAction();
}

void ASCPCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME_CONDITION(ASCPCharacter, FullBlinkTime, COND_OwnerOnly);
	DOREPLIFETIME_CONDITION(ASCPCharacter, BlinkTimeRate, COND_OwnerOnly);
	DOREPLIFETIME_CONDITION(ASCPCharacter, RemainingBlinkTime, COND_OwnerOnly);
	DOREPLIFETIME_CONDITION(ASCPCharacter, bIsBlinking, COND_None);
	DOREPLIFETIME_CONDITION(ASCPCharacter, bIsForcingBlink, COND_OwnerOnly);

	DOREPLIFETIME_CONDITION(ASCPCharacter, MaxSprintTime, COND_OwnerOnly);
	DOREPLIFETIME_CONDITION(ASCPCharacter, CurrentSprintTime, COND_OwnerOnly);
	DOREPLIFETIME_CONDITION(ASCPCharacter, SprintRegenRate, COND_OwnerOnly);
	DOREPLIFETIME_CONDITION(ASCPCharacter, bIsSprinting, COND_OwnerOnly);
}

void ASCPCharacter::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);

	R_TearUpUIClient();
}

void ASCPCharacter::UnPossessed()
{
	Super::UnPossessed();

	R_TearDownUIClient();
}

void ASCPCharacter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	UpdateBlinkTime(DeltaSeconds);
	UpdateSprint(DeltaSeconds);
	if (IsLocallyViewed()) // Run only when has a Client
	{
		UpdateLookingAt();
	}
}

void ASCPCharacter::MoveForward(float Value)
{
	if (Value != 0.0f)
	{
		AddMovementInput(GetActorForwardVector(), Value);
	}
}

void ASCPCharacter::MoveRight(float Value)
{
	if (Value != 0.0f)
	{
		AddMovementInput(GetActorRightVector(), Value);
	}
}

void ASCPCharacter::TurnAtRate(float Rate)
{
	AddControllerYawInput(Rate * BaseTurnRate * GetWorld()->GetDeltaSeconds());
}

void ASCPCharacter::LookUpAtRate(float Rate)
{
	AddControllerPitchInput(Rate * BaseLookUpRate * GetWorld()->GetDeltaSeconds());
}

void ASCPCharacter::Interact()
{
	if (!IsValid(CurrentLookingComponent))
	{
		return;
	}
	if (CurrentLookingComponent->GetOwner<IInteractive>())
	{
		R_InteractServer(CurrentLookingComponent);
	}
}

void ASCPCharacter::ToggleInventory()
{
	if (!InventoryWidget)
	{
		return;
	}
	if (bIsInventoryOpen)
	{
		InventoryWidget->SetVisibility(ESlateVisibility::Collapsed);
		bIsInventoryOpen = false;
		if (APlayerController* PCtrl = Cast<APlayerController>(GetController()))
		{
			UWidgetBlueprintLibrary::SetInputMode_GameOnly(PCtrl);
			PCtrl->SetShowMouseCursor(false);
		}
	}
	else
	{
		InventoryWidget->SetVisibility(ESlateVisibility::Visible);
		bIsInventoryOpen = true;
		if (APlayerController* PCtrl = Cast<APlayerController>(GetController()))
		{
			UWidgetBlueprintLibrary::SetInputMode_GameAndUIEx(PCtrl, InventoryWidget, EMouseLockMode::DoNotLock, false);
			PCtrl->SetShowMouseCursor(true);
		}
	}
}

void ASCPCharacter::ForceBlink(bool bPressed)
{
	bIsForcingBlink = bPressed;
	R_SetForceBlinkServer(bPressed);
}

void ASCPCharacter::StartSprint(bool bSprinting)
{
	bIsSprinting = bSprinting;
	R_SetSprintingServer(bSprinting);
}

void ASCPCharacter::UpdateLookingAt()
{
	FVector CameraPos = FirstPersonCameraComponent->GetComponentLocation();
	FVector CameraDir = FRotationMatrix(FirstPersonCameraComponent->GetComponentRotation()).GetScaledAxis(EAxis::X);
	FHitResult Hit;
	FCollisionQueryParams Param(FName(TEXT("SCPPlayerTrace")), true, this);
	Param.bTraceComplex = true;
	bool bDidHit = GetWorld()->LineTraceSingleByChannel(
		Hit,
		CameraPos + CameraDir * 30,
		CameraPos + CameraDir * MaxGrabDistance,
		ECC_InteractiveTraceChannel,
		Param
	);

	bool bIsInteractive = bDidHit && Hit.GetActor()->Implements<UInteractive>();

	UPrimitiveComponent* HitComp = bDidHit && bIsInteractive ? Hit.GetComponent() : nullptr;
	if (HitComp == CurrentLookingComponent)
	{
		return;
	}
	if (IsValid(CurrentLookingComponent))
	{
		CurrentLookingComponent->GetOwner<IInteractive>()->OnHover(this, CurrentLookingComponent, false);
	}
	CurrentLookingComponent = HitComp;
	if (CurrentLookingComponent)
	{
		CurrentLookingComponent->GetOwner<IInteractive>()->OnHover(this, CurrentLookingComponent, true);
	}
}

void ASCPCharacter::UpdateBlinkTime(float DeltaSeconds)
{
	if (bIsForcingBlink)
	{
		if (RemainingBlinkTime > 0)
		{
			RemainingBlinkTime = -BlinkDuration * (RemainingBlinkTime / FullBlinkTime);
		}
	}
	else
	{
		RemainingBlinkTime += DeltaSeconds * BlinkTimeRate;
	}

	if (RemainingBlinkTime < 0)
	{
		bIsBlinking = true;
	}
	else if (bIsBlinking && RemainingBlinkTime >= 0)
	{
		bIsBlinking = false;
		R_BlinkClient(false);
	}

	if (RemainingBlinkTime > FullBlinkTime)
	{
		R_BlinkClient(true);
		RemainingBlinkTime = -BlinkDuration;
	}
}

void ASCPCharacter::UpdateSprint(float DeltaSeconds)
{
	const bool bCanSprint = CurrentSprintTime < MaxSprintTime;
	if (bCanSprint && bIsSprinting)
	{
		CurrentSprintTime += DeltaSeconds;
		GetCharacterMovement()->MaxWalkSpeed = SprintSpeed;
	}
	else
	{
		GetCharacterMovement()->MaxWalkSpeed = WalkSpeed;
		if (!bIsSprinting && CurrentSprintTime > 0)
		{
			CurrentSprintTime = FMath::Max(0.0f, CurrentSprintTime - (DeltaSeconds * SprintRegenRate));
		}
	}
}

void ASCPCharacter::R_TearUpUIClient_Implementation()
{
	if (APlayerController* PController = GetController<APlayerController>(); PController && PController->IsLocalController())
	{
		PlayerHUDWidget = CreateWidget(PController, PlayerHUDWidgetClass.Get());
		PlayerHUDWidget->AddToPlayerScreen(1);

		InventoryWidget = CreateWidget(PController, InventoryWidgetClass.Get());
		InventoryWidget->AddToPlayerScreen(2);
		InventoryWidget->SetVisibility(ESlateVisibility::Collapsed);
		bIsInventoryOpen = false;
	}
}

void ASCPCharacter::R_TearDownUIClient_Implementation()
{
	if (InventoryWidget)
	{
		InventoryWidget->Destruct();
		InventoryWidget = nullptr;
	}
}

void ASCPCharacter::R_InteractServer_Implementation(UPrimitiveComponent* TargetComp)
{
	if (TargetComp == nullptr)
	{
		return;
	}
	AActor* TargetActor = TargetComp->GetOwner();
	if (TargetActor && TargetActor->Implements<UInteractive>())
	{
		const float MaxDistance = MaxGrabDistance + TargetComp->Bounds.SphereRadius;
		const FVector CameraPos = FirstPersonCameraComponent->GetComponentLocation();
		const FVector TargetPos = TargetComp->GetComponentLocation();
		if (FVector::DistSquared(CameraPos, TargetPos) > MaxDistance * MaxDistance * 1.4)
		{
			return;
		}
	}
	if (IInteractive* Interactive = Cast<IInteractive>(TargetActor))
	{
		Interactive->OnUse(this, TargetComp);
	}
}

void ASCPCharacter::R_SetForceBlinkServer_Implementation(bool bForce)
{
	bIsForcingBlink = bForce;
	if (bForce)
	{
		R_BlinkClient(true);
	}
}

void ASCPCharacter::R_BlinkClient_Implementation(bool bClosed)
{
	if (BlinkMaterial == nullptr)
	{
		return;
	}

	BlinkMaterial->SetScalarParameterValue("Closed", bClosed ? 1.0f : 0.0f);
	BlinkMaterial->SetScalarParameterValue("StartBlinkTime", GetWorld()->GetTimeSeconds());
	BlinkMaterial->SetScalarParameterValue("BlinkDuration", BlinkDuration);
}

void ASCPCharacter::R_SetSprintingServer_Implementation(bool bSprinting)
{
	bIsSprinting = bSprinting;
}
