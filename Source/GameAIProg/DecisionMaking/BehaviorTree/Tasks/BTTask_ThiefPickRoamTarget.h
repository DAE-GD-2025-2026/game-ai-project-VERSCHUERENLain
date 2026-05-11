#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "BTTask_ThiefPickRoamTarget.generated.h"

UCLASS()
class GAMEAIPROG_API UBTTask_ThiefPickRoamTarget : public UBTTaskNode
{
	GENERATED_BODY()

public:
	UBTTask_ThiefPickRoamTarget();

protected:
	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
};
