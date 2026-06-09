#pragma once
#include "CheckSystem.h"
#include "DeadRisingEx/MtFramework/Randomiser/TimeManager.h"
#include "DeadRisingEx/MtFramework/Randomiser/AreaKeySystem.h"
#include <unordered_map>
#include <vector>

struct CheckAvailabilityInfo
{
    uint32_t earliestTime;      // Earliest tick this check can be obtained
    bool requiresSpecialItem;   // Needs chainsaw, key, etc.
    ZoneID requiredZone;        // ZoneID::COUNT = no area key needed (always reachable)
    const char* notes;
};

class CheckAvailability
{
public:
    static void Initialize();
    static uint32_t GetEarliestTime(CheckId check);
    static ZoneID   GetRequiredZone(CheckId check);
    static bool     IsAvailableAt(CheckId check, uint32_t time);
    static std::string GetCheckName(CheckId check);

    static const std::vector<uint32_t>* GetCostumeCheckIds(uint32_t costumeId, ZoneID zone);
    static uint32_t GetCostumeCheckCount();

private:
    static std::unordered_map<CheckId, CheckAvailabilityInfo, CheckIdHash> s_availabilityDB;

    static void RegisterPPStickerTimes();
    static void RegisterSurvivorTimes();
    static void RegisterPsychopathTimes();
    static void RegisterCostumeTimes();
    static void BuildCostumeLookup();
};