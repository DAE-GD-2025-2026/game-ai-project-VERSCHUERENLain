#include "Flock.h"
#include "FlockingSteeringBehaviors.h"
#include "Shared/ImGuiHelpers.h"
#include "DrawDebugHelpers.h"


Flock::Flock(
	UWorld* pWorld,
	TSubclassOf<ASteeringAgent> AgentClass,
	int FlockSize,
	float WorldSize,
	ASteeringAgent* const pAgentToEvade,
	bool bTrimWorld)
	: pWorld{pWorld}
	, FlockSize{ FlockSize }
	, pAgentToEvade{pAgentToEvade}
	, WorldSize{WorldSize}
	, AgentClass{AgentClass}
{
	Agents.SetNum(FlockSize);

	// memory pool: fixed-size array, NrOfNeighbors is the active element count
	Neighbors.SetNum(FlockSize);

	// spawn agents at random posiitons within the world bounds
	float halfSize = WorldSize * 0.5f;
	for (int i = 0; i < FlockSize; ++i)
	{
		FVector spawnPos{
			FMath::RandRange(-halfSize, halfSize),
			FMath::RandRange(-halfSize, halfSize),
			90.f
		};
		Agents[i] = pWorld->SpawnActor<ASteeringAgent>(AgentClass, spawnPos, FRotator::ZeroRotator);
		if (IsValid(Agents[i]))
			Agents[i]->SetActorTickEnabled(false); // manually driven from Flock::Tick
	}

	// create shared behaviors (same state for all agents is fine)
	pCohesionBehavior   = std::make_unique<Cohesion>(this);
	pSeparationBehavior = std::make_unique<Separation>(this);
	pVelMatchBehavior   = std::make_unique<VelocityMatch>(this);
	pSeekBehavior       = std::make_unique<Seek>();
	pEvadeBehavior      = std::make_unique<Evade>();

	// per-agent wander, blended, and priority
	// wander has per-agent mutable state (angle + target), so one instance per agent is required
	pWanderBehaviors.SetNum(FlockSize);
	pBlendedSteeringPerAgent.SetNum(FlockSize);
	pPrioritySteeringPerAgent.SetNum(FlockSize);

	for (int i = 0; i < FlockSize; ++i)
	{
		pWanderBehaviors[i] = std::make_unique<Wander>();

		pBlendedSteeringPerAgent[i] = std::make_unique<BlendedSteering>(std::vector<BlendedSteering::WeightedBehavior>{
			{pCohesionBehavior.get(),   0.2f},
			{pSeparationBehavior.get(), 0.8f},
			{pVelMatchBehavior.get(),   0.4f},
			{pSeekBehavior.get(),       0.2f},
			{pWanderBehaviors[i].get(), 0.2f},
		});

		pPrioritySteeringPerAgent[i] = std::make_unique<PrioritySteering>(std::vector<ISteeringBehavior*>{
			pEvadeBehavior.get(),
			pBlendedSteeringPerAgent[i].get(),
		});

		if (IsValid(Agents[i]))
			Agents[i]->SetSteeringBehavior(pPrioritySteeringPerAgent[i].get());
	}
}

Flock::~Flock()
{
	for (ASteeringAgent* agent : Agents)
		if (IsValid(agent)) agent->Destroy();
}

void Flock::Tick(float DeltaTime)
{
	// keep evade target in sync with the evade agent
	if (IsValid(pAgentToEvade) && pEvadeBehavior)
	{
		FSteeringParams evadeTarget{};
		evadeTarget.Position       = pAgentToEvade->GetPosition();
		evadeTarget.Orientation    = pAgentToEvade->GetRotation();
		evadeTarget.LinearVelocity = pAgentToEvade->GetLinearVelocity();
		pEvadeBehavior->SetTarget(evadeTarget);
	}

	for (ASteeringAgent* agent : Agents)
	{
		if (!IsValid(agent)) continue;
		RegisterNeighbors(agent); // fill memory pool for this specific agent
		agent->Tick(DeltaTime);   // neighbors valid only during this call
	}
}

void Flock::RenderDebug()
{
	if (DebugRenderSteering)
	{
		// draw velocity arrow for each agent
		for (ASteeringAgent* agent : Agents)
		{
			if (!IsValid(agent)) continue;
			FVector agentPos = agent->GetActorLocation();
			FVector2D vel2D = agent->GetLinearVelocity();
			FVector velEnd = agentPos + FVector{vel2D.X, vel2D.Y, 0.f} * 0.3f;
			DrawDebugLine(pWorld, agentPos, velEnd, FColor::Cyan, false, -1.f, 0, 3.f);
			DrawDebugPoint(pWorld, velEnd, 8.f, FColor::Cyan, false, -1.f);
		}
	}

	if (DebugRenderNeighborhood)
		RenderNeighborhood();
}

void Flock::ImGuiRender(ImVec2 const& WindowPos, ImVec2 const& WindowSize)
{
#ifdef PLATFORM_WINDOWS
#pragma region UI
	//UI
	{
		//Setup
		bool bWindowActive = true;
		ImGui::SetNextWindowPos(WindowPos);
		ImGui::SetNextWindowSize(WindowSize);
		ImGui::Begin("Gameplay Programming", &bWindowActive, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse);

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

		ImGui::Text("Flocking");
		ImGui::Spacing();

		ImGui::Checkbox("Debug Neighborhood", &DebugRenderNeighborhood);
		ImGui::Checkbox("Debug Steering", &DebugRenderSteering);

		ImGui::Spacing();
		ImGui::Text("Behavior Weights");
		ImGui::Spacing();

		// use agent 0's weights for display; setters propagate the change to all agents
		if (pBlendedSteeringPerAgent.Num() > 0 && pBlendedSteeringPerAgent[0])
		{
			auto& w0 = pBlendedSteeringPerAgent[0]->GetWeightedBehaviorsRef();

			auto SetAllWeights = [&](int idx, float v)
			{
				for (auto& blend : pBlendedSteeringPerAgent)
					if (blend) blend->GetWeightedBehaviorsRef()[idx].Weight = v;
			};

			ImGuiHelpers::ImGuiSliderFloatWithSetter("Cohesion",   w0[0].Weight, 0.f, 1.f, [&](float v){ SetAllWeights(0, v); });
			ImGuiHelpers::ImGuiSliderFloatWithSetter("Separation", w0[1].Weight, 0.f, 1.f, [&](float v){ SetAllWeights(1, v); });
			ImGuiHelpers::ImGuiSliderFloatWithSetter("Alignment",  w0[2].Weight, 0.f, 1.f, [&](float v){ SetAllWeights(2, v); });
			ImGuiHelpers::ImGuiSliderFloatWithSetter("Seek",       w0[3].Weight, 0.f, 1.f, [&](float v){ SetAllWeights(3, v); });
			ImGuiHelpers::ImGuiSliderFloatWithSetter("Wander",     w0[4].Weight, 0.f, 1.f, [&](float v){ SetAllWeights(4, v); });
		}

		//End
		ImGui::End();
	}
#pragma endregion
#endif
}

void Flock::RenderNeighborhood()
{
	if (Agents.Num() == 0 || !IsValid(Agents[0])) return;

	// re-register to get current neighbors for agent 0
	RegisterNeighbors(Agents[0]);

	FVector firstPos = Agents[0]->GetActorLocation();

	// yellow circle showing neighborhood radius around first agent
	DrawDebugCircle(pWorld, firstPos, NeighborhoodRadius, 32,
		FColor::Yellow, false, -1.f, 0, 3.f, FVector::YAxisVector, FVector::XAxisVector);

	// green lines to each current neighbor
	for (int i = 0; i < NrOfNeighbors; ++i)
	{
		if (!IsValid(Neighbors[i])) continue;
		DrawDebugLine(pWorld, firstPos, Neighbors[i]->GetActorLocation(), FColor::Green, false, -1.f, 0, 3.f);
	}
}

#ifndef GAMEAI_USE_SPACE_PARTITIONING
void Flock::RegisterNeighbors(ASteeringAgent* const pAgent)
{
	NrOfNeighbors = 0;
	FVector2D agentPos = pAgent->GetPosition();

	for (ASteeringAgent* other : Agents)
	{
		if (other == pAgent || !IsValid(other)) continue;
		float dist = FVector2D::Distance(agentPos, other->GetPosition());
		if (dist < NeighborhoodRadius)
		{
			// memory pool: overwrite at index, no push_back or clear
			Neighbors[NrOfNeighbors] = other;
			++NrOfNeighbors;
		}
	}
}
#endif

FVector2D Flock::GetAverageNeighborPos() const
{
	FVector2D avgPosition = FVector2D::ZeroVector;

	for (int i = 0; i < NrOfNeighbors; ++i)
		avgPosition += Neighbors[i]->GetPosition();

	if (NrOfNeighbors > 0)
		avgPosition /= static_cast<float>(NrOfNeighbors);

	return avgPosition;
}

FVector2D Flock::GetAverageNeighborVelocity() const
{
	FVector2D avgVelocity = FVector2D::ZeroVector;

	for (int i = 0; i < NrOfNeighbors; ++i)
		avgVelocity += Neighbors[i]->GetLinearVelocity();

	if (NrOfNeighbors > 0)
		avgVelocity /= static_cast<float>(NrOfNeighbors);

	return avgVelocity;
}

void Flock::SetTarget_Seek(FSteeringParams const& Target)
{
	if (pSeekBehavior)
		pSeekBehavior->SetTarget(Target);
}
