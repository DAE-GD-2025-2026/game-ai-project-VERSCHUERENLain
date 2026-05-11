#pragma once

#include <memory>
#include <vector>

#include "State.h"
#include "Transition.h"

namespace GameAI::FSM
{
	class FSM final
	{
	public:
		void AddState(std::unique_ptr<State>&& NewState);
		void AddTransition(State* From, State* To, std::function<bool()> EvalFunc);
		void SetInitialState(State* NewInitialState);

		State* GetCurrentState() const { return CurrentState; }

		void Start();
		void Stop();
		void Update(float DeltaTime);

	private:
		std::vector<std::unique_ptr<State>> States{};
		std::vector<Transition> Transitions{};
		State* InitialState{};
		State* CurrentState{};
		bool bHasStarted{false};
	};
}
