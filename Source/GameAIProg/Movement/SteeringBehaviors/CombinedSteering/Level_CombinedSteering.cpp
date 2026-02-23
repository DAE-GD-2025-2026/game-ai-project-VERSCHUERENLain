#include "Level_CombinedSteering.h"

#include <memory>
#include "imgui.h"
#include "DrawDebugHelpers.h"


// Sets default values
ALevel_CombinedSteering::ALevel_CombinedSteering()
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
}

// Called when the game starts or when spawned
void ALevel_CombinedSteering::BeginPlay()
{
	Super::BeginPlay();

	// spawn wanderer agent — plain wander
	WandererAgent = GetWorld()->SpawnActor<ASteeringAgent>(SteeringAgentClass, FVector{200, 0, 90}, FRotator::ZeroRotator);
	pWanderBehavior2 = std::make_unique<Wander>();
	WandererAgent->SetSteeringBehavior(pWanderBehavior2.get());

	// spawn seeker agent — priority: evade wanderer first, else blended seek+wander
	SeekerAgent = GetWorld()->SpawnActor<ASteeringAgent>(SteeringAgentClass, FVector{-200, 0, 90}, FRotator::ZeroRotator);

	pSeekBehavior    = std::make_unique<Seek>();
	pWanderBehavior1 = std::make_unique<Wander>();
	pEvadeBehavior   = std::make_unique<Evade>();

	pBlendedSteering = std::make_unique<BlendedSteering>(std::vector<BlendedSteering::WeightedBehavior>{
		{pSeekBehavior.get(),    0.5f},
		{pWanderBehavior1.get(), 0.5f}
	});

	pPrioritySteering = std::make_unique<PrioritySteering>(std::vector<ISteeringBehavior*>{
		pEvadeBehavior.get(),
		pBlendedSteering.get()
	});

	SeekerAgent->SetSteeringBehavior(pPrioritySteering.get());
	SeekerAgent->SetDebugRenderingEnabled(true);
}

void ALevel_CombinedSteering::BeginDestroy()
{
	Super::BeginDestroy();

}

// Called every frame
void ALevel_CombinedSteering::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

#pragma region UI
	//UI
	{
		//Setup
		bool windowActive = true;
		ImGui::SetNextWindowPos(WindowPos);
		ImGui::SetNextWindowSize(WindowSize);
		ImGui::Begin("Game AI", &windowActive, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse);

		//Elements
		ImGui::Text("CONTROLS");
		ImGui::Indent();
		ImGui::Text("LMB: place target");
		ImGui::Text("RMB: move cam.");
		ImGui::Text("Scrollwheel: zoom cam.");
		ImGui::Unindent();

		ImGui::Spacing();
		ImGui::Separator();
		ImGui::Spacing();
		ImGui::Spacing();

		ImGui::Text("STATS");
		ImGui::Indent();
		ImGui::Text("%.3f ms/frame", 1000.0f / ImGui::GetIO().Framerate);
		ImGui::Text("%.1f FPS", ImGui::GetIO().Framerate);
		ImGui::Unindent();

		ImGui::Spacing();
		ImGui::Separator();
		ImGui::Spacing();
		ImGui::Spacing();

		ImGui::Text("Combined Steering");
		ImGui::Spacing();
		ImGui::Spacing();

		if (ImGui::Checkbox("Debug Rendering", &CanDebugRender))
		{
			if (SeekerAgent)  SeekerAgent->SetDebugRenderingEnabled(CanDebugRender);
			if (WandererAgent) WandererAgent->SetDebugRenderingEnabled(CanDebugRender);
		}
		ImGui::Checkbox("Trim World", &TrimWorld->bShouldTrimWorld);
		if (TrimWorld->bShouldTrimWorld)
		{
			ImGuiHelpers::ImGuiSliderFloatWithSetter("Trim Size",
				TrimWorld->GetTrimWorldSize(), 1000.f, 3000.f,
				[this](float InVal) { TrimWorld->SetTrimWorldSize(InVal); });
		}

		ImGui::Spacing();
		ImGui::Spacing();
		ImGui::Spacing();

		ImGui::Text("Behavior Weights");
		ImGui::Spacing();

		if (pBlendedSteering)
		{
			ImGuiHelpers::ImGuiSliderFloatWithSetter("Seek",
				pBlendedSteering->GetWeightedBehaviorsRef()[0].Weight, 0.f, 1.f,
				[this](float InVal) { pBlendedSteering->GetWeightedBehaviorsRef()[0].Weight = InVal; }, "%.2f");

			ImGuiHelpers::ImGuiSliderFloatWithSetter("Wander",
				pBlendedSteering->GetWeightedBehaviorsRef()[1].Weight, 0.f, 1.f,
				[this](float InVal) { pBlendedSteering->GetWeightedBehaviorsRef()[1].Weight = InVal; }, "%.2f");
		}

		//End
		ImGui::End();
	}
#pragma endregion

	// update seek target to mouse position
	if (pSeekBehavior)
		pSeekBehavior->SetTarget(MouseTarget);

	// update evade target to the wanderer agent
	if (pEvadeBehavior && WandererAgent)
	{
		FTargetData wandererData;
		wandererData.Position        = WandererAgent->GetPosition();
		wandererData.Orientation     = WandererAgent->GetRotation();
		wandererData.LinearVelocity  = WandererAgent->GetLinearVelocity();
		wandererData.AngularVelocity = WandererAgent->GetAngularVelocity();
		pEvadeBehavior->SetTarget(wandererData);
	}

	// debug drawing
	if (CanDebugRender && SeekerAgent)
	{
		FVector seekerPos = SeekerAgent->GetActorLocation();

		// line to mouse target
		FVector mousePos{MouseTarget.Position.X, MouseTarget.Position.Y, seekerPos.Z};
		DrawDebugLine(GetWorld(), seekerPos, mousePos, FColor::Cyan, false, -1.f, 0, 1.f);
		DrawDebugPoint(GetWorld(), mousePos, 10.f, FColor::Cyan, false, -1.f);

		// evade radius around wanderer
		if (WandererAgent && pEvadeBehavior)
		{
			FVector wandererPos = WandererAgent->GetActorLocation();
			DrawDebugCircle(GetWorld(), wandererPos, pEvadeBehavior->GetFleeRadius(), 32,
				FColor::Orange, false, -1.f, 0, 1.f, FVector::YAxisVector, FVector::XAxisVector);
		}
	}
}
