#pragma once

#include "BehaviorTree/BehaviorTree.h"
#include "BehaviorTree/BlackboardData.h"
#include "BehaviorTree/Composites/BTComposite_Selector.h"
#include "BehaviorTree/Composites/BTComposite_Sequence.h"

namespace GameAI::BT::Builder
{
	template <typename T>
	T* AddBlackboardKey(UBlackboardData* BlackboardAsset, FName const KeyName)
	{
		return BlackboardAsset ? BlackboardAsset->UpdatePersistentKey<T>(KeyName) : nullptr;
	}

	inline void FinalizeBlackboard(UBlackboardData* BlackboardAsset)
	{
		if (!BlackboardAsset)
		{
			return;
		}

		BlackboardAsset->UpdateParentKeys();
		BlackboardAsset->UpdateKeyIDs();
		BlackboardAsset->UpdateIfHasSynchronizedKeys();
	}

	inline UBehaviorTree* CreateTree(UObject* Outer, UBlackboardData* BlackboardAsset)
	{
		UBehaviorTree* Tree = NewObject<UBehaviorTree>(Outer);
		Tree->BlackboardAsset = BlackboardAsset;
		return Tree;
	}

	inline UBTComposite_Selector* SetSelectorRoot(UBehaviorTree* Tree)
	{
		if (!Tree)
		{
			return nullptr;
		}

		UBTComposite_Selector* Root = NewObject<UBTComposite_Selector>(Tree);
		Root->InitializeFromAsset(*Tree);
		Tree->RootNode = Root;
		return Root;
	}

	inline UBTComposite_Sequence* SetSequenceRoot(UBehaviorTree* Tree)
	{
		if (!Tree)
		{
			return nullptr;
		}

		UBTComposite_Sequence* Root = NewObject<UBTComposite_Sequence>(Tree);
		Root->InitializeFromAsset(*Tree);
		Tree->RootNode = Root;
		return Root;
	}

	template <typename T>
	T* AddSelector(UBTCompositeNode& ParentNode)
	{
		T* Node = NewObject<T>(ParentNode.GetTreeAsset());
		Node->InitializeFromAsset(*ParentNode.GetTreeAsset());
		ParentNode.Children.AddZeroed();
		ParentNode.Children.Last().ChildComposite = Node;
		return Node;
	}

	template <typename T>
	T* AddTask(UBTCompositeNode& ParentNode)
	{
		T* Task = NewObject<T>(ParentNode.GetTreeAsset());
		Task->InitializeFromAsset(*ParentNode.GetTreeAsset());
		ParentNode.Children.AddZeroed();
		ParentNode.Children.Last().ChildTask = Task;
		return Task;
	}

	template <typename T>
	T* AddDecoratorToLastChild(UBTCompositeNode& ParentNode)
	{
		check(ParentNode.Children.Num() > 0);
		T* Decorator = NewObject<T>(ParentNode.GetTreeAsset());
		Decorator->InitializeFromAsset(*ParentNode.GetTreeAsset());
		ParentNode.Children.Last().Decorators.Add(Decorator);
		return Decorator;
	}
}
