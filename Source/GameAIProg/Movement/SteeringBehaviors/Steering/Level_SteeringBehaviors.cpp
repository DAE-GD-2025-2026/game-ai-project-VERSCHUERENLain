// Fill out your copyright notice in the Description page of Project Settings.

#include "Level_SteeringBehaviors.h"

#include <format>
#include <string>
#include "imgui.h"
#include "DrawDebugHelpers.h"


// Sets default values
ALevel_SteeringBehaviors::ALevel_SteeringBehaviors()
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
}

// Called when the game starts or when spawned
void ALevel_SteeringBehaviors::BeginPlay()
{
	Super::BeginPlay();

	AddAgent(BehaviorTypes::Seek);
	SteeringAgents[0].Agent->SetDebugRenderingEnabled(true);
}

void ALevel_SteeringBehaviors::BeginDestroy()
{
	Super::BeginDestroy();
}

// Called every frame
void ALevel_SteeringBehaviors::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

#pragma region UI
	ImGui::SetNextWindowPos(WindowPos);
	ImGui::SetNextWindowSize(WindowSize);
	ImGui::Begin("Game AI", nullptr, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse);

	//Elements
	ImGui::Text("CONTROLS");
	ImGui::Indent();
	ImGui::Text("LMB: place target");
	ImGui::Text("WASD: move cam");
	ImGui::Text("Scrollwheel: zoom cam");
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
	
	ImGui::Text("Steering Behaviors");
	ImGui::Spacing();
	ImGui::Spacing();
	
	ImGui::Checkbox("Trim World", &TrimWorld->bShouldTrimWorld);
	if (TrimWorld->bShouldTrimWorld)
	{
		ImGuiHelpers::ImGuiSliderFloatWithSetter("Trim Size",
			TrimWorld->GetTrimWorldSize(), 1000.f, 3000.f,
			[this](float InVal) { TrimWorld->SetTrimWorldSize(InVal); });
	}
	ImGui::Spacing();

#pragma region PerAgentUI
	if (ImGui::Button("Add Agent"))
		AddAgent(BehaviorTypes::Seek);
	ImGui::Separator();

	for (int i{0}; i < SteeringAgents.size(); ++i)
	{
		ImGui::PushID(i);
		ImGui_Agent& a = SteeringAgents[i];
		
		std::string agentHeader{std::format("Agent {}:", i)};
		if (ImGui::CollapsingHeader(agentHeader.c_str()))
		{
			ImGui::Indent();
			//Actor Props
			if (ImGui::CollapsingHeader("Properties"))
			{
				float v = a.Agent->GetMaxLinearSpeed();
				if (ImGui::SliderFloat("Lin", &v, 0.f, 600.f, "%.2f"))
					a.Agent->SetMaxLinearSpeed(v);

				v = a.Agent->GetMaxAngularSpeed();
				if (ImGui::SliderFloat("Ang", &v, 0.f, 360.f, "%.2f"))
					a.Agent->SetMaxAngularSpeed(v);

				v = a.Agent->GetMass();
				if (ImGui::SliderFloat("Mass ", &v, 0.f, 100.f, "%.2f"))
					a.Agent->SetMass(v);
			}
			
			bool bBehaviourModified = false;

			ImGui::Spacing();
			ImGui::PushID(i + 50);
			ImGui::Text(" Behavior: ");
			ImGui::SameLine();
			ImGui::PushItemWidth(100);

			// Add the names of your steering behaviors
			if (ImGui::Combo("", &a.SelectedBehavior, "Seek\0Wander\0Flee\0Arrive\0Evade\0Pursuit\0Face", 5))
			{
				bBehaviourModified = true;
			}
			ImGui::PopItemWidth();
			ImGui::PopID();

			
			ImGui::Spacing();
			ImGui::PushID(i + 100);
			ImGui::Text(" Target: ");
			ImGui::SameLine();
			ImGui::PushItemWidth(100);
			
			int selectedTargetOffset = a.SelectedTarget + 1;
			std::string const Label{""};
			std::string Targets{};
			for (auto const & Target : TargetLabels)
			{
				Targets += Target;
				Targets += '\0';
			}
			if (ImGui::Combo(Label.c_str(), &selectedTargetOffset, Targets.c_str()))
			{
				a.SelectedTarget = selectedTargetOffset - 1;
				bBehaviourModified = true;
			}
			
			ImGui::PopItemWidth();
			ImGui::PopID();
			ImGui::Spacing();
			ImGui::Spacing();
			
			
			if (bBehaviourModified)
				SetAgentBehavior(a);

			if (ImGui::Button("x"))
			{
				AgentIndexToRemove = i;
			}

			ImGui::SameLine(0, 20);

			bool isChecked = a.Agent->GetDebugRenderingEnabled();
			if (ImGui::Checkbox("Debug Rendering", &isChecked))
			{
				a.Agent->SetDebugRenderingEnabled(isChecked);
			}

			ImGui::Unindent();
		}
#pragma endregion 
		
		ImGui::PopID();
	}

	if (AgentIndexToRemove >= 0)
	{
		RemoveAgent(AgentIndexToRemove);
		AgentIndexToRemove = -1;
	}
	
	ImGui::End();
#pragma endregion

	for (ImGui_Agent& a : SteeringAgents)
	{
		if (a.Agent)
		{
			UpdateTarget(a);

			if (!a.Agent->GetDebugRenderingEnabled())
				continue;

			FVector AgentPos = a.Agent->GetActorLocation();
			FVector2D TargetPos2D = a.Behavior->GetTarget().Position;
			FVector TargetPos{TargetPos2D.X, TargetPos2D.Y, AgentPos.Z};

			if (a.SelectedBehavior == static_cast<int>(BehaviorTypes::Seek))
			{
				// seek: line from agent to target + dot at target
				DrawDebugLine(GetWorld(), AgentPos, TargetPos, FColor::Cyan, false, -1.f, 0, 1.f);
				DrawDebugPoint(GetWorld(), TargetPos, 10.f, FColor::Cyan, false, -1.f);
			}
			else if (a.SelectedBehavior == static_cast<int>(BehaviorTypes::Wander))
			{
				// wander: circle + dot at wander target + line from agent to target
				Wander* wander = a.Behavior->As<Wander>();
				float Yaw = FMath::DegreesToRadians(a.Agent->GetActorRotation().Yaw);
				FVector Forward{FMath::Cos(Yaw), FMath::Sin(Yaw), 0.f};
				FVector CircleCenter = AgentPos + Forward * wander->GetOffsetDistance();
				DrawDebugCircle(GetWorld(), CircleCenter, wander->GetRadius(), 32,
					FColor::Green, false, -1.f, 0, 1.f, FVector::YAxisVector, FVector::XAxisVector);
				// wander target = where the behavior is actually seeking
				FVector2D WanderTarget2D = a.Behavior->GetTarget().Position;
				FVector WanderTarget{WanderTarget2D.X, WanderTarget2D.Y, AgentPos.Z};
				DrawDebugPoint(GetWorld(), WanderTarget, 10.f, FColor::Green, false, -1.f);
				DrawDebugLine(GetWorld(), AgentPos, WanderTarget, FColor::Green, false, -1.f, 0, 1.f);
			}
			else if (a.SelectedBehavior == static_cast<int>(BehaviorTypes::Arrive))
			{
				// arrive: line + dot + slow radius (yellow) + target radius (green)
				DrawDebugLine(GetWorld(), AgentPos, TargetPos, FColor::Cyan, false, -1.f, 0, 1.f);
				DrawDebugPoint(GetWorld(), TargetPos, 10.f, FColor::Cyan, false, -1.f);
				Arrive* arrive = a.Behavior->As<Arrive>();
				DrawDebugCircle(GetWorld(), TargetPos, arrive->GetSlowRadius(), 32,
					FColor::Yellow, false, -1.f, 0, 1.f, FVector::YAxisVector, FVector::XAxisVector);
				DrawDebugCircle(GetWorld(), TargetPos, arrive->GetTargetRadius(), 32,
					FColor::Green, false, -1.f, 0, 1.f, FVector::YAxisVector, FVector::XAxisVector);
			}
			else if (a.SelectedBehavior == static_cast<int>(BehaviorTypes::Face))
			{
				// face: magenta line to target + white facing direction
				DrawDebugLine(GetWorld(), AgentPos, TargetPos, FColor::Magenta, false, -1.f, 0, 1.f);
				float Yaw = a.Agent->GetActorRotation().Yaw;
				float Rad = FMath::DegreesToRadians(Yaw);
				FVector FacingEnd = AgentPos + FVector{FMath::Cos(Rad), FMath::Sin(Rad), 0.f} * 150.f;
				DrawDebugLine(GetWorld(), AgentPos, FacingEnd, FColor::White, false, -1.f, 0, 2.f);
			}
			else if (a.SelectedBehavior == static_cast<int>(BehaviorTypes::Pursuit))
			{
				// pursuit: cyan line to actual target, yellow line to predicted position
				DrawDebugLine(GetWorld(), AgentPos, TargetPos, FColor::Cyan, false, -1.f, 0, 1.f);
				float Dist = FVector::Dist(AgentPos, TargetPos);
				float t = Dist / a.Agent->GetMaxLinearSpeed();
				FVector2D TargetVel = a.Behavior->GetTarget().LinearVelocity;
				FVector PredictedPos = TargetPos + FVector{TargetVel.X, TargetVel.Y, 0.f} * t;
				DrawDebugLine(GetWorld(), AgentPos, PredictedPos, FColor::Yellow, false, -1.f, 0, 2.f);
				DrawDebugPoint(GetWorld(), PredictedPos, 10.f, FColor::Yellow, false, -1.f);
			}
			else if (a.SelectedBehavior == static_cast<int>(BehaviorTypes::Flee))
			{
				// flee: line from agent to target + flee radius circle
				DrawDebugLine(GetWorld(), AgentPos, TargetPos, FColor::Red, false, -1.f, 0, 1.f);
				Flee* flee = a.Behavior->As<Flee>();
				DrawDebugCircle(GetWorld(), TargetPos, flee->GetFleeRadius(), 32,
					FColor::Red, false, -1.f, 0, 1.f, FVector::YAxisVector, FVector::XAxisVector);
			}
			else if (a.SelectedBehavior == static_cast<int>(BehaviorTypes::Evade))
			{
				// evade: red line to actual target, orange line to predicted position + flee radius
				DrawDebugLine(GetWorld(), AgentPos, TargetPos, FColor::Red, false, -1.f, 0, 1.f);
				float Dist = FVector::Dist(AgentPos, TargetPos);
				float t = Dist / a.Agent->GetMaxLinearSpeed();
				FVector2D TargetVel = a.Behavior->GetTarget().LinearVelocity;
				FVector PredictedPos = TargetPos + FVector{TargetVel.X, TargetVel.Y, 0.f} * t;
				DrawDebugLine(GetWorld(), AgentPos, PredictedPos, FColor::Orange, false, -1.f, 0, 2.f);
				DrawDebugPoint(GetWorld(), PredictedPos, 10.f, FColor::Orange, false, -1.f);
				Evade* evade = a.Behavior->As<Evade>();
				DrawDebugCircle(GetWorld(), PredictedPos, evade->GetFleeRadius(), 32,
					FColor::Orange, false, -1.f, 0, 1.f, FVector::YAxisVector, FVector::XAxisVector);
			}
		}
	}
}

bool ALevel_SteeringBehaviors::AddAgent(BehaviorTypes BehaviorType, bool AutoOrient)
{
	ImGui_Agent ImGuiAgent = {};
	ImGuiAgent.Agent = GetWorld()->SpawnActor<ASteeringAgent>(SteeringAgentClass, FVector{0,0,90}, FRotator::ZeroRotator);
	if (IsValid(ImGuiAgent.Agent))
	{
		ImGuiAgent.SelectedBehavior = static_cast<int>(BehaviorType);
		ImGuiAgent.SelectedTarget = -1; // Mouse
		
		SetAgentBehavior(ImGuiAgent);

		SteeringAgents.push_back(std::move(ImGuiAgent));
		
		RefreshTargetLabels();

		return true;
	}

	return false;
}

void ALevel_SteeringBehaviors::RemoveAgent(unsigned int Index)
{
	SteeringAgents[Index].Agent->Destroy();
	SteeringAgents.erase(SteeringAgents.begin() + Index);

	RefreshTargetLabels();
	RefreshAgentTargets(Index);
}

void ALevel_SteeringBehaviors::SetAgentBehavior(ImGui_Agent& Agent)
{
	// restore auto-orient if switching away from face
	if (Agent.PreviousBehavior == static_cast<int>(BehaviorTypes::Face))
		Agent.Agent->SetIsAutoOrienting(true);

	// restore speed if switching away from arrive
	if (Agent.Behavior && Agent.PreviousBehavior == static_cast<int>(BehaviorTypes::Arrive))
	{
		Arrive* arrive = Agent.Behavior->As<Arrive>();
		if (arrive->GetOriginalMaxSpeed() > 0.f)
			Agent.Agent->SetMaxLinearSpeed(arrive->GetOriginalMaxSpeed());
	}

	Agent.PreviousBehavior = Agent.SelectedBehavior;
	Agent.Behavior.reset();
	
	switch (static_cast<BehaviorTypes>(Agent.SelectedBehavior))
	{
	case BehaviorTypes::Seek:
		Agent.Behavior = std::make_unique<Seek>();
		break;
	case BehaviorTypes::Wander:
		Agent.Behavior = std::make_unique<Wander>();
		break;
	case BehaviorTypes::Flee:
		Agent.Behavior = std::make_unique<Flee>();
		break;
	case BehaviorTypes::Arrive:
		Agent.Behavior = std::make_unique<Arrive>();
		break;
	case BehaviorTypes::Face:
		Agent.Behavior = std::make_unique<Face>();
		Agent.Agent->SetIsAutoOrienting(false);
		break;
	case BehaviorTypes::Pursuit:
		Agent.Behavior = std::make_unique<Pursuit>();
		break;
	case BehaviorTypes::Evade:
		Agent.Behavior = std::make_unique<Evade>();
		break;
	default:
		Agent.Behavior = std::make_unique<Seek>();
		break;
	}

	UpdateTarget(Agent);
	
	Agent.Agent->SetSteeringBehavior(Agent.Behavior.get());
}

void ALevel_SteeringBehaviors::RefreshTargetLabels()
{
	TargetLabels.clear();
	
	TargetLabels.push_back("Mouse");
	for (int i{0}; i < SteeringAgents.size(); ++i)
	{
		TargetLabels.push_back(std::format("Agent {}", i));
	}
}

void ALevel_SteeringBehaviors::UpdateTarget(ImGui_Agent& Agent)
{
	// Note: MouseTarget position is updated via Level BP every click
	
	bool const bUseMouseAsTarget = Agent.SelectedTarget < 0;
	if (!bUseMouseAsTarget)
	{
		ASteeringAgent* const TargetAgent = SteeringAgents[Agent.SelectedTarget].Agent;

		FTargetData Target;
		Target.Position = TargetAgent->GetPosition();
		Target.Orientation = TargetAgent->GetRotation();
		Target.LinearVelocity = TargetAgent->GetLinearVelocity();
		Target.AngularVelocity = TargetAgent->GetAngularVelocity();

		Agent.Behavior->SetTarget(Target);
	}
	else
	{
		Agent.Behavior->SetTarget(MouseTarget);
	}
}

void ALevel_SteeringBehaviors::RefreshAgentTargets(unsigned int IndexRemoved)
{
	for (UINT i = 0; i < SteeringAgents.size(); ++i)
	{
		if (i >= IndexRemoved)
		{
			auto& Agent = SteeringAgents[i];
			if (Agent.SelectedTarget == IndexRemoved || i  == Agent.SelectedTarget)
			{
				--Agent.SelectedTarget;
			}
		}
	}
}

