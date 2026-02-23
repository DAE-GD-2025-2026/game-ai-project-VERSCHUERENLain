#include "FlockingSteeringBehaviors.h"
#include "Flock.h"
#include "../SteeringAgent.h"
#include "../SteeringHelpers.h"

//COHESION (FLOCKING)
SteeringOutput Cohesion::CalculateSteering(float deltaT, ASteeringAgent& pAgent)
{
	if (pFlock->GetNrOfNeighbors() == 0)
		return SteeringOutput{};

	FSteeringParams target{};
	target.Position = pFlock->GetAverageNeighborPos();
	SetTarget(target);
	return Seek::CalculateSteering(deltaT, pAgent);
}

//SEPARATION (FLOCKING)
SteeringOutput Separation::CalculateSteering(float deltaT, ASteeringAgent& pAgent)
{
	SteeringOutput output{};
	if (pFlock->GetNrOfNeighbors() == 0)
		return output;

	FVector2D agentPos = pAgent.GetPosition();
	for (int i = 0; i < pFlock->GetNrOfNeighbors(); ++i)
	{
		FVector2D toAgent = agentPos - pFlock->GetNeighbors()[i]->GetPosition();
		float dist = toAgent.Length();
		if (dist > 0.f)
			output.LinearVelocity += toAgent.GetSafeNormal() * (1.f / dist);
	}

	// normalize accumulated repulsion direction then scale to max speed
	// matches the seek output contract: desired velocity of magnitude MaxSpeed
	float len = output.LinearVelocity.Length();
	if (len > 0.f)
		output.LinearVelocity = (output.LinearVelocity / len) * pAgent.GetMaxLinearSpeed();

	output.IsValid = true;
	return output;
}

//VELOCITY MATCH (FLOCKING)
SteeringOutput VelocityMatch::CalculateSteering(float deltaT, ASteeringAgent& pAgent)
{
	SteeringOutput output{};
	if (pFlock->GetNrOfNeighbors() == 0)
		return output;

	FVector2D avgVel = pFlock->GetAverageNeighborVelocity();
	float len = avgVel.Length();
	if (len > 0.f)
		output.LinearVelocity = (avgVel / len) * pAgent.GetMaxLinearSpeed();

	output.IsValid = true;
	return output;
}
