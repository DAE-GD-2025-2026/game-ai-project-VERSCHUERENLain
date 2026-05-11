#include "BTSteeringControllerBase.h"

#include "BehaviorTree/BehaviorTree.h"
#include "BehaviorTree/BehaviorTreeComponent.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "GameAIProg/Movement/SteeringBehaviors/SteeringAgent.h"

ABTSteeringControllerBase::ABTSteeringControllerBase()
{
	PrimaryActorTick.bCanEverTick = false;
	BehaviorTreeComponent = CreateDefaultSubobject<UBehaviorTreeComponent>(TEXT("BehaviorTreeComponent"));
	BrainComponent = BehaviorTreeComponent;
}

ASteeringAgent* ABTSteeringControllerBase::GetControlledSteeringAgent() const
{
	return Cast<ASteeringAgent>(GetPawn());
}

UBlackboardComponent* ABTSteeringControllerBase::GetBlackboardComponentMutable() const
{
	return const_cast<UBlackboardComponent*>(GetBlackboardComponent());
}

void ABTSteeringControllerBase::OnUnPossess()
{
	Super::OnUnPossess();

	if (BehaviorTreeComponent && BehaviorTreeComponent->IsRunning())
	{
		BehaviorTreeComponent->StopTree(EBTStopMode::Forced);
	}

	bBehaviorTreeStarted = false;
	DebugMode = TEXT("none");
}

bool ABTSteeringControllerBase::StartRuntimeBehaviorTree()
{
	if (!BehaviorTreeComponent || !RuntimeBehaviorTree || !RuntimeBlackboard)
	{
		return false;
	}

	UBlackboardComponent* BlackboardComp{};
	if (!UseBlackboard(RuntimeBlackboard, BlackboardComp))
	{
		return false;
	}

	Blackboard = BlackboardComp;

	if (BehaviorTreeComponent->IsRunning())
	{
		BehaviorTreeComponent->StopTree(EBTStopMode::Forced);
	}

	bool const bStarted = RunBehaviorTree(RuntimeBehaviorTree);
	bBehaviorTreeStarted = bStarted;
	return bStarted;
}
