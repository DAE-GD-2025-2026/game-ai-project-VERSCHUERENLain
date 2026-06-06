#include "DecisionMaking/BehaviorTree/Tasks/BTTask_GuardSearch.h"

#include "BehaviorTree/BlackboardComponent.h"
#include "DecisionMaking/BehaviorTree/BehaviorTreeBlackboardKeys.h"
#include "DecisionMaking/BehaviorTree/GuardBTController.h"
#include "GameAIProg/Movement/SteeringBehaviors/SteeringAgent.h"

UBTTask_GuardSearch::UBTTask_GuardSearch()
{
	NodeName = TEXT("guard search");
	ForceInstancing(true);
	INIT_TASK_NODE_NOTIFY_FLAGS();
}

EBTNodeResult::Type UBTTask_GuardSearch::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	AGuardBTController* Controller = Cast<AGuardBTController>(OwnerComp.GetAIOwner());
	UBlackboardComponent* BlackboardComponent = OwnerComp.GetBlackboardComponent();
	ASteeringAgent* Agent = Controller ? Controller->GetControlledSteeringAgent() : nullptr;
	if (!Controller || !BlackboardComponent || !Agent ||
		!BlackboardComponent->GetValueAsBool(GameAI::BT::BlackboardKeys::HasLastKnownLocation))
	{
		return EBTNodeResult::Failed;
	}

	Controller->SetDebugMode(TEXT("search"));
	Agent->SetMaxLinearSpeed(Controller->GetConfiguredMaxLinearSpeed());
	Agent->SetSteeringBehavior(&MoveToLastKnownLocationBehavior);
	bReachedLastKnownLocation = false;

	FVector const LastKnownLocation3D = BlackboardComponent->GetValueAsVector(GameAI::BT::BlackboardKeys::LastKnownLocation);
	SearchAnchor = FVector2D{LastKnownLocation3D.X, LastKnownLocation3D.Y};
	CurrentSearchPoint = SearchAnchor;
	MoveToLastKnownLocationBehavior.SetTarget(FTargetData{SearchAnchor});
	return EBTNodeResult::InProgress;
}

void UBTTask_GuardSearch::TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
	AGuardBTController* Controller = Cast<AGuardBTController>(OwnerComp.GetAIOwner());
	UBlackboardComponent* BlackboardComponent = OwnerComp.GetBlackboardComponent();
	ASteeringAgent* Agent = Controller ? Controller->GetControlledSteeringAgent() : nullptr;
	if (!Controller || !BlackboardComponent || !Agent)
	{
		FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
		return;
	}

	if (!BlackboardComponent->GetValueAsBool(GameAI::BT::BlackboardKeys::HasLastKnownLocation))
	{
		FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
		return;
	}

	if (!bReachedLastKnownLocation)
	{
		FVector const LastKnownLocation3D = BlackboardComponent->GetValueAsVector(GameAI::BT::BlackboardKeys::LastKnownLocation);
		SearchAnchor = FVector2D{LastKnownLocation3D.X, LastKnownLocation3D.Y};
		MoveToLastKnownLocationBehavior.SetTarget(FTargetData{SearchAnchor});

		if ((SearchAnchor - Agent->GetPosition()).Size() <= SearchPointTolerance)
		{
			bReachedLastKnownLocation = true;
			Agent->SetMaxLinearSpeed(Controller->GetConfiguredMaxLinearSpeed());
			PickNextSearchPoint();
			SearchPointBehavior.SetTarget(FTargetData{CurrentSearchPoint});
			Agent->SetSteeringBehavior(&SearchPointBehavior);
		}

		return;
	}

	if ((CurrentSearchPoint - Agent->GetPosition()).Size() <= SearchPointTolerance)
	{
		PickNextSearchPoint();
	}

	SearchPointBehavior.SetTarget(FTargetData{CurrentSearchPoint});
	Agent->SetSteeringBehavior(&SearchPointBehavior);
}

void UBTTask_GuardSearch::PickNextSearchPoint()
{
	FVector2D const RandomOffset = FMath::RandPointInCircle(SearchRadius);
	CurrentSearchPoint = SearchAnchor + RandomOffset;
}
