#include "DecisionMaking/BehaviorTree/Tasks/BTTask_MoveSteeringToVector.h"

#include "BehaviorTree/BlackboardComponent.h"
#include "DecisionMaking/BehaviorTree/BTSteeringControllerBase.h"
#include "GameAIProg/Movement/SteeringBehaviors/SteeringAgent.h"

UBTTask_MoveSteeringToVector::UBTTask_MoveSteeringToVector()
{
	NodeName = TEXT("move steering to vector");
	ForceInstancing(true);
	INIT_TASK_NODE_NOTIFY_FLAGS();
}

EBTNodeResult::Type UBTTask_MoveSteeringToVector::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	ABTSteeringControllerBase* Controller = Cast<ABTSteeringControllerBase>(OwnerComp.GetAIOwner());
	UBlackboardComponent* BlackboardComponent = OwnerComp.GetBlackboardComponent();
	ASteeringAgent* Agent = Controller ? Controller->GetControlledSteeringAgent() : nullptr;
	if (!Controller || !BlackboardComponent || !Agent)
	{
		return EBTNodeResult::Failed;
	}

	Agent->SetMaxLinearSpeed(Controller->GetConfiguredMaxLinearSpeed());
	Agent->SetSteeringBehavior(&MoveBehavior);

	FVector const TargetLocation = BlackboardComponent->GetValueAsVector(BlackboardKeyName);
	MoveBehavior.SetTarget(FTargetData{FVector2D{TargetLocation.X, TargetLocation.Y}});
	return EBTNodeResult::InProgress;
}

void UBTTask_MoveSteeringToVector::TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
	ABTSteeringControllerBase* Controller = Cast<ABTSteeringControllerBase>(OwnerComp.GetAIOwner());
	UBlackboardComponent* BlackboardComponent = OwnerComp.GetBlackboardComponent();
	ASteeringAgent* Agent = Controller ? Controller->GetControlledSteeringAgent() : nullptr;
	if (!Controller || !BlackboardComponent || !Agent)
	{
		FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
		return;
	}

	FVector const TargetLocation = BlackboardComponent->GetValueAsVector(BlackboardKeyName);
	FVector2D const Target2D{TargetLocation.X, TargetLocation.Y};
	MoveBehavior.SetTarget(FTargetData{Target2D});

	if ((Target2D - Agent->GetPosition()).Size() <= AcceptableRadius)
	{
		FinishLatentTask(OwnerComp, EBTNodeResult::Succeeded);
	}
}

EBTNodeResult::Type UBTTask_MoveSteeringToVector::AbortTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	if (ABTSteeringControllerBase* Controller = Cast<ABTSteeringControllerBase>(OwnerComp.GetAIOwner()))
	{
		if (ASteeringAgent* Agent = Controller->GetControlledSteeringAgent())
		{
			Agent->SetSteeringBehavior(nullptr);
		}
	}

	return EBTNodeResult::Aborted;
}
