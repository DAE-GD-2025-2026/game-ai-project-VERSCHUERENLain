#pragma once

#include <vector>

#include "CoreMinimal.h"
#include "BTSteeringControllerBase.h"
#include "Perception/AIPerceptionTypes.h"
#include "GuardBTController.generated.h"

class UAIPerceptionComponent;
class UAISenseConfig_Sight;

UCLASS()
class GAMEAIPROG_API AGuardBTController : public ABTSteeringControllerBase
{
	GENERATED_BODY()

public:
	AGuardBTController();

	void ConfigureGuard(AActor* InTargetActor, TArray<FVector> const& InPatrolPoints,
		float InSightRadius, float InLoseSightRadius, float InSearchDuration, float InGuardSpeed);

	FString GetCurrentGuardMode() const { return GetDebugMode(); }
	bool IsTargetVisibleDebug() const;
	bool HasLastKnownLocation() const;
	FVector GetLastKnownLocation() const;
	float GetSearchDuration() const { return SearchDuration; }
	float GetSearchTimeRemaining() const;
	std::vector<FVector2D> const& GetPatrolPoints() const { return PatrolPoints; }
	AActor* GetTrackedTargetActor() const { return TargetActor.Get(); }

protected:
	virtual void OnPossess(APawn* InPawn) override;

private:
	UFUNCTION()
	void HandleTargetPerceptionUpdated(AActor* Actor, FAIStimulus Stimulus);

	void ApplySightConfig();
	void BuildRuntimeBlackboard();
	void BuildRuntimeTree();
	void TryStartBehaviorTree();

	UPROPERTY()
	TObjectPtr<UAISenseConfig_Sight> SightConfig{};

	TWeakObjectPtr<AActor> TargetActor{};
	std::vector<FVector2D> PatrolPoints{};
	float SightRadius{800.0f};
	float LoseSightRadius{900.0f};
	float SearchDuration{5.0f};
	bool bGuardConfigured{false};
};
