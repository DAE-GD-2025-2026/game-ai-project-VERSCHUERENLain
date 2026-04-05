#include "TriPolygon.h"

#include <ranges>

#include "Shared/Utils/GeoUtilities.h"

#pragma region Triangle/Edge
bool TriPolygon::Edge::operator==(const Edge& Other) const
{
	for (int Idx : EdgeIndices)
	{
		if (std::ranges::find(Other.EdgeIndices, Idx) == Other.EdgeIndices.end())
		{
			return false;
		}
	}
	return true;
}

std::array<FVector, 3> TriPolygon::Triangle::GetVertices(TriPolygon const& Poly) const
{
	return { Poly.Vertices[VertexIndices[0]], Poly.Vertices[VertexIndices[1]], Poly.Vertices[VertexIndices[2]] };
}

std::vector<int> TriPolygon::Triangle::GetNeighbors(TriPolygon const& Poly) const
{
	return Poly.GetTriangleNeighbors(*this);
}

std::array<TriPolygon::Edge, 3> TriPolygon::Triangle::GetEdges() const
{
	return std::array{
		Edge{VertexIndices[0], VertexIndices[1]},
		Edge{VertexIndices[1], VertexIndices[2]},
		Edge{VertexIndices[2], VertexIndices[0]},
	};
}

bool TriPolygon::Triangle::operator==(const Triangle& Other) const
{
	for (int VertexIndex : VertexIndices)
	{
		if (std::ranges::find(Other.VertexIndices, VertexIndex) == Other.VertexIndices.end())
		{
			return false;
		}
	}
	return true;
}

bool TriPolygon::Triangle::Equals(const TArray<FVector>& VertexData, TriPolygon const& Poly) const
{
	std::array<int, 3> DataIndices{};
	int Idx{ 0 };
	for (FVector const& Data : VertexData)
	{
		DataIndices[Idx] = Poly.FindVertexIndex(Data).value_or(-1);
		++Idx;
	}
	return *this == Triangle{ DataIndices };
}

bool TriPolygon::Triangle::HasEdge(Edge const& Edge) const
{
	for (int Idx : Edge.EdgeIndices)
	{
		if (std::ranges::find(VertexIndices, Idx) == VertexIndices.end())
		{
			return false;
		}
	}
	return true;
}

void TriPolygon::Triangle::DebugDraw(UWorld const* World, TriPolygon const& Poly, FColor const& Color) const
{
	DrawDebugLine(World, GetVertex0(Poly), GetVertex1(Poly), Color);
	DrawDebugLine(World, GetVertex1(Poly), GetVertex2(Poly), Color);
	DrawDebugLine(World, GetVertex2(Poly), GetVertex0(Poly), Color);
}
#pragma endregion

int TriPolygon::AddTriangle(TArray<FVector> const& TriangleData)
{
	if (auto const Result = FindTriangleIndex(TriangleData); Result.has_value())
	{
		return Result.value();
	}

	std::array<int, 3> TriVertexIndices{};
	int Index{ 0 };
	for (auto const& Vertex : TriangleData)
	{
		TriVertexIndices[Index] = AddVertex(Vertex);
		++Index;
	}

	Triangles.emplace_back(Triangle{ TriVertexIndices });

	for (auto const& PossiblyNewEdge : Triangles[Triangles.size() - 1].GetEdges())
	{
		AddEdge(PossiblyNewEdge);
	}

	return static_cast<int>(Triangles.size()) - 1;
}

void TriPolygon::DrawDebug(UWorld const* World, FColor const& Color) const
{
	for (Triangle const& Triangle : Triangles)
	{
		Triangle.DebugDraw(World, *this, Color);
	}
}

std::vector<int> TriPolygon::GetTriangleNeighbors(int InTriangleIndex) const
{
	Triangle const& TriangleToCheck = Triangles[InTriangleIndex];
	std::vector<int> Neighbors{};

	for (int CurrentIdx = 0; CurrentIdx < static_cast<int>(Triangles.size()); ++CurrentIdx)
	{
		if (InTriangleIndex == CurrentIdx) continue;
		Triangle const& OtherTriangle = Triangles[CurrentIdx];

		int SharedIdxs{ 0 };
		for (int VertexIndex : TriangleToCheck.VertexIndices)
		{
			if (std::ranges::find(OtherTriangle.VertexIndices, VertexIndex) != OtherTriangle.VertexIndices.end())
			{
				++SharedIdxs;
			}
		}

		if (SharedIdxs >= 2)
		{
			Neighbors.push_back(CurrentIdx);
		}
	}

	return Neighbors;
}

std::vector<int> TriPolygon::GetTriangleNeighbors(Triangle const& InTriangle) const
{
	for (int Idx = 0; Idx < static_cast<int>(Triangles.size()); ++Idx)
	{
		if (Triangles[Idx] == InTriangle)
		{
			return GetTriangleNeighbors(Idx);
		}
	}
	return {};
}

std::optional<int> TriPolygon::FindTriangleIndex(TArray<FVector> const& TriangleData) const
{
	for (int TriangleIndex = 0; TriangleIndex < static_cast<int>(Triangles.size()); ++TriangleIndex)
	{
		if (Triangles[TriangleIndex].Equals(TriangleData, *this))
		{
			return TriangleIndex;
		}
	}
	return std::nullopt;
}

std::optional<int> TriPolygon::FindVertexIndex(FVector const& Vertex) const
{
	for (int Idx = 0; Idx < static_cast<int>(Vertices.size()); ++Idx)
	{
		if (Vertices[Idx] == Vertex)
		{
			return Idx;
		}
	}
	return std::nullopt;
}

std::optional<int> TriPolygon::FindEdgeIndex(Edge const& Edge) const
{
	for (int Idx = 0; Idx < static_cast<int>(Edges.size()); ++Idx)
	{
		if (Edges[Idx] == Edge)
		{
			return Idx;
		}
	}
	return std::nullopt;
}

TriPolygon::Triangle const* TriPolygon::GetClosestTriangleToPosition(FVector2D const& DesiredPosition, FVector2D& OutPosition) const
{
	if (auto const TriangleAtPos = GetTriangleAtPosition(DesiredPosition, true))
	{
		OutPosition = DesiredPosition;
		return TriangleAtPos;
	}

	Edge const* ClosestEdge = nullptr;
	float ClosestEdgeDistSq = -1.f;
	for (int idx = 0; idx < static_cast<int>(Edges.size()); ++idx)
	{
		Edge const& CurrentEdge = Edges[idx];

		const FVector2D point = GameAI::Utilities::Geo::ProjectOnLineSegment(
			FVector2D{ Vertices[CurrentEdge.EdgeIndices[0]] },
			FVector2D{ Vertices[CurrentEdge.EdgeIndices[1]] },
			DesiredPosition);
		const float distSq = FVector2D{ point - DesiredPosition }.SquaredLength();

		if (ClosestEdgeDistSq < 0 || distSq < ClosestEdgeDistSq)
		{
			ClosestEdge = &CurrentEdge;
			OutPosition = point;
			ClosestEdgeDistSq = distSq;
		}
	}

	if (!ClosestEdge)
	{
		return nullptr;
	}

	for (int i = 0; i < static_cast<int>(Triangles.size()); i++)
	{
		for (auto& Edge : Triangles[i].GetEdges())
		{
			int lineIdx = FindEdgeIndex(Edge).value_or(-1);
			if (lineIdx == FindEdgeIndex(*ClosestEdge))
			{
				return &Triangles[i];
			}
		}
	}
	return nullptr;
}

TriPolygon::Triangle const* TriPolygon::GetTriangleAtPosition(FVector2D const& Position, bool OnLineAllowed) const
{
	for (size_t i = 0; i < Triangles.size(); i++)
	{
		if (GameAI::Utilities::Geo::PointInTriangle(Position,
			FVector2D{ Triangles[i].GetVertex0(*this) },
			FVector2D{ Triangles[i].GetVertex1(*this) },
			FVector2D{ Triangles[i].GetVertex2(*this) },
			OnLineAllowed))
		{
			return &Triangles[i];
		}
	}
	return nullptr;
}

int TriPolygon::AddVertex(FVector const& Vertex)
{
	for (int i = 0; i < static_cast<int>(Vertices.size()); ++i)
	{
		if (Vertices[i] == Vertex)
			return i;
	}
	Vertices.push_back(Vertex);
	return static_cast<int>(Vertices.size()) - 1;
}

int TriPolygon::AddEdge(Edge const& Edge)
{
	for (int i = 0; i < static_cast<int>(Edges.size()); ++i)
	{
		if (Edges[i] == Edge)
			return i;
	}
	Edges.push_back(Edge);
	return static_cast<int>(Edges.size()) - 1;
}
