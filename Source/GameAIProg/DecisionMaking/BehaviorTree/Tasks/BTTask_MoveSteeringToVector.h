#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "GameAIProg/Movement/SteeringBehaviors/Steering/SteeringBehaviors.h"
#include "BTTask_MoveSteeringToVector.generated.h"

UCLASS()
class GAMEAIPROG_API UBTTask_MoveSteeringToVector : public UBTTaskNode
{
	GENERATED_BODY()

public:
	UBTTask_MoveSteeringToVector();

	void SetBlackboardKey(FName const InBlackboardKeyName) { BlackboardKeyName = InBlackboardKeyName; }

protected:
	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
	virtual void TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;
	virtual EBTNodeResult::Type AbortTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;

	UPROPERTY()
	FName BlackboardKeyName{};

	Arrive MoveBehavior{};
	float AcceptableRadius{45.0f};
};
