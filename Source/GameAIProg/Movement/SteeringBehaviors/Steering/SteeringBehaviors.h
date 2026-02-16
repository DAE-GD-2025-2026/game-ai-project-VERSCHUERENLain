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
