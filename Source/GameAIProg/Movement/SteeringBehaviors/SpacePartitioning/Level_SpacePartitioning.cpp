// Fill out your copyright notice in the Description page of Project Settings.

#include "Level_SpacePartitioning.h"
#include "imgui.h"
#include "Shared/ImGuiHelpers.h"


ALevel_SpacePartitioning::ALevel_SpacePartitioning()
{
	PrimaryActorTick.bCanEverTick = true;
}

void ALevel_SpacePartitioning::BeginPlay()
{
	Super::BeginPlay();
	RebuildFlock();
}

void ALevel_SpacePartitioning::RebuildFlock()
{
	TrimWorld->SetTrimWorldSize(3000.f);
	TrimWorld->bShouldTrimWorld = true;

	// destroy old evade agent if valid
	if (IsValid(pAgentToEvade))
	{
		pAgentToEvade->Destroy();
		pAgentToEvade = nullptr;
	}
	pEvadeAgentWander.reset();

	// destroy old flock (destroys all flock agents)
	pFlock.Reset();

	// spawn evade agent
	UClass* evadeClass = LoadObject<UClass>(nullptr, TEXT("/Game/Movement/Steering/BP_EvadeAgent.BP_EvadeAgent_C"));
	if (!evadeClass) evadeClass = SteeringAgentClass; // fallback
	pAgentToEvade = GetWorld()->SpawnActor<ASteeringAgent>(evadeClass, FVector{0.f, 0.f, 90.f}, FRotator::ZeroRotator);
	if (IsValid(pAgentToEvade))
	{
		pEvadeAgentWander = std::make_unique<Wander>();
		pAgentToEvade->SetSteeringBehavior(pEvadeAgentWander.get());
	}

	// apply pending size
	FlockSize = PendingFlockSize;

	pFlock = TUniquePtr<Flock>(
		new Flock(
			GetWorld(),
			SteeringAgentClass,
			FlockSize,
			TrimWorld->GetTrimWorldSize(),
			pAgentToEvade,
			true)
	);
}

void ALevel_SpacePartitioning::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

#ifdef PLATFORM_WINDOWS
#pragma region UI
	{
		bool bWindowActive = true;
		ImGui::SetNextWindowPos(WindowPos);
		ImGui::SetNextWindowSize(WindowSize);
		ImGui::Begin("Gameplay Programming", &bWindowActive, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse);

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
		ImGui::Text("Agents: %d", FlockSize);
		ImGui::Unindent();

		ImGui::Spacing();
		ImGui::Separator();
		ImGui::Spacing();

		ImGui::Text("Flocking");
		ImGui::Spacing();

		// flock size control
		ImGui::SliderInt("Flock Size", &PendingFlockSize, 10, 2000);
		if (PendingFlockSize != FlockSize)
		{
			if (ImGui::Button("Apply"))
				RebuildFlock();
			ImGui::SameLine();
			ImGui::TextDisabled("(%d -> %d)", FlockSize, PendingFlockSize);
		}

		ImGui::Spacing();

		if (pFlock)
		{
			ImGui::Checkbox("Use Space Partitioning", &pFlock->GetUseSpacePartitioning());
			ImGui::Checkbox("Debug render neighborhood", &pFlock->GetDebugRenderNeighborhood());
			ImGui::Checkbox("Debug render steering", &pFlock->GetDebugRenderSteering());
			ImGui::Checkbox("Debug render partitions", &pFlock->GetDebugRenderPartitions());

			ImGui::Spacing();
			ImGui::Text("Behavior Weights");
			ImGui::Spacing();

			auto& blended = pFlock->GetBlendedSteeringPerAgent();
			if (blended.Num() > 0 && blended[0])
			{
				auto& w0 = blended[0]->GetWeightedBehaviorsRef();

				auto SetAllWeights = [&](int idx, float v)
				{
					for (auto& blend : blended)
						if (blend) blend->GetWeightedBehaviorsRef()[idx].Weight = v;
				};

				ImGuiHelpers::ImGuiSliderFloatWithSetter("Cohesion",   w0[0].Weight, 0.f, 1.f, [&](float v){ SetAllWeights(0, v); });
				ImGuiHelpers::ImGuiSliderFloatWithSetter("Separation", w0[1].Weight, 0.f, 1.f, [&](float v){ SetAllWeights(1, v); });
				ImGuiHelpers::ImGuiSliderFloatWithSetter("Alignment",  w0[2].Weight, 0.f, 1.f, [&](float v){ SetAllWeights(2, v); });
				ImGuiHelpers::ImGuiSliderFloatWithSetter("Seek",       w0[3].Weight, 0.f, 1.f, [&](float v){ SetAllWeights(3, v); });
				ImGuiHelpers::ImGuiSliderFloatWithSetter("Wander",     w0[4].Weight, 0.f, 1.f, [&](float v){ SetAllWeights(4, v); });
			}
		}

		ImGui::End();
	}
#pragma endregion
#endif

	if (pFlock)
	{
		pFlock->Tick(DeltaTime);
		pFlock->RenderDebug();

		if (bUseMouseTarget)
			pFlock->SetTarget_Seek(MouseTarget);
	}
}
