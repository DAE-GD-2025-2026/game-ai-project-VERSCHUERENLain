
#pragma once

#include "CoreMinimal.h"
#include "Movement/SteeringBehaviors/Flocking/Flock.h"
#include "Shared/Level_Base.h"
#include "Movement/SteeringBehaviors/Steering/SteeringBehaviors.h"
#include <memory>
#include "Level_FlockSpacePartitioning.generated.h"

UCLASS()
class GAMEAIPROG_API ALevel_FlockSpacePartitioning : public ALevel_Base
{
	GENERATED_BODY()

public:
	ALevel_FlockSpacePartitioning();

	virtual void Tick(float DeltaTime) override;

protected:
	virtual void BeginPlay() override;

	bool bUseMouseTarget{true};

	int FlockSize{500};
	int PendingFlockSize{500}; // slider value before apply
	TUniquePtr<Flock> pFlock{};

	ASteeringAgent* pAgentToEvade{nullptr}; // non owning, spawned in RebuildFlock

	// wander behavior for the evade agent so it moves around
	std::unique_ptr<Wander> pEvadeAgentWander{};

private:
	void RebuildFlock();
};
