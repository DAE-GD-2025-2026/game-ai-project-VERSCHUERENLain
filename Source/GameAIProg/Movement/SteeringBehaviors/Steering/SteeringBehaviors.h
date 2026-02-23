#pragma once

#include <Movement/SteeringBehaviors/SteeringHelpers.h>
#include "Kismet/KismetMathLibrary.h"

class ASteeringAgent;

// SteeringBehavior base, all steering behaviors should derive from this.
class ISteeringBehavior
{
public:
	ISteeringBehavior() = default;
	virtual ~ISteeringBehavior() = default;

	// Override to implement your own behavior
	virtual SteeringOutput CalculateSteering(float DeltaT, ASteeringAgent & Agent) = 0;

	void SetTarget(const FTargetData& NewTarget) { Target = NewTarget; }
	const FTargetData& GetTarget() const { return Target; }
	template<class T, std::enable_if_t<std::is_base_of_v<ISteeringBehavior, T>>* = nullptr>
	T* As()
	{ return static_cast<T*>(this); }

protected:
	FTargetData Target;
};

// Your own SteeringBehaviors should follow here...

//SEEK
//****
class Seek : public ISteeringBehavior
{
public:
	Seek() = default;
	virtual ~Seek() = default;

	SteeringOutput CalculateSteering(float DeltaT, ASteeringAgent& Agent) override;

protected:
	float StopRadius{10.f};
};

//WANDER
class Wander : public Seek
{
public:
	Wander() = default;
	virtual ~Wander() = default;

	SteeringOutput CalculateSteering(float DeltaT, ASteeringAgent& Agent) override;

	void SetWanderOffset(float offset) { OffsetDistance = offset; }
	void SetWanderRadius(float radius) { Radius = radius; }
	void SetMaxAngleChange(float rad) { MaxAngleChange = rad; }

	float GetOffsetDistance() const { return OffsetDistance; }
	float GetRadius() const { return Radius; }
	float GetWanderAngle() const { return WanderAngle; }

protected:
	float OffsetDistance{200.f};
	float Radius{100.f};
	float MaxAngleChange{FMath::DegreesToRadians(10.f)};
	float WanderAngle{0.f};
};

//FLEE
class Flee : public Seek
{
public:
	Flee() = default;
	virtual ~Flee() = default;

	SteeringOutput CalculateSteering(float DeltaT, ASteeringAgent& Agent) override;
	float GetFleeRadius() const { return FleeRadius; }

protected:
	float FleeRadius{300.f};
};

//ARRIVE
class Arrive : public Seek
{
public:
	Arrive() = default;
	virtual ~Arrive() = default;

	SteeringOutput CalculateSteering(float DeltaT, ASteeringAgent& Agent) override;
	float GetSlowRadius() const { return SlowRadius; }
	float GetTargetRadius() const { return TargetRadius; }
	float GetOriginalMaxSpeed() const { return OriginalMaxSpeed; }

protected:
	float SlowRadius{200.f};
	float TargetRadius{10.f};
	float OriginalMaxSpeed{0.f};
};

//FACE
class Face : public ISteeringBehavior
{
public:
	Face() = default;
	virtual ~Face() = default;

	SteeringOutput CalculateSteering(float DeltaT, ASteeringAgent& Agent) override;
};

//PURSUIT
class Pursuit : public Seek
{
public:
	Pursuit() = default;
	virtual ~Pursuit() = default;

	SteeringOutput CalculateSteering(float DeltaT, ASteeringAgent& Agent) override;
};

//EVADE
class Evade : public Flee
{
public:
	Evade() = default;
	virtual ~Evade() = default;

	SteeringOutput CalculateSteering(float DeltaT, ASteeringAgent& Agent) override;
};
