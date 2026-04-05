// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "InputAction.h"
#include <memory>
#include <vector>
#include "Movement/SteeringBehaviors/PathFollow/PathFollowSteeringBehavior.h"
#include "Shared/Level_Base.h"
#include "Shared/Graph/NavGraph/NavGraph.h"
#include "Shared/Graph/GraphRenderer.h"
#include "GraphTheory/Algorithms/NavGraphPathfinding.h"
#include "Level_Navmesh.generated.h"

UCLASS()
class GAMEAIPROG_API ALevel_Navmesh : public ALevel_Base
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NavmeshLevel|Input")
	UInputAction* SetTargetAction{};

	ALevel_Navmesh();

	virtual void Tick(float DeltaTime) override;
	virtual void BindLevelInputActions() override;

protected:
	virtual void BeginPlay() override;

private:
	std::unique_ptr<GameAI::NavGraph> NavigationGraph;
	std::unique_ptr<GameAI::GraphRenderer> Renderer;

	UPROPERTY()
	ASteeringAgent* Agent{nullptr};
	PathFollow PathFollow{};
	std::vector<FVector2D> DebugDrawPath{};
	std::vector<FVector2D> DebugNodePositions{};
	std::vector<GameAI::NavLine> DebugPortals{};

	bool bDrawNavPolyVertices{false};
	bool bDrawNavPoly{true};
	bool bDrawNavGraph{true};
	bool bDrawPath{true};
	bool bDrawPortals{false};

	void UpdateImGui();

	TArray<TArray<FVector>> ExtractNavMeshTris() const;

	void SetTarget();
};
