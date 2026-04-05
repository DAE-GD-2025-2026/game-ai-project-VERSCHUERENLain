#include "NavGraphPathfinding.h"

#include "AStar.h"
#include "Heuristics.h"
#include "PathSmoothing.h"
#include "Shared/Graph/NavGraph/NavGraph.h"
#include "Shared/Graph/NavGraph/NavGraphNode.h"
#include "Shared/Utils/GeoUtilities.h"

using namespace GameAI;

namespace
{
	struct TriangleStripBuildResult final
	{
		std::vector<int> TriangleStrip{};
		int FailedPortalIndex{ -1 };
		int FailedEdgeIndex{ -1 };
	};

	void SetUndirectedConnectionWeight(NavGraph& Graph, int FromNodeId, int ToNodeId, float Weight)
	{
		if (Connection* Connection = Graph.FindConnection(FromNodeId, ToNodeId))
		{
			Connection->SetWeight(Weight);
		}
		if (Connection* InverseConnection = Graph.FindConnection(ToNodeId, FromNodeId))
		{
			InverseConnection->SetWeight(Weight);
		}
	}

	void ConnectTempNodeToTriangle(NavGraph& Graph, TriPolygon const& NavPoly, TriPolygon::Triangle const& Triangle,
		int TempNodeId, FVector2D const& TempNodePosition, bool bTempNodeIsSource)
	{
		for (TriPolygon::Edge const& Edge : Triangle.GetEdges())
		{
			int const EdgeIdx = NavPoly.FindEdgeIndex(Edge).value_or(Graphs::InvalidNodeId);
			int const ExistingNodeId = Graph.GetNodeIdFromEdgeIndex(EdgeIdx);
			if (ExistingNodeId == Graphs::InvalidNodeId)
			{
				continue;
			}

			if (bTempNodeIsSource)
			{
				Graph.AddConnection(TempNodeId, ExistingNodeId);
			}
			else
			{
				Graph.AddConnection(ExistingNodeId, TempNodeId);
			}

			float const Weight = FVector2D::Distance(TempNodePosition, Graph.GetNode(ExistingNodeId)->GetPosition());
			SetUndirectedConnectionWeight(Graph, TempNodeId, ExistingNodeId, Weight);
		}
	}

	std::optional<TriPolygon::Edge> TryFindSharedEdge(TriPolygon const& NavPoly, int TriangleAIdx, int TriangleBIdx)
	{
		TriPolygon::Triangle const& TriangleA = NavPoly.GetTriangle(TriangleAIdx);
		TriPolygon::Triangle const& TriangleB = NavPoly.GetTriangle(TriangleBIdx);

		std::optional<TriPolygon::Edge> SharedEdge{};
		for (TriPolygon::Edge const& Edge : TriangleA.GetEdges())
		{
			if (!TriangleB.HasEdge(Edge))
			{
				continue;
			}

			if (SharedEdge.has_value())
			{
				return std::nullopt;
			}

			SharedEdge = Edge;
		}

		return SharedEdge;
	}

	TriangleStripBuildResult BuildTriangleStripFromNodePath(std::vector<Node*> const& NodePath, TriPolygon const& NavPoly,
		int StartTriangleIdx, int EndTriangleIdx)
	{
		TriangleStripBuildResult Result{};
		Result.TriangleStrip.push_back(StartTriangleIdx);

		int CurrentTriangleIdx = StartTriangleIdx;
		for (int NodeIdx = 1; NodeIdx < static_cast<int>(NodePath.size()) - 1; ++NodeIdx)
		{
			NavGraphNode const* const PortalNode = static_cast<NavGraphNode const*>(NodePath[NodeIdx]);
			Result.FailedPortalIndex = NodeIdx - 1;
			Result.FailedEdgeIndex = PortalNode->GetEdgeIdx();

			if (PortalNode->GetEdgeIdx() < 0)
			{
				return Result;
			}

			std::vector<int> const TriangleIndices = NavPoly.GetTriangleIndicesFromEdgeIndex(PortalNode->GetEdgeIdx());
			if (TriangleIndices.size() != 2)
			{
				return Result;
			}

			int NextTriangleIdx = Graphs::InvalidNodeId;
			if (TriangleIndices[0] == CurrentTriangleIdx)
			{
				NextTriangleIdx = TriangleIndices[1];
			}
			else if (TriangleIndices[1] == CurrentTriangleIdx)
			{
				NextTriangleIdx = TriangleIndices[0];
			}
			else
			{
				return Result;
			}

			std::optional<TriPolygon::Edge> const SharedEdge = TryFindSharedEdge(NavPoly, CurrentTriangleIdx, NextTriangleIdx);
			if (!SharedEdge.has_value())
			{
				Result.FailedEdgeIndex = -1;
				return Result;
			}

			int const SharedEdgeIdx = NavPoly.FindEdgeIndex(SharedEdge.value()).value_or(Graphs::InvalidNodeId);
			if (SharedEdgeIdx != PortalNode->GetEdgeIdx())
			{
				Result.FailedEdgeIndex = SharedEdgeIdx;
				return Result;
			}

			Result.TriangleStrip.push_back(NextTriangleIdx);
			CurrentTriangleIdx = NextTriangleIdx;
		}

		if (CurrentTriangleIdx != EndTriangleIdx)
		{
			return Result;
		}

		Result.FailedPortalIndex = -1;
		Result.FailedEdgeIndex = -1;
		return Result;
	}

	bool BuildPortalsFromTriangleStrip(TriPolygon const& NavPoly, std::vector<int> const& TriangleStrip,
		FVector2D const& StartPos, FVector2D const& EndPos, std::vector<NavLine>& OutPortals,
		int& OutFailedPortalIndex, int& OutFailedEdgeIndex)
	{
		struct PortalBuildData final
		{
			FVector2D A{};
			FVector2D B{};
			FVector2D Midpoint{};
			FVector2D FallbackDirection{};
			int VertexIdxA{ Graphs::InvalidNodeId };
			int VertexIdxB{ Graphs::InvalidNodeId };
			int EdgeIdx{ Graphs::InvalidNodeId };
		};

		OutPortals.clear();
		OutFailedPortalIndex = -1;
		OutFailedEdgeIndex = -1;

		if (TriangleStrip.empty())
		{
			return false;
		}

		std::vector<PortalBuildData> PortalData{};
		PortalData.reserve(TriangleStrip.size() > 0 ? TriangleStrip.size() - 1 : 0);

		for (int StripIdx = 1; StripIdx < static_cast<int>(TriangleStrip.size()); ++StripIdx)
		{
			int const CurrentTriangleIdx = TriangleStrip[StripIdx - 1];
			int const NextTriangleIdx = TriangleStrip[StripIdx];
			OutFailedPortalIndex = StripIdx - 1;

			std::optional<TriPolygon::Edge> const SharedEdge = TryFindSharedEdge(NavPoly, CurrentTriangleIdx, NextTriangleIdx);
			if (!SharedEdge.has_value())
			{
				return false;
			}

			TriPolygon::Triangle const& CurrentTriangle = NavPoly.GetTriangle(CurrentTriangleIdx);
			TriPolygon::Triangle const& NextTriangle = NavPoly.GetTriangle(NextTriangleIdx);
			FVector2D const CurrentCentroid = CurrentTriangle.GetCentroid(NavPoly);
			FVector2D const NextCentroid = NextTriangle.GetCentroid(NavPoly);

			int const SharedEdgeIdx = NavPoly.FindEdgeIndex(SharedEdge.value()).value_or(Graphs::InvalidNodeId);
			OutFailedEdgeIndex = SharedEdgeIdx;
			if (SharedEdgeIdx == Graphs::InvalidNodeId)
			{
				return false;
			}

			TriPolygon::Edge const& PortalEdge = NavPoly.GetEdges()[SharedEdgeIdx];
			FVector2D const PortalA{ PortalEdge.GetP1(NavPoly) };
			FVector2D const PortalB{ PortalEdge.GetP2(NavPoly) };
			PortalBuildData Data{};
			Data.A = PortalA;
			Data.B = PortalB;
			Data.Midpoint = (PortalA + PortalB) * 0.5f;
			Data.FallbackDirection = NextCentroid - CurrentCentroid;
			Data.VertexIdxA = PortalEdge.EdgeIndices[0];
			Data.VertexIdxB = PortalEdge.EdgeIndices[1];
			Data.EdgeIdx = SharedEdgeIdx;
			PortalData.push_back(Data);
		}

		OutPortals.push_back({ StartPos, StartPos });
		if (PortalData.empty())
		{
			OutPortals.push_back({ EndPos, EndPos });
			return true;
		}

		auto OrientPortalFromDirection = [](PortalBuildData const& Portal, FVector2D const& Anchor, FVector2D const& Direction,
			NavLine& OutPortal, int& OutRightVertexIdx, int& OutLeftVertexIdx)
		{
			float const CrossA = Utilities::Geo::CrossZ(Direction, Portal.A - Anchor);
			float const CrossB = Utilities::Geo::CrossZ(Direction, Portal.B - Anchor);

			if (CrossA <= CrossB)
			{
				OutPortal = { Portal.A, Portal.B };
				OutRightVertexIdx = Portal.VertexIdxA;
				OutLeftVertexIdx = Portal.VertexIdxB;
			}
			else
			{
				OutPortal = { Portal.B, Portal.A };
				OutRightVertexIdx = Portal.VertexIdxB;
				OutLeftVertexIdx = Portal.VertexIdxA;
			}
		};

		int PreviousRightVertexIdx = Graphs::InvalidNodeId;
		int PreviousLeftVertexIdx = Graphs::InvalidNodeId;
		{
			PortalBuildData const& FirstPortal = PortalData[0];
			FVector2D InitialDirection = (PortalData.size() > 1) ? (PortalData[1].Midpoint - StartPos) : (EndPos - StartPos);
			if (InitialDirection.SquaredLength() <= KINDA_SMALL_NUMBER)
			{
				InitialDirection = FirstPortal.FallbackDirection;
			}
			if (InitialDirection.SquaredLength() <= KINDA_SMALL_NUMBER)
			{
				return false;
			}

			NavLine OrientedFirstPortal{};
			OrientPortalFromDirection(FirstPortal, StartPos, InitialDirection, OrientedFirstPortal, PreviousRightVertexIdx, PreviousLeftVertexIdx);
			OutPortals.push_back(OrientedFirstPortal);
		}

		for (int PortalIdx = 1; PortalIdx < static_cast<int>(PortalData.size()); ++PortalIdx)
		{
			PortalBuildData const& Portal = PortalData[PortalIdx];
			OutFailedPortalIndex = PortalIdx;
			OutFailedEdgeIndex = Portal.EdgeIdx;

			bool const bSharedA = (Portal.VertexIdxA == PreviousRightVertexIdx || Portal.VertexIdxA == PreviousLeftVertexIdx);
			bool const bSharedB = (Portal.VertexIdxB == PreviousRightVertexIdx || Portal.VertexIdxB == PreviousLeftVertexIdx);

			NavLine OrientedPortal{};
			int CurrentRightVertexIdx = Graphs::InvalidNodeId;
			int CurrentLeftVertexIdx = Graphs::InvalidNodeId;

			if (bSharedA != bSharedB)
			{
				if (Portal.VertexIdxA == PreviousRightVertexIdx)
				{
					OrientedPortal = { Portal.A, Portal.B };
					CurrentRightVertexIdx = Portal.VertexIdxA;
					CurrentLeftVertexIdx = Portal.VertexIdxB;
				}
				else if (Portal.VertexIdxA == PreviousLeftVertexIdx)
				{
					OrientedPortal = { Portal.B, Portal.A };
					CurrentRightVertexIdx = Portal.VertexIdxB;
					CurrentLeftVertexIdx = Portal.VertexIdxA;
				}
				else if (Portal.VertexIdxB == PreviousRightVertexIdx)
				{
					OrientedPortal = { Portal.B, Portal.A };
					CurrentRightVertexIdx = Portal.VertexIdxB;
					CurrentLeftVertexIdx = Portal.VertexIdxA;
				}
				else
				{
					OrientedPortal = { Portal.A, Portal.B };
					CurrentRightVertexIdx = Portal.VertexIdxA;
					CurrentLeftVertexIdx = Portal.VertexIdxB;
				}
			}
			else
			{
				FVector2D const PrevPoint = PortalData[PortalIdx - 1].Midpoint;
				FVector2D const NextPoint = (PortalIdx + 1 < static_cast<int>(PortalData.size())) ? PortalData[PortalIdx + 1].Midpoint : EndPos;

				FVector2D TravelDirection = NextPoint - PrevPoint;
				if (TravelDirection.SquaredLength() <= KINDA_SMALL_NUMBER)
				{
					TravelDirection = Portal.FallbackDirection;
				}
				if (TravelDirection.SquaredLength() <= KINDA_SMALL_NUMBER)
				{
					return false;
				}

				OrientPortalFromDirection(Portal, PrevPoint, TravelDirection, OrientedPortal, CurrentRightVertexIdx, CurrentLeftVertexIdx);
			}

			OutPortals.push_back(OrientedPortal);
			PreviousRightVertexIdx = CurrentRightVertexIdx;
			PreviousLeftVertexIdx = CurrentLeftVertexIdx;
		}
		OutPortals.push_back({ EndPos, EndPos });
		OutFailedPortalIndex = -1;
		OutFailedEdgeIndex = -1;
		return true;
	}
}

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

	std::optional<int> const StartTriangleIdx = NavPoly.FindTriangleIndex(*startTriangle);
	std::optional<int> const EndTriangleIdx = NavPoly.FindTriangleIndex(*endTriangle);
	if (!StartTriangleIdx.has_value() || !EndTriangleIdx.has_value())
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
	ConnectTempNodeToTriangle(*GraphCopy, NavPoly, *startTriangle, startNodeId, projectedStartPos, true);

	int const endNodeId = GraphCopy->AddNode(std::make_unique<NavGraphNode>(projectedEndPos, -1));
	ConnectTempNodeToTriangle(*GraphCopy, NavPoly, *endTriangle, endNodeId, projectedEndPos, false);

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

	TriangleStripBuildResult const TriangleStripResult =
		BuildTriangleStripFromNodePath(nodePath, NavPoly, StartTriangleIdx.value(), EndTriangleIdx.value());

	int const ExpectedTriangleStripSize = static_cast<int>(nodePath.size()) - 1;
	if (TriangleStripResult.TriangleStrip.size() != ExpectedTriangleStripSize
		|| TriangleStripResult.TriangleStrip.back() != EndTriangleIdx.value())
	{
		debugPortals.clear();
		UE_LOG(LogTemp, Warning,
			TEXT("Portal corridor reconstruction failed while building the triangle strip at portal %d (edge %d)."),
			TriangleStripResult.FailedPortalIndex, TriangleStripResult.FailedEdgeIndex);
		return rawPath;
	}

	int FailedPortalIndex = -1;
	int FailedEdgeIndex = -1;
	if (!BuildPortalsFromTriangleStrip(NavPoly, TriangleStripResult.TriangleStrip,
		projectedStartPos, projectedEndPos, debugPortals, FailedPortalIndex, FailedEdgeIndex))
	{
		debugPortals.clear();
		UE_LOG(LogTemp, Warning,
			TEXT("Portal corridor reconstruction failed while building portals at portal %d (edge %d)."),
			FailedPortalIndex, FailedEdgeIndex);
		return rawPath;
	}

	std::vector<FVector2D> SmoothedPath = SSFA::OptimizePortals(debugPortals, &FailedPortalIndex);
	if (SmoothedPath.size() >= 2)
	{
		return SmoothedPath;
	}

	UE_LOG(LogTemp, Warning, TEXT("Portal smoothing failed at portal index %d, falling back to the raw navgraph node path."),
		FailedPortalIndex);
	return rawPath;
}

std::vector<FVector2D> NavMeshPathfinding::FindPath(const FVector2D& startPos, const FVector2D& endPos, NavGraph* const pNavGraph)
{
	std::vector<FVector2D> debugNodePositions{};
	std::vector<NavLine> debugPortals{};

	return FindPath(startPos, endPos, pNavGraph, debugNodePositions, debugPortals);
}
