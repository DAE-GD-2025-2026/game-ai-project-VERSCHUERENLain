	#pragma once

#include "FlockingSteeringBehaviors.h"
#include "Movement/SteeringBehaviors/SteeringAgent.h"
#include "Movement/SteeringBehaviors/SteeringHelpers.h"
#include "Movement/SteeringBehaviors/CombinedSteering/CombinedSteeringBehaviors.h"
#include <memory>
#include "imgui.h"

// forward declarations — avoid including SpacePartitioning.h in header
class CellSpace;
class QuadTree;

class Flock final
{
public:
	Flock(
	UWorld* pWorld,
	TSubclassOf<ASteeringAgent> AgentClass,
	int FlockSize = 10,
	float WorldSize = 100.f,
	ASteeringAgent* const pAgentToEvade = nullptr,
	bool bTrimWorld = false);

	~Flock();

	void Tick(float DeltaTime);
	void RenderDebug();
	void ImGuiRender(ImVec2 const& WindowPos, ImVec2 const& WindowSize);

	void RegisterNeighbors(ASteeringAgent* const Agent);
	int GetNrOfNeighbors() const { return NrOfNeighbors; }
	const TArray<ASteeringAgent*>& GetNeighbors() const { return Neighbors; }

	FVector2D GetAverageNeighborPos() const;
	FVector2D GetAverageNeighborVelocity() const;

	void SetTarget_Seek(FSteeringParams const & Target);

	// public access for external ImGui rendering (Level_SpacePartitioning)
	bool& GetDebugRenderSteering() { return DebugRenderSteering; }
	bool& GetDebugRenderNeighborhood() { return DebugRenderNeighborhood; }
	bool& GetDebugRenderPartitions() { return DebugRenderPartitions; }
	bool& GetUseSpacePartitioning() { return bUseSpacePartitioning; }
	bool& GetUseHISP() { return bUseHISP; }
	TArray<std::unique_ptr<BlendedSteering>>& GetBlendedSteeringPerAgent() { return pBlendedSteeringPerAgent; }

private:
	// For debug rendering purposes
	UWorld* pWorld{nullptr};

	int FlockSize{0};
	TArray<ASteeringAgent*> Agents{};

	// spatial partitioning
	bool bUseSpacePartitioning{false};
	bool bUseHISP{false};
	std::unique_ptr<CellSpace> pCellSpace{};
	std::unique_ptr<QuadTree> pQuadTree{};
	TArray<FVector2D> OldPositions{};

	// unified neighbor storage (both paths write here)
	TArray<ASteeringAgent*> Neighbors{};
	int NrOfNeighbors{0};

	float NeighborhoodRadius{350.f};

	ASteeringAgent* pAgentToEvade{nullptr};

	// shared steering behaviors (one instance, all agents read same data)
	std::unique_ptr<Separation>    pSeparationBehavior{};
	std::unique_ptr<Cohesion>      pCohesionBehavior{};
	std::unique_ptr<VelocityMatch> pVelMatchBehavior{};
	std::unique_ptr<Seek>          pSeekBehavior{};
	std::unique_ptr<Evade>         pEvadeBehavior{};

	// per-agent wander (each agent needs its own wander state)
	TArray<std::unique_ptr<Wander>>           pWanderBehaviors{};
	// per-agent blended + priority (cuss blended contains a per-agent wander ptr)
	TArray<std::unique_ptr<BlendedSteering>>  pBlendedSteeringPerAgent{};
	TArray<std::unique_ptr<PrioritySteering>> pPrioritySteeringPerAgent{};

	float WorldSize{0.f};
	TSubclassOf<ASteeringAgent> AgentClass{};

	// UI and rendering
	bool DebugRenderSteering{false};
	bool DebugRenderNeighborhood{true};
	bool DebugRenderPartitions{true};

	void RenderNeighborhood();
	void RegisterNeighbors_BruteForce(ASteeringAgent* const Agent);
	void RegisterNeighbors_Partitioned(ASteeringAgent* const Agent);
	void RegisterNeighbors_QuadTree(ASteeringAgent* const Agent);
};
