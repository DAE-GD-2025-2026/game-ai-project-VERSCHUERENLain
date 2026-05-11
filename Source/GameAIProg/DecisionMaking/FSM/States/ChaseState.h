#pragma once

#include "DecisionMaking/FSM/State.h"
#include "GameAIProg/Movement/SteeringBehaviors/Steering/SteeringBehaviors.h"

namespace GameAI::FSM
{
	class ChaseState final : public State
	{
	public:
		void Enter() override;
		void Update(float DeltaTime) override;
		FString GetDebugName() const override { return TEXT("chase"); }

	private:
		Seek ChaseBehavior{};
	};
}
