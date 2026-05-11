#include "PatrolState.h"

#include "GameAIProg/Movement/SteeringBehaviors/SteeringAgent.h"

GameAI::FSM::PatrolState::PatrolState(std::vector<FVector2D> InPatrolPoints)
	: PatrolPoints(std::move(InPatrolPoints))
{
}

void GameAI::FSM::PatrolState::Enter()
{
	ASteeringAgent* const Agent = GetControlledAgent();
	if (!Agent || PatrolPoints.empty())
	{
		return;
	}

	Agent->SetMaxLinearSpeed(GetDefaultMaxLinearSpeed());
	Agent->SetSteeringBehavior(&PatrolBehavior);
	PatrolBehavior.SetTarget(FTargetData{PatrolPoints[CurrentPatrolPointIdx]});
}

void GameAI::FSM::PatrolState::Update(float DeltaTime)
{
	ASteeringAgent* const Agent = GetControlledAgent();
	if (!Agent || PatrolPoints.empty())
	{
		return;
	}

	FVector2D const CurrentPatrolPoint = PatrolPoints[CurrentPatrolPointIdx];
	if ((CurrentPatrolPoint - Agent->GetPosition()).Size() <= PatrolPointTolerance)
	{
		CurrentPatrolPointIdx = (CurrentPatrolPointIdx + 1) % static_cast<int>(PatrolPoints.size());
	}

	PatrolBehavior.SetTarget(FTargetData{PatrolPoints[CurrentPatrolPointIdx]});
}
