// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Shared/Level_Base.h"
#include "Movement/SteeringBehaviors/Steering/SteeringBehaviors.h"
#include "Movement/SteeringBehaviors/SpacePartitioning/SpacePartitioning.h"
#include <memory>
#include "Level_SpacePartitioning.generated.h"

UCLASS()
class GAMEAIPROG_API ALevel_SpacePartitioning : public ALevel_Base
{
	GENERATED_BODY()

public:
	ALevel_SpacePartitioning();

	virtual void Tick(float DeltaTime) override;

protected:
	virtual void BeginPlay() override;

private:
	// behavior dropdown (applies to all agents at once)
	enum class BehaviorTypes
	{
		Wander,
		Seek,
		Flee,
		Arrive,

		// @ End
		Count
	};

	int SelectedBehavior{static_cast<int>(BehaviorTypes::Wander)};
	int PreviousBehavior{static_cast<int>(BehaviorTypes::Wander)};

	// agent management
	int AgentCount{500};
	int PendingAgentCount{500};
	TArray<ASteeringAgent*> Agents{};
	TArray<std::unique_ptr<ISteeringBehavior>> Behaviors{};

	// spatial partitioning (owned directly)
	std::unique_ptr<CellSpace> pCellSpace{};
	std::unique_ptr<QuadTree> pQuadTree{};
	TArray<FVector2D> OldPositions{};

	bool bUseSpacePartitioning{false};
	bool bUseHISP{false};

	// debug
	bool bDebugRenderSteering{false};
	bool bDebugRenderNeighborhood{true};
	bool bDebugRenderPartitions{true};

	// neighbor query (for debug visualization)
	float NeighborhoodRadius{350.f};
	TArray<ASteeringAgent*> Neighbors{};
	int NrOfNeighbors{0};

	float WorldSize{4000.f};

	void RebuildAgents();
	void SetAllAgentsBehavior();
	void RegisterNeighbors(ASteeringAgent* Agent);
	void RenderDebug();
};
