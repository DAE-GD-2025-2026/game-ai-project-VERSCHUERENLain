#include "Level_Base.h"

// Sets default values
ALevel_Base::ALevel_Base()
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
}

// Called when the game starts or when spawned
void ALevel_Base::BeginPlay()
{
	Super::BeginPlay();

	// Set ImGui defaults
	FImGuiModule::Get().GetProperties().SetInputEnabled(true);
	FImGuiModule::Get().GetProperties().SetMouseInputShared(true);
	FImGuiModule::Get().GetProperties().SetKeyboardNavigationEnabled(false);
	FImGuiModule::Get().GetProperties().SetKeyboardInputShared(true);
	FImGuiModule::Get().GetProperties().SetGamepadInputShared(true);
	FImGuiModule::Get().GetProperties().SetGamepadNavigationEnabled(false);

	// Spawn our trimworld
	TrimWorld = GetWorld()->SpawnActor<AWorldTrimVolume>(FVector{0,0,0}, FRotator::ZeroRotator);

	// Get needed input binding vars
	SetupEnhancedInputAttachment();
	if (CanBindLevelInput())
	{
		BindLevelInputMappingContexts();
		BindLevelInputActions();
	}
}

// Called every frame
void ALevel_Base::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// Update viewport data
	GetWorld()->GetGameViewport()->GetViewportSize(ViewportSize);
	WindowSize = {MenuWidth, static_cast<float>(ViewportSize.Y) - 20};
	WindowPos = {static_cast<float>(ViewportSize.X) - MenuWidth - 10, 10};

	// Update mouse pos
	UpdateLatestMouseWorldPos();

	// 	//Render Target
	// 	if(VisualizeMouseTarget)
	// 		DEBUGRENDERER2D->DrawSolidCircle(MouseTarget.Position, 0.3f, { 0.f,0.f }, { 1.f,0.f,0.f },-0.8f);
}

bool ALevel_Base::CanBindLevelInput() const
{
	return PlayerController && EnhancedInputSubsystem && PlayerEnhancedInputComponent;
}

void ALevel_Base::SetupEnhancedInputAttachment()
{
	PlayerController = GetWorld()->GetFirstPlayerController();
	if (!PlayerController)
	{
		return;
	}

	PlayerEnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerController->InputComponent);
	if (!PlayerEnhancedInputComponent)
	{
		UE_LOG(LogTemp, Error,
			TEXT("Player Controller is not using Enhanced Input. Set InputComponentClass to UEnhancedInputComponent."));
		return;
	}

	ULocalPlayer* LocalPlayer = PlayerController->GetLocalPlayer();
	if (!LocalPlayer)
	{
		return;
	}

	EnhancedInputSubsystem = LocalPlayer->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>();
}

void ALevel_Base::BindLevelInputMappingContexts()
{
	for (auto& InputMappingContextPair : InputMappingContexts)
	{
		if (!InputMappingContextPair.InputMappingContext)
		{
			continue;
		}

		EnhancedInputSubsystem->AddMappingContext(InputMappingContextPair.InputMappingContext,
			InputMappingContextPair.Priority);
	}
}

void ALevel_Base::BindLevelInputActions()
{
	// implement your own in derived levels
}

std::optional<FVector> ALevel_Base::GetMouseWorldPos() const
{
	if (!PlayerController)
	{
		return std::nullopt;
	}

	FVector MouseWorldPos{};
	FVector MouseWorldDirection{};
	PlayerController->DeprojectMousePositionToWorld(MouseWorldPos, MouseWorldDirection);

	float constexpr MaxTraceDistance{20000.0f};

	if (FHitResult HitResult{};
		GetWorld()->LineTraceSingleByChannel(HitResult, MouseWorldPos,
			MouseWorldPos + MouseWorldDirection * MaxTraceDistance, ECC_Visibility))
	{
		return HitResult.Location;
	}

	return std::nullopt;
}

void ALevel_Base::UpdateLatestMouseWorldPos()
{
	if (auto const MouseWorldResult = GetMouseWorldPos(); MouseWorldResult.has_value())
	{
		LatestMouseWorldPos = MouseWorldResult.value();
	}
}

