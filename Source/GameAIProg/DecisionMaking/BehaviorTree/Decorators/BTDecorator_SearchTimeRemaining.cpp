#include "DecisionMaking/BehaviorTree/Decorators/BTDecorator_SearchTimeRemaining.h"

#include "BehaviorTree/BlackboardComponent.h"
#include "DecisionMaking/BehaviorTree/BehaviorTreeBlackboardKeys.h"
#include "DecisionMaking/BehaviorTree/GuardBTController.h"

UBTDecorator_SearchTimeRemaining::UBTDecorator_SearchTimeRemaining()
{
	NodeName = TEXT("search not too long");
	FlowAbortMode = EBTFlowAbortMode::Self;
}

bool UBTDecorator_SearchTimeRemaining::CalculateRawConditionValue(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) const
{
	AGuardBTController* GuardController = Cast<AGuardBTController>(OwnerComp.GetAIOwner());
	UBlackboardComponent* BlackboardComponent = OwnerComp.GetBlackboardComponent();
	UWorld* World = OwnerComp.GetWorld();
	if (!GuardController || !BlackboardComponent || !World)
	{
		return false;
	}

	float const SearchStartedAt = BlackboardComponent->GetValueAsFloat(GameAI::BT::BlackboardKeys::SearchStartedAt);
	return World->GetTimeSeconds() - SearchStartedAt < GuardController->GetSearchDuration();
}
