#include "GuardBTController.h"

#include "BehaviorTree/BehaviorTree.h"
#include "BehaviorTree/Blackboard/BlackboardKeyType_Bool.h"
#include "BehaviorTree/Blackboard/BlackboardKeyType_Float.h"
#include "BehaviorTree/Blackboard/BlackboardKeyType_Int.h"
#include "BehaviorTree/Blackboard/BlackboardKeyType_Object.h"
#include "BehaviorTree/Blackboard/BlackboardKeyType_Vector.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "BehaviorTree/Composites/BTComposite_Selector.h"
#include "DecisionMaking/BehaviorTree/BehaviorTreeBlackboardKeys.h"
#include "DecisionMaking/BehaviorTree/BehaviorTreeBuilder.h"
#include "DecisionMaking/BehaviorTree/Decorators/BTDecorator_BlackboardBoolMatch.h"
#include "DecisionMaking/BehaviorTree/Decorators/BTDecorator_SearchTimeRemaining.h"
#include "DecisionMaking/BehaviorTree/Tasks/BTTask_GuardChase.h"
#include "DecisionMaking/BehaviorTree/Tasks/BTTask_GuardPatrol.h"
#include "DecisionMaking/BehaviorTree/Tasks/BTTask_GuardSearch.h"
#include "Perception/AIPerceptionComponent.h"
#include "Perception/AISenseConfig_Sight.h"

AGuardBTController::AGuardBTController()
{
	PerceptionComponent = CreateDefaultSubobject<UAIPerceptionComponent>(TEXT("PerceptionComponent"));
	SightConfig = CreateDefaultSubobject<UAISenseConfig_Sight>(TEXT("SightConfig"));

	if (PerceptionComponent && SightConfig)
	{
		PerceptionComponent->ConfigureSense(*SightConfig);
		PerceptionComponent->SetDominantSense(UAISense_Sight::StaticClass());
		PerceptionComponent->OnTargetPerceptionUpdated.AddDynamic(this, &AGuardBTController::HandleTargetPerceptionUpdated);
	}

	ApplySightConfig();
}

void AGuardBTController::ConfigureGuard(AActor* InTargetActor, TArray<FVector> const& InPatrolPoints,
	float InSightRadius, float InLoseSightRadius, float InSearchDuration, float InGuardSpeed)
{
	TargetActor = InTargetActor;
	SightRadius = InSightRadius;
	LoseSightRadius = InLoseSightRadius;
	SearchDuration = InSearchDuration;
	SetConfiguredMaxLinearSpeed(InGuardSpeed);

	PatrolPoints.clear();
	PatrolPoints.reserve(InPatrolPoints.Num());
	for (FVector const& PatrolPoint : InPatrolPoints)
	{
		PatrolPoints.emplace_back(PatrolPoint.X, PatrolPoint.Y);
	}

	ApplySightConfig();
	BuildRuntimeBlackboard();
	BuildRuntimeTree();
	bGuardConfigured = !PatrolPoints.empty() && TargetActor.IsValid();
	TryStartBehaviorTree();
}

bool AGuardBTController::IsTargetVisibleDebug() const
{
	if (UBlackboardComponent* BlackboardComponent = GetBlackboardComponentMutable())
	{
		return BlackboardComponent->GetValueAsBool(GameAI::BT::BlackboardKeys::TargetVisible);
	}

	return false;
}

bool AGuardBTController::HasLastKnownLocation() const
{
	if (UBlackboardComponent* BlackboardComponent = GetBlackboardComponentMutable())
	{
		return BlackboardComponent->GetValueAsBool(GameAI::BT::BlackboardKeys::HasLastKnownLocation);
	}

	return false;
}

FVector AGuardBTController::GetLastKnownLocation() const
{
	if (UBlackboardComponent* BlackboardComponent = GetBlackboardComponentMutable())
	{
		return BlackboardComponent->GetValueAsVector(GameAI::BT::BlackboardKeys::LastKnownLocation);
	}

	return FVector::ZeroVector;
}

float AGuardBTController::GetSearchTimeRemaining() const
{
	UBlackboardComponent* BlackboardComponent = GetBlackboardComponentMutable();
	UWorld* World = GetWorld();
	if (!BlackboardComponent || !World || !HasLastKnownLocation())
	{
		return 0.0f;
	}

	float const SearchStartedAt = BlackboardComponent->GetValueAsFloat(GameAI::BT::BlackboardKeys::SearchStartedAt);
	return FMath::Max(0.0f, SearchDuration - (World->GetTimeSeconds() - SearchStartedAt));
}

void AGuardBTController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);
	TryStartBehaviorTree();
}

void AGuardBTController::HandleTargetPerceptionUpdated(AActor* Actor, FAIStimulus Stimulus)
{
	if (Actor != TargetActor.Get())
	{
		return;
	}

	UBlackboardComponent* BlackboardComponent = GetBlackboardComponentMutable();
	UWorld* World = GetWorld();
	if (!BlackboardComponent || !World)
	{
		return;
	}

	if (Stimulus.WasSuccessfullySensed())
	{
		BlackboardComponent->SetValueAsObject(GameAI::BT::BlackboardKeys::TargetActor, Actor);
		BlackboardComponent->SetValueAsBool(GameAI::BT::BlackboardKeys::TargetVisible, true);
		BlackboardComponent->SetValueAsBool(GameAI::BT::BlackboardKeys::HasLastKnownLocation, true);
		BlackboardComponent->SetValueAsVector(GameAI::BT::BlackboardKeys::LastKnownLocation, Actor->GetActorLocation());
		if (BehaviorTreeComponent && BehaviorTreeComponent->TreeHasBeenStarted())
		{
			BehaviorTreeComponent->RestartTree();
		}
		return;
	}

	FVector const LastSeenLocation = Stimulus.StimulusLocation.IsNearlyZero() ? Actor->GetActorLocation() : Stimulus.StimulusLocation;
	BlackboardComponent->ClearValue(GameAI::BT::BlackboardKeys::TargetActor);
	BlackboardComponent->SetValueAsBool(GameAI::BT::BlackboardKeys::TargetVisible, false);
	BlackboardComponent->SetValueAsBool(GameAI::BT::BlackboardKeys::HasLastKnownLocation, true);
	BlackboardComponent->SetValueAsVector(GameAI::BT::BlackboardKeys::LastKnownLocation, LastSeenLocation);
	BlackboardComponent->SetValueAsFloat(GameAI::BT::BlackboardKeys::SearchStartedAt, World->GetTimeSeconds());
	if (BehaviorTreeComponent && BehaviorTreeComponent->TreeHasBeenStarted())
	{
		BehaviorTreeComponent->RestartTree();
	}
}

void AGuardBTController::ApplySightConfig()
{
	if (!SightConfig || !PerceptionComponent)
	{
		return;
	}

	SightConfig->SightRadius = SightRadius;
	SightConfig->LoseSightRadius = LoseSightRadius;
	SightConfig->PeripheralVisionAngleDegrees = 90.0f;
	SightConfig->AutoSuccessRangeFromLastSeenLocation = FAISystem::InvalidRange;
	SightConfig->DetectionByAffiliation.bDetectEnemies = true;
	SightConfig->DetectionByAffiliation.bDetectFriendlies = true;
	SightConfig->DetectionByAffiliation.bDetectNeutrals = true;

	PerceptionComponent->ConfigureSense(*SightConfig);
	PerceptionComponent->SetDominantSense(UAISense_Sight::StaticClass());
}

void AGuardBTController::BuildRuntimeBlackboard()
{
	RuntimeBlackboard = NewObject<UBlackboardData>(this);
	GameAI::BT::Builder::AddBlackboardKey<UBlackboardKeyType_Object>(RuntimeBlackboard, GameAI::BT::BlackboardKeys::TargetActor);
	GameAI::BT::Builder::AddBlackboardKey<UBlackboardKeyType_Bool>(RuntimeBlackboard, GameAI::BT::BlackboardKeys::TargetVisible);
	GameAI::BT::Builder::AddBlackboardKey<UBlackboardKeyType_Vector>(RuntimeBlackboard, GameAI::BT::BlackboardKeys::LastKnownLocation);
	GameAI::BT::Builder::AddBlackboardKey<UBlackboardKeyType_Bool>(RuntimeBlackboard, GameAI::BT::BlackboardKeys::HasLastKnownLocation);
	GameAI::BT::Builder::AddBlackboardKey<UBlackboardKeyType_Float>(RuntimeBlackboard, GameAI::BT::BlackboardKeys::SearchStartedAt);
	GameAI::BT::Builder::AddBlackboardKey<UBlackboardKeyType_Int>(RuntimeBlackboard, GameAI::BT::BlackboardKeys::PatrolIndex);
	GameAI::BT::Builder::FinalizeBlackboard(RuntimeBlackboard);
}

void AGuardBTController::BuildRuntimeTree()
{
	RuntimeBehaviorTree = GameAI::BT::Builder::CreateTree(this, RuntimeBlackboard);
	UBTComposite_Selector* Root = GameAI::BT::Builder::SetSelectorRoot(RuntimeBehaviorTree);
	if (!Root)
	{
		return;
	}

	GameAI::BT::Builder::AddTask<UBTTask_GuardChase>(*Root);
	if (UBTDecorator_BlackboardBoolMatch* ChaseVisibleDecorator =
		GameAI::BT::Builder::AddDecoratorToLastChild<UBTDecorator_BlackboardBoolMatch>(*Root))
	{
		ChaseVisibleDecorator->SetKeyName(GameAI::BT::BlackboardKeys::TargetVisible);
		ChaseVisibleDecorator->SetExpectedValue(true);
		ChaseVisibleDecorator->SetAbortMode(EBTFlowAbortMode::LowerPriority);
	}

	GameAI::BT::Builder::AddTask<UBTTask_GuardSearch>(*Root);
	if (UBTDecorator_BlackboardBoolMatch* SearchHasLocationDecorator =
		GameAI::BT::Builder::AddDecoratorToLastChild<UBTDecorator_BlackboardBoolMatch>(*Root))
	{
		SearchHasLocationDecorator->SetKeyName(GameAI::BT::BlackboardKeys::HasLastKnownLocation);
		SearchHasLocationDecorator->SetExpectedValue(true);
		SearchHasLocationDecorator->SetAbortMode(EBTFlowAbortMode::Self);
	}
	GameAI::BT::Builder::AddDecoratorToLastChild<UBTDecorator_SearchTimeRemaining>(*Root);

	GameAI::BT::Builder::AddTask<UBTTask_GuardPatrol>(*Root);
}

void AGuardBTController::TryStartBehaviorTree()
{
	if (!GetPawn() || !bGuardConfigured || !RuntimeBehaviorTree || !RuntimeBlackboard)
	{
		return;
	}

	if (!StartRuntimeBehaviorTree())
	{
		return;
	}

	if (UBlackboardComponent* BlackboardComponent = GetBlackboardComponentMutable())
	{
		BlackboardComponent->ClearValue(GameAI::BT::BlackboardKeys::TargetActor);
		BlackboardComponent->SetValueAsBool(GameAI::BT::BlackboardKeys::TargetVisible, false);
		BlackboardComponent->SetValueAsBool(GameAI::BT::BlackboardKeys::HasLastKnownLocation, false);
		BlackboardComponent->SetValueAsFloat(GameAI::BT::BlackboardKeys::SearchStartedAt, 0.0f);
		BlackboardComponent->SetValueAsInt(GameAI::BT::BlackboardKeys::PatrolIndex, 0);
	}
}
