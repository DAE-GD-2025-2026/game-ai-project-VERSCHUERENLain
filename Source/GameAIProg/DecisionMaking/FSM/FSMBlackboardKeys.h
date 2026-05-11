#pragma once

#include "CoreMinimal.h"

namespace GameAI::FSM::BlackboardKeys
{
	inline FName const TargetActor{TEXT("SelfActor")};
	inline FName const LastKnownLocation{TEXT("LootLocation")};
	inline FName const SearchStartTime{TEXT("SearchStartTime")};
}
