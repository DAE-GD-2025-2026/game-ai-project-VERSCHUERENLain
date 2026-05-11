#include "DecisionMaking/BehaviorTree/Decorators/BTDecorator_BlackboardBoolMatch.h"

#include "BehaviorTree/BlackboardComponent.h"

UBTDecorator_BlackboardBoolMatch::UBTDecorator_BlackboardBoolMatch()
{
	NodeName = TEXT("bb bool match");
	FlowAbortMode = EBTFlowAbortMode::None;
}

bool UBTDecorator_BlackboardBoolMatch::CalculateRawConditionValue(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) const
{
	UBlackboardComponent* BlackboardComponent = OwnerComp.GetBlackboardComponent();
	return BlackboardComponent && BlackboardComponent->GetValueAsBool(KeyName) == bExpectedValue;
}
