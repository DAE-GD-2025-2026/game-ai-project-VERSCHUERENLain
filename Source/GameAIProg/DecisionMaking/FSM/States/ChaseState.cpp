#include "ChaseState.h"

#include "BehaviorTree/BlackboardComponent.h"
#include "DecisionMaking/FSM/FSMBlackboardKeys.h"
#include "GameAIProg/Movement/SteeringBehaviors/SteeringAgent.h"

void GameAI::FSM::ChaseState::Enter()
{
	ASteeringAgent* const Agent = GetControlledAgent();
	if (!Agent)
	{
		return;
	}

	Agent->SetMaxLinearSpeed(GetDefaultMaxLinearSpeed());
	Agent->SetSteeringBehavior(&ChaseBehavior);
}

void GameAI::FSM::ChaseState::Update(float DeltaTime)
{
	ASteeringAgent* const Agent = GetControlledAgent();
	UBlackboardComponent* const BlackboardComponent = GetBlackboardComponent();
	if (!Agent || !BlackboardComponent)
	{
		return;
	}

	AActor* const TargetActor = Cast<AActor>(BlackboardComponent->GetValueAsObject(BlackboardKeys::TargetActor));
	if (!TargetActor)
	{
		return;
	}

	FVector const TargetLocation = TargetActor->GetActorLocation();
	ChaseBehavior.SetTarget(FTargetData{FVector2D{TargetLocation.X, TargetLocation.Y}});
	BlackboardComponent->SetValueAsVector(BlackboardKeys::LastKnownLocation, TargetActor->GetActorLocation());
}
