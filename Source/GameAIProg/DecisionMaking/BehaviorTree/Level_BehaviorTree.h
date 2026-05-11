#pragma once

#include "CoreMinimal.h"
#include "Shared/Level_Base.h"
#include "Level_BehaviorTree.generated.h"

class AGuardBTController;
class AThiefBTController;
class ASteeringAgent;
class UAIPerceptionStimuliSourceComponent;

UCLASS()
class GAMEAIPROG_API ALevel_BehaviorTree : public ALevel_Base
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="BTLevel|Agents")
	TSubclassOf<ASteeringAgent> GuardAgentClass{};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="BTLevel|Agents")
	TSubclassOf<ASteeringAgent> ThiefAgentClass{};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="BTLevel|AI")
	TArray<FVector> GuardPatrolPoints{};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="BTLevel|AI")
	FVector GuardSpawnLocation{2500.0f, 2100.0f, 90.0f};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="BTLevel|AI")
	FVector ThiefSpawnLocation{1700.0f, 2100.0f, 90.0f};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="BTLevel|AI")
	float GuardSightRadius{800.0f};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="BTLevel|AI")
	float GuardLoseSightRadius{900.0f};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="BTLevel|AI")
	float GuardSearchDuration{5.0f};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="BTLevel|AI")
	float GuardSpeed{500.0f};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="BTLevel|AI")
	float ThiefSpeed{700.0f};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="BTLevel|AI")
	FVector ThiefRoamCenter{2100.0f, 2100.0f, 90.0f};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="BTLevel|AI")
	float ThiefRoamRadius{700.0f};

	ALevel_BehaviorTree();

	virtual void Tick(float DeltaTime) override;

protected:
	virtual void BeginPlay() override;

private:
	UPROPERTY()
	ASteeringAgent* GuardAgent{nullptr};

	UPROPERTY()
	ASteeringAgent* ThiefAgent{nullptr};

	UPROPERTY()
	UAIPerceptionStimuliSourceComponent* ThiefStimuliSource{nullptr};

	void SetupControllers();
	void RegisterThiefStimuliSource();
	void UpdateImGui();
	void DrawDebugInfo() const;
	AGuardBTController* GetGuardController() const;
	AThiefBTController* GetThiefController() const;
};
