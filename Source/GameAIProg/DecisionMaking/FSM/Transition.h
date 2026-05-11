#pragma once

#include <functional>

namespace GameAI::FSM
{
	class State;

	struct Transition final
	{
		State* From{};
		State* To{};
		std::function<bool()> EvalFunc{};
	};
}
