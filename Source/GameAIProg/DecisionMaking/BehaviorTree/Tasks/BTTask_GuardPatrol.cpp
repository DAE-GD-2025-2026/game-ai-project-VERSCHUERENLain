#include "DecisionMaking/BehaviorTree/Tasks/BTTask_GuardPatrol.h"

#include "BehaviorTree/BlackboardComponent.h"
#include "DecisionMaking/BehaviorTree/BehaviorTreeBlackboardKeys.h"
#include "DecisionMaking/BehaviorTree/GuardBTController.h"
#include "GameAIProg/Movement/SteeringBehaviors/SteeringAgent.h"

UBTTask_GuardPatrol::UBTTask_GuardPatrol()
{
	NodeName = TEXT("guard patrol");
	ForceInstancing(true);
	INIT_TASK_NODE_NOTIFY_FLAGS();
}

EBTNodeResult::Type UBTTask_GuardPatrol::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	AGuardBTController* Controller = Cast<AGuardBTController>(OwnerComp.GetAIOwner());
	UBlackboardComponent* BlackboardComponent = OwnerComp.GetBlackboardComponent();
	ASteeringAgent* Agent = Controller ? Controller->GetControlledSteeringAgent() : nullptr;
	if (!Controller || !BlackboardComponent || !Agent || Controller->GetPatrolPoints().empty())
	{
		return EBTNodeResult::Failed;
	}

	Controller->SetDebugMode(TEXT("patrol"));
	Agent->SetMaxLinearSpeed(Controller->GetConfiguredMaxLinearSpeed());
	Agent->SetSteeringBehavior(&PatrolBehavior);
	UpdatePatrolTarget(OwnerComp);
	return EBTNodeResult::InProgress;
}

void UBTTask_GuardPatrol::TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
	AGuardBTController* Controller = Cast<AGuardBTController>(OwnerComp.GetAIOwner());
	UBlackboardComponent* BlackboardComponent = OwnerComp.GetBlackboardComponent();
	ASteeringAgent* Agent = Controller ? Controller->GetControlledSteeringAgent() : nullptr;
	if (!Controller || !BlackboardComponent || !Agent || Controller->GetPatrolPoints().empty())
	{
		FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
		return;
	}

	int32 PatrolIndex = BlackboardComponent->GetValueAsInt(GameAI::BT::BlackboardKeys::PatrolIndex);
	if (!Controller->GetPatrolPoints().empty())
	{
		PatrolIndex = FMath::Clamp(PatrolIndex, 0, static_cast<int32>(Controller->GetPatrolPoints().size() - 1));
	}

	FVector2D const CurrentPatrolPoint = Controller->GetPatrolPoints()[PatrolIndex];
	PatrolBehavior.SetTarget(FTargetData{CurrentPatrolPoint});

	if ((CurrentPatrolPoint - Agent->GetPosition()).Size() <= WaypointTolerance)
	{
		int32 const NextPatrolIndex = (PatrolIndex + 1) % static_cast<int32>(Controller->GetPatrolPoints().size());
		BlackboardComponent->SetValueAsInt(GameAI::BT::BlackboardKeys::PatrolIndex, NextPatrolIndex);
		UpdatePatrolTarget(OwnerComp);
	}
}

void UBTTask_GuardPatrol::UpdatePatrolTarget(UBehaviorTreeComponent& OwnerComp)
{
	AGuardBTController* Controller = Cast<AGuardBTController>(OwnerComp.GetAIOwner());
	UBlackboardComponent* BlackboardComponent = OwnerComp.GetBlackboardComponent();
	if (!Controller || !BlackboardComponent || Controller->GetPatrolPoints().empty())
	{
		return;
	}

	int32 PatrolIndex = BlackboardComponent->GetValueAsInt(GameAI::BT::BlackboardKeys::PatrolIndex);
	PatrolIndex = FMath::Clamp(PatrolIndex, 0, static_cast<int32>(Controller->GetPatrolPoints().size() - 1));
	PatrolBehavior.SetTarget(FTargetData{Controller->GetPatrolPoints()[PatrolIndex]});
}
