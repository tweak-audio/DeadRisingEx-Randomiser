#pragma once
#include <string>
#include <vector>
#include <cstdint>

class SaveStateManager
{
public:
    static void Initialize();
    static void SaveCheckState();
    static void LoadCheckState();

    static void MarkCheckCompleted(uint16_t checkId);
    static bool IsCheckCompleted(uint16_t checkId);
    static std::vector<uint16_t> GetCompletedChecks();
    static void ResetCompletedChecks();

    static void    SetRewardCostume(uint8_t slot, uint8_t id);
    static bool    HasRewardCostume(uint8_t slot);
    static uint8_t GetRewardCostumeId(uint8_t slot);

private:
    static std::string GetSaveFilePath();
    static std::vector<uint16_t> s_completedChecks;
    static uint8_t s_rewardCostumeIds[6];
    static bool    s_hasRewardCostume[6];
};