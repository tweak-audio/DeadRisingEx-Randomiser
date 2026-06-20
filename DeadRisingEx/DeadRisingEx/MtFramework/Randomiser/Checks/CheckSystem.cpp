#include "CheckSystem.h"
#include "CheckAvailability.h"
#include "DeadRisingEx/MtFramework/Randomiser/AreaKeySystem.h"
#include "SurvivorPhotoCheck.h"
#include "DeadRisingEx/MtFramework/Randomiser/InputSystem.h"
#include "DeadRisingEx/MtFramework/Randomiser/Rewards/SetItemRewardSystem.h"
#include "DeadRisingEx/MtFramework/Randomiser/Rewards/TimeChunkReward.h"
#include "DeadRisingEx/MtFramework/Randomiser/Rewards/CostumeRewardSystem.h"
#include "ChecksManager.h"
#include "DeadRisingEx/MtFramework/Randomiser/RandomiserConfig.h"

#include "DeadRisingEx/Utilities/DebugLog.h"
#include "DeadRisingEx/MtFramework/Randomiser/RandomiserSave.h"
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <Windows.h>

// ─────────────────────────────────────────────
//  State
// ─────────────────────────────────────────────

struct CheckRange
{
    CheckType   type;
    uint32_t    idMin;
    uint32_t    idMax;
};

static std::vector<CheckRange>                      s_checkRanges;
static std::vector<CheckId>                         s_allChecks;
static std::unordered_map<CheckId, Reward,
    CheckIdHash>                                    s_rewardMap;
static std::unordered_set<uint64_t>                 s_completed;
static std::vector<CheckCallback>                   s_callbacks;

static uint32_t s_seed     = 0;
static uint32_t s_rngState = 0;

static int s_setItemCount = 0;

static CheckSystemState                            s_state       = CheckSystemState::UNINITIALIZED;
static std::vector<std::pair<CheckType, uint32_t>> s_pendingChecks;

// ─────────────────────────────────────────────
//  RNG
// ─────────────────────────────────────────────

static void     RngSeed(uint32_t seed) { s_rngState = seed; }
static uint32_t RngNext()
{
    s_rngState = s_rngState * 1664525u + 1013904223u;
    return s_rngState;
}
static int RngRange(int min, int max)
{
    return min + (int)(RngNext() % (uint32_t)(max - min));
}

static uint64_t CheckKey(CheckId c)
{
    return ((uint64_t)c.type << 32) | c.id;
}

// Helper to convert CheckId to uint16_t for SaveStateManager
static uint16_t CheckIdToUInt16(CheckId c)
{
    // Simple encoding: type in upper bits, id in lower bits
    // Assumes id fits in 12 bits (0-4095) and type fits in 4 bits (0-15)
    return ((uint16_t)c.type << 12) | (c.id & 0xFFF);
}

static CheckId UInt16ToCheckId(uint16_t val)
{
    CheckType type = (CheckType)(val >> 12);
    uint32_t id = val & 0xFFF;
    return { type, id };
}

// ─────────────────────────────────────────────
//  Internal
// ─────────────────────────────────────────────

static void RebuildCheckList()
{
    s_allChecks.clear();
    for (auto& range : s_checkRanges)
        for (uint32_t id = range.idMin; id <= range.idMax; id++)
            s_allChecks.push_back({ range.type, id });
}

static void DrainPendingChecks()
{
    // Move out first so any check that re-queues during drain doesn't corrupt iteration
    auto pending = std::move(s_pendingChecks);
    s_pendingChecks.clear();
    for (auto& [type, id] : pending)
        CheckSystem::CompleteCheck(type, id);
}

void CheckSystem::GenerateRewardMap()
{
    int total = (int)s_allChecks.size();
    if (total == 0) return;

    RngSeed(s_seed);

    // Fixed rewards
    const RandomiserConfig& cfg = RandomiserConfig::Get();
    std::vector<Reward> levelUps, batteries, timeChunks, areaKeys, keyItems;
    for (int i = 0; i < LEVEL_UP_REWARDS; i++)
        levelUps.push_back({ RewardType::LevelUp, 0 });
    for (int i = 0; i < BATTERY_REFILL_REWARDS; i++)
        batteries.push_back({ RewardType::BatteryRefill, 0 });
    if (cfg.timeChunks)
    {
        int chunkCount = cfg.sixHourChunks ? 11 : 5;
        for (int i = 0; i < chunkCount; i++)
            timeChunks.push_back({ RewardType::TimeChunk, i + 1 });
    }
    if (cfg.areaKeyRewards)
    {
        for (int i = 0; i < static_cast<int>(ZoneID::COUNT); i++)
        {
            if (i == static_cast<int>(ZoneID::ParadisePlaza)) continue;
            areaKeys.push_back({ RewardType::AreaKey, i });  // value = ZoneID index
        }
    }
    if (cfg.keyItemRewards)
    {
        for (int i = 0; i < TOTAL_KEY_ITEMS; i++)
            keyItems.push_back({ RewardType::KeyItem, i });  // value = keyId
    }

    // Randomly split remaining slots between costumes and set items
    int effectiveTimeChunkRewards = cfg.timeChunks ? (cfg.sixHourChunks ? 11 : 5) : 0;
    int effectiveAreaKeyRewards   = cfg.areaKeyRewards ? AREA_KEY_REWARDS : 0;
    int effectiveKeyItemRewards   = cfg.keyItemRewards ? TOTAL_KEY_ITEMS : 0;
    int remaining = total - LEVEL_UP_REWARDS - BATTERY_REFILL_REWARDS - effectiveTimeChunkRewards - effectiveAreaKeyRewards - effectiveKeyItemRewards;
    int maxCostume    = min(remaining, COSTUME_POOL_SIZE);  // can't exceed pool size
    int costumeCount  = RngRange(COSTUME_REWARDS_MIN, maxCostume + 1);
    int setItemCount  = remaining - costumeCount;
    s_setItemCount = setItemCount;

    char buf[128];
    sprintf_s(buf, "[CHECKS] Reward split: %d time chunks, %d area keys, %d key items, %d costumes, %d set items (seed %u)",
              effectiveTimeChunkRewards, effectiveAreaKeyRewards, effectiveKeyItemRewards, costumeCount, setItemCount, s_seed);
    LogLine(buf);

    std::vector<Reward> costumes, setItems;
    for (int i = 0; i < costumeCount; i++)
        costumes.push_back({ RewardType::Costume, 0 });
    for (int i = 0; i < setItemCount; i++)
        setItems.push_back({ RewardType::SetItem, 0 });

    // Interleave: spread each type evenly across the check list
    std::vector<Reward> pool(total, { RewardType::SetItem, 0 });

    auto PlaceEvenly = [&](std::vector<Reward>& rewards)
    {
        if (rewards.empty()) return;
        float step = (float)total / (float)rewards.size();
        for (int i = 0; i < (int)rewards.size(); i++)
        {
            int idx = (int)(step * i + step * 0.5f) % total;
            // Find nearest empty (SetItem placeholder) slot
            for (int offset = 0; offset < total; offset++)
            {
                int candidate = (idx + offset) % total;
                if (pool[candidate].type == RewardType::SetItem &&
                    pool[candidate].value == 0)
                {
                    pool[candidate] = rewards[i];
                    break;
                }
            }
        }
    };

    // Place time chunks first (most important for beatability), then area keys and key items
    PlaceEvenly(timeChunks);
    PlaceEvenly(areaKeys);
    PlaceEvenly(keyItems);
    PlaceEvenly(levelUps);
    PlaceEvenly(batteries);
    PlaceEvenly(costumes);
    // SetItem fills all remaining slots — already in pool as placeholders

    // Fisher-Yates shuffle within each reward type's allocated positions
    // to add randomness while preserving spread
    RngSeed(s_seed);
    for (int i = total - 1; i > 0; i--)
    {
        int j = RngRange(0, i + 1);
        Reward tmp = pool[i];
        pool[i]    = pool[j];
        pool[j]    = tmp;
    }

    s_rewardMap.clear();
    for (int i = 0; i < total; i++)
        s_rewardMap[s_allChecks[i]] = pool[i];
}

int CheckSystem::GetSetItemCount() { return s_setItemCount; }

// BFS from PP through the zone graph. A zone is reachable if its key is held
// AND there is a path to it from PP through already-reachable unlocked zones.
// The PP-EP direct corridor is blocked until Day 2 7AM (19 hours from game start).
static void ComputeReachableZones(const bool* zonesUnlocked, bool* zonesReachable, uint32_t currentTime, bool mtKeyItemHeld)
{
    static constexpr uint32_t EP_PATH_OPEN_TICK =
        TimeManager::GAME_START_TICK + 19 * TimeManager::TICKS_PER_HOUR;

    memset(zonesReachable, 0, static_cast<int>(ZoneID::COUNT) * sizeof(bool));
    zonesReachable[static_cast<int>(ZoneID::ParadisePlaza)] = true;

    bool changed = true;
    while (changed)
    {
        changed = false;
        for (int zi = 0; zi < static_cast<int>(ZoneID::COUNT); zi++)
        {
            if (!zonesReachable[zi]) continue;
            ZoneID from = static_cast<ZoneID>(zi);

            for (ZoneID to : AreaKeySystem::GetAdjacentZones(from))
            {
                int ti = static_cast<int>(to);
                if (zonesReachable[ti]) continue;
                if (!zonesUnlocked[ti]) continue;

                // Mall zones ↔ MT requires the MT key item in both directions;
                // LP and MeatProcessing are exempt
                bool isLockedMtEdge =
                    (from == ZoneID::MaintenanceTunnel &&
                     to != ZoneID::LeisurePark && to != ZoneID::MeatProcessing) ||
                    (to == ZoneID::MaintenanceTunnel &&
                     from != ZoneID::LeisurePark && from != ZoneID::MeatProcessing);
                if (isLockedMtEdge && !mtKeyItemHeld) continue;

                // PP-EP corridor blocked before Day 2 7AM; use AFP or MT route instead
                if (from == ZoneID::ParadisePlaza && to == ZoneID::EntrancePlaza &&
                    currentTime < EP_PATH_OPEN_TICK) continue;

                zonesReachable[ti] = true;
                changed = true;
            }
        }
    }
}

bool CheckSystem::IsSeedBeatable(bool quiet)
{
    if (s_allChecks.empty() || s_rewardMap.empty())
    {
        if (!quiet) LogLine("[SEED VALIDATION] ERROR: check list or reward map not built");
        return false;
    }

    TimeManager::ChunkSize mode = TimeChunkReward::GetChunkMode();
    int totalChunks = TimeChunkReward::GetTotalChunks();

    char buf[256];
    if (!quiet)
    {
        LogLine("[SEED VALIDATION] === CHECKING SEED BEATABILITY ===");
        sprintf_s(buf, "[SEED VALIDATION] Mode: %s, Total chunks needed: %d",
                  (mode == TimeManager::ChunkSize::SIX_HOURS) ? "6hr" : "12hr",
                  totalChunks - 1);
        LogLine(buf);
    }

    // Starting state: chunk 0 unlocked, Frank starts in Paradise Plaza
    int chunksUnlocked = 1;
    uint32_t currentMaxTime = TimeManager::GetChunk(mode, 0).endTick;

    bool zonesUnlocked[static_cast<int>(ZoneID::COUNT)] = {};
    zonesUnlocked[static_cast<int>(ZoneID::ParadisePlaza)] = true;
    bool mtKeyUnlocked = false;

    std::unordered_set<uint64_t> usedChecks;

    // Each pass collects all currently reachable time chunks and area keys.
    // Repeat until no new progress can be made.
    bool anyProgress = true;
    while (anyProgress)
    {
        anyProgress = false;

        bool zonesReachable[static_cast<int>(ZoneID::COUNT)] = {};
        ComputeReachableZones(zonesUnlocked, zonesReachable, currentMaxTime, mtKeyUnlocked);

        for (const auto& check : s_allChecks)
        {
            uint64_t ckey = CheckKey(check);
            if (usedChecks.count(ckey)) continue;

            uint32_t availableAt = CheckAvailability::GetEarliestTime(check);
            if (availableAt > currentMaxTime) continue;

            ZoneID zone = CheckAvailability::GetRequiredZone(check);
            if (zone != ZoneID::COUNT && !zonesReachable[static_cast<int>(zone)]) continue;

            Reward reward = GetRewardForCheck(check);

            if (reward.type == RewardType::TimeChunk)
            {
                usedChecks.insert(ckey);
                chunksUnlocked++;
                currentMaxTime = TimeManager::GetChunk(mode, chunksUnlocked - 1).endTick;
                anyProgress = true;

                if (!quiet)
                {
                    sprintf_s(buf, "[SEED VALIDATION]   +TimeChunk -> %d/%d unlocked, limit now %s",
                              chunksUnlocked, totalChunks,
                              TimeManager::TicksToTimeString(currentMaxTime).c_str());
                    LogLine(buf);
                }
            }
            else if (reward.type == RewardType::AreaKey)
            {
                ZoneID grantedZone = static_cast<ZoneID>(reward.value);
                if (grantedZone < ZoneID::COUNT && !zonesUnlocked[static_cast<int>(grantedZone)])
                {
                    usedChecks.insert(ckey);
                    zonesUnlocked[static_cast<int>(grantedZone)] = true;
                    anyProgress = true;

                    if (!quiet)
                    {
                        sprintf_s(buf, "[SEED VALIDATION]   +AreaKey -> unlocked %s",
                                  AreaKeySystem::GetZoneName(grantedZone));
                        LogLine(buf);
                    }
                }
            }
            else if (reward.type == RewardType::KeyItem && reward.value == 0 && !mtKeyUnlocked)
            {
                usedChecks.insert(ckey);
                mtKeyUnlocked = true;
                anyProgress = true;
                if (!quiet)
                    LogLine("[SEED VALIDATION]   +KeyItem(MT) -> MT exits now usable");
            }
        }
    }

    if (chunksUnlocked >= totalChunks)
    {
        if (!quiet) LogLine("[SEED VALIDATION] SEED IS BEATABLE");
        return true;
    }

    if (!quiet)
    {
        sprintf_s(buf, "[SEED VALIDATION] UNBEATABLE: stuck at %d/%d chunks", chunksUnlocked, totalChunks);
        LogLine(buf);
    }
    return false;
}

void CheckSystem::GenerateBeatableSeed()
{
    LogLine("[SEED] Generating beatable seed...");
    
    int attempts = 0;
    const int MAX_ATTEMPTS = 10000;
    
    while (attempts < MAX_ATTEMPTS)
    {
        SetSeed(0);  // Generate random seed

        if (IsSeedBeatable(/*quiet=*/true))
        {
            char buf[128];
            sprintf_s(buf, "[SEED] ✓ Found beatable seed after %d attempts: %u", 
                      attempts + 1, s_seed);
            LogLine(buf);
            
            // Write analysis to file
            WriteSeedAnalysisToFile();
            
            return;
        }
        
        attempts++;
        
        if (attempts % 100 == 0)
        {
            char buf[64];
            sprintf_s(buf, "[SEED] Still searching... (%d attempts)", attempts);
            LogLine(buf);
        }
    }
    
    LogLine("[SEED] ✗ WARNING: Could not find beatable seed after 10000 attempts!");
    LogLine("[SEED] Using last generated seed anyway - may be unbeatable!");
    
    // Write analysis anyway so you can see why it failed
    WriteSeedAnalysisToFile();
}

void CheckSystem::WriteSeedAnalysisToFile()
{
    const char* filename = "DeadRisingEx_SeedAnalysis.txt";
    const RandomiserConfig& cfg = RandomiserConfig::Get();
    
    FILE* f = nullptr;
    fopen_s(&f, filename, "w");
    if (!f)
    {
        LogLine("[CHECKS] ERROR: Could not create seed analysis file");
        return;
    }
    
    // Header
    fprintf(f, "═══════════════════════════════════════════════════════════\n");
    fprintf(f, "  DEAD RISING RANDOMIZER - SEED ANALYSIS\n");
    fprintf(f, "═══════════════════════════════════════════════════════════\n\n");
    fprintf(f, "Seed: %u\n", s_seed);
    fprintf(f, "Generated: %s\n", GetCurrentTimeString().c_str());
    fprintf(f, "Total Checks: %d\n\n", (int)s_allChecks.size());
    
    // Chunk mode info
    TimeManager::ChunkSize mode = TimeChunkReward::GetChunkMode();
    int totalChunks = TimeChunkReward::GetTotalChunks();
    fprintf(f, "Time Gate Mode: %s (%d chunks)\n\n",
            (mode == TimeManager::ChunkSize::SIX_HOURS) ? "6-hour" : "12-hour",
            totalChunks);
    
    fprintf(f, "═══════════════════════════════════════════════════════════\n");
    fprintf(f, "  GAME FLOW WALKTHROUGH\n");
    fprintf(f, "═══════════════════════════════════════════════════════════\n\n");
    fprintf(f, "Checks appear when they first become reachable (time + zone).\n");
    fprintf(f, "★ Progress = time chunks / area keys needed to advance.\n");
    if (cfg.timeChunks)
        fprintf(f, "Need %d time chunks total to reach the end of the game.\n\n", totalChunks - 1);
    else
        fprintf(f, "Time gating disabled — all time periods open from start.\n\n");

    // ── Simulation state ─────────────────────────────────────────
    // If time chunks disabled, treat all time as available from start.
    // If area key rewards disabled, all zones open from start.
    int      simChunks  = 1;
    uint32_t simMaxTime = cfg.timeChunks
                            ? TimeManager::GetChunk(mode, 0).endTick
                            : TimeManager::GetChunk(mode, totalChunks - 1).endTick;
    bool simMtKeyUnlocked = false;

    bool simZones[static_cast<int>(ZoneID::COUNT)] = {};
    if (cfg.areaKeyRewards)
    {
        simZones[static_cast<int>(ZoneID::ParadisePlaza)] = true;
    }
    else
    {
        for (int z = 0; z < static_cast<int>(ZoneID::COUNT); z++)
            simZones[z] = true;
    }

    std::unordered_set<uint64_t> processedChecks;
    int  phaseNum       = 0;
    bool anyPhaseProgress = true;

    while (anyPhaseProgress)
    {
        anyPhaseProgress = false;

        bool simZonesReachable[static_cast<int>(ZoneID::COUNT)] = {};
        ComputeReachableZones(simZones, simZonesReachable, simMaxTime, simMtKeyUnlocked);

        std::vector<CheckId> phaseProgress;
        std::vector<CheckId> phaseRewards;

        for (const auto& check : s_allChecks)
        {
            uint64_t ckey = CheckKey(check);
            if (processedChecks.count(ckey)) continue;

            uint32_t availAt = CheckAvailability::GetEarliestTime(check);
            ZoneID   reqZone = CheckAvailability::GetRequiredZone(check);

            if (availAt > simMaxTime) continue;
            if (reqZone != ZoneID::COUNT && !simZonesReachable[static_cast<int>(reqZone)]) continue;

            processedChecks.insert(ckey);
            anyPhaseProgress = true;

            Reward r = GetRewardForCheck(check);
            bool isProgress = (r.type == RewardType::TimeChunk) ||
                              (r.type == RewardType::AreaKey &&
                               static_cast<ZoneID>(r.value) < ZoneID::COUNT &&
                               !simZones[r.value]) ||
                              (r.type == RewardType::KeyItem && r.value == 0 && !simMtKeyUnlocked);
            if (isProgress) phaseProgress.push_back(check);
            else            phaseRewards.push_back(check);
        }

        if (!anyPhaseProgress) break;
        phaseNum++;

        // ── Phase header ─────────────────────────────────────────
        fprintf(f, "─────────────────────────────────────────────────────────\n");
        fprintf(f, "PHASE %d\n", phaseNum);
        if (cfg.timeChunks)
            fprintf(f, "  Time:   up to %s\n", TimeManager::TicksToTimeString(simMaxTime).c_str());
        fprintf(f, "  Zones: ");
        bool firstZone = true;
        for (int z = 0; z < static_cast<int>(ZoneID::COUNT); z++)
        {
            if (!simZonesReachable[z]) continue;
            if (!firstZone) fprintf(f, ", ");
            fprintf(f, "%s", AreaKeySystem::GetZoneName(static_cast<ZoneID>(z)));
            firstZone = false;
        }
        fprintf(f, "\n\n");

        // ── Progress checks ───────────────────────────────────────
        if (!phaseProgress.empty())
        {
            fprintf(f, "  ★ Progress (%d):\n", (int)phaseProgress.size());
            int previewChunks = simChunks;
            for (const auto& check : phaseProgress)
            {
                std::string name    = CheckAvailability::GetCheckName(check);
                Reward      r       = GetRewardForCheck(check);
                ZoneID      reqZone = CheckAvailability::GetRequiredZone(check);
                bool needsKey = (reqZone != ZoneID::COUNT && reqZone != ZoneID::ParadisePlaza);

                if (r.type == RewardType::TimeChunk)
                {
                    previewChunks++;
                    std::string newLimit = (previewChunks <= totalChunks)
                        ? TimeManager::TicksToTimeString(TimeManager::GetChunk(mode, previewChunks - 1).endTick)
                        : "end of game";
                    if (needsKey)
                        fprintf(f, "    %s  →  Time Chunk  (unlocks to %s)  [%s key]\n",
                                name.c_str(), newLimit.c_str(), AreaKeySystem::GetZoneName(reqZone));
                    else
                        fprintf(f, "    %s  →  Time Chunk  (unlocks to %s)\n",
                                name.c_str(), newLimit.c_str());
                }
                else if (r.type == RewardType::AreaKey)
                {
                    const char* granted = AreaKeySystem::GetZoneName(static_cast<ZoneID>(r.value));
                    if (needsKey)
                        fprintf(f, "    %s  →  Area Key: %s  [%s key]\n",
                                name.c_str(), granted, AreaKeySystem::GetZoneName(reqZone));
                    else
                        fprintf(f, "    %s  →  Area Key: %s\n", name.c_str(), granted);
                }
                else if (r.type == RewardType::KeyItem && r.value == 0)
                {
                    if (needsKey)
                        fprintf(f, "    %s  →  MT Key (enables MT exits)  [%s key]\n",
                                name.c_str(), AreaKeySystem::GetZoneName(reqZone));
                    else
                        fprintf(f, "    %s  →  MT Key (enables MT exits)\n", name.c_str());
                }
            }
            fprintf(f, "\n");
        }

        // ── Reward checks ─────────────────────────────────────────
        if (!phaseRewards.empty())
        {
            fprintf(f, "  Rewards (%d):\n", (int)phaseRewards.size());
            for (const auto& check : phaseRewards)
            {
                std::string name    = CheckAvailability::GetCheckName(check);
                Reward      r       = GetRewardForCheck(check);
                ZoneID      reqZone = CheckAvailability::GetRequiredZone(check);
                bool needsKey = (reqZone != ZoneID::COUNT && reqZone != ZoneID::ParadisePlaza);

                const char* typeName = "Unknown";
                switch (r.type)
                {
                    case RewardType::LevelUp:       typeName = "Level Up";       break;
                    case RewardType::BatteryRefill: typeName = "Battery Refill"; break;
                    case RewardType::SetItem:       typeName = "Set Item";       break;
                    case RewardType::Costume:       typeName = "Costume";        break;
                    case RewardType::KeyItem:       typeName = "Key Item";       break;
                    default: break;
                }
                if (needsKey)
                    fprintf(f, "    %s  →  %s  [%s key]\n",
                            name.c_str(), typeName, AreaKeySystem::GetZoneName(reqZone));
                else
                    fprintf(f, "    %s  →  %s\n", name.c_str(), typeName);
            }
            fprintf(f, "\n");
        }

        // ── Apply progress events ─────────────────────────────────
        bool anyUnlock = false;
        for (const auto& check : phaseProgress)
        {
            Reward r = GetRewardForCheck(check);
            if (r.type == RewardType::TimeChunk)
            {
                simChunks++;
                if (simChunks <= totalChunks)
                {
                    simMaxTime = TimeManager::GetChunk(mode, simChunks - 1).endTick;
                    if (!anyUnlock) { fprintf(f, "  Unlocks:\n"); anyUnlock = true; }
                    fprintf(f, "    → Time extended to %s\n",
                            TimeManager::TicksToTimeString(simMaxTime).c_str());
                }
            }
            else if (r.type == RewardType::AreaKey)
            {
                ZoneID z = static_cast<ZoneID>(r.value);
                if (z < ZoneID::COUNT && !simZones[static_cast<int>(z)])
                {
                    simZones[static_cast<int>(z)] = true;
                    if (!anyUnlock) { fprintf(f, "  Unlocks:\n"); anyUnlock = true; }
                    fprintf(f, "    → Zone: %s\n", AreaKeySystem::GetZoneName(z));
                }
            }
            else if (r.type == RewardType::KeyItem && r.value == 0)
            {
                simMtKeyUnlocked = true;
                if (!anyUnlock) { fprintf(f, "  Unlocks:\n"); anyUnlock = true; }
                fprintf(f, "    → MT Key: exits from MT now usable\n");
            }
        }
        if (anyUnlock) fprintf(f, "\n");

        fprintf(f, "  Total this phase: %d progress + %d rewards\n\n",
                (int)phaseProgress.size(), (int)phaseRewards.size());
    }

    // ── Final status ──────────────────────────────────────────────
    fprintf(f, "─────────────────────────────────────────────────────────\n");
    if (!cfg.timeChunks || simChunks >= totalChunks)
        fprintf(f, "✓  SEED IS BEATABLE\n\n");
    else
        fprintf(f, "✗  STUCK at %d / %d time chunks — seed may be unbeatable\n\n",
                simChunks, totalChunks);
    
    // Area key progression
    fprintf(f, "═══════════════════════════════════════════════════════════\n");
    fprintf(f, "  AREA KEY PROGRESSION\n");
    fprintf(f, "═══════════════════════════════════════════════════════════\n\n");

    if (!cfg.areaKeyRewards)
    {
        fprintf(f, "Area key rewards disabled — all zones open from start.\n\n");
    }
    else
    {
        fprintf(f, "Zone key sources (which check grants each area key):\n\n");
        for (int z = 0; z < static_cast<int>(ZoneID::COUNT); z++)
        {
            ZoneID zone = static_cast<ZoneID>(z);
            if (zone == ZoneID::ParadisePlaza) continue;

            bool found = false;
            for (const auto& check : s_allChecks)
            {
                Reward reward = GetRewardForCheck(check);
                if (reward.type == RewardType::AreaKey && static_cast<ZoneID>(reward.value) == zone)
                {
                    std::string checkName   = CheckAvailability::GetCheckName(check);
                    uint32_t    availAt     = CheckAvailability::GetEarliestTime(check);
                    ZoneID      reqZone     = CheckAvailability::GetRequiredZone(check);
                    bool        needsKey    = (reqZone != ZoneID::COUNT && reqZone != ZoneID::ParadisePlaza);

                    if (needsKey)
                        fprintf(f, "  %s  ←  %s [requires %s key] (available: %s)\n",
                                AreaKeySystem::GetZoneName(zone), checkName.c_str(),
                                AreaKeySystem::GetZoneName(reqZone),
                                TimeManager::TicksToTimeString(availAt).c_str());
                    else
                        fprintf(f, "  %s  ←  %s (available: %s)\n",
                                AreaKeySystem::GetZoneName(zone), checkName.c_str(),
                                TimeManager::TicksToTimeString(availAt).c_str());
                    found = true;
                    break;
                }
            }
            if (!found)
                fprintf(f, "  %s  ←  NOT IN REWARD POOL\n", AreaKeySystem::GetZoneName(zone));
        }
        fprintf(f, "\n");
    }

    // Summary statistics
    fprintf(f, "═══════════════════════════════════════════════════════════\n");
    fprintf(f, "  REWARD DISTRIBUTION SUMMARY\n");
    fprintf(f, "═══════════════════════════════════════════════════════════\n\n");
    
    int levelUps = 0, batteries = 0, timeChunks = 0, setItems = 0, areaKeys = 0, costumes = 0, keyItems = 0;
    for (const auto& pair : s_rewardMap)
    {
        switch (pair.second.type)
        {
            case RewardType::LevelUp:       levelUps++; break;
            case RewardType::BatteryRefill: batteries++; break;
            case RewardType::TimeChunk:     timeChunks++; break;
            case RewardType::SetItem:       setItems++; break;
            case RewardType::AreaKey:       areaKeys++; break;
            case RewardType::Costume:       costumes++; break;
            case RewardType::KeyItem:       keyItems++; break;
            default: break;
        }
    }

    fprintf(f, "Level Ups: %d\n", levelUps);
    fprintf(f, "Battery Refills: %d\n", batteries);
    fprintf(f, "Time Chunks: %d\n", timeChunks);
    fprintf(f, "Area Keys: %d\n", areaKeys);
    fprintf(f, "Key Items: %d\n", keyItems);
    fprintf(f, "Set Items: %d\n", setItems);
    fprintf(f, "Costumes: %d\n", costumes);
    fprintf(f, "\nTotal Rewards: %d\n", levelUps + batteries + timeChunks + areaKeys + keyItems + setItems + costumes);
    
    fclose(f);
    
    char buf[128];
    sprintf_s(buf, "[CHECKS] Seed analysis written to %s", filename);
    LogLine(buf);
}

// Helper to get current timestamp
std::string CheckSystem::GetCurrentTimeString()
{
    time_t now = time(nullptr);
    struct tm timeinfo;
    localtime_s(&timeinfo, &now);
    
    char buf[64];
    strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", &timeinfo);
    return std::string(buf);
}

// ─────────────────────────────────────────────
//  Persistence
// ─────────────────────────────────────────────

void CheckSystem::SaveSeedToFile()
{
    RandomiserSave::SetSeed(s_seed);
}

bool CheckSystem::LoadSeedFromFile()
{
    uint32_t loaded = RandomiserSave::GetSeed();
    if (loaded != 0) { s_seed = loaded; return true; }
    return false;
}

void CheckSystem::LoadCompletedChecks()
{
    std::vector<uint16_t> completed = SaveStateManager::GetCompletedChecks();
    
    for (uint16_t val : completed)
    {
        CheckId check = UInt16ToCheckId(val);
        uint64_t key = CheckKey(check);
        s_completed.insert(key);
    }
    
    char buf[128];
    sprintf_s(buf, "[CHECKS] Loaded %d completed checks from save state", (int)completed.size());
    LogLine(buf);
}

// ─────────────────────────────────────────────
//  Public API
// ─────────────────────────────────────────────

void CheckSystem::Initialize()
{
    if (s_state != CheckSystemState::UNINITIALIZED)
        return;

    s_state = CheckSystemState::BOOTSTRAPPING;

    RandomiserSave::Load();
    SaveStateManager::Initialize();
    TimeChunkReward::Initialize();
    
    // Register all check ranges BEFORE building check list
    const RandomiserConfig& cfg = RandomiserConfig::Get();

    //Photosanity
    if (cfg.EffectivePPSticker())
    {
        // PP Stickers
        RegisterCheckRange(CheckType::PPSticker, 128, 227);
    }

    if (cfg.EffectiveSurvivorPhoto())
    {
        // Survivors - register only the ranges you have
        RegisterCheckRange(CheckType::SurvivorPhoto, 0x4D0, 0x4EB);  // 0x4D0-0x4EB (no 0x4EC)
        RegisterCheckRange(CheckType::SurvivorPhoto, 0x4ED, 0x4F1);  // 0x4ED-0x4F1 (skip 0x4EC)
        RegisterCheckRange(CheckType::SurvivorPhoto, 0x510, 0x510);  // 0x510 only (no 0x511)
        RegisterCheckRange(CheckType::SurvivorPhoto, 0x512, 0x512);  // 0x512 only (no 0x513)
        RegisterCheckRange(CheckType::SurvivorPhoto, 0x514, 0x514);  // 0x514 only (no 0x515)
        RegisterCheckRange(CheckType::SurvivorPhoto, 0x516, 0x516);  // 0x516 only
        RegisterCheckRange(CheckType::SurvivorPhoto, 0x51C, 0x520);  // 0x51C-0x520 (skip gaps)
        RegisterCheckRange(CheckType::SurvivorPhoto, 0x522, 0x522);  // 0x522 only (no 0x523)
        RegisterCheckRange(CheckType::SurvivorPhoto, 0x524, 0x527);  // 0x524-0x527 (no 0x528)
        RegisterCheckRange(CheckType::SurvivorPhoto, 0x529, 0x52A);  // 0x529-0x52A
        RegisterCheckRange(CheckType::SurvivorPhoto, 0x550, 0x556);  // 0x550-0x556
        RegisterCheckRange(CheckType::SurvivorPhoto, 0x560, 0x568);  // 0x560-0x568
        RegisterCheckRange(CheckType::SurvivorPhoto, 0x56C, 0x56C);  // 0x56C only
    }

    if (cfg.EffectivePsychopathPhoto())
    {
        // Psychopaths - register only what you have (no 0x6E1)
        RegisterCheckRange(CheckType::PsychopathPhoto, 0x6D4, 0x6DB);  // Kent through Jo
        RegisterCheckRange(CheckType::PsychopathPhoto, 0x6DC, 0x6DF);  // Convicts + Cletus
        RegisterCheckRange(CheckType::PsychopathPhoto, 0x6E3, 0x6E3);  // Carlito
        RegisterCheckRange(CheckType::PsychopathPhoto, 0x6E6, 0x6E9);  // Isabella + Hall Family (no 0x6E1)
    }

    if (cfg.costumeChecks)
    {
        // One check per physical store location — count driven by kCostumeDB row count
        uint32_t count = CheckAvailability::GetCostumeCheckCount();
        if (count > 0)
            RegisterCheckRange(CheckType::Costume, 1, count);
    }

    // Key items are reward-only — no check locations registered.

    // Build the check list from registered ranges
    RebuildCheckList();
    
    // Initialize check availability database
    CheckAvailability::Initialize();
    
    // Load or generate seed
    if (cfg.customSeed != 0)
    {
        // Fixed seed from config — always wins over saved seed
        SetSeed(cfg.customSeed);
        char buf[64];
        sprintf_s(buf, "[CHECKS] Using config seed: %u", cfg.customSeed);
        LogLine(buf);
    }
    else if (!LoadSeedFromFile())
    {
        LogLine("[CHECKS] No saved seed found - generating new beatable seed");
        GenerateBeatableSeed();
    }
    else
    {
        GenerateRewardMap();  // rebuild from loaded seed

        char buf[64];
        sprintf_s(buf, "[CHECKS] Loaded seed: %u", s_seed);
        LogLine(buf);

        if (!IsSeedBeatable())
        {
            LogLine("[CHECKS] Loaded seed is UNBEATABLE! Generating new one...");
            GenerateBeatableSeed();
        }
        else
        {
            LogLine("[CHECKS] Loaded seed is beatable!");
            WriteSeedAnalysisToFile();
        }
    }

    LoadCompletedChecks();

    s_state = CheckSystemState::SEED_READY;
    DrainPendingChecks();   // replay any checks that arrived during BOOTSTRAPPING
    s_state = CheckSystemState::READY;
}

void CheckSystem::SetSeed(uint32_t seed)
{
    CheckSystemState prev = s_state;
    s_state = CheckSystemState::RESEEDING;

    if (seed == 0)
    {
        // Use performance counter so rapid successive calls get different seeds
        LARGE_INTEGER pc;
        QueryPerformanceCounter(&pc);
        seed = (uint32_t)(pc.QuadPart ^ (pc.QuadPart >> 17) ^ (uint64_t)time(nullptr));
        if (seed == 0) seed = 1;
    }
    s_seed     = seed;
    s_rngState = 0;     // reset stale LCG state before GenerateRewardMap seeds it
    s_completed.clear();
    SaveStateManager::ResetCompletedChecks();   // keep disk in sync with cleared memory
    RebuildCheckList();
    GenerateRewardMap();
    SaveSeedToFile();
    WriteSeedAnalysisToFile();
    ResetRewardSlots();

    s_state = (prev == CheckSystemState::READY) ? CheckSystemState::READY : prev;

    char buf[64];
    sprintf_s(buf, "[CHECKS] Seed set: %u", s_seed);
    LogLine(buf);
}

uint32_t          CheckSystem::GetSeed()   { return s_seed; }
bool              CheckSystem::IsReady()  { return s_state == CheckSystemState::READY; }
CheckSystemState  CheckSystem::GetState() { return s_state; }
int      CheckSystem::GetTotalChecks()   { return (int)s_allChecks.size(); }
int      CheckSystem::GetCompletedChecks() { return (int)s_completed.size(); }

void CheckSystem::RegisterCheckRange(CheckType type, uint32_t idMin, uint32_t idMax)
{
    s_checkRanges.push_back({ type, idMin, idMax });
}

Reward CheckSystem::GetRewardForCheck(CheckId check)
{
    auto it = s_rewardMap.find(check);
    if (it == s_rewardMap.end()) return { RewardType::None, 0 };
    return it->second;
}

bool CheckSystem::IsCompleted(CheckType type, uint32_t id)
{
    return s_completed.count(CheckKey({ type, id })) > 0;
}

void CheckSystem::CompleteCheck(CheckType type, uint32_t id)
{
    char buf[128];

    // Debug — log state at time of check
    sprintf_s(buf,
        "[CHECKS] CompleteCheck called. state=%u total=%d completed=%d type=%u id=%u",
        (uint32_t)s_state,
        (int)s_allChecks.size(),
        (int)s_completed.size(),
        (uint32_t)type,
        id);
    LogLine(buf);

    if (s_state != CheckSystemState::READY)
    {
        s_pendingChecks.push_back({ type, id });
        LogLine("[CHECKS] Queued check — system not READY yet");
        return;
    }

    CheckId  check = { type, id };
    uint64_t key   = CheckKey(check);

    if (s_completed.count(key))
    {
        LogLine("[CHECKS] Already completed — skipping.");
        return;
    }
    
    // Mark as completed in memory
    s_completed.insert(key);
    
    // Save to persistent storage
    uint16_t checkVal = CheckIdToUInt16(check);
    SaveStateManager::MarkCheckCompleted(checkVal);

    Reward reward = GetRewardForCheck(check);

    sprintf_s(buf, "[CHECKS] Reward found: type=%u value=%d mapSize=%d",
        (uint32_t)reward.type, reward.value, (int)s_rewardMap.size());
    LogLine(buf);

    for (auto& cb : s_callbacks)
        cb(check, reward);
}

void CheckSystem::OnCheckCompleted(CheckCallback callback)
{
    s_callbacks.push_back(callback);
}