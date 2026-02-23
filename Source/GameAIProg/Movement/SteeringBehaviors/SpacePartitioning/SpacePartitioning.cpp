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
	Cells[idx].Agents.push_back(&Agent);
}

void CellSpace::UpdateAgentCell(ASteeringAgent& Agent, const FVector2D& OldPos)
{
	int oldIdx = PositionToIndex(OldPos);
	int newIdx = PositionToIndex(Agent.GetPosition());

	if (oldIdx == newIdx) return;

	Cells[oldIdx].Agents.remove(&Agent);
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
	}
}

int CellSpace::PositionToIndex(FVector2D const & Pos) const
{
	// offset position so origin is at (0,0)
	float localX = Pos.X - CellOrigin.X;
	float localY = Pos.Y - CellOrigin.Y;

	int col = FMath::Clamp(static_cast<int>(localX / CellWidth), 0, NrOfCols - 1);
	int row = FMath::Clamp(static_cast<int>(localY / CellHeight), 0, NrOfRows - 1);

	return row * NrOfCols + col;
}

bool CellSpace::DoRectsOverlap(FRect const & RectA, FRect const & RectB)
{
	if (RectA.Max.X < RectB.Min.X || RectA.Min.X > RectB.Max.X) return false;
	if (RectA.Max.Y < RectB.Min.Y || RectA.Min.Y > RectB.Max.Y) return false;
	return true;
}
