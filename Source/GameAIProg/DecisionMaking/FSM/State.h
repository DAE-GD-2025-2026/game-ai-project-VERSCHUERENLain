#pragma once

#include "CoreMinimal.h"

class AGameAIController;
class ASteeringAgent;
class UBlackboardComponent;
class UFSMComponent;

namespace GameAI::FSM
{
	class State
	{
	public:
		virtual ~State() = default;

		virtual void Enter() {}
		virtual void Exit() {}
		virtual void Update(float DeltaTime) = 0;
		virtual FString GetDebugName() const { return TEXT("state"); }

		void SetOwner(UFSMComponent* InOwner) { Owner = InOwner; }

	protected:
		UFSMComponent* GetOwnerComponent() const { return Owner; }
		AGameAIController* GetAIController() const;
		UBlackboardComponent* GetBlackboardComponent() const;
		ASteeringAgent* GetControlledAgent() const;
		float GetDefaultMaxLinearSpeed() const;

	private:
		UFSMComponent* Owner{};
	};
}
