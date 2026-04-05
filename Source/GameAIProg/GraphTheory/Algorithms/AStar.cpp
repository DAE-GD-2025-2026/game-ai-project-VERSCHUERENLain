#include "AStar.h"

#include <algorithm>

using namespace GameAI;

AStar::AStar(Graph* const pGraph, HeuristicFunctions::Heuristic hFunction)
	: pGraph(pGraph)
	, HeuristicFunction(hFunction)
{
}

std::vector<Node*> AStar::FindPath(Node* const pStartNode, Node* const pGoalNode)
{
	std::vector<Node*> path{};
	if (!pGraph || !pStartNode || !pGoalNode)
	{
		return path;
	}

	if (pStartNode == pGoalNode)
	{
		path.push_back(pStartNode);
		return path;
	}

	std::vector<NodeRecord> openList{};
	std::vector<NodeRecord> closedList{};

	NodeRecord startRecord{};
	startRecord.pNode = pStartNode;
	startRecord.pConnection = nullptr;
	startRecord.costSoFar = 0.0f;
	startRecord.estimatedTotalCost = GetHeuristicCost(pStartNode, pGoalNode);
	openList.push_back(startRecord);

	NodeRecord currentRecord{};
	bool bFoundGoal = false;

	while (!openList.empty())
	{
		auto currentIt = std::min_element(openList.begin(), openList.end());
		currentRecord = *currentIt;

		if (currentRecord.pNode == pGoalNode)
		{
			bFoundGoal = true;
			break;
		}

		openList.erase(currentIt);

		auto Connections = pGraph->FindConnectionsFrom(currentRecord.pNode->GetId());
		for (Connection* Connection : Connections)
		{
			Node* pNextNode = pGraph->GetNode(Connection->GetToId()).get();
			float const costSoFar = currentRecord.costSoFar + Connection->GetWeight();

			auto closedIt = std::find_if(closedList.begin(), closedList.end(),
				[pNextNode](NodeRecord const& Record) { return Record.pNode == pNextNode; });
			if (closedIt != closedList.end())
			{
				if (closedIt->costSoFar <= costSoFar)
				{
					continue;
				}

				closedList.erase(closedIt);
			}

			auto openIt = std::find_if(openList.begin(), openList.end(),
				[pNextNode](NodeRecord const& Record) { return Record.pNode == pNextNode; });
			if (openIt != openList.end())
			{
				if (openIt->costSoFar <= costSoFar)
				{
					continue;
				}

				openList.erase(openIt);
			}

			NodeRecord nextRecord{};
			nextRecord.pNode = pNextNode;
			nextRecord.pConnection = Connection;
			nextRecord.costSoFar = costSoFar;
			nextRecord.estimatedTotalCost = costSoFar + GetHeuristicCost(pNextNode, pGoalNode);
			openList.push_back(nextRecord);
		}

		closedList.push_back(currentRecord);
	}

	if (!bFoundGoal)
	{
		return path;
	}

	while (currentRecord.pNode != pStartNode)
	{
		path.push_back(currentRecord.pNode);

		if (!currentRecord.pConnection)
		{
			return {};
		}

		auto PreviousIt = std::find_if(closedList.begin(), closedList.end(),
			[&currentRecord](NodeRecord const& Record)
			{
				return Record.pNode && Record.pNode->GetId() == currentRecord.pConnection->GetFromId();
			});

		if (PreviousIt == closedList.end())
		{
			return {};
		}

		currentRecord = *PreviousIt;
	}

	path.push_back(pStartNode);
	std::reverse(path.begin(), path.end());
	return path;
}

float AStar::GetHeuristicCost(Node* const pStartNode, Node* const pEndNode) const
{
	FVector2D toDestination =
		pGraph->GetNode(pEndNode->GetId())->GetPosition() - pGraph->GetNode(pStartNode->GetId())->GetPosition();
	return HeuristicFunction(FMath::Abs(toDestination.X), FMath::Abs(toDestination.Y));
}
