// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include <memory>
#include "CombinedSteeringBehaviors.h"
#include "GameAIProg/Shared/Level_Base.h"
#include "GameAIProg/Movement/SteeringBehaviors/Steering/SteeringBehaviors.h"
#include "GameAIProg/Movement/SteeringBehaviors/SteeringAgent.h"
#include "Level_CombinedSteering.generated.h"

UCLASS()
class GAMEAIPROG_API ALevel_CombinedSteering : public ALevel_Base
{
	GENERATED_BODY()

public:
	// Sets default values for this actor's properties
	ALevel_CombinedSteering();

	// Called every frame
	virtual void Tick(float DeltaTime) override;

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	virtual void BeginDestroy() override;

private:
	//Datamembers
	bool UseMouseTarget = false;
	bool CanDebugRender = false;

	ASteeringAgent* SeekerAgent{nullptr};
	ASteeringAgent* WandererAgent{nullptr};

	std::unique_ptr<Seek>             pSeekBehavior;
	std::unique_ptr<Wander>           pWanderBehavior1;   // seeker's wander (for blended)
	std::unique_ptr<Evade>            pEvadeBehavior;      // seeker evades wanderer
	std::unique_ptr<BlendedSteering>  pBlendedSteering;   // seek + wander blend
	std::unique_ptr<PrioritySteering> pPrioritySteering;  // evade first, then blended
	std::unique_ptr<Wander>           pWanderBehavior2;   // wanderer agent behavior
};
