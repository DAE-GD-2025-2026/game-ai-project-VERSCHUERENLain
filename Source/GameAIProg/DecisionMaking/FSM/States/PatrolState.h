#pragma once

#include <vector>

#include "DecisionMaking/FSM/State.h"
#include "GameAIProg/Movement/SteeringBehaviors/Steering/SteeringBehaviors.h"

namespace GameAI::FSM
{
	class PatrolState final : public State
	{
	public:
		explicit PatrolState(std::vector<FVector2D> InPatrolPoints);

		void Enter() override;
		void Update(float DeltaTime) override;
		FString GetDebugName() const override { return TEXT("patrol"); }

	private:
		Arrive PatrolBehavior{};
		std::vector<FVector2D> PatrolPoints{};
		int CurrentPatrolPointIdx{0};
		float PatrolPointTolerance{60.0f};
	};
}
