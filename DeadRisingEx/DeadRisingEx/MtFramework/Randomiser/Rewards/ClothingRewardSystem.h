

#pragma once
#include <cstdint>


struct ClothingRewardResult
{
    int         costumeId;
    const char* name;
};

constexpr int COSTUME_POOL_SIZE = 76; 
constexpr bool RANDOMISE_STARTING_OUTFIT = true;

void                 ApplyRandomStartingOutfit();
void                 GenerateClothingRewardSlots();
void                 ResetClothingRewardSlots();
ClothingRewardResult GiveNextClothingReward();
void                 ReapplyRewardedCostumes();

