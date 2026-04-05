#pragma once

#include <vector>

#include "NavGraphPathfinding.h"
#include "Movement/Pathfinding/Navmesh/TriPolygon.h"
#include "Shared/Graph/Graph.h"
#include "Shared/Graph/NavGraph/NavGraphNode.h"

namespace GameAI
{
	class SSFA final
	{
	public:
		static std::vector<NavLine> FindPortals(std::vector<Node*> const& Path, TriPolygon const& NavPoly)
		{
			std::vector<NavLine> Portals{};
			if (Path.size() < 2)
			{
				return Portals;
			}

			FVector2D const startPos = Path.front()->GetPosition();
			Portals.push_back({ startPos, startPos });

			auto cross2D = [](FVector2D const& A, FVector2D const& B)
			{
				return A.X * B.Y - A.Y * B.X;
			};

			for (int PathIdx = 1; PathIdx < static_cast<int>(Path.size()) - 1; ++PathIdx)
			{
				NavGraphNode const* const NavNode = static_cast<NavGraphNode const*>(Path[PathIdx]);
				if (NavNode->GetEdgeIdx() < 0)
				{
					continue;
				}

				TriPolygon::Edge const& Edge = NavPoly.GetEdges()[NavNode->GetEdgeIdx()];
				FVector2D portalA{ Edge.GetP1(NavPoly) };
				FVector2D portalB{ Edge.GetP2(NavPoly) };

				FVector2D const prevPos = Path[PathIdx - 1]->GetPosition();
				FVector2D const nextPos = Path[PathIdx + 1]->GetPosition();
				FVector2D const corridorDir = nextPos - prevPos;
				FVector2D const toA = portalA - prevPos;
				FVector2D const toB = portalB - prevPos;

				float const crossA = cross2D(corridorDir, toA);
				float const crossB = cross2D(corridorDir, toB);

				if (crossA <= crossB)
				{
					Portals.push_back({ portalA, portalB });
				}
				else
				{
					Portals.push_back({ portalB, portalA });
				}
			}

			FVector2D const endPos = Path.back()->GetPosition();
			Portals.push_back({ endPos, endPos });

			return Portals;
		}

		static std::vector<FVector2D> OptimizePortals(std::vector<NavLine> const& Portals, TriPolygon const& NavPoly)
		{
			static_cast<void>(NavPoly);

			std::vector<FVector2D> Path{};
			if (Portals.empty())
			{
				return Path;
			}

			auto TriArea2 = [](FVector2D const& A, FVector2D const& B, FVector2D const& C)
			{
				FVector2D const AB = B - A;
				FVector2D const AC = C - A;
				return AB.X * AC.Y - AB.Y * AC.X;
			};
			auto PushIfDifferent = [&Path](FVector2D const& Point)
			{
				if (Path.empty() || !Path.back().Equals(Point, 0.1f))
				{
					Path.push_back(Point);
				}
			};

			FVector2D apex = Portals[0].P1;
			FVector2D left = Portals[0].P2;
			FVector2D right = Portals[0].P1;
			int apexIndex = 0;
			int leftIndex = 0;
			int rightIndex = 0;

			PushIfDifferent(apex);

			for (int portalIdx = 1; portalIdx < static_cast<int>(Portals.size()); ++portalIdx)
			{
				FVector2D const newRight = Portals[portalIdx].P1;
				FVector2D const newLeft = Portals[portalIdx].P2;

				if (TriArea2(apex, right, newRight) <= 0.0f)
				{
					if (apex == right || TriArea2(apex, left, newRight) > 0.0f)
					{
						right = newRight;
						rightIndex = portalIdx;
					}
					else
					{
						apex = left;
						apexIndex = leftIndex;
						PushIfDifferent(apex);

						left = apex;
						right = apex;
						leftIndex = apexIndex;
						rightIndex = apexIndex;
						portalIdx = apexIndex;
						continue;
					}
				}

				if (TriArea2(apex, left, newLeft) >= 0.0f)
				{
					if (apex == left || TriArea2(apex, right, newLeft) < 0.0f)
					{
						left = newLeft;
						leftIndex = portalIdx;
					}
					else
					{
						apex = right;
						apexIndex = rightIndex;
						PushIfDifferent(apex);

						left = apex;
						right = apex;
						leftIndex = apexIndex;
						rightIndex = apexIndex;
						portalIdx = apexIndex;
						continue;
					}
				}
			}

			PushIfDifferent(Portals.back().P1);
			return Path;
		}

	private:
		SSFA() {}
		~SSFA() {}
	};
}
