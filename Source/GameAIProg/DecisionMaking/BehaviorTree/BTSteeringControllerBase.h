#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "BTSteeringControllerBase.generated.h"

class ASteeringAgent;
class UBehaviorTree;
class UBehaviorTreeComponent;
class UBlackboardData;
class UBlackboardComponent;

UCLASS(Abstract)
class GAMEAIPROG_API ABTSteeringControllerBase : public AAIController
{
	GENERATED_BODY()

public:
	ABTSteeringControllerBase();

	ASteeringAgent* GetControlledSteeringAgent() const;
	UBlackboardComponent* GetBlackboardComponentMutable() const;
	UBehaviorTreeComponent* GetBehaviorTreeComponent() const { return BehaviorTreeComponent; }
	FString GetDebugMode() const { return DebugMode.ToString(); }
	void SetDebugMode(FName const NewDebugMode) { DebugMode = NewDebugMode; }
	float GetConfiguredMaxLinearSpeed() const { return ConfiguredMaxLinearSpeed; }

protected:
	virtual void OnUnPossess() override;

	void SetConfiguredMaxLinearSpeed(float InConfiguredMaxLinearSpeed) { ConfiguredMaxLinearSpeed = InConfiguredMaxLinearSpeed; }
	bool StartRuntimeBehaviorTree();

	UPROPERTY(VisibleAnywhere, Category="AI|BT")
	TObjectPtr<UBehaviorTreeComponent> BehaviorTreeComponent{};

	UPROPERTY(Transient)
	TObjectPtr<UBehaviorTree> RuntimeBehaviorTree{};

	UPROPERTY(Transient)
	TObjectPtr<UBlackboardData> RuntimeBlackboard{};

	FName DebugMode{TEXT("none")};
	float ConfiguredMaxLinearSpeed{0.0f};
	bool bBehaviorTreeStarted{false};
};
