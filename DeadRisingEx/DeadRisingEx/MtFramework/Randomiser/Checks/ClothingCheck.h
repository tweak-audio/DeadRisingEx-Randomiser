

#pragma once
#include <cstdint>

namespace ClothingCheck
{
    void InitializeHooks();
    void* GetStateMachineObj();
    void SetStateMachineObj(void* obj);
}

constexpr int TOTAL_COSTUMES = 66;