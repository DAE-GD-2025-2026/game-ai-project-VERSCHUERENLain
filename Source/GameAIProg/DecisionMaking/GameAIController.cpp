// Fill out your copyright notice in the Description page of Project Settings.

#include "GameAIController.h"

#include "BehaviorTree/BlackboardComponent.h"
#include "BehaviorTree/BlackboardData.h"
#include "DecisionMaking/FSM/FSMBlackboardKeys.h"
#include "DecisionMaking/FSM/FSMComponent.h"
#include "DecisionMaking/FSM/States/ChaseState.h"
#include "DecisionMaking/FSM/States/PatrolState.h"
#include "DecisionMaking/FSM/States/SearchState.h"
#include "GameAIProg/Movement/SteeringBehaviors/SteeringAgent.h"
#include "UObject/ConstructorHelpers.h"
#include <vector>

AGameAIController::AGameAIController()
{
	PrimaryActorTick.bCanEverTick = false;
	BrainComponent = CreateDefaultSubobject<UFSMComponent>(TEXT("FSMComponent"));

	static ConstructorHelpers::FObjectFinder<UBlackboardData> BlackboardFinder(
		TEXT("/Game/DecisionMaking/BB_TEST.BB_TEST"));
	if (BlackboardFinder.Succeeded())
	{
		FSMBlackboardAsset = BlackboardFinder.Object;
	}
}

void AGameAIController::BeginPlay()
{
	Super::BeginPlay();
	InitFiniteStateMachine();
}

void AGameAIController::InitFiniteStateMachine()
{
	if (!FSMBlackboardAsset)
	{
		return;
	}

	UBlackboardComponent* BlackboardComp{};
	if (UseBlackboard(FSMBlackboardAsset, BlackboardComp))
	{
		Blackboard = BlackboardComp;
	}
}

void AGameAIController::ConfigureGuardFSM(ASteeringAgent* InTargetAgent, TArray<FVector> const& InPatrolPoints,
	float InDetectionRadius, float InSearchDuration)
{
	TargetAgent = InTargetAgent;
	DetectionRadius = InDetectionRadius;
	SearchDuration = InSearchDuration;

	PatrolPoints.clear();
	PatrolPoints.reserve(InPatrolPoints.Num());
	for (FVector const& PatrolPoint : InPatrolPoints)
	{
		PatrolPoints.emplace_back(PatrolPoint.X, PatrolPoint.Y);
	}

	InitFiniteStateMachine();

	UFSMComponent* const FSMComponent = GetFSMComponent();
	if (!FSMComponent || PatrolPoints.empty())
	{
		return;
	}

	FSMComponent->ResetFSM();

	auto PatrolStateInstance = std::make_unique<GameAI::FSM::PatrolState>(PatrolPoints);
	GameAI::FSM::State* const PatrolState = PatrolStateInstance.get();
	FSMComponent->AddState(std::move(PatrolStateInstance));

	auto ChaseStateInstance = std::make_unique<GameAI::FSM::ChaseState>();
	GameAI::FSM::State* const ChaseState = ChaseStateInstance.get();
	FSMComponent->AddState(std::move(ChaseStateInstance));

	auto SearchStateInstance = std::make_unique<GameAI::FSM::SearchState>();
	GameAI::FSM::State* const SearchState = SearchStateInstance.get();
	FSMComponent->AddState(std::move(SearchStateInstance));

	FSMComponent->AddTransition(PatrolState, ChaseState, [this]() { return IsTargetVisible(); });
	FSMComponent->AddTransition(ChaseState, SearchState, [this]() { return !IsTargetVisible(); });
	FSMComponent->AddTransition(SearchState, ChaseState, [this]() { return IsTargetVisible(); });
	FSMComponent->AddTransition(SearchState, PatrolState, [this]() { return IsSearchingTooLong(); });
	FSMComponent->SetInitialState(PatrolState);

	if (UBlackboardComponent* const BlackboardComponent = GetBlackboardComponentMutable())
	{
		BlackboardComponent->SetValueAsObject(GameAI::FSM::BlackboardKeys::TargetActor, InTargetAgent);
	}

	bHasConfiguredFSM = true;
}

void AGameAIController::RunFiniteStateMachine()
{
	if (!bHasConfiguredFSM)
	{
		return;
	}

	if (UFSMComponent* const FSMComponent = GetFSMComponent())
	{
		FSMComponent->StartLogic();
	}
}

FString AGameAIController::GetCurrentStateName() const
{
	if (UFSMComponent* const FSMComponent = GetFSMComponent())
	{
		return FSMComponent->GetCurrentStateName();
	}

	return TEXT("none");
}

bool AGameAIController::IsTargetVisibleDebug() const
{
	return CheckTargetVisible(false);
}

void AGameAIController::SetSearchStartedAt(float InSearchStartedAt)
{
	SearchStartedAt = InSearchStartedAt;
}

UFSMComponent* AGameAIController::GetFSMComponent() const
{
	return BrainComponent ? Cast<UFSMComponent>(BrainComponent) : FindComponentByClass<UFSMComponent>();
}

UBlackboardComponent* AGameAIController::GetBlackboardComponentMutable() const
{
	return const_cast<UBlackboardComponent*>(GetBlackboardComponent());
}

bool AGameAIController::IsTargetVisible()
{
	return CheckTargetVisible(true);
}

bool AGameAIController::CheckTargetVisible(bool bWriteBlackboardData) const
{
	ASteeringAgent* const GuardAgent = Cast<ASteeringAgent>(GetPawn());
	ASteeringAgent* const Target = TargetAgent.Get();
	UBlackboardComponent* const BlackboardComponent = bWriteBlackboardData ? GetBlackboardComponentMutable() : nullptr;
	UWorld* const World = GetWorld();

	if (!GuardAgent || !Target || !World)
	{
		return false;
	}

	if ((Target->GetPosition() - GuardAgent->GetPosition()).Size() > DetectionRadius)
	{
		return false;
	}

	FCollisionQueryParams QueryParams{};
	QueryParams.AddIgnoredActor(GuardAgent);

	FHitResult HitResult{};
	bool const bHasBlockingHit = World->LineTraceSingleByChannel(
		HitResult,
		GuardAgent->GetActorLocation(),
		Target->GetActorLocation(),
		ECC_Visibility,
		QueryParams);

	bool const bIsVisible = !bHasBlockingHit || HitResult.GetActor() == Target;
	if (!bIsVisible)
	{
		return false;
	}

	if (bWriteBlackboardData && BlackboardComponent)
	{
		BlackboardComponent->SetValueAsObject(GameAI::FSM::BlackboardKeys::TargetActor, Target);
		BlackboardComponent->SetValueAsVector(GameAI::FSM::BlackboardKeys::LastKnownLocation, Target->GetActorLocation());
	}

	return true;
}

bool AGameAIController::IsSearchingTooLong() const
{
	UWorld* const World = GetWorld();
	if (!World)
	{
		return false;
	}

	return World->GetTimeSeconds() - SearchStartedAt >= SearchDuration;
}
