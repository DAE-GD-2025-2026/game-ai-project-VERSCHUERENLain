#include "State.h"

#include "FSMComponent.h"

AGameAIController* GameAI::FSM::State::GetAIController() const
{
	return Owner ? Owner->GetAIController() : nullptr;
}

UBlackboardComponent* GameAI::FSM::State::GetBlackboardComponent() const
{
	return Owner ? Owner->GetBlackboardComponent() : nullptr;
}

ASteeringAgent* GameAI::FSM::State::GetControlledAgent() const
{
	return Owner ? Owner->GetControlledAgent() : nullptr;
}

float GameAI::FSM::State::GetDefaultMaxLinearSpeed() const
{
	return Owner ? Owner->GetDefaultMaxLinearSpeed() : 0.0f;
}
