#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTDecorator.h"
#include "BTDecorator_SearchTimeRemaining.generated.h"

UCLASS()
class GAMEAIPROG_API UBTDecorator_SearchTimeRemaining : public UBTDecorator
{
	GENERATED_BODY()

public:
	UBTDecorator_SearchTimeRemaining();

protected:
	virtual bool CalculateRawConditionValue(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) const override;
};
