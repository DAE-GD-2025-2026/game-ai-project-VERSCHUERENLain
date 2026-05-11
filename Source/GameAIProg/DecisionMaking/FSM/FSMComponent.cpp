// Fill out your copyright notice in the Description page of Project Settings.

#include "FSMComponent.h"

#include "DecisionMaking/GameAIController.h"
#include "GameAIProg/Movement/SteeringBehaviors/SteeringAgent.h"

UFSMComponent::UFSMComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	FSMInstance = std::make_unique<GameAI::FSM::FSM>();
}

void UFSMComponent::BeginPlay()
{
	Super::BeginPlay();

	if (ASteeringAgent* const Agent = GetControlledAgent())
	{
		DefaultMaxLinearSpeed = Agent->GetMaxLinearSpeed();
	}
}

void UFSMComponent::TickComponent(float DeltaTime, ELevelTick TickType,
	FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (!bIsRunning || !FSMInstance)
	{
		return;
	}

	FSMInstance->Update(DeltaTime);
}

void UFSMComponent::StartLogic()
{
	if (bIsRunning || !FSMInstance)
	{
		return;
	}

	FSMInstance->Start();
	bIsRunning = FSMInstance->GetCurrentState() != nullptr;
}

void UFSMComponent::StopLogic(const FString& Reason)
{
	Super::StopLogic(Reason);

	if (!FSMInstance)
	{
		bIsRunning = false;
		return;
	}

	FSMInstance->Stop();
	bIsRunning = false;
}

bool UFSMComponent::IsRunning() const
{
	return bIsRunning;
}

void UFSMComponent::ResetFSM()
{
	if (bIsRunning)
	{
		StopLogic(TEXT("fsm reset"));
	}

	FSMInstance = std::make_unique<GameAI::FSM::FSM>();
}

void UFSMComponent::AddState(std::unique_ptr<GameAI::FSM::State>&& NewState)
{
	if (!FSMInstance || !NewState)
	{
		return;
	}

	NewState->SetOwner(this);
	FSMInstance->AddState(std::move(NewState));
}

void UFSMComponent::AddTransition(GameAI::FSM::State* From, GameAI::FSM::State* To, std::function<bool()> EvalFunc)
{
	if (!FSMInstance)
	{
		return;
	}

	FSMInstance->AddTransition(From, To, std::move(EvalFunc));
}

void UFSMComponent::SetInitialState(GameAI::FSM::State* InitialState)
{
	if (!FSMInstance)
	{
		return;
	}

	FSMInstance->SetInitialState(InitialState);
}

ASteeringAgent* UFSMComponent::GetControlledAgent() const
{
	return GetAIOwner() ? Cast<ASteeringAgent>(GetAIOwner()->GetPawn()) : nullptr;
}

UBlackboardComponent* UFSMComponent::GetBlackboardComponent() const
{
	return GetAIOwner() ? GetAIOwner()->GetBlackboardComponent() : nullptr;
}

AGameAIController* UFSMComponent::GetAIController() const
{
	return GetAIOwner() ? Cast<AGameAIController>(GetAIOwner()) : nullptr;
}

float UFSMComponent::GetDefaultMaxLinearSpeed() const
{
	if (DefaultMaxLinearSpeed > 0.0f)
	{
		return DefaultMaxLinearSpeed;
	}

	if (ASteeringAgent* const Agent = GetControlledAgent())
	{
		return Agent->GetMaxLinearSpeed();
	}

	return 0.0f;
}

FString UFSMComponent::GetCurrentStateName() const
{
	if (!FSMInstance || !FSMInstance->GetCurrentState())
	{
		return TEXT("none");
	}

	return FSMInstance->GetCurrentState()->GetDebugName();
}
