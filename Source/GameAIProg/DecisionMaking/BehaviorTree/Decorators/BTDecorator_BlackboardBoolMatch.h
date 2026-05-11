#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTDecorator.h"
#include "BTDecorator_BlackboardBoolMatch.generated.h"

UCLASS()
class GAMEAIPROG_API UBTDecorator_BlackboardBoolMatch : public UBTDecorator
{
	GENERATED_BODY()

public:
	UBTDecorator_BlackboardBoolMatch();

	void SetKeyName(FName const InKeyName) { KeyName = InKeyName; }
	void SetExpectedValue(bool const bInExpectedValue) { bExpectedValue = bInExpectedValue; }
	void SetAbortMode(EBTFlowAbortMode::Type const InAbortMode) { FlowAbortMode = InAbortMode; }

protected:
	virtual bool CalculateRawConditionValue(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) const override;

	UPROPERTY()
	FName KeyName{};

	UPROPERTY()
	bool bExpectedValue{false};
};
