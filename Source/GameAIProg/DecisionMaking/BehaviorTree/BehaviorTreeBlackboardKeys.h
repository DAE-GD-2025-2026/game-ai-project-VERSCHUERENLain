#pragma once

#include "CoreMinimal.h"

namespace GameAI::BT::BlackboardKeys
{
	inline FName const TargetActor{TEXT("TargetActor")};
	inline FName const TargetVisible{TEXT("bTargetVisible")};
	inline FName const LastKnownLocation{TEXT("LastKnownLocation")};
	inline FName const HasLastKnownLocation{TEXT("bHasLastKnownLocation")};
	inline FName const SearchStartedAt{TEXT("SearchStartedAt")};
	inline FName const PatrolIndex{TEXT("PatrolIndex")};
	inline FName const RoamTarget{TEXT("RoamTarget")};
}
