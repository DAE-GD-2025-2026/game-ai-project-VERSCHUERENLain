#include "NavGraph.h"

#include "NavGraphNode.h"

using namespace GameAI;

NavGraph::NavGraph(std::unique_ptr<TriPolygon>&& NavPoly)
	: Graph{ false }
	, pNavPoly{ std::move(NavPoly) }
{
	CreateNavigationGraph();
}

NavGraph::NavGraph(const NavGraph& Other)
	: Graph(false)
	, pNavPoly(std::make_unique<TriPolygon>(*Other.pNavPoly))
{
	Nodes.reserve(Other.Nodes.size());
	for (std::unique_ptr<Node> const& OtherNode : Other.Nodes)
	{
		Nodes.push_back(std::make_unique<NavGraphNode>(static_cast<NavGraphNode const&>(*OtherNode)));
	}

	Connections.reserve(Other.Connections.size());
	for (std::unique_ptr<Connection> const& OtherConnection : Other.Connections)
	{
		Connections.push_back(std::make_unique<Connection>(*OtherConnection));
	}
}

std::unique_ptr<NavGraph> NavGraph::Clone() const
{
	return std::make_unique<NavGraph>(*this);
}

int NavGraph::GetNodeIdFromEdgeIndex(int EdgeIdx) const
{
	if (EdgeIdx >= 0)
	{
		for (auto const& pNode : Nodes)
		{
			if (static_cast<NavGraphNode const*>(pNode.get())->GetEdgeIdx() == EdgeIdx)
			{
				return pNode->GetId();
			}
		}
	}

	return Graphs::InvalidNodeId;
}

void NavGraph::CreateNavigationGraph()
{
	for (int EdgeIdx = 0; EdgeIdx < static_cast<int>(pNavPoly->GetEdges().size()); ++EdgeIdx)
	{
		TriPolygon::Edge const& Edge = pNavPoly->GetEdges()[EdgeIdx];

		int sharedCount{};
		for (TriPolygon::Triangle const& Triangle : pNavPoly->GetTriangles())
		{
			if (Triangle.HasEdge(Edge))
			{
				++sharedCount;
			}
		}

		if (sharedCount < 2)
		{
			continue;
		}

		FVector const midpoint3D = (Edge.GetP1(*pNavPoly) + Edge.GetP2(*pNavPoly)) * 0.5f;
		AddNode(std::make_unique<NavGraphNode>(FVector2D{ midpoint3D }, EdgeIdx));
	}

	for (TriPolygon::Triangle const& Triangle : pNavPoly->GetTriangles())
	{
		std::vector<int> triangleNodeIds{};
		for (TriPolygon::Edge const& Edge : Triangle.GetEdges())
		{
			int const EdgeIdx = pNavPoly->FindEdgeIndex(Edge).value_or(Graphs::InvalidNodeId);
			int const NodeId = GetNodeIdFromEdgeIndex(EdgeIdx);
			if (NodeId != Graphs::InvalidNodeId)
			{
				triangleNodeIds.push_back(NodeId);
			}
		}

		if (triangleNodeIds.size() == 2)
		{
			AddConnection(triangleNodeIds[0], triangleNodeIds[1]);
		}
		else if (triangleNodeIds.size() == 3)
		{
			AddConnection(triangleNodeIds[0], triangleNodeIds[1]);
			AddConnection(triangleNodeIds[1], triangleNodeIds[2]);
			AddConnection(triangleNodeIds[2], triangleNodeIds[0]);
		}
	}

	SetConnectionCostsToDistances();
}
