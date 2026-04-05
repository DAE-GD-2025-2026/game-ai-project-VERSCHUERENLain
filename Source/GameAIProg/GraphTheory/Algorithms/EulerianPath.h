#pragma once
#include <stack>
#include "Shared/Graph/Graph.h"

namespace GameAI
{
	enum class Eulerianity
	{
		notEulerian,
		semiEulerian,
		eulerian,
	};

	class EulerianPath final
	{
	public:
		EulerianPath(Graph* const pGraph);

		Eulerianity IsEulerian() const;
		std::vector<Node*> FindPath(Eulerianity& eulerianity) const;

	private:
		void VisitAllNodesDFS(const std::vector<Node*>& pNodes, std::vector<bool>& visited, int startIndex) const;
		bool IsConnected() const;

		Graph* m_pGraph;
	};

	inline EulerianPath::EulerianPath(Graph* const pGraph)
		: m_pGraph(pGraph)
	{
	}

	inline Eulerianity EulerianPath::IsEulerian() const
	{
		if (!m_pGraph || !IsConnected())
		{
			return Eulerianity::notEulerian;
		}

		int oddDegreeNodeCount{};
		for (Node* Node : m_pGraph->GetActiveNodes())
		{
			const int connectionCount = static_cast<int>(m_pGraph->FindConnectionsFrom(Node->GetId()).size());
			if (connectionCount % 2 != 0)
			{
				++oddDegreeNodeCount;
			}
		}

		if (oddDegreeNodeCount > 2)
		{
			return Eulerianity::notEulerian;
		}

		if (oddDegreeNodeCount == 2)
		{
			return Eulerianity::semiEulerian;
		}

		if (oddDegreeNodeCount == 0)
		{
			return Eulerianity::eulerian;
		}

		return Eulerianity::notEulerian;
	}

	inline std::vector<Node*> EulerianPath::FindPath(Eulerianity& eulerianity) const
	{
		// Get a copy of the graph because this algorithm involves removing edges
		Graph graphCopy = m_pGraph->Clone();
		std::vector<Node*> Path = {};
		std::vector<Node*> Nodes = graphCopy.GetActiveNodes();
		int currentNodeId{ Graphs::InvalidNodeId };
		
		eulerianity = IsEulerian();
		if (eulerianity == Eulerianity::notEulerian || Nodes.empty())
		{
			return Path;
		}

		if (eulerianity == Eulerianity::semiEulerian)
		{
			for (Node* Node : Nodes)
			{
				const int connectionCount = static_cast<int>(graphCopy.FindConnectionsFrom(Node->GetId()).size());
				if (connectionCount % 2 != 0)
				{
					currentNodeId = Node->GetId();
					break;
				}
			}
		}
		else
		{
			currentNodeId = Nodes.front()->GetId();
			for (Node* Node : Nodes)
			{
				if (!graphCopy.FindConnectionsFrom(Node->GetId()).empty())
				{
					currentNodeId = Node->GetId();
					break;
				}
			}
		}
		
		std::stack<int> nodeStack;
		while (!nodeStack.empty() || !graphCopy.FindConnectionsFrom(currentNodeId).empty())
		{
			std::vector<Connection*> connections = graphCopy.FindConnectionsFrom(currentNodeId);
			if (!connections.empty())
			{
				nodeStack.push(currentNodeId);

				const int nextNodeId = connections.front()->GetToId();
				graphCopy.RemoveConnection(currentNodeId, nextNodeId);
				currentNodeId = nextNodeId;
			}
			else
			{
				Path.push_back(m_pGraph->GetNode(currentNodeId).get());
				currentNodeId = nodeStack.top();
				nodeStack.pop();
			}
		}

		Path.push_back(m_pGraph->GetNode(currentNodeId).get());

		std::reverse(Path.begin(), Path.end());
		return Path;
	}

	inline void EulerianPath::VisitAllNodesDFS(const std::vector<Node*>& Nodes, std::vector<bool>& visited, int startIndex ) const
	{
		visited[startIndex] = true;

		const std::vector<Connection*> connections = m_pGraph->FindConnectionsFrom(Nodes[startIndex]->GetId());
		for (Connection* connection : connections)
		{
			const int connectedNodeId = connection->GetToId();
			for (int nodeIndex{}; nodeIndex < static_cast<int>(Nodes.size()); ++nodeIndex)
			{
				if (visited[nodeIndex])
				{
					continue;
				}

				if (Nodes[nodeIndex]->GetId() == connectedNodeId)
				{
					VisitAllNodesDFS(Nodes, visited, nodeIndex);
					break;
				}
			}
		}
	}

	inline bool EulerianPath::IsConnected() const
	{
		std::vector<Node*> Nodes = m_pGraph->GetActiveNodes();
		if (Nodes.size() == 0)
			return false;

		int startIndex{};
		for (int nodeIndex{}; nodeIndex < static_cast<int>(Nodes.size()); ++nodeIndex)
		{
			if (!m_pGraph->FindConnectionsFrom(Nodes[nodeIndex]->GetId()).empty())
			{
				startIndex = nodeIndex;
				break;
			}
		}

		std::vector<bool> visited(Nodes.size(), false);
		VisitAllNodesDFS(Nodes, visited, startIndex);

		for (bool wasVisited : visited)
		{
			if (!wasVisited)
			{
				return false;
			}
		}

		return true;
	}
}
