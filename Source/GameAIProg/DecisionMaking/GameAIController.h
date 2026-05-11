// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include <vector>

#include "CoreMinimal.h"
#include "AIController.h"
#include "GameAIController.generated.h"

class ASteeringAgent;
class UBlackboardData;
class UFSMComponent;

UCLASS()
class GAMEAIPROG_API AGameAIController : public AAIController
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|FSM")
	TObjectPtr<UBlackboardData> FSMBlackboardAsset{};

	AGameAIController();

	void InitFiniteStateMachine();
	void ConfigureGuardFSM(ASteeringAgent* InTargetAgent, TArray<FVector> const& InPatrolPoints,
		float InDetectionRadius, float InSearchDuration);
	void RunFiniteStateMachine();
	UBlackboardComponent* GetBlackboardComponentMutable() const;
	FString GetCurrentStateName() const;
	bool IsTargetVisibleDebug() const;
	void SetSearchStartedAt(float InSearchStartedAt);

protected:
	virtual void BeginPlay() override;

private:
	UFSMComponent* GetFSMComponent() const;
	bool IsTargetVisible();
	bool IsSearchingTooLong() const;
	bool CheckTargetVisible(bool bWriteBlackboardData) const;

	TWeakObjectPtr<ASteeringAgent> TargetAgent{};
	std::vector<FVector2D> PatrolPoints{};
	float DetectionRadius{800.0f};
	float SearchDuration{5.0f};
	float SearchStartedAt{0.0f};
	bool bHasConfiguredFSM{false};
};
