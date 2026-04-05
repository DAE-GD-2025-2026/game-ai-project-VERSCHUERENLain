#include "NavGraphPathfinding.h"

#include "AStar.h"
#include "Heuristics.h"
#include "PathSmoothing.h"
#include "Shared/Graph/NavGraph/NavGraph.h"
#include "Shared/Graph/NavGraph/NavGraphNode.h"

using namespace GameAI;

std::vector<FVector2D> NavMeshPathfinding::FindPath(const FVector2D& startPos, const FVector2D& endPos,
	NavGraph* const pNavGraph, std::vector<FVector2D>& debugNodePositions, std::vector<NavLine>& debugPortals)
{
	std::vector<FVector2D> finalPath{};
	debugNodePositions.clear();
	debugPortals.clear();

	if (!pNavGraph || !pNavGraph->GetNavPolygon())
	{
		return finalPath;
	}

	TriPolygon const& NavPoly = *pNavGraph->GetNavPolygon();

	FVector2D projectedStartPos = startPos;
	FVector2D projectedEndPos = endPos;

	TriPolygon::Triangle const* startTriangle = NavPoly.GetTriangleAtPosition(startPos, true);
	if (!startTriangle)
	{
		startTriangle = NavPoly.GetClosestTriangleToPosition(startPos, projectedStartPos);
	}

	TriPolygon::Triangle const* endTriangle = NavPoly.GetTriangleAtPosition(endPos, true);
	if (!endTriangle)
	{
		endTriangle = NavPoly.GetClosestTriangleToPosition(endPos, projectedEndPos);
	}

	if (!startTriangle || !endTriangle)
	{
		return finalPath;
	}

	if (*startTriangle == *endTriangle)
	{
		finalPath.push_back(projectedStartPos);
		finalPath.push_back(projectedEndPos);
		debugNodePositions = finalPath;
		return finalPath;
	}

	std::unique_ptr<NavGraph> GraphCopy = pNavGraph->Clone();

	int const startNodeId = GraphCopy->AddNode(std::make_unique<NavGraphNode>(projectedStartPos, -1));
	for (TriPolygon::Edge const& Edge : startTriangle->GetEdges())
	{
		int const EdgeIdx = NavPoly.FindEdgeIndex(Edge).value_or(Graphs::InvalidNodeId);
		int const ExistingNodeId = GraphCopy->GetNodeIdFromEdgeIndex(EdgeIdx);
		if (ExistingNodeId != Graphs::InvalidNodeId)
		{
			GraphCopy->AddConnection(startNodeId, ExistingNodeId);
			if (Connection* NewConnection = GraphCopy->FindConnection(startNodeId, ExistingNodeId))
			{
				NewConnection->SetWeight(FVector2D::Distance(projectedStartPos, GraphCopy->GetNode(ExistingNodeId)->GetPosition()));
			}
			if (Connection* InverseConnection = GraphCopy->FindConnection(ExistingNodeId, startNodeId))
			{
				InverseConnection->SetWeight(FVector2D::Distance(projectedStartPos, GraphCopy->GetNode(ExistingNodeId)->GetPosition()));
			}
		}
	}

	int const endNodeId = GraphCopy->AddNode(std::make_unique<NavGraphNode>(projectedEndPos, -1));
	for (TriPolygon::Edge const& Edge : endTriangle->GetEdges())
	{
		int const EdgeIdx = NavPoly.FindEdgeIndex(Edge).value_or(Graphs::InvalidNodeId);
		int const ExistingNodeId = GraphCopy->GetNodeIdFromEdgeIndex(EdgeIdx);
		if (ExistingNodeId != Graphs::InvalidNodeId)
		{
			GraphCopy->AddConnection(ExistingNodeId, endNodeId);
			if (Connection* NewConnection = GraphCopy->FindConnection(ExistingNodeId, endNodeId))
			{
				NewConnection->SetWeight(FVector2D::Distance(projectedEndPos, GraphCopy->GetNode(ExistingNodeId)->GetPosition()));
			}
			if (Connection* InverseConnection = GraphCopy->FindConnection(endNodeId, ExistingNodeId))
			{
				InverseConnection->SetWeight(FVector2D::Distance(projectedEndPos, GraphCopy->GetNode(ExistingNodeId)->GetPosition()));
			}
		}
	}

	AStar PathFinder{ GraphCopy.get(), HeuristicFunctions::Euclidean };
	std::vector<Node*> nodePath = PathFinder.FindPath(GraphCopy->GetNode(startNodeId).get(), GraphCopy->GetNode(endNodeId).get());
	if (nodePath.empty())
	{
		return finalPath;
	}

	std::vector<FVector2D> rawPath{};
	for (Node* Node : nodePath)
	{
		debugNodePositions.push_back(Node->GetPosition());
		rawPath.push_back(Node->GetPosition());
	}

	debugPortals = SSFA::FindPortals(nodePath, NavPoly);
	std::vector<FVector2D> SmoothedPath = SSFA::OptimizePortals(debugPortals, NavPoly);
	if (SmoothedPath.size() >= 2)
	{
		return SmoothedPath;
	}

	return rawPath;
}

std::vector<FVector2D> NavMeshPathfinding::FindPath(const FVector2D& startPos, const FVector2D& endPos, NavGraph* const pNavGraph)
{
	std::vector<FVector2D> debugNodePositions{};
	std::vector<NavLine> debugPortals{};

	return FindPath(startPos, endPos, pNavGraph, debugNodePositions, debugPortals);
}
