#pragma once

#include "DecisionMaking/FSM/State.h"
#include "GameAIProg/Movement/SteeringBehaviors/Steering/SteeringBehaviors.h"

namespace GameAI::FSM
{
	class SearchState final : public State
	{
	public:
		void Enter() override;
		void Update(float DeltaTime) override;
		FString GetDebugName() const override { return TEXT("search"); }

	private:
		Arrive MoveToLastKnownLocationBehavior{};
		Wander SearchBehavior{};
		bool bReachedLastKnownLocation{false};
		float SearchPointTolerance{70.0f};
	};
}
