// Fill out your copyright notice in the Description page of Project Settings.

#include "Level_PathfindingAStar.h"

#include "GraphTheory/Algorithms/AStar.h"
#include "Shared/GameAISpectator.h"

using namespace GameAI;

ALevel_PathfindingAStar::ALevel_PathfindingAStar()
{
	PrimaryActorTick.bCanEverTick = true;
}

void ALevel_PathfindingAStar::BeginPlay()
{
	Super::BeginPlay();

	if (TrimWorld)
	{
		TrimWorld->bShouldTrimWorld = false;
	}

	if (PlayerController)
	{
		if (AGameAISpectator* Player = Cast<AGameAISpectator>(PlayerController->GetPawnOrSpectator()); Player)
		{
			Player->SetCameraProjection(ECameraProjectionMode::Orthographic);
		}
	}

	Agent = GetWorld()->SpawnActor<ASteeringAgent>(SteeringAgentClass,
		FVector{0, 0, 90}, FRotator::ZeroRotator);
	if (Agent)
	{
		Agent->SetDebugRenderingEnabled(false);
		Agent->SetSteeringBehavior(&PathFollow);
	}

	Renderer = new GraphRenderer{GetWorld()};
	GraphRenderOptions RenderOptions{};
	RenderOptions.bDrawConnectionWeights = false;
	RenderOptions.bDrawConnections = false;
	RenderOptions.bDrawNodeIds = false;
	RenderOptions.bDrawNodes = false;
	Renderer->SetRenderOptions(RenderOptions);

	NodeFactory = new TerrainNodeFactory{};
	TerrainGraph = new TerrainGridGraph{NodeFactory, 10, 10, 200.0f, 1.0f,
		FVector2D{-1000.0f, -1000.0f}, false};

	CalculatePath();
}

void ALevel_PathfindingAStar::BeginDestroy()
{
	Super::BeginDestroy();

	delete Renderer;
	delete TerrainGraph;
	delete NodeFactory;
}

void ALevel_PathfindingAStar::BindLevelInputActions()
{
	Super::BindLevelInputActions();

	if (!PlayerEnhancedInputComponent)
	{
		return;
	}

	PlayerEnhancedInputComponent->BindAction(SetStartNodeAction, ETriggerEvent::Triggered, this,
		&ALevel_PathfindingAStar::SetStartNodeId);
	PlayerEnhancedInputComponent->BindAction(SetEndNodeAction, ETriggerEvent::Triggered, this,
		&ALevel_PathfindingAStar::SetEndNodeId);

	PlayerEnhancedInputComponent->BindAction(SetNodeTerrainClearAction, ETriggerEvent::Triggered, this,
		&ALevel_PathfindingAStar::SetNodeTerrain, TerrainNode::Type::Clear);
	PlayerEnhancedInputComponent->BindAction(SetNodeTerrainMudAction, ETriggerEvent::Triggered, this,
		&ALevel_PathfindingAStar::SetNodeTerrain, TerrainNode::Type::Mud);
	PlayerEnhancedInputComponent->BindAction(SetNodeTerrainWaterAction, ETriggerEvent::Triggered, this,
		&ALevel_PathfindingAStar::SetNodeTerrain, TerrainNode::Type::Water);
}

void ALevel_PathfindingAStar::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	UpdateImGui();

	if (Renderer && TerrainGraph)
	{
		Renderer->RenderGraph(*TerrainGraph);
		if (bDrawGrid)
		{
			TerrainGraph->DebugDrawCells(GetWorld());
		}
		TerrainGraph->DrawTerrain(GetWorld());
	}
}

void ALevel_PathfindingAStar::CalculatePath()
{
	if (!TerrainGraph || !Renderer)
	{
		return;
	}

	if (PathStartNodeId != Graphs::InvalidNodeId
		&& PathEndNodeId != Graphs::InvalidNodeId
		&& PathStartNodeId != PathEndNodeId)
	{
		AStar pathfinder = AStar(TerrainGraph, HeuristicFunction);
		TerrainNode* const startNode = TerrainGraph->GetNodeAs<TerrainNode>(PathStartNodeId);
		TerrainNode* const endNode = TerrainGraph->GetNodeAs<TerrainNode>(PathEndNodeId);

		FoundPath = pathfinder.FindPath(startNode, endNode);
		UE_LOG(LogTemp, Log, TEXT("New A* path calculated"));
		UpdateAgentPath(FoundPath);
	}
	else
	{
		UE_LOG(LogTemp, Log, TEXT("No valid start & end node... Start: %d, End: %d"), PathStartNodeId, PathEndNodeId);
		FoundPath.clear();
		UpdateAgentPath(FoundPath);
	}

	std::vector<std::pair<int, FColor>> PathToHighlight{};
	PathToHighlight.push_back({PathStartNodeId, FColor::Green});
	if (!FoundPath.empty())
	{
		for (int Idx = 1; Idx < static_cast<int>(FoundPath.size()) - 1; ++Idx)
		{
			PathToHighlight.push_back({FoundPath[Idx]->GetId(), FColor::Yellow});
		}
	}
	PathToHighlight.push_back({PathEndNodeId, FColor::Red});
	Renderer->SetHighlightedNodes(PathToHighlight);
}

void ALevel_PathfindingAStar::UpdateAgentPath(std::vector<Node*> const& Path)
{
	if (!Agent)
	{
		return;
	}

	std::vector<FVector2D> pathPositions{};
	pathPositions.reserve(Path.size());
	for (Node* const pNode : Path)
	{
		pathPositions.emplace_back(pNode->GetPosition());
	}

	PathFollow.SetPath(pathPositions);
	if (!pathPositions.empty())
	{
		Agent->SetPosition(pathPositions[0]);
	}
}

void ALevel_PathfindingAStar::UpdateImGui()
{
#pragma region UI
	{
		ImGui::SetNextWindowPos(WindowPos);
		ImGui::SetNextWindowSize(WindowSize);
		ImGui::Begin("Gameplay Programming", nullptr,
			ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse);

		ImGui::Text("CONTROLS");
		ImGui::Indent();
		ImGui::Text("LMB: Set Path Start");
		ImGui::Text("RMB: Set Path End");
		ImGui::Text("1: Set terrain to Clear");
		ImGui::Text("2: Set terrain to Mud");
		ImGui::Text("3: Set terrain to Water");
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

		ImGui::Text("A* Pathfinding");
		ImGui::Spacing();

		if (Renderer)
		{
			GraphRenderOptions RenderOptions = Renderer->GetRenderOptions();
			bool bOptionsChanged = false;
			ImGui::Checkbox("Grid", &bDrawGrid);
			if (ImGui::Checkbox("Nodes", &RenderOptions.bDrawNodes))
			{
				bOptionsChanged = true;
			}
			if (ImGui::Checkbox("Node Numbers", &RenderOptions.bDrawNodeIds))
			{
				bOptionsChanged = true;
			}
			if (ImGui::Checkbox("Connections", &RenderOptions.bDrawConnections))
			{
				bOptionsChanged = true;
			}
			if (ImGui::Checkbox("Connection Costs", &RenderOptions.bDrawConnectionWeights))
			{
				bOptionsChanged = true;
			}
			if (bOptionsChanged)
			{
				Renderer->SetRenderOptions(RenderOptions);
			}
		}

		if (ImGui::Combo("Heuristic", &SelectedHeuristic,
			"Manhattan\0Euclidean\0SqEuclidean\0Octile\0Chebyshev", 5))
		{
			switch (SelectedHeuristic)
			{
			case 0:
				HeuristicFunction = HeuristicFunctions::Manhattan;
				break;
			case 1:
				HeuristicFunction = HeuristicFunctions::Euclidean;
				break;
			case 2:
				HeuristicFunction = HeuristicFunctions::SqEuclidean;
				break;
			case 3:
				HeuristicFunction = HeuristicFunctions::Octile;
				break;
			default:
			case 4:
				HeuristicFunction = HeuristicFunctions::Chebyshev;
				break;
			}

			CalculatePath();
		}
		ImGui::Spacing();

		ImGui::End();
	}
#pragma endregion UI
}

void ALevel_PathfindingAStar::SetStartNodeId()
{
	if (!TerrainGraph)
	{
		return;
	}

	int const NewStart = TerrainGraph->GetNodeIdAtPosition(FVector2D{LatestMouseWorldPos});
	if (NewStart >= 0 && NewStart != PathEndNodeId)
	{
		PathStartNodeId = NewStart;
		CalculatePath();
	}
}

void ALevel_PathfindingAStar::SetEndNodeId()
{
	if (!TerrainGraph)
	{
		return;
	}

	int const NewEnd = TerrainGraph->GetNodeIdAtPosition(FVector2D{LatestMouseWorldPos});
	if (NewEnd >= 0 && NewEnd != PathStartNodeId)
	{
		PathEndNodeId = NewEnd;
		CalculatePath();
	}
}

void ALevel_PathfindingAStar::SetNodeTerrain(TerrainNode::Type TerrainType)
{
	if (!TerrainGraph)
	{
		return;
	}

	TerrainGraph->PaintNodeAtPosition(FVector2D{LatestMouseWorldPos}, TerrainType);
	CalculatePath();
}
