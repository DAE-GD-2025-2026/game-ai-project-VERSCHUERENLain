// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include <memory>

#include "CoreMinimal.h"
#include "BrainComponent.h"
#include "FSM.h"
#include "FSMComponent.generated.h"

class AGameAIController;
class ASteeringAgent;
class UBlackboardComponent;

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class GAMEAIPROG_API UFSMComponent : public UBrainComponent
{
	GENERATED_BODY()

public:
	UFSMComponent();

	virtual void TickComponent(float DeltaTime, ELevelTick TickType,
		FActorComponentTickFunction* ThisTickFunction) override;

	virtual void StartLogic() override;
	virtual void StopLogic(const FString& Reason) override;
	virtual bool IsRunning() const override;

	void ResetFSM();
	void AddState(std::unique_ptr<GameAI::FSM::State>&& NewState);
	void AddTransition(GameAI::FSM::State* From, GameAI::FSM::State* To, std::function<bool()> EvalFunc);
	void SetInitialState(GameAI::FSM::State* InitialState);

	ASteeringAgent* GetControlledAgent() const;
	UBlackboardComponent* GetBlackboardComponent() const;
	AGameAIController* GetAIController() const;
	float GetDefaultMaxLinearSpeed() const;
	FString GetCurrentStateName() const;

protected:
	virtual void BeginPlay() override;

private:
	std::unique_ptr<GameAI::FSM::FSM> FSMInstance{};
	bool bIsRunning{false};
	float DefaultMaxLinearSpeed{0.0f};
};
