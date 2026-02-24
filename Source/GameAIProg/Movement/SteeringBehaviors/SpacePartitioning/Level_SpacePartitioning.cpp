#include "Level_SpacePartitioning.h"
#include "imgui.h"
#include "Shared/ImGuiHelpers.h"
#include "DrawDebugHelpers.h"


ALevel_SpacePartitioning::ALevel_SpacePartitioning()
{
	PrimaryActorTick.bCanEverTick = true;
}

void ALevel_SpacePartitioning::BeginPlay()
{
	Super::BeginPlay();
	TrimWorld->SetTrimWorldSize(WorldSize);
	TrimWorld->bShouldTrimWorld = true;
	RebuildAgents();
}

void ALevel_SpacePartitioning::RebuildAgents()
{
	// destroy old agents
	for (ASteeringAgent* agent : Agents)
		if (IsValid(agent)) agent->Destroy();
	Agents.Empty();
	Behaviors.Empty();
	OldPositions.Empty();

	AgentCount = PendingAgentCount;
	float halfSize = WorldSize * 0.5f;

	Agents.SetNum(AgentCount);
	Behaviors.SetNum(AgentCount);
	OldPositions.SetNum(AgentCount);
	Neighbors.SetNum(AgentCount);

	// spawn agents at random positions
	for (int i = 0; i < AgentCount; ++i)
	{
		FVector spawnPos{
			FMath::RandRange(-halfSize, halfSize),
			FMath::RandRange(-halfSize, halfSize),
			90.f
		};
		Agents[i] = GetWorld()->SpawnActor<ASteeringAgent>(SteeringAgentClass, spawnPos, FRotator::ZeroRotator);
		if (IsValid(Agents[i]))
			OldPositions[i] = Agents[i]->GetPosition();
	}

	// create flat 25x25 grid
	pCellSpace = std::make_unique<CellSpace>(GetWorld(), WorldSize, WorldSize, 25, 25, AgentCount);
	for (int i = 0; i < AgentCount; ++i)
		if (IsValid(Agents[i])) pCellSpace->AddAgent(*Agents[i]);

	// create quadtree
	FRect quadBounds;
	quadBounds.Min = { -halfSize, -halfSize };
	quadBounds.Max = {  halfSize,  halfSize };
	pQuadTree = std::make_unique<QuadTree>(GetWorld(), quadBounds, AgentCount, 4, 8);

	SetAllAgentsBehavior();
}

void ALevel_SpacePartitioning::SetAllAgentsBehavior()
{
	// restore max speed if switching away from arrive
	if (PreviousBehavior == static_cast<int>(BehaviorTypes::Arrive))
	{
		for (int i = 0; i < AgentCount; ++i)
		{
			if (!IsValid(Agents[i]) || !Behaviors[i]) continue;
			Arrive* arrive = Behaviors[i]->As<Arrive>();
			if (arrive && arrive->GetOriginalMaxSpeed() > 0.f)
				Agents[i]->SetMaxLinearSpeed(arrive->GetOriginalMaxSpeed());
		}
	}
	PreviousBehavior = SelectedBehavior;

	for (int i = 0; i < AgentCount; ++i)
	{
		if (!IsValid(Agents[i])) continue;

		Behaviors[i].reset();

		switch (static_cast<BehaviorTypes>(SelectedBehavior))
		{
		case BehaviorTypes::Wander:
			Behaviors[i] = std::make_unique<Wander>();
			break;
		case BehaviorTypes::Seek:
			Behaviors[i] = std::make_unique<Seek>();
			break;
		case BehaviorTypes::Flee:
			Behaviors[i] = std::make_unique<Flee>();
			break;
		case BehaviorTypes::Arrive:
			Behaviors[i] = std::make_unique<Arrive>();
			break;
		default:
			Behaviors[i] = std::make_unique<Wander>();
			break;
		}

		Agents[i]->SetSteeringBehavior(Behaviors[i].get());
	}
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
		ImGui::Text("Agents: %d", AgentCount);
		ImGui::Unindent();

		ImGui::Spacing();
		ImGui::Separator();
		ImGui::Spacing();

		ImGui::Text("Space Partitioning");
		ImGui::Spacing();

		// agent count slider + apply
		ImGui::SliderInt("Agent Count", &PendingAgentCount, 10, 2000);
		if (PendingAgentCount != AgentCount)
		{
			if (ImGui::Button("Apply"))
				RebuildAgents();
			ImGui::SameLine();
			ImGui::TextDisabled("(%d -> %d)", AgentCount, PendingAgentCount);
		}

		ImGui::Spacing();

		// behavior dropdown
		if (ImGui::Combo("Behavior", &SelectedBehavior, "Wander\0Seek\0Flee\0Arrive\0"))
			SetAllAgentsBehavior();

		ImGui::Spacing();

		// spatial partitioning toggles
		bool prevUseSP = bUseSpacePartitioning;
		ImGui::Checkbox("Use Space Partitioning", &bUseSpacePartitioning);
		// re-add agents to cellspace when toggling on
		if (bUseSpacePartitioning && !prevUseSP && pCellSpace)
		{
			pCellSpace->EmptyCells();
			for (int i = 0; i < Agents.Num(); ++i)
			{
				if (IsValid(Agents[i]))
				{
					pCellSpace->AddAgent(*Agents[i]);
					OldPositions[i] = Agents[i]->GetPosition();
				}
			}
		}
		ImGui::Checkbox("Use HISP", &bUseHISP);

		ImGui::Spacing();

		// debug toggles
		ImGui::Checkbox("Debug render neighborhood", &bDebugRenderNeighborhood);
		ImGui::Checkbox("Debug render steering", &bDebugRenderSteering);
		ImGui::Checkbox("Debug render partitions", &bDebugRenderPartitions);

		ImGui::End();
	}
#pragma endregion
#endif

	// set target for behaviors that need it
	if (SelectedBehavior != static_cast<int>(BehaviorTypes::Wander))
	{
		for (int i = 0; i < AgentCount; ++i)
		{
			if (IsValid(Agents[i]) && Behaviors[i])
				Behaviors[i]->SetTarget(MouseTarget);
		}
	}

	// update quadtree (rebuild each frame)
	if (bUseHISP && pQuadTree)
	{
		pQuadTree->Clear();
		for (ASteeringAgent* agent : Agents)
			if (IsValid(agent)) pQuadTree->Insert(agent);
	}

	// update flat grid cells
	if (bUseSpacePartitioning && !bUseHISP && pCellSpace)
	{
		for (int i = 0; i < Agents.Num(); ++i)
		{
			if (!IsValid(Agents[i])) continue;
			pCellSpace->UpdateAgentCell(*Agents[i], OldPositions[i]);
			OldPositions[i] = Agents[i]->GetPosition();
		}
	}

	RenderDebug();
}

void ALevel_SpacePartitioning::RegisterNeighbors(ASteeringAgent* Agent)
{
	if (bUseHISP && pQuadTree)
	{
		pQuadTree->RegisterNeighbors(*Agent, NeighborhoodRadius);
		NrOfNeighbors = pQuadTree->GetNrOfNeighbors();
		const auto& qtNeighbors = pQuadTree->GetNeighbors();
		for (int i = 0; i < NrOfNeighbors; ++i)
			Neighbors[i] = qtNeighbors[i];
	}
	else if (bUseSpacePartitioning && pCellSpace)
	{
		pCellSpace->RegisterNeighbors(*Agent, NeighborhoodRadius);
		NrOfNeighbors = pCellSpace->GetNrOfNeighbors();
		const auto& cellNeighbors = pCellSpace->GetNeighbors();
		for (int i = 0; i < NrOfNeighbors; ++i)
			Neighbors[i] = cellNeighbors[i];
	}
	else
	{
		// brute force
		NrOfNeighbors = 0;
		FVector2D agentPos = Agent->GetPosition();
		for (ASteeringAgent* other : Agents)
		{
			if (other == Agent || !IsValid(other)) continue;
			float dist = FVector2D::Distance(agentPos, other->GetPosition());
			if (dist < NeighborhoodRadius)
			{
				Neighbors[NrOfNeighbors] = other;
				++NrOfNeighbors;
			}
		}
	}
}

void ALevel_SpacePartitioning::RenderDebug()
{
	if (bDebugRenderSteering)
	{
		for (ASteeringAgent* agent : Agents)
		{
			if (!IsValid(agent)) continue;
			FVector agentPos = agent->GetActorLocation();
			FVector2D vel2D = agent->GetLinearVelocity();
			FVector velEnd = agentPos + FVector{vel2D.X, vel2D.Y, 0.f} * 0.3f;
			DrawDebugLine(GetWorld(), agentPos, velEnd, FColor::Cyan, false, -1.f, 0, 3.f);
			DrawDebugPoint(GetWorld(), velEnd, 8.f, FColor::Cyan, false, -1.f);
		}
	}

	if (bDebugRenderNeighborhood && Agents.Num() > 0 && IsValid(Agents[0]))
	{
		RegisterNeighbors(Agents[0]);
		FVector firstPos = Agents[0]->GetActorLocation();
		DrawDebugCircle(GetWorld(), firstPos, NeighborhoodRadius, 32,
			FColor::Yellow, false, -1.f, 0, 3.f, FVector::YAxisVector, FVector::XAxisVector);
		for (int i = 0; i < NrOfNeighbors; ++i)
		{
			if (!IsValid(Neighbors[i])) continue;
			DrawDebugLine(GetWorld(), firstPos, Neighbors[i]->GetActorLocation(), FColor::Green, false, -1.f, 0, 3.f);
		}
	}

	if (bDebugRenderPartitions)
	{
		if (bUseHISP && pQuadTree)
		{
			std::vector<FRect> highlighted;
			if (Agents.Num() > 0 && IsValid(Agents[0]))
			{
				pQuadTree->RegisterNeighbors(*Agents[0], NeighborhoodRadius);
				highlighted = pQuadTree->GetLastQueriedLeaves();
			}
			pQuadTree->RenderCells(highlighted);
		}
		else if (pCellSpace)
		{
			std::vector<int> highlighted;
			if (bUseSpacePartitioning && Agents.Num() > 0 && IsValid(Agents[0]))
			{
				pCellSpace->RegisterNeighbors(*Agents[0], NeighborhoodRadius);
				highlighted = pCellSpace->GetLastQueriedCells();
			}
			pCellSpace->RenderCells(highlighted);
		}
	}
}
