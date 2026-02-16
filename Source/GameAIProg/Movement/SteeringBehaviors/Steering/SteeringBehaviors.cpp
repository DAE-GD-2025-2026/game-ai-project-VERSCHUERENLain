#include "SteeringBehaviors.h"
#include "GameAIProg/Movement/SteeringBehaviors/SteeringAgent.h"

//SEEK
SteeringOutput Seek::CalculateSteering(float DeltaT, ASteeringAgent& Agent)
{
	SteeringOutput steering{};
	steering.LinearVelocity = Target.Position - Agent.GetPosition();

	float Distance = steering.LinearVelocity.Size();
	if (Distance < StopRadius)
	{
		steering.LinearVelocity = FVector2D::ZeroVector;
		return steering;
	}

	steering.LinearVelocity /= Distance; // normalize
	steering.LinearVelocity *= Agent.GetMaxLinearSpeed();
	return steering;
}

//FLEE
SteeringOutput Flee::CalculateSteering(float DeltaT, ASteeringAgent& Agent)
{
	SteeringOutput steering{};
	steering.LinearVelocity = Agent.GetPosition() - Target.Position;

	float Distance = steering.LinearVelocity.Size();
	if (Distance > FleeRadius)
	{
		steering.LinearVelocity = FVector2D::ZeroVector;
		return steering;
	}

	steering.LinearVelocity /= Distance;
	steering.LinearVelocity *= Agent.GetMaxLinearSpeed();
	return steering;
}

//ARRIVE
SteeringOutput Arrive::CalculateSteering(float DeltaT, ASteeringAgent& Agent)
{
	if (OriginalMaxSpeed == 0.f)
		OriginalMaxSpeed = Agent.GetMaxLinearSpeed();

	SteeringOutput steering{};
	steering.LinearVelocity = Target.Position - Agent.GetPosition();

	float Distance = steering.LinearVelocity.Size();

	if (Distance < TargetRadius)
	{
		Agent.SetMaxLinearSpeed(OriginalMaxSpeed);
		steering.LinearVelocity = FVector2D::ZeroVector;
		return steering;
	}

	if (Distance > SlowRadius)
	{
		Agent.SetMaxLinearSpeed(OriginalMaxSpeed);
	}
	else
	{
		float speed = OriginalMaxSpeed * (Distance - TargetRadius) / (SlowRadius - TargetRadius);
		Agent.SetMaxLinearSpeed(speed);
	}

	steering.LinearVelocity /= Distance;
	steering.LinearVelocity *= Agent.GetMaxLinearSpeed();
	return steering;
}