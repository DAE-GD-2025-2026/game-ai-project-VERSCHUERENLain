/*=============================================================================*/
// SpacePartitioning.h: Contains Cell and Cellspace which are used to partition a space in segments.
// Cells contain pointers to all the agents within.
// These are used to avoid unnecessary distance comparisons to agents that are far away.

// Heavily based on chapter 3 of "Programming Game AI by Example" - Mat Buckland
/*=============================================================================*/

#pragma once
#include <list>
#include <memory>
#include <vector>
#include <iterator>

#include "Debug/ReporterGraph.h"
#include "Movement/SteeringBehaviors/SteeringAgent.h"

// --- Cell ---
// ------------
struct Cell final
{
	Cell(float Left, float Bottom, float Width, float Height);

	std::vector<FVector2D> GetRectPoints() const;

	// all the agents currently in this cell
	std::list<ASteeringAgent*> Agents;
	FRect BoundingBox;
};

// --- Partitioned Space ---
// -------------------------
class CellSpace final
{
public:
	CellSpace(UWorld* pWorld, float Width, float Height, int Rows, int Cols, int MaxEntities);

	void AddAgent(ASteeringAgent& Agent);
	void UpdateAgentCell(ASteeringAgent& Agent, const FVector2D& OldPos);

	void RegisterNeighbors(ASteeringAgent& Agent, float QueryRadius);
	const TArray<ASteeringAgent*>& GetNeighbors() const { return Neighbors; }
	int GetNrOfNeighbors() const { return NrOfNeighbors; }

	//empties the cells of entities
	void EmptyCells();
	void RenderCells(const std::vector<int>& HighlightedCells) const;

	// indices of cells checked during the last RegisterNeighbors call
	const std::vector<int>& GetLastQueriedCells() const { return LastQueriedCells; }

private:
	// For debug draw purposes
	UWorld* pWorld{};

	// Cells and properties
	std::vector<Cell> Cells;
	FVector2D CellOrigin{};

	float SpaceWidth;
	float SpaceHeight;

	int NrOfRows;
	int NrOfCols;

	float CellWidth;
	float CellHeight;

	// Members to avoid memory allocation on every frame
	TArray<ASteeringAgent*> Neighbors;
	int NrOfNeighbors;

	// tracks which cells were checked in the last RegisterNeighbors call
	std::vector<int> LastQueriedCells;

	// Helper functions
	int PositionToIndex(FVector2D const & Pos) const;
	bool DoRectsOverlap(FRect const& RectA, FRect const& RectB);
};

// --- QuadTree ---
// ----------------
class QuadTree final
{
public:
	QuadTree(UWorld* pWorld, FRect Bounds, int MaxEntities, int MaxAgentsPerNode = 4, int MaxDepth = 8);

	// rebuild every frame: clear tree then re-insert all agents
	void Clear();
	void Insert(ASteeringAgent* Agent);

	// range query: finds neighbors within QueryRadius of Agent
	void RegisterNeighbors(ASteeringAgent& Agent, float QueryRadius);
	const TArray<ASteeringAgent*>& GetNeighbors() const { return Neighbors; }
	int GetNrOfNeighbors() const { return NrOfNeighbors; }

	// debug draw — queriedRects collects bounds of visited leaves for red highlighting
	void RenderCells(const std::vector<FRect>& QueriedLeaves) const;

	// get the leaf bounds that were visited during the last query (for red debug)
	const std::vector<FRect>& GetLastQueriedLeaves() const { return LastQueriedLeaves; }

private:
	struct Node
	{
		FRect Bounds;
		std::vector<ASteeringAgent*> Agents;
		std::unique_ptr<Node> Children[4]; // NW, NE, SW, SE
		int Depth{0};

		bool IsLeaf() const { return Children[0] == nullptr; }
	};

	void Subdivide(Node& node);
	void InsertIntoNode(Node& node, ASteeringAgent* Agent);
	void QueryNode(const Node& node, const FRect& QueryRect, const FVector2D& AgentPos,
		float QueryRadius, ASteeringAgent* ExcludeAgent);
	void RenderNode(const Node& node, const std::vector<FRect>& QueriedLeaves) const;
	void DrawRect(const FRect& Rect, FColor Color, float Thickness) const;
	static bool DoRectsOverlap(const FRect& A, const FRect& B);

	std::unique_ptr<Node> Root;
	UWorld* pWorld{};
	int MaxAgentsPerNode;
	int MaxDepth;

	// query results — pre-allocated pool
	TArray<ASteeringAgent*> Neighbors;
	int NrOfNeighbors{0};

	// tracks which leaf bounds were visited during last query
	std::vector<FRect> LastQueriedLeaves;
};
