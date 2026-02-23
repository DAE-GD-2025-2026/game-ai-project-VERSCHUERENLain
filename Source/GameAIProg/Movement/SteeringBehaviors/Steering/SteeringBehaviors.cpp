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

//WANDER
SteeringOutput Wander::CalculateSteering(float DeltaT, ASteeringAgent& Agent)
{
	// random angle offset from previous wander angle
	float RandomOffset = FMath::FRandRange(-MaxAngleChange, MaxAngleChange);
	WanderAngle += RandomOffset;

	// circle center = agent position + forward direction * offset distance
	float AgentYaw = FMath::DegreesToRadians(Agent.GetRotation());
	FVector2D AgentForward{FMath::Cos(AgentYaw), FMath::Sin(AgentYaw)};
	FVector2D CircleCenter = Agent.GetPosition() + AgentForward * OffsetDistance;

	// point on circle using wander angle
	FVector2D WanderTarget = CircleCenter + FVector2D{FMath::Cos(WanderAngle), FMath::Sin(WanderAngle)} * Radius;

	// seek towards the wander target
	Target.Position = WanderTarget;
	return Seek::CalculateSteering(DeltaT, Agent);
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

//FACE
SteeringOutput Face::CalculateSteering(float DeltaT, ASteeringAgent& Agent)
{
	SteeringOutput steering{};

	FVector2D Direction = Target.Position - Agent.GetPosition();
	if (Direction.IsNearlyZero())
		return steering;

	float DesiredAngle = FMath::RadiansToDegrees(FMath::Atan2(Direction.Y, Direction.X));
	float CurrentAngle = Agent.GetRotation();
	float AngleDiff = FMath::FindDeltaAngleDegrees(CurrentAngle, DesiredAngle);

	float MaxAngular = Agent.GetMaxAngularSpeed();
	steering.AngularVelocity = FMath::Clamp(AngleDiff, -MaxAngular, MaxAngular);

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

//PURSUIT
SteeringOutput Pursuit::CalculateSteering(float DeltaT, ASteeringAgent& Agent)
{
	float Distance = (Target.Position - Agent.GetPosition()).Size();
	float t = Distance / Agent.GetMaxLinearSpeed();

	FTargetData OriginalTarget = Target;
	Target.Position += Target.LinearVelocity * t;

	SteeringOutput steering = Seek::CalculateSteering(DeltaT, Agent);

	Target = OriginalTarget;
	return steering;
}

//EVADE
SteeringOutput Evade::CalculateSteering(float DeltaT, ASteeringAgent& Agent)
{
	float Distance = (Target.Position - Agent.GetPosition()).Size();
	float t = Distance / Agent.GetMaxLinearSpeed();

	FTargetData OriginalTarget = Target;
	Target.Position += Target.LinearVelocity * t;

	SteeringOutput steering = Flee::CalculateSteering(DeltaT, Agent);

	Target = OriginalTarget;

	// outside flee radius: mark invalid so priority steering can fall through
	if (steering.LinearVelocity.IsNearlyZero())
		steering.IsValid = false;

	return steering;
}