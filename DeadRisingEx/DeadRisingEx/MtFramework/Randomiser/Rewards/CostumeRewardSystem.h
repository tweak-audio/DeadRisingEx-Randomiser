

#pragma once
#include <cstdint>


struct CostumeRewardResult
{
    int         costumeId;
    const char* name;
};

constexpr int COSTUME_POOL_SIZE = 76; 
constexpr bool RANDOMISE_STARTING_OUTFIT = true;

void                 ApplyRandomStartingOutfit();
void                 GenerateCostumeRewardSlots();
void                 ResetCostumeRewardSlots();
CostumeRewardResult  GiveNextCostumeReward();
void                 ReapplyRewardedCostumes();

