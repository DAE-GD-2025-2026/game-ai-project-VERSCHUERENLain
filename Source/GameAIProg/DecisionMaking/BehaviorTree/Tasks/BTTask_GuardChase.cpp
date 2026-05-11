#include "DecisionMaking/BehaviorTree/Tasks/BTTask_GuardChase.h"

#include "BehaviorTree/BlackboardComponent.h"
#include "DecisionMaking/BehaviorTree/BehaviorTreeBlackboardKeys.h"
#include "DecisionMaking/BehaviorTree/GuardBTController.h"
#include "GameAIProg/Movement/SteeringBehaviors/SteeringAgent.h"

UBTTask_GuardChase::UBTTask_GuardChase()
{
	NodeName = TEXT("guard chase");
	ForceInstancing(true);
	INIT_TASK_NODE_NOTIFY_FLAGS();
}

EBTNodeResult::Type UBTTask_GuardChase::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	AGuardBTController* Controller = Cast<AGuardBTController>(OwnerComp.GetAIOwner());
	UBlackboardComponent* BlackboardComponent = OwnerComp.GetBlackboardComponent();
	ASteeringAgent* Agent = Controller ? Controller->GetControlledSteeringAgent() : nullptr;
	if (!Controller || !BlackboardComponent || !Agent)
	{
		return EBTNodeResult::Failed;
	}

	Controller->SetDebugMode(TEXT("chase"));
	Agent->SetMaxLinearSpeed(Controller->GetConfiguredMaxLinearSpeed());
	Agent->SetSteeringBehavior(&ChaseBehavior);
	return EBTNodeResult::InProgress;
}

void UBTTask_GuardChase::TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
	AGuardBTController* Controller = Cast<AGuardBTController>(OwnerComp.GetAIOwner());
	UBlackboardComponent* BlackboardComponent = OwnerComp.GetBlackboardComponent();
	ASteeringAgent* Agent = Controller ? Controller->GetControlledSteeringAgent() : nullptr;
	if (!Controller || !BlackboardComponent || !Agent)
	{
		FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
		return;
	}

	AActor* TargetActor = Cast<AActor>(BlackboardComponent->GetValueAsObject(GameAI::BT::BlackboardKeys::TargetActor));
	if (!TargetActor)
	{
		FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
		return;
	}

	FTargetData TargetData{};
	TargetData.Position = FVector2D{TargetActor->GetActorLocation().X, TargetActor->GetActorLocation().Y};
	if (ASteeringAgent* TargetSteeringAgent = Cast<ASteeringAgent>(TargetActor))
	{
		TargetData.LinearVelocity = TargetSteeringAgent->GetLinearVelocity();
	}

	ChaseBehavior.SetTarget(TargetData);
	BlackboardComponent->SetValueAsVector(GameAI::BT::BlackboardKeys::LastKnownLocation, TargetActor->GetActorLocation());
	BlackboardComponent->SetValueAsBool(GameAI::BT::BlackboardKeys::HasLastKnownLocation, true);
}
