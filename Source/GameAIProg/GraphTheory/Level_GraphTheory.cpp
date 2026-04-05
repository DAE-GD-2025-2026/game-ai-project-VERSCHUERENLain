// Fill out your copyright notice in the Description page of Project Settings.


#include "Level_GraphTheory.h"

#include "Algorithms/EulerianPath.h"
#include "Shared/GameAISpectator.h"

using namespace GameAI;

// Sets default values
ALevel_GraphTheory::ALevel_GraphTheory()
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
}

// Called when the game starts or when spawned
void ALevel_GraphTheory::BeginPlay()
{
	Super::BeginPlay();

	Renderer = GraphRenderer{GetWorld()};
	
	// Add the graph editor to our player
	if (PlayerController = Cast<APlayerController>(GetWorld()->GetFirstLocalPlayerFromController()->PlayerController); 
		GraphEditorClass && PlayerController)
	{
		PlayerGraphEditor = NewObject<UGraphEditorComponent>(PlayerController->GetPawn(), GraphEditorClass);
		PlayerGraphEditor->RegisterComponent();
		PlayerGraphEditor->SetEditedGraph(&Graph);
		PlayerGraphEditor->SetNodeFactory(&NodeFactory);
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("Unable to get PlayerController from LocalPlayer or GraphEditorClass is null"))
		return;
	}
	
	// Make the view orthogonal for less perspective issues
	if (AGameAISpectator* Player = Cast<AGameAISpectator>(PlayerController->GetPawnOrSpectator()); Player)
	{
		Player->SetCameraProjection(ECameraProjectionMode::Orthographic);
	}
	
	const int topNodeId = Graph.AddNode(std::make_unique<Node>(FVector2D{0.f, 450.f}));
	const int leftMidNodeId = Graph.AddNode(std::make_unique<Node>(FVector2D{-250.f, 200.f}));
	const int rightMidNodeId = Graph.AddNode(std::make_unique<Node>(FVector2D{250.f, 200.f}));
	const int leftBottomNodeId = Graph.AddNode(std::make_unique<Node>(FVector2D{-250.f, -100.f}));
	const int rightBottomNodeId = Graph.AddNode(std::make_unique<Node>(FVector2D{250.f, -100.f}));

	Graph.AddConnection(topNodeId, leftMidNodeId);
	Graph.AddConnection(topNodeId, rightMidNodeId);
	Graph.AddConnection(leftMidNodeId, rightMidNodeId);
	Graph.AddConnection(leftMidNodeId, leftBottomNodeId);
	Graph.AddConnection(rightMidNodeId, rightBottomNodeId);
	Graph.AddConnection(leftMidNodeId, rightBottomNodeId);
	Graph.AddConnection(rightMidNodeId, leftBottomNodeId);
	Graph.AddConnection(leftBottomNodeId, rightBottomNodeId);
	Graph.SetConnectionCostsToDistances();
	
	// Spawn the Agent
	Agent = GetWorld()->SpawnActor<ASteeringAgent>(SteeringAgentClass, 
	FVector{0,0,90}, FRotator::ZeroRotator);
	if (Agent)
	{
		Agent->SetSteeringBehavior(&PathFollow);

		EulerianPath eulerianPath{&Graph};
		Eulerianity eulerianity{};
		UpdateAgentPath(eulerianPath.FindPath(eulerianity));
	}
}

void ALevel_GraphTheory::BeginDestroy()
{
	Super::BeginDestroy();
}

void ALevel_GraphTheory::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	
#pragma region UI
	{
		//Setup
		bool windowActive = true;
		ImGui::SetNextWindowPos(WindowPos);
		ImGui::SetNextWindowSize(WindowSize);
		ImGui::Begin("Gameplay Programming", &windowActive, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse);
		ImGui::SetWindowFocus();
		ImGui::PushItemWidth(70);
		//Elements
		ImGui::Text("CONTROLS");
		ImGui::Indent();
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

		ImGui::Text("Graph Theory");
		ImGui::Spacing();
		ImGui::Spacing();

		//End
		ImGui::End();
	}
#pragma endregion UI
	
	Renderer.RenderGraph(Graph);
	
	if (PlayerGraphEditor && PlayerGraphEditor->HasGraphUpdated())
	{
		Graph.SetConnectionCostsToDistances();

		EulerianPath eulerianPath{&Graph};
		Eulerianity eulerianity{};
		UpdateAgentPath(eulerianPath.FindPath(eulerianity));
	}
}

void ALevel_GraphTheory::UpdateAgentPath(std::vector<Node*> const& Trail)
{
	if (!Agent)
	{
		return;
	}

	std::vector<FVector2D> path{};
	
	for (Node* node : Trail)
	{
		if (node)
		{
			path.push_back(node->GetPosition());
		}
	}

	PathFollow.SetPath(path);
	if (path.size() > 0)
	{
		Agent->SetPosition(path[0]);
	}
}




