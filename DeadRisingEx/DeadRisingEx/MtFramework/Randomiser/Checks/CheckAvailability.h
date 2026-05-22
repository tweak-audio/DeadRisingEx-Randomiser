#pragma once
#include "CheckSystem.h"
#include "../TimeManager.h"
#include <unordered_map>

struct CheckAvailabilityInfo
{
    uint32_t earliestTime;      // Earliest tick this check can be obtained
    bool requiresSpecialItem;   // Needs chainsaw, key, etc.
    const char* notes;          // Why this time? (for debugging)
};

class CheckAvailability
{
public:
    static void Initialize();
    static uint32_t GetEarliestTime(CheckId check);
    static bool IsAvailableAt(CheckId check, uint32_t time);

    static std::string GetCheckName(CheckId check);
    
private:
    static std::unordered_map<CheckId, CheckAvailabilityInfo, CheckIdHash> s_availabilityDB;
    
    static void RegisterPPStickerTimes();
    static void RegisterSurvivorTimes();
    static void RegisterPsychopathTimes();
};