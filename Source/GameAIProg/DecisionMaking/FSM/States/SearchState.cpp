#include "SearchState.h"

#include "BehaviorTree/BlackboardComponent.h"
#include "DecisionMaking/GameAIController.h"
#include "DecisionMaking/FSM/FSMBlackboardKeys.h"
#include "GameAIProg/Movement/SteeringBehaviors/SteeringAgent.h"

void GameAI::FSM::SearchState::Enter()
{
	ASteeringAgent* const Agent = GetControlledAgent();
	UBlackboardComponent* const BlackboardComponent = GetBlackboardComponent();
	UWorld* const World = Agent ? Agent->GetWorld() : nullptr;
	if (!Agent || !BlackboardComponent || !World)
	{
		return;
	}

	bReachedLastKnownLocation = false;
	Agent->SetMaxLinearSpeed(GetDefaultMaxLinearSpeed());
	Agent->SetSteeringBehavior(&MoveToLastKnownLocationBehavior);
	if (AGameAIController* const AIController = GetAIController())
	{
		AIController->SetSearchStartedAt(World->GetTimeSeconds());
	}
	FVector const LastKnownLocation3D = BlackboardComponent->GetValueAsVector(BlackboardKeys::LastKnownLocation);
	MoveToLastKnownLocationBehavior.SetTarget(FTargetData{FVector2D{LastKnownLocation3D.X, LastKnownLocation3D.Y}});
}

void GameAI::FSM::SearchState::Update(float DeltaTime)
{
	ASteeringAgent* const Agent = GetControlledAgent();
	UBlackboardComponent* const BlackboardComponent = GetBlackboardComponent();
	if (!Agent || !BlackboardComponent)
	{
		return;
	}

	if (!bReachedLastKnownLocation)
	{
		FVector const LastKnownLocation3D = BlackboardComponent->GetValueAsVector(BlackboardKeys::LastKnownLocation);
		FVector2D const LastKnownLocation = FVector2D{LastKnownLocation3D.X, LastKnownLocation3D.Y};
		MoveToLastKnownLocationBehavior.SetTarget(FTargetData{LastKnownLocation});

		if ((LastKnownLocation - Agent->GetPosition()).Size() <= SearchPointTolerance)
		{
			bReachedLastKnownLocation = true;
			Agent->SetMaxLinearSpeed(GetDefaultMaxLinearSpeed());
			Agent->SetSteeringBehavior(&SearchBehavior);
		}

		return;
	}

	Agent->SetSteeringBehavior(&SearchBehavior);
}
