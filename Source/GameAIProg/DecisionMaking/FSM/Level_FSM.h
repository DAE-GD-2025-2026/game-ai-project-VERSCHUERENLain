// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "InputAction.h"
#include "BehaviorTree/BlackboardData.h"
#include "GameAIProg/Movement/SteeringBehaviors/Steering/SteeringBehaviors.h"
#include "Shared/Level_Base.h"
#include "Level_FSM.generated.h"

class AGameAIController;

UCLASS()
class GAMEAIPROG_API ALevel_FSM : public ALevel_Base
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="FSMLevel|Input")
	UInputAction* SetTargetAction{};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="FSMLevel|Agents")
	TSubclassOf<ASteeringAgent> GuardAgentClass{};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="FSMLevel|Agents")
	TSubclassOf<ASteeringAgent> ThiefAgentClass{};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="FSMLevel|AI")
	TObjectPtr<UBlackboardData> BlackboardAsset{};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="FSMLevel|AI")
	TArray<FVector> GuardPatrolPoints{};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="FSMLevel|AI")
	FVector GuardSpawnLocation{2500.0f, 2100.0f, 90.0f};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="FSMLevel|AI")
	FVector ThiefSpawnLocation{1700.0f, 2100.0f, 90.0f};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="FSMLevel|AI")
	float GuardDetectionRadius{800.0f};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="FSMLevel|AI")
	float GuardSearchDuration{5.0f};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="FSMLevel|AI")
	float GuardMaxLinearSpeed{500.0f};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="FSMLevel|AI")
	float ThiefMaxLinearSpeed{700.0f};

	ALevel_FSM();

	virtual void Tick(float DeltaTime) override;
	virtual void BindLevelInputActions() override;

protected:
	virtual void BeginPlay() override;

private:
	UPROPERTY()
	ASteeringAgent* GuardAgent{nullptr};

	UPROPERTY()
	ASteeringAgent* ThiefAgent{nullptr};

	Arrive ThiefMoveBehavior{};

	void UpdateImGui();
	void SetThiefTarget();
	void SetupGuardController();
	void DrawDebugInfo() const;
	AGameAIController* GetGuardController() const;
};
