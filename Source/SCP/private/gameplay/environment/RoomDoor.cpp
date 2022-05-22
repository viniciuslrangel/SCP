#include "gameplay/environment/RoomDoor.h"

#include "Net/UnrealNetwork.h"


// Sets default values
ARoomDoor::ARoomDoor()
{
	bReplicates = true;

	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));

	DoorPanel = CreateDefaultSubobject<USceneComponent>(TEXT("DoorPanelRoot"));
	DoorPanel->SetupAttachment(RootComponent);
	DoorPanel->SetIsReplicated(true);

	OpenMaxTranslation = 190.0f;
	OpenTime = 1.5f;
	bIsOpen = false;
}

void ARoomDoor::BeginPlay()
{
	Super::BeginPlay();

	OpeningTimeline = NewObject<UTimelineComponent>(this, FName("OpeningTimeline"));
	// OpeningTimeline->SetNetAddressable();
	// OpeningTimeline->SetIsReplicated(true);
	OpeningTimeline->SetLooping(false);
	OpeningTimeline->SetTimelineLength(OpenTime);
	OpeningTimeline->SetTimelineLengthMode(ETimelineLengthMode::TL_TimelineLength);
	OpeningTimeline->SetPlaybackPosition(0.0f, false);
	if (OpeningCurve)
	{
		FOnTimelineFloat TimelineCallback;
		TimelineCallback.BindDynamic(this, &ARoomDoor::OnTimelineCallback);
		OpeningTimeline->AddInterpFloat(OpeningCurve, TimelineCallback);
	}
	OpeningTimeline->RegisterComponent();
}

void ARoomDoor::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
}

void ARoomDoor::OnUse(ASCPCharacter* Subject, UPrimitiveComponent* Comp)
{
	if (!Comp) { return; }

	if (!Comp->ComponentHasTag(TEXT("BUTTON")))
	{
		return;
	}

	if (bIsOpen)
	{
		OpeningTimeline->Reverse();
	}
	else
	{
		OpeningTimeline->Play();
	}
	bIsOpen = !bIsOpen;
}

void ARoomDoor::OnHover(ASCPCharacter* Subject, UPrimitiveComponent* Comp, bool bIsStartHover)
{
	if (!Comp) { return; }

	if (!Comp->ComponentHasTag(TEXT("BUTTON")))
	{
		return;
	}

	const int32 ChildrenCount = Comp->GetNumChildrenComponents();
	if(ChildrenCount > 0)
	{
		USceneComponent* Child = Comp->GetChildComponent(0);
		IInteractive::OnHover(Subject, Cast<UPrimitiveComponent>(Child), bIsStartHover);
	}
}

void ARoomDoor::OnTimelineCallback(float Val)
{
	DoorPanel->SetRelativeLocation({Val * OpenMaxTranslation, 0.0f, 0.0f});
}
