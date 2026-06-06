#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "GameAIProg/Movement/SteeringBehaviors/Steering/SteeringBehaviors.h"
#include "BTTask_GuardSearch.generated.h"

UCLASS()
class GAMEAIPROG_API UBTTask_GuardSearch : public UBTTaskNode
{
	GENERATED_BODY()

public:
	UBTTask_GuardSearch();

protected:
	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
	virtual void TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;

private:
	void PickNextSearchPoint();

	Arrive MoveToLastKnownLocationBehavior{};
	Arrive SearchPointBehavior{};
	bool bReachedLastKnownLocation{false};
	FVector2D SearchAnchor{};
	FVector2D CurrentSearchPoint{};
	float SearchPointTolerance{55.0f};
	float SearchRadius{220.0f};
};
