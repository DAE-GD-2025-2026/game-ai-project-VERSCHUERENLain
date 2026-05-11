#include "FSM.h"

void GameAI::FSM::FSM::AddState(std::unique_ptr<State>&& NewState)
{
	if (!NewState)
	{
		return;
	}

	States.push_back(std::move(NewState));
}

void GameAI::FSM::FSM::AddTransition(State* From, State* To, std::function<bool()> EvalFunc)
{
	if (!From || !To || !EvalFunc)
	{
		return;
	}

	Transitions.push_back(Transition{From, To, std::move(EvalFunc)});
}

void GameAI::FSM::FSM::SetInitialState(State* NewInitialState)
{
	InitialState = NewInitialState;
}

void GameAI::FSM::FSM::Start()
{
	if (bHasStarted || !InitialState)
	{
		return;
	}

	CurrentState = InitialState;
	bHasStarted = true;
	CurrentState->Enter();
}

void GameAI::FSM::FSM::Stop()
{
	if (!bHasStarted)
	{
		return;
	}

	if (CurrentState)
	{
		CurrentState->Exit();
	}

	CurrentState = nullptr;
	bHasStarted = false;
}

void GameAI::FSM::FSM::Update(float DeltaTime)
{
	if (!bHasStarted || !CurrentState)
	{
		return;
	}

	State* NextState{};
	for (Transition const& Transition : Transitions)
	{
		if (Transition.From != CurrentState)
		{
			continue;
		}

		if (Transition.EvalFunc && Transition.EvalFunc())
		{
			NextState = Transition.To;
			break;
		}
	}

	if (NextState && NextState != CurrentState)
	{
		CurrentState->Exit();
		CurrentState = NextState;
		CurrentState->Enter();
	}

	if (CurrentState)
	{
		CurrentState->Update(DeltaTime);
	}
}
