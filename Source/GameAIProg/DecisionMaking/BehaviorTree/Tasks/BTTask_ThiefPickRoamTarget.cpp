#include "DecisionMaking/BehaviorTree/Tasks/BTTask_ThiefPickRoamTarget.h"

#include "BehaviorTree/BlackboardComponent.h"
#include "DecisionMaking/BehaviorTree/BehaviorTreeBlackboardKeys.h"
#include "DecisionMaking/BehaviorTree/ThiefBTController.h"
#include "NavigationSystem.h"

UBTTask_ThiefPickRoamTarget::UBTTask_ThiefPickRoamTarget()
{
	NodeName = TEXT("pick roam target");
}

EBTNodeResult::Type UBTTask_ThiefPickRoamTarget::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	AThiefBTController* Controller = Cast<AThiefBTController>(OwnerComp.GetAIOwner());
	UBlackboardComponent* BlackboardComponent = OwnerComp.GetBlackboardComponent();
	UWorld* World = OwnerComp.GetWorld();
	if (!Controller || !BlackboardComponent || !World)
	{
		return EBTNodeResult::Failed;
	}

	FVector const RoamCenter = Controller->GetRoamCenter();
	float const RoamRadius = Controller->GetRoamRadius();
	FVector NewTarget = RoamCenter;

	if (UNavigationSystemV1* NavigationSystem = FNavigationSystem::GetCurrent<UNavigationSystemV1>(World))
	{
		FNavLocation NavLocation{};
		if (NavigationSystem->GetRandomReachablePointInRadius(RoamCenter, RoamRadius, NavLocation))
		{
			NewTarget = NavLocation.Location;
		}
		else
		{
			float const RandomAngle = FMath::FRandRange(0.0f, 2.0f * PI);
			float const RandomDistance = FMath::FRandRange(0.0f, RoamRadius);
			NewTarget.X += FMath::Cos(RandomAngle) * RandomDistance;
			NewTarget.Y += FMath::Sin(RandomAngle) * RandomDistance;
		}
	}

	BlackboardComponent->SetValueAsVector(GameAI::BT::BlackboardKeys::RoamTarget, NewTarget);
	return EBTNodeResult::Succeeded;
}
