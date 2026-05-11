#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "GameAIProg/Movement/SteeringBehaviors/Steering/SteeringBehaviors.h"
#include "BTTask_GuardPatrol.generated.h"

UCLASS()
class GAMEAIPROG_API UBTTask_GuardPatrol : public UBTTaskNode
{
	GENERATED_BODY()

public:
	UBTTask_GuardPatrol();

protected:
	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
	virtual void TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;

private:
	void UpdatePatrolTarget(UBehaviorTreeComponent& OwnerComp);

	Arrive PatrolBehavior{};
	float WaypointTolerance{45.0f};
};
