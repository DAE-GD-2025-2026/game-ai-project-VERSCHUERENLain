#include "SpacePartitioning.h"
#include "DrawDebugHelpers.h"

// --- Cell ---
// ------------
Cell::Cell(float Left, float Bottom, float Width, float Height)
{
	BoundingBox.Min = { Left, Bottom };
	BoundingBox.Max = { BoundingBox.Min.X + Width, BoundingBox.Min.Y + Height };
}

std::vector<FVector2D> Cell::GetRectPoints() const
{
	const float left = BoundingBox.Min.X;
	const float bottom = BoundingBox.Min.Y;
	const float width = BoundingBox.Max.X - BoundingBox.Min.X;
	const float height = BoundingBox.Max.Y - BoundingBox.Min.Y;

	std::vector<FVector2D> rectPoints =
	{
		{ left , bottom  },
		{ left , bottom + height  },
		{ left + width , bottom + height },
		{ left + width , bottom  },
	};

	return rectPoints;
}

// --- Partitioned Space ---
// -------------------------
CellSpace::CellSpace(UWorld* pWorld, float Width, float Height, int Rows, int Cols, int MaxEntities)
	: pWorld{pWorld}
	, SpaceWidth{Width}
	, SpaceHeight{Height}
	, NrOfRows{Rows}
	, NrOfCols{Cols}
	, NrOfNeighbors{0}
{
	Neighbors.SetNum(MaxEntities);

	CellWidth = Width / Cols;
	CellHeight = Height / Rows;

	// origin is bottom-left of the grid (world goes from -Width/2 to +Width/2)
	CellOrigin = FVector2D(-Width * 0.5f, -Height * 0.5f);

	// create the cells row by row
	for (int row = 0; row < Rows; ++row)
	{
		for (int col = 0; col < Cols; ++col)
		{
			float left = CellOrigin.X + col * CellWidth;
			float bottom = CellOrigin.Y + row * CellHeight;
			Cells.emplace_back(left, bottom, CellWidth, CellHeight);
		}
	}
}

void CellSpace::AddAgent(ASteeringAgent& Agent)
{
	int idx = PositionToIndex(Agent.GetPosition());
	if (idx < 0) return; // skip agents outside grid bounds
	Cells[idx].Agents.push_back(&Agent);
}

void CellSpace::UpdateAgentCell(ASteeringAgent& Agent, const FVector2D& OldPos)
{
	int oldIdx = PositionToIndex(OldPos);
	int newIdx = PositionToIndex(Agent.GetPosition());

	if (oldIdx == newIdx) return;

	if (oldIdx >= 0)
		Cells[oldIdx].Agents.remove(&Agent);
	if (newIdx >= 0)
		Cells[newIdx].Agents.push_back(&Agent);
}

void CellSpace::RegisterNeighbors(ASteeringAgent& Agent, float QueryRadius)
{
	NrOfNeighbors = 0;
	LastQueriedCells.clear();

	FVector2D agentPos = Agent.GetPosition();

	// build a query rect around the agent
	FRect queryRect;
	queryRect.Min = { agentPos.X - QueryRadius, agentPos.Y - QueryRadius };
	queryRect.Max = { agentPos.X + QueryRadius, agentPos.Y + QueryRadius };

	for (int i = 0; i < static_cast<int>(Cells.size()); ++i)
	{
		if (!DoRectsOverlap(queryRect, Cells[i].BoundingBox)) continue;

		LastQueriedCells.push_back(i);

		for (ASteeringAgent* other : Cells[i].Agents)
		{
			if (other == &Agent || !IsValid(other)) continue;

			float dist = FVector2D::Distance(agentPos, other->GetPosition());
			if (dist < QueryRadius)
			{
				Neighbors[NrOfNeighbors] = other;
				++NrOfNeighbors;
			}
		}
	}
}

void CellSpace::EmptyCells()
{
	for (Cell& c : Cells)
		c.Agents.clear();
}

void CellSpace::RenderCells(const std::vector<int>& HighlightedCells) const
{
	const float z = 91.f;

	// flush old debug strings so we don't hit the engine cap
	FlushDebugStrings(pWorld);

	for (int i = 0; i < static_cast<int>(Cells.size()); ++i)
	{
		// check if this cell is in the highlighted set
		bool bHighlighted = false;
		for (int hi : HighlightedCells)
		{
			if (hi == i) { bHighlighted = true; break; }
		}

		FColor color = bHighlighted ? FColor::Red : FColor::Blue;
		float thickness = bHighlighted ? 8.f : 5.f;

		auto pts = Cells[i].GetRectPoints();
		for (int j = 0; j < 4; ++j)
		{
			FVector a{pts[j].X, pts[j].Y, z};
			FVector b{pts[(j + 1) % 4].X, pts[(j + 1) % 4].Y, z};
			DrawDebugLine(pWorld, a, b, color, false, -1.f, SDPG_Foreground, thickness);
		}

		// draw agent count at center of cell
		int count = static_cast<int>(Cells[i].Agents.size());
		if (count > 0)
		{
			FVector center{
				(Cells[i].BoundingBox.Min.X + Cells[i].BoundingBox.Max.X) * 0.5f,
				(Cells[i].BoundingBox.Min.Y + Cells[i].BoundingBox.Max.Y) * 0.5f,
				z
			};
			DrawDebugString(pWorld, center, FString::Printf(TEXT("%d"), count), nullptr, FColor::White, -1.f, true);
		}
	}
}

int CellSpace::PositionToIndex(FVector2D const & Pos) const
{
	// offset position so origin is at (0,0)
	float localX = Pos.X - CellOrigin.X;
	float localY = Pos.Y - CellOrigin.Y;

	// skip agents outside grid bounds (return -1)
	if (localX < 0.f || localX >= SpaceWidth || localY < 0.f || localY >= SpaceHeight)
		return -1;

	int col = static_cast<int>(localX / CellWidth);
	int row = static_cast<int>(localY / CellHeight);
	return row * NrOfCols + col;
}

bool CellSpace::DoRectsOverlap(FRect const & RectA, FRect const & RectB)
{
	if (RectA.Max.X < RectB.Min.X || RectA.Min.X > RectB.Max.X) return false;
	if (RectA.Max.Y < RectB.Min.Y || RectA.Min.Y > RectB.Max.Y) return false;
	return true;
}

// QuadTree
QuadTree::QuadTree(UWorld* pWorld, FRect Bounds, int MaxEntities, int MaxAgentsPerNode, int MaxDepth)
	: pWorld{pWorld}
	, MaxAgentsPerNode{MaxAgentsPerNode}
	, MaxDepth{MaxDepth}
{
	Neighbors.SetNum(MaxEntities);
	Root = std::make_unique<Node>();
	Root->Bounds = Bounds;
	Root->Depth = 0;
}

void QuadTree::Clear()
{
	FRect bounds = Root->Bounds;
	Root = std::make_unique<Node>();
	Root->Bounds = bounds;
	Root->Depth = 0;
}

void QuadTree::Subdivide(Node& node)
{
	float midX = (node.Bounds.Min.X + node.Bounds.Max.X) * 0.5f;
	float midY = (node.Bounds.Min.Y + node.Bounds.Max.Y) * 0.5f;

	for (int i = 0; i < 4; ++i)
	{
		node.Children[i] = std::make_unique<Node>();
		node.Children[i]->Depth = node.Depth + 1;
	}

	// Nw: top-left
	node.Children[0]->Bounds = { {node.Bounds.Min.X, midY}, {midX, node.Bounds.Max.Y} };
	// NE: top-right
	node.Children[1]->Bounds = { {midX, midY}, {node.Bounds.Max.X, node.Bounds.Max.Y} };
	// SW: bottom-let
	node.Children[2]->Bounds = { {node.Bounds.Min.X, node.Bounds.Min.Y}, {midX, midY} };
	// SE: bottom-right
	node.Children[3]->Bounds = { {midX, node.Bounds.Min.Y}, {node.Bounds.Max.X, midY} };

	// redistribute agents into children
	for (ASteeringAgent* agent : node.Agents)
	{
		for (int i = 0; i < 4; ++i)
		{
			FVector2D pos = agent->GetPosition();
			if (pos.X >= node.Children[i]->Bounds.Min.X && pos.X < node.Children[i]->Bounds.Max.X &&
				pos.Y >= node.Children[i]->Bounds.Min.Y && pos.Y < node.Children[i]->Bounds.Max.Y)
			{
				InsertIntoNode(*node.Children[i], agent);
				break;
			}
		}
	}
	node.Agents.clear();
}

void QuadTree::Insert(ASteeringAgent* Agent)
{
	if (!Agent || !IsValid(Agent)) return;
	InsertIntoNode(*Root, Agent);
}

void QuadTree::InsertIntoNode(Node& node, ASteeringAgent* Agent)
{
	// skip: if agent is outside this node's bounds, don't insert
	FVector2D pos = Agent->GetPosition();
	if (pos.X < node.Bounds.Min.X || pos.X > node.Bounds.Max.X ||
		pos.Y < node.Bounds.Min.Y || pos.Y > node.Bounds.Max.Y)
		return;

	if (node.IsLeaf())
	{
		node.Agents.push_back(Agent);

		// subdivide if over capacity and not at max depth
		if (static_cast<int>(node.Agents.size()) > MaxAgentsPerNode && node.Depth < MaxDepth)
			Subdivide(node);
	}
	else
	{
		// insert into the correct child
		for (int i = 0; i < 4; ++i)
		{
			if (pos.X >= node.Children[i]->Bounds.Min.X && pos.X < node.Children[i]->Bounds.Max.X &&
				pos.Y >= node.Children[i]->Bounds.Min.Y && pos.Y < node.Children[i]->Bounds.Max.Y)
			{
				InsertIntoNode(*node.Children[i], Agent);
				return;
			}
		}
		// edge case: agent exactly on max boundary — put in last child 
		InsertIntoNode(*node.Children[3], Agent);
	}
}

void QuadTree::RegisterNeighbors(ASteeringAgent& Agent, float QueryRadius)
{
	NrOfNeighbors = 0;
	LastQueriedLeaves.clear();

	FVector2D agentPos = Agent.GetPosition();
	FRect queryRect;
	queryRect.Min = { agentPos.X - QueryRadius, agentPos.Y - QueryRadius };
	queryRect.Max = { agentPos.X + QueryRadius, agentPos.Y + QueryRadius };

	QueryNode(*Root, queryRect, agentPos, QueryRadius, &Agent);
}

void QuadTree::QueryNode(const Node& node, const FRect& QueryRect, const FVector2D& AgentPos,
	float QueryRadius, ASteeringAgent* ExcludeAgent)
{
	// prune: skip if this node doesn't overlap the query area
	if (!DoRectsOverlap(node.Bounds, QueryRect)) return;

	if (node.IsLeaf())
	{
		LastQueriedLeaves.push_back(node.Bounds);

		for (ASteeringAgent* other : node.Agents)
		{
			if (other == ExcludeAgent || !IsValid(other)) continue;
			float dist = FVector2D::Distance(AgentPos, other->GetPosition());
			if (dist < QueryRadius)
			{
				Neighbors[NrOfNeighbors] = other;
				++NrOfNeighbors;
			}
		}
	}
	else
	{
		for (int i = 0; i < 4; ++i)
			QueryNode(*node.Children[i], QueryRect, AgentPos, QueryRadius, ExcludeAgent);
	}
}

void QuadTree::RenderCells(const std::vector<FRect>& QueriedLeaves) const
{
	// flush old debug strings so we don't hit the engine cap
	FlushDebugStrings(pWorld);

	RenderNode(*Root, QueriedLeaves);
}

void QuadTree::RenderNode(const Node& node, const std::vector<FRect>& QueriedLeaves) const
{
	if (node.IsLeaf())
	{
		// check if this leaf was queried
		bool bHighlighted = false;
		for (const FRect& qr : QueriedLeaves)
		{
			if (qr.Min == node.Bounds.Min && qr.Max == node.Bounds.Max)
			{
				bHighlighted = true;
				break;
			}
		}

		FColor color = bHighlighted ? FColor::Red : FColor::Blue;
		float thickness = bHighlighted ? 8.f : 5.f;
		DrawRect(node.Bounds, color, thickness);

		// draw agent count at center of leaf
		int count = static_cast<int>(node.Agents.size());
		if (count > 0)
		{
			FVector center{
				(node.Bounds.Min.X + node.Bounds.Max.X) * 0.5f,
				(node.Bounds.Min.Y + node.Bounds.Max.Y) * 0.5f,
				91.f
			};
			DrawDebugString(pWorld, center, FString::Printf(TEXT("%d"), count), nullptr, FColor::White, -1.f, true);
		}
	}
	else
	{
		// draw this node's bounds in blue
		DrawRect(node.Bounds, FColor::Blue, 5.f);
		for (int i = 0; i < 4; ++i)
			RenderNode(*node.Children[i], QueriedLeaves);
	}
}

void QuadTree::DrawRect(const FRect& Rect, FColor Color, float Thickness) const
{
	const float z = 91.f;
	FVector bl{Rect.Min.X, Rect.Min.Y, z};
	FVector tl{Rect.Min.X, Rect.Max.Y, z};
	FVector tr{Rect.Max.X, Rect.Max.Y, z};
	FVector br{Rect.Max.X, Rect.Min.Y, z};

	DrawDebugLine(pWorld, bl, tl, Color, false, -1.f, SDPG_Foreground, Thickness);
	DrawDebugLine(pWorld, tl, tr, Color, false, -1.f, SDPG_Foreground, Thickness);
	DrawDebugLine(pWorld, tr, br, Color, false, -1.f, SDPG_Foreground, Thickness);
	DrawDebugLine(pWorld, br, bl, Color, false, -1.f, SDPG_Foreground, Thickness);
}

bool QuadTree::DoRectsOverlap(const FRect& A, const FRect& B)
{
	if (A.Max.X < B.Min.X || A.Min.X > B.Max.X) return false;
	if (A.Max.Y < B.Min.Y || A.Min.Y > B.Max.Y) return false;
	return true;
}
