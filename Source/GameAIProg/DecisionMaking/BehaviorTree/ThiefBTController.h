#pragma once

#include "CoreMinimal.h"
#include "BTSteeringControllerBase.h"
#include "ThiefBTController.generated.h"

UCLASS()
class GAMEAIPROG_API AThiefBTController : public ABTSteeringControllerBase
{
	GENERATED_BODY()

public:
	AThiefBTController();

	void ConfigureThief(FVector const& InRoamCenter, float InRoamRadius, float InThiefSpeed);
	FVector GetRoamCenter() const { return RoamCenter; }
	float GetRoamRadius() const { return RoamRadius; }
	FVector GetCurrentRoamTarget() const;

protected:
	virtual void OnPossess(APawn* InPawn) override;

private:
	void BuildRuntimeBlackboard();
	void BuildRuntimeTree();
	void TryStartBehaviorTree();

	FVector RoamCenter{2100.0f, 2100.0f, 90.0f};
	float RoamRadius{700.0f};
	bool bThiefConfigured{false};
};
