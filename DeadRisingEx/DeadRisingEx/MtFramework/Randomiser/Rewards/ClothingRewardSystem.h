

#pragma once
#include <cstdint>

struct ClothingRewardResult
{
    int         costumeId;
    const char* name;
};

void                 GenerateClothingRewardSlots();
void                 ResetClothingRewardSlots();
ClothingRewardResult GiveNextClothingReward();

constexpr int COSTUME_POOL_SIZE = 76; 