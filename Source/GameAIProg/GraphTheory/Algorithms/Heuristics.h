#pragma once

#include <algorithm>
#include <cmath>

namespace GameAI::HeuristicFunctions
{
	typedef float(*Heuristic)(float, float);

	static float Manhattan(float x, float y)
	{
		return x + y;
	}

	static float Euclidean(float x, float y)
	{
		return sqrtf(x * x + y * y);
	}

	static float SqEuclidean(float x, float y)
	{
		return x * x + y * y;
	}

	static float Octile(float x, float y)
	{
		float constexpr f = 0.414213562373095048801f;
		return x < y ? f * x + y : f * y + x;
	}

	static float Chebyshev(float x, float y)
	{
		return std::max(x, y);
	}
}
