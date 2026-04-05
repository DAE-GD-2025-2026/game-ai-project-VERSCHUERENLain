// Fill out your copyright notice in the Description page of Project Settings.

#include "Level_Navmesh.h"

#include "NavigationSystem.h"
#include "AI/NavigationSystemBase.h"
#include "DrawDebugHelpers.h"
#include "NavMesh/RecastNavMesh.h"
#include "Runtime/Navmesh/Public/Detour/DetourNavMesh.h"
#include "Shared/GameAISpectator.h"
#include "Shared/Graph/GraphRenderer.h"

FORCEINLINE FVector RecastToUnreal(const double* RecastVertex)
{
	return FVector(
		static_cast<float>(-RecastVertex[0]),
		static_cast<float>(-RecastVertex[2]),
		static_cast<float>(RecastVertex[1])
	);
}

ALevel_Navmesh::ALevel_Navmesh()
{
	PrimaryActorTick.bCanEverTick = true;
}

void ALevel_Navmesh::BeginPlay()
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
		FVector{2100.0, 2100.0, 90}, FRotator::ZeroRotator);
	if (Agent)
	{
		Agent->SetDebugRenderingEnabled(false);
		Agent->SetSteeringBehavior(&PathFollow);
	}

	auto NavPoly = std::make_unique<TriPolygon>();
	for (TArray<FVector> const& Tri : ExtractNavMeshTris())
	{
		NavPoly->AddTriangle(Tri);
	}

	NavigationGraph = std::make_unique<GameAI::NavGraph>(std::move(NavPoly));
	Renderer = std::make_unique<GameAI::GraphRenderer>(GetWorld());

	GameAI::GraphRenderOptions RenderOptions{};
	RenderOptions.bDrawNodes = true;
	RenderOptions.bDrawNodeIds = false;
	RenderOptions.bDrawHighlightedNodes = false;
	RenderOptions.bDrawConnections = true;
	RenderOptions.bDrawConnectionWeights = false;
	Renderer->SetRenderOptions(RenderOptions);
}

void ALevel_Navmesh::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (NavigationGraph)
	{
		if (bDrawNavPoly)
		{
			NavigationGraph->GetNavPolygon()->DrawDebug(GetWorld(), FColor::Yellow);
		}

		if (bDrawNavPolyVertices)
		{
			for (const FVector& Vertex : NavigationGraph->GetNavPolygon()->GetVertices())
			{
				DrawDebugPoint(GetWorld(), Vertex, 10.0f, FColor::Cyan);
			}
		}

		if (bDrawNavGraph && Renderer)
		{
			Renderer->RenderGraph(*NavigationGraph);
		}
	}

	if (bDrawPath)
	{
		bool bSmoothedPathDiffersFromRaw = DebugDrawPath.size() != DebugNodePositions.size();
		if (!bSmoothedPathDiffersFromRaw)
		{
			for (int PathIdx = 0; PathIdx < static_cast<int>(DebugDrawPath.size()); ++PathIdx)
			{
				if (!DebugDrawPath[PathIdx].Equals(DebugNodePositions[PathIdx], 0.1f))
				{
					bSmoothedPathDiffersFromRaw = true;
					break;
				}
			}
		}

		for (int PathIdx = 1; PathIdx < static_cast<int>(DebugNodePositions.size()); ++PathIdx)
		{
			DrawDebugLine(
				GetWorld(),
				FVector{ DebugNodePositions[PathIdx - 1], 7.5f },
				FVector{ DebugNodePositions[PathIdx], 7.5f },
				FColor::Cyan, false, -1, 0, 2.0f);
		}

		if (bSmoothedPathDiffersFromRaw)
		{
			for (int PathIdx = 1; PathIdx < static_cast<int>(DebugDrawPath.size()); ++PathIdx)
			{
				DrawDebugLine(
					GetWorld(),
					FVector{ DebugDrawPath[PathIdx - 1], 5.0f },
					FVector{ DebugDrawPath[PathIdx], 5.0f },
					FColor::Magenta, false, -1, 1, 10);
			}
		}
	}

	if (bDrawPortals)
	{
		for (int PortalIdx = 0; PortalIdx < static_cast<int>(DebugPortals.size()); ++PortalIdx)
		{
			GameAI::NavLine const& Portal = DebugPortals[PortalIdx];
			DrawDebugLine(GetWorld(), FVector{ Portal.P1, 15.0f }, FVector{ Portal.P2, 15.0f },
				FColor::Green, false, -1, 0, 3.0f);
			DrawDebugPoint(GetWorld(), FVector{ Portal.P1, 15.0f }, 12.0f, FColor::Red, false, -1.0f, 0);
			DrawDebugPoint(GetWorld(), FVector{ Portal.P2, 15.0f }, 12.0f, FColor::Blue, false, -1.0f, 0);

			bool const bIsDegenerateStartOrEnd =
				Portal.P1.Equals(Portal.P2, 0.1f) && (PortalIdx == 0 || PortalIdx == static_cast<int>(DebugPortals.size()) - 1);
			if (!bIsDegenerateStartOrEnd)
			{
				FVector2D const PortalCenter = (Portal.P1 + Portal.P2) * 0.5f;
				DrawDebugString(GetWorld(), FVector{ PortalCenter, 20.0f },
					FString::Printf(TEXT("%d"), PortalIdx - 1), nullptr, FColor::White, 0.0f, false);
			}
		}
	}

	UpdateImGui();
}

void ALevel_Navmesh::BindLevelInputActions()
{
	Super::BindLevelInputActions();

	if (!PlayerEnhancedInputComponent)
	{
		return;
	}

	PlayerEnhancedInputComponent->BindAction(SetTargetAction, ETriggerEvent::Triggered,
		this, &ALevel_Navmesh::SetTarget);
}

void ALevel_Navmesh::UpdateImGui()
{
#pragma region UI
	{
		ImGui::SetNextWindowPos(WindowPos);
		ImGui::SetNextWindowSize(WindowSize);
		ImGui::Begin("Gameplay Programming", nullptr, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse);

		ImGui::Text("CONTROLS");
		ImGui::Indent();
		ImGui::Text("LMB: Set Target");
		ImGui::Unindent();

		ImGui::Spacing(); ImGui::Separator(); ImGui::Spacing(); ImGui::Spacing();

		ImGui::Text("STATS");
		ImGui::Indent();
		ImGui::Text("%.3f ms/frame", 1000.0f / ImGui::GetIO().Framerate);
		ImGui::Text("%.1f FPS", ImGui::GetIO().Framerate);
		ImGui::Unindent();

		ImGui::Spacing(); ImGui::Separator(); ImGui::Spacing(); ImGui::Spacing();

		ImGui::Text("Navmesh Pathfinding");
		ImGui::Spacing();

		ImGui::Checkbox("NavPolyVertices", &bDrawNavPolyVertices);
		ImGui::Checkbox("NavPoly", &bDrawNavPoly);
		ImGui::Checkbox("NavGraph", &bDrawNavGraph);
		ImGui::Checkbox("Path", &bDrawPath);
		ImGui::Checkbox("Portals", &bDrawPortals);
		ImGui::TextUnformatted("path legend: cyan raw, magenta smoothed");
		ImGui::TextUnformatted("portal legend: red right, blue left");

		ImGui::End();
	}
#pragma endregion
}

TArray<TArray<FVector>> ALevel_Navmesh::ExtractNavMeshTris() const
{
	TArray<TArray<FVector>> Polys{};

	ANavigationData* NavData = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld())->GetDefaultNavDataInstance();
	if (dtNavMesh const* NavMesh = Cast<ARecastNavMesh>(NavData)->GetRecastMesh())
	{
		for (int TileIdx{ 0 }; TileIdx < NavMesh->getMaxTiles(); ++TileIdx)
		{
			dtMeshTile const* Tile{ NavMesh->getTile(TileIdx) };
			if (!Tile || !Tile->header || !Tile->polys) continue;

			for (int i = 0; i < Tile->header->detailMeshCount; ++i)
			{
				const dtPolyDetail* DetailMesh = &Tile->detailMeshes[i];
				const dtPoly* Poly = &Tile->polys[i];

				for (int triIdx = 0; triIdx < DetailMesh->triCount; ++triIdx)
				{
					const unsigned char* TriData = &Tile->detailTris[(DetailMesh->triBase + triIdx) * 4];

					TArray<FVector> TriVerts{};
					for (int corner = 0; corner < 3; ++corner)
					{
						unsigned char idx = TriData[corner];
						const double* Vert;

						if (idx < Poly->vertCount)
						{
							Vert = &Tile->verts[Poly->verts[idx] * 3];
						}
						else
						{
							int detailVertIdx = DetailMesh->vertBase + (idx - Poly->vertCount);
							Vert = &Tile->detailVerts[detailVertIdx * 3];
						}

						TriVerts.Add(RecastToUnreal(Vert));
					}
					Polys.Add(TriVerts);
				}
			}
		}
	}

	return Polys;
}

void ALevel_Navmesh::SetTarget()
{
	if (!Agent || !NavigationGraph)
	{
		return;
	}

	GameAI::NavMeshPathfinding Pathfinder{};
	std::vector<FVector2D> Path = Pathfinder.FindPath(Agent->GetPosition(),
		FVector2D{ LatestMouseWorldPos }, NavigationGraph.get(), DebugNodePositions, DebugPortals);

	DebugDrawPath = Path;

	PathFollow.SetPath(Path);
	if (!Path.empty())
	{
		Agent->SetPosition(Path[0]);
	}
}
