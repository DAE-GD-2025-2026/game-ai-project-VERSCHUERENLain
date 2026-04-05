#pragma once

#include <vector>

#include "NavGraphPathfinding.h"
#include "Shared/Utils/GeoUtilities.h"

namespace GameAI
{
	class SSFA final
	{
	public:
		static std::vector<FVector2D> OptimizePortals(std::vector<NavLine> const& Portals, int* OutFailedPortalIndex = nullptr)
		{
			std::vector<FVector2D> Path{};
			if (Portals.empty())
			{
				if (OutFailedPortalIndex)
				{
					*OutFailedPortalIndex = 0;
				}
				return Path;
			}
			auto PushIfDifferent = [&Path](FVector2D const& Point)
			{
				if (Path.empty() || !Path.back().Equals(Point, 0.1f))
				{
					Path.push_back(Point);
				}
			};

			if (OutFailedPortalIndex)
			{
				*OutFailedPortalIndex = -1;
			}

			FVector2D apex = Portals[0].P1;
			int apexIndex = 0;
			PushIfDifferent(apex);

			if (Portals.size() == 1)
			{
				return Path;
			}

			int leftIndex = 1;
			int rightIndex = 1;
			FVector2D leftLeg = Portals[leftIndex].P2 - apex;
			FVector2D rightLeg = Portals[rightIndex].P1 - apex;

			for (int portalIdx = 2; portalIdx < static_cast<int>(Portals.size()); ++portalIdx)
			{
				NavLine const& Portal = Portals[portalIdx];
				FVector2D const newRightLeg = Portal.P1 - apex;

				// right check: inward means ccw
				if (Utilities::Geo::CrossZ(rightLeg, newRightLeg) > 0.0f)
				{
					// crossed the left leg
					if (Utilities::Geo::CrossZ(leftLeg, newRightLeg) > 0.0f)
					{
						apex += leftLeg;
						apexIndex = leftIndex;
						PushIfDifferent(apex);

						int const nextPortalIdx = apexIndex + 1;
						leftIndex = nextPortalIdx;
						rightIndex = nextPortalIdx;

						if (nextPortalIdx < static_cast<int>(Portals.size()))
						{
							rightLeg = Portals[rightIndex].P1 - apex;
							leftLeg = Portals[leftIndex].P2 - apex;
						}
						portalIdx = apexIndex;
						continue;
					}
					else
					{
						rightLeg = newRightLeg;
						rightIndex = portalIdx;
					}
				}

				FVector2D const newLeftLeg = Portal.P2 - apex;

				// left check: inward means cw
				if (Utilities::Geo::CrossZ(leftLeg, newLeftLeg) < 0.0f)
				{
					// crossed the right leg
					if (Utilities::Geo::CrossZ(rightLeg, newLeftLeg) < 0.0f)
					{
						apex += rightLeg;
						apexIndex = rightIndex;
						PushIfDifferent(apex);

						int const nextPortalIdx = apexIndex + 1;
						leftIndex = nextPortalIdx;
						rightIndex = nextPortalIdx;

						if (nextPortalIdx < static_cast<int>(Portals.size()))
						{
							rightLeg = Portals[rightIndex].P1 - apex;
							leftLeg = Portals[leftIndex].P2 - apex;
						}
						portalIdx = apexIndex;
						continue;
					}
					else
					{
						leftLeg = newLeftLeg;
						leftIndex = portalIdx;
					}
				}

				if (apex.ContainsNaN() || leftLeg.ContainsNaN() || rightLeg.ContainsNaN())
				{
					if (OutFailedPortalIndex)
					{
						*OutFailedPortalIndex = portalIdx;
					}
					return {};
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
