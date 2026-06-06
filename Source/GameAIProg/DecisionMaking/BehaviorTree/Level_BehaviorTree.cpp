#include "Level_BehaviorTree.h"

#include "DecisionMaking/BehaviorTree/BehaviorTreeBlackboardKeys.h"
#include "DecisionMaking/BehaviorTree/GuardBTController.h"
#include "DecisionMaking/BehaviorTree/ThiefBTController.h"
#include "DrawDebugHelpers.h"
#include "GameAIProg/Shared/GameAISpectator.h"
#include "Perception/AIPerceptionStimuliSourceComponent.h"
#include "Perception/AISense_Sight.h"

ALevel_BehaviorTree::ALevel_BehaviorTree()
{
	PrimaryActorTick.bCanEverTick = true;

	GuardPatrolPoints = {
		FVector{1700.0f, 1700.0f, 90.0f},
		FVector{2500.0f, 1700.0f, 90.0f},
		FVector{2500.0f, 2500.0f, 90.0f},
		FVector{1700.0f, 2500.0f, 90.0f},
	};
}

void ALevel_BehaviorTree::BeginPlay()
{
	Super::BeginPlay();

	if (TrimWorld)
	{
		TrimWorld->bShouldTrimWorld = false;
	}

	if (PlayerController)
	{
		if (AGameAISpectator* Player = Cast<AGameAISpectator>(PlayerController->GetPawnOrSpectator()); Player)
		{
			Player->SetCameraProjection(ECameraProjectionMode::Orthographic);
		}
	}

	TSubclassOf<ASteeringAgent> GuardClass = GuardAgentClass;
	if (!GuardClass)
	{
		GuardClass = SteeringAgentClass ? SteeringAgentClass : TSubclassOf<ASteeringAgent>{ASteeringAgent::StaticClass()};
	}

	TSubclassOf<ASteeringAgent> ThiefClass = ThiefAgentClass;
	if (!ThiefClass)
	{
		ThiefClass = SteeringAgentClass ? SteeringAgentClass : TSubclassOf<ASteeringAgent>{ASteeringAgent::StaticClass()};
	}

	ThiefAgent = GetWorld()->SpawnActor<ASteeringAgent>(ThiefClass, ThiefSpawnLocation, FRotator::ZeroRotator);
	if (ThiefAgent)
	{
		ThiefAgent->SetDebugRenderingEnabled(false);
		ThiefAgent->SetMaxLinearSpeed(ThiefSpeed);
	}

	GuardAgent = GetWorld()->SpawnActor<ASteeringAgent>(GuardClass, GuardSpawnLocation, FRotator::ZeroRotator);
	if (GuardAgent)
	{
		GuardAgent->SetDebugRenderingEnabled(false);
		GuardAgent->SetMaxLinearSpeed(GuardSpeed);
	}

	RegisterThiefStimuliSource();
	SetupControllers();
}

void ALevel_BehaviorTree::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	DrawDebugInfo();
	UpdateImGui();
}

void ALevel_BehaviorTree::SetupControllers()
{
	if (!GuardAgent || !ThiefAgent)
	{
		return;
	}

	AController* GuardController = GuardAgent->GetController();
	if (!Cast<AGuardBTController>(GuardController))
	{
		if (GuardController)
		{
			GuardAgent->DetachFromControllerPendingDestroy();
		}

		GuardAgent->AIControllerClass = AGuardBTController::StaticClass();
		GuardAgent->SpawnDefaultController();
	}

	if (AGuardBTController* GuardBTController = GetGuardController())
	{
		GuardBTController->ConfigureGuard(ThiefAgent, GuardPatrolPoints, GuardSightRadius, GuardLoseSightRadius, GuardSearchDuration, GuardSpeed);
	}

	AController* ThiefController = ThiefAgent->GetController();
	if (!Cast<AThiefBTController>(ThiefController))
	{
		if (ThiefController)
		{
			ThiefAgent->DetachFromControllerPendingDestroy();
		}

		ThiefAgent->AIControllerClass = AThiefBTController::StaticClass();
		ThiefAgent->SpawnDefaultController();
	}

	if (AThiefBTController* ThiefBTController = GetThiefController())
	{
		ThiefBTController->ConfigureThief(ThiefRoamCenter, ThiefRoamRadius, ThiefSpeed);
	}
}

void ALevel_BehaviorTree::RegisterThiefStimuliSource()
{
	if (!ThiefAgent || ThiefStimuliSource)
	{
		return;
	}

	ThiefStimuliSource = NewObject<UAIPerceptionStimuliSourceComponent>(ThiefAgent, TEXT("ThiefStimuliSource"));
	if (!ThiefStimuliSource)
	{
		return;
	}

	ThiefStimuliSource->RegisterForSense(UAISense_Sight::StaticClass());
	ThiefStimuliSource->RegisterComponent();
	ThiefStimuliSource->RegisterWithPerceptionSystem();
}

void ALevel_BehaviorTree::UpdateImGui()
{
#pragma region UI
	{
		ImGui::SetNextWindowPos(WindowPos);
		ImGui::SetNextWindowSize(WindowSize);
		ImGui::Begin("Gameplay Programming", nullptr,
			ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse);

		ImGui::Text("CONTROLS");
		ImGui::Indent();
		ImGui::Text("none this time");
		ImGui::Unindent();

		ImGui::Spacing();
		ImGui::Separator();
		ImGui::Spacing();
		ImGui::Spacing();

		ImGui::Text("STATS");
		ImGui::Indent();
		ImGui::Text("%.3f ms/frame", 1000.0f / ImGui::GetIO().Framerate);
		ImGui::Text("%.1f FPS", ImGui::GetIO().Framerate);
		ImGui::Unindent();

		ImGui::Spacing();
		ImGui::Separator();
		ImGui::Spacing();
		ImGui::Spacing();

		ImGui::Text("behavior tree");
		ImGui::Text("guard sight: %.0f", GuardSightRadius);
		ImGui::Text("guard lose sight: %.0f", GuardLoseSightRadius);
		ImGui::Text("search time: %.1f", GuardSearchDuration);
		ImGui::Text("guard speed: %.0f", GuardSpeed);
		ImGui::Text("thief speed: %.0f", ThiefSpeed);

		if (AGuardBTController* GuardController = GetGuardController())
		{
			ImGui::Text("guard mode: %s", TCHAR_TO_ANSI(*GuardController->GetCurrentGuardMode()));
			ImGui::Text("target visible: %s", GuardController->IsTargetVisibleDebug() ? "yes" : "no");

			if (GuardController->HasLastKnownLocation())
			{
				FVector const LastKnownLocation = GuardController->GetLastKnownLocation();
				ImGui::Text("last known: %.0f %.0f", LastKnownLocation.X, LastKnownLocation.Y);
			}
			else
			{
				ImGui::Text("last known: none");
			}

			ImGui::Text("search left: %.2f", GuardController->GetSearchTimeRemaining());
		}

		if (AThiefBTController* ThiefController = GetThiefController())
		{
			FVector const RoamTarget = ThiefController->GetCurrentRoamTarget();
			ImGui::Text("thief roam: %.0f %.0f", RoamTarget.X, RoamTarget.Y);
		}

		ImGui::End();
	}
#pragma endregion UI
}

void ALevel_BehaviorTree::DrawDebugInfo() const
{
	if (GuardAgent)
	{
		DrawDebugCircle(GetWorld(), FVector{GuardAgent->GetPosition(), GuardAgent->GetActorLocation().Z},
			GuardSightRadius, 32, FColor::Yellow, false, -1.0f, 0, 2.0f, FVector::YAxisVector, FVector::XAxisVector);
		DrawDebugCircle(GetWorld(), FVector{GuardAgent->GetPosition(), GuardAgent->GetActorLocation().Z},
			GuardLoseSightRadius, 32, FColor{180, 180, 0}, false, -1.0f, 0, 1.25f, FVector::YAxisVector, FVector::XAxisVector);
	}

	if (GuardPatrolPoints.Num() > 0)
	{
		for (int PatrolPointIdx = 0; PatrolPointIdx < GuardPatrolPoints.Num(); ++PatrolPointIdx)
		{
			FVector const Start = GuardPatrolPoints[PatrolPointIdx];
			FVector const End = GuardPatrolPoints[(PatrolPointIdx + 1) % GuardPatrolPoints.Num()];
			DrawDebugPoint(GetWorld(), Start, 14.0f, FColor::Green, false, -1.0f, 0);
			DrawDebugLine(GetWorld(), Start, End, FColor::Green, false, -1.0f, 0, 2.0f);
		}
	}

	if (AGuardBTController* GuardController = GetGuardController())
	{
		if (GuardController->HasLastKnownLocation())
		{
			FVector const LastKnownLocation = GuardController->GetLastKnownLocation();
			DrawDebugPoint(GetWorld(), LastKnownLocation, 28.0f, FColor::Orange, false, -1.0f, 0);
			DrawDebugCircle(GetWorld(), LastKnownLocation, 70.0f, 24, FColor::Orange, false, -1.0f, 0, 2.5f,
				FVector::YAxisVector, FVector::XAxisVector);
			DrawDebugString(GetWorld(), LastKnownLocation + FVector{0.0f, 0.0f, 40.0f},
				TEXT("last known"), nullptr, FColor::Orange, 0.0f, false);

			if (GuardAgent)
			{
				DrawDebugLine(GetWorld(), GuardAgent->GetActorLocation(), LastKnownLocation, FColor::Orange, false, -1.0f, 0, 1.5f);
			}
		}
	}

	if (AThiefBTController* ThiefController = GetThiefController())
	{
		FVector const RoamTarget = ThiefController->GetCurrentRoamTarget();
		DrawDebugPoint(GetWorld(), RoamTarget, 18.0f, FColor::Cyan, false, -1.0f, 0);
		DrawDebugCircle(GetWorld(), FVector{ThiefRoamCenter.X, ThiefRoamCenter.Y, ThiefRoamCenter.Z},
			ThiefRoamRadius, 32, FColor::Cyan, false, -1.0f, 0, 1.25f, FVector::YAxisVector, FVector::XAxisVector);

		if (ThiefAgent)
		{
			DrawDebugLine(GetWorld(), ThiefAgent->GetActorLocation(), RoamTarget, FColor::Cyan, false, -1.0f, 0, 1.0f);
		}
	}
}

AGuardBTController* ALevel_BehaviorTree::GetGuardController() const
{
	return GuardAgent ? Cast<AGuardBTController>(GuardAgent->GetController()) : nullptr;
}

AThiefBTController* ALevel_BehaviorTree::GetThiefController() const
{
	return ThiefAgent ? Cast<AThiefBTController>(ThiefAgent->GetController()) : nullptr;
}
