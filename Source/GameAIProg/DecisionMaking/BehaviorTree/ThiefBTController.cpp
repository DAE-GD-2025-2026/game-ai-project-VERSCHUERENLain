#include "ThiefBTController.h"

#include "BehaviorTree/Blackboard/BlackboardKeyType_Vector.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "BehaviorTree/Tasks/BTTask_Wait.h"
#include "DecisionMaking/BehaviorTree/BehaviorTreeBlackboardKeys.h"
#include "DecisionMaking/BehaviorTree/BehaviorTreeBuilder.h"
#include "DecisionMaking/BehaviorTree/Tasks/BTTask_MoveSteeringToVector.h"
#include "DecisionMaking/BehaviorTree/Tasks/BTTask_ThiefPickRoamTarget.h"

AThiefBTController::AThiefBTController()
{
}

void AThiefBTController::ConfigureThief(FVector const& InRoamCenter, float InRoamRadius, float InThiefSpeed)
{
	RoamCenter = InRoamCenter;
	RoamRadius = InRoamRadius;
	SetConfiguredMaxLinearSpeed(InThiefSpeed);

	BuildRuntimeBlackboard();
	BuildRuntimeTree();
	bThiefConfigured = true;
	TryStartBehaviorTree();
}

FVector AThiefBTController::GetCurrentRoamTarget() const
{
	if (UBlackboardComponent* BlackboardComponent = GetBlackboardComponentMutable())
	{
		return BlackboardComponent->GetValueAsVector(GameAI::BT::BlackboardKeys::RoamTarget);
	}

	return FVector::ZeroVector;
}

void AThiefBTController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);
	TryStartBehaviorTree();
}

void AThiefBTController::BuildRuntimeBlackboard()
{
	RuntimeBlackboard = NewObject<UBlackboardData>(this);
	GameAI::BT::Builder::AddBlackboardKey<UBlackboardKeyType_Vector>(RuntimeBlackboard, GameAI::BT::BlackboardKeys::RoamTarget);
	GameAI::BT::Builder::FinalizeBlackboard(RuntimeBlackboard);
}

void AThiefBTController::BuildRuntimeTree()
{
	RuntimeBehaviorTree = GameAI::BT::Builder::CreateTree(this, RuntimeBlackboard);
	UBTComposite_Sequence* Root = GameAI::BT::Builder::SetSequenceRoot(RuntimeBehaviorTree);
	if (!Root)
	{
		return;
	}

	GameAI::BT::Builder::AddTask<UBTTask_ThiefPickRoamTarget>(*Root);

	if (UBTTask_MoveSteeringToVector* MoveTask = GameAI::BT::Builder::AddTask<UBTTask_MoveSteeringToVector>(*Root))
	{
		MoveTask->SetBlackboardKey(GameAI::BT::BlackboardKeys::RoamTarget);
	}

	UBTTask_Wait* WaitTask = GameAI::BT::Builder::AddTask<UBTTask_Wait>(*Root);
	WaitTask->WaitTime = FValueOrBBKey_Float(0.35f);
	WaitTask->RandomDeviation = FValueOrBBKey_Float(0.15f);
}

void AThiefBTController::TryStartBehaviorTree()
{
	if (!GetPawn() || !bThiefConfigured || !RuntimeBehaviorTree || !RuntimeBlackboard)
	{
		return;
	}

	StartRuntimeBehaviorTree();
}
