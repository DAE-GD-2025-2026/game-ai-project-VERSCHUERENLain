// Fill out your copyright notice in the Description page of Project Settings.

#include "Level_FSM.h"

#include "BehaviorTree/BlackboardComponent.h"
#include "DecisionMaking/GameAIController.h"
#include "DecisionMaking/FSM/FSMBlackboardKeys.h"
#include "DrawDebugHelpers.h"
#include "GameAIProg/Shared/GameAISpectator.h"
#include "UObject/ConstructorHelpers.h"

ALevel_FSM::ALevel_FSM()
{
	PrimaryActorTick.bCanEverTick = true;

	static ConstructorHelpers::FObjectFinder<UInputAction> TargetActionFinder(TEXT("/Game/Input/IA_Target.IA_Target"));
	if (TargetActionFinder.Succeeded())
	{
		SetTargetAction = TargetActionFinder.Object;
	}

	static ConstructorHelpers::FObjectFinder<UBlackboardData> BlackboardFinder(TEXT("/Game/DecisionMaking/BB_TEST.BB_TEST"));
	if (BlackboardFinder.Succeeded())
	{
		BlackboardAsset = BlackboardFinder.Object;
	}

	GuardPatrolPoints = {
		FVector{1700.0f, 1700.0f, 90.0f},
		FVector{2500.0f, 1700.0f, 90.0f},
		FVector{2500.0f, 2500.0f, 90.0f},
		FVector{1700.0f, 2500.0f, 90.0f},
	};
}

void ALevel_FSM::BeginPlay()
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
		ThiefAgent->SetMaxLinearSpeed(ThiefMaxLinearSpeed);
		ThiefAgent->SetSteeringBehavior(&ThiefMoveBehavior);
		ThiefMoveBehavior.SetTarget(FTargetData{FVector2D{ThiefSpawnLocation.X, ThiefSpawnLocation.Y}});
	}

	GuardAgent = GetWorld()->SpawnActor<ASteeringAgent>(GuardClass, GuardSpawnLocation, FRotator::ZeroRotator);
	if (GuardAgent)
	{
		GuardAgent->SetDebugRenderingEnabled(false);
		GuardAgent->SetMaxLinearSpeed(GuardMaxLinearSpeed);
	}

	SetupGuardController();
}

void ALevel_FSM::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	DrawDebugInfo();
	UpdateImGui();
}

void ALevel_FSM::BindLevelInputActions()
{
	Super::BindLevelInputActions();

	if (!PlayerEnhancedInputComponent || !SetTargetAction)
	{
		return;
	}

	PlayerEnhancedInputComponent->BindAction(SetTargetAction, ETriggerEvent::Triggered, this, &ALevel_FSM::SetThiefTarget);
}

void ALevel_FSM::UpdateImGui()
{
#pragma region UI
	{
		ImGui::SetNextWindowPos(WindowPos);
		ImGui::SetNextWindowSize(WindowSize);
		ImGui::Begin("Gameplay Programming", nullptr,
			ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse);

		ImGui::Text("CONTROLS");
		ImGui::Indent();
		ImGui::Text("LMB: move thief");
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

		ImGui::Text("fsm test");
		ImGui::Text("guard radius: %.0f", GuardDetectionRadius);
		ImGui::Text("search time: %.1f", GuardSearchDuration);
		ImGui::Text("guard speed: %.0f", GuardMaxLinearSpeed);
		ImGui::Text("thief speed: %.0f", ThiefMaxLinearSpeed);
		if (AGameAIController* const GuardController = GetGuardController(); GuardController)
		{
			ImGui::Text("guard state: %s", TCHAR_TO_ANSI(*GuardController->GetCurrentStateName()));
			ImGui::Text("target visible: %s", GuardController->IsTargetVisibleDebug() ? "yes" : "no");

			if (UBlackboardComponent* const BlackboardComponent = GuardController->GetBlackboardComponentMutable())
			{
				FVector const LastKnownLocation = BlackboardComponent->GetValueAsVector(GameAI::FSM::BlackboardKeys::LastKnownLocation);
				bool const bHasValidLastKnownLocation = LastKnownLocation.GetAbsMax() < 1000000.0;
				if (bHasValidLastKnownLocation)
				{
					ImGui::Text("last known: %.0f %.0f", LastKnownLocation.X, LastKnownLocation.Y);
				}
				else
				{
					ImGui::Text("last known: none");
				}
			}
		}

		ImGui::End();
	}
#pragma endregion UI
}

void ALevel_FSM::SetThiefTarget()
{
	if (!ThiefAgent)
	{
		return;
	}

	ThiefMoveBehavior.SetTarget(FTargetData{FVector2D{LatestMouseWorldPos.X, LatestMouseWorldPos.Y}});
}

void ALevel_FSM::SetupGuardController()
{
	if (!GuardAgent || !ThiefAgent)
	{
		return;
	}

	AController* CurrentController = GuardAgent->GetController();
	if (!Cast<AGameAIController>(CurrentController))
	{
		if (CurrentController)
		{
			GuardAgent->DetachFromControllerPendingDestroy();
		}

		GuardAgent->AIControllerClass = AGameAIController::StaticClass();
		GuardAgent->SpawnDefaultController();
	}

	AGameAIController* const AIController = Cast<AGameAIController>(GuardAgent->GetController());
	if (!AIController)
	{
		return;
	}

	if (BlackboardAsset)
	{
		AIController->FSMBlackboardAsset = BlackboardAsset;
		AIController->InitFiniteStateMachine();
	}

	AIController->ConfigureGuardFSM(ThiefAgent, GuardPatrolPoints, GuardDetectionRadius, GuardSearchDuration);
	AIController->RunFiniteStateMachine();
}

void ALevel_FSM::DrawDebugInfo() const
{
	if (GuardAgent)
	{
		DrawDebugCircle(GetWorld(), FVector{GuardAgent->GetPosition(), GuardAgent->GetActorLocation().Z},
			GuardDetectionRadius, 32, FColor::Yellow, false, -1.0f, 0, 2.0f, FVector::YAxisVector, FVector::XAxisVector);
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

	if (AGameAIController* const GuardController = GetGuardController(); GuardController)
	{
		if (UBlackboardComponent* const BlackboardComponent = GuardController->GetBlackboardComponentMutable())
		{
			FVector const LastKnownLocation = BlackboardComponent->GetValueAsVector(GameAI::FSM::BlackboardKeys::LastKnownLocation);
			bool const bHasValidLastKnownLocation = LastKnownLocation.GetAbsMax() < 1000000.0;
			if (bHasValidLastKnownLocation)
			{
				DrawDebugPoint(GetWorld(), LastKnownLocation, 28.0f, FColor::Orange, false, -1.0f, 0);
				DrawDebugCircle(GetWorld(), LastKnownLocation, 70.0f, 24, FColor::Orange, false, -1.0f, 0, 2.5f,
					FVector::YAxisVector, FVector::XAxisVector);
				DrawDebugString(GetWorld(), LastKnownLocation + FVector{0.0f, 0.0f, 40.0f},
					TEXT("last known"), nullptr, FColor::Orange, 0.0f, false);

				if (GuardAgent)
				{
					DrawDebugLine(GetWorld(), GuardAgent->GetActorLocation(), LastKnownLocation,
						FColor::Orange, false, -1.0f, 0, 1.5f);
				}
			}
		}
	}
}

AGameAIController* ALevel_FSM::GetGuardController() const
{
	return GuardAgent ? Cast<AGameAIController>(GuardAgent->GetController()) : nullptr;
}
