#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "GameAIProg/Movement/SteeringBehaviors/Steering/SteeringBehaviors.h"
#include "BTTask_GuardChase.generated.h"

UCLASS()
class GAMEAIPROG_API UBTTask_GuardChase : public UBTTaskNode
{
	GENERATED_BODY()

public:
	UBTTask_GuardChase();

protected:
	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
	virtual void TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;

private:
	Pursuit ChaseBehavior{};
};
