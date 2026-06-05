#include "CheckSystem.h"
#include "CheckAvailability.h"
#include "DeadRisingEx/MtFramework/Randomiser/AreaKeySystem.h"
#include "SurvivorPhotoCheck.h"
#include "DeadRisingEx/MtFramework/Randomiser/InputSystem.h"
#include "DeadRisingEx/MtFramework/Randomiser/Rewards/SetItemRewardSystem.h"
#include "DeadRisingEx/MtFramework/Randomiser/Rewards/TimeChunkReward.h"
#include "DeadRisingEx/MtFramework/Randomiser/Rewards/ClothingRewardSystem.h"
#include "ChecksManager.h"

#include "DeadRisingEx/Utilities/DebugLog.h"
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>

static const char* SEED_FILE_PATH = "DeadRisingEx_Randomiser_Seed.dat";

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
static bool     s_ready    = false;

static int s_setItemCount = 0;

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

void CheckSystem::GenerateRewardMap()
{
    int total = (int)s_allChecks.size();
    if (total == 0) return;

    RngSeed(s_seed);

    // Fixed rewards
    std::vector<Reward> levelUps, batteries, timeChunks, areaKeys;
    for (int i = 0; i < LEVEL_UP_REWARDS; i++)
        levelUps.push_back({ RewardType::LevelUp, 0 });
    for (int i = 0; i < BATTERY_REFILL_REWARDS; i++)
        batteries.push_back({ RewardType::BatteryRefill, 0 });
    for (int i = 0; i < TIME_CHUNK_REWARDS; i++)
        timeChunks.push_back({ RewardType::TimeChunk, i + 1 });
    for (int i = 0; i < AREA_KEY_REWARDS; i++)
        areaKeys.push_back({ RewardType::AreaKey, i });  // value = ZoneID index

    // Randomly split remaining slots between clothing and set items
    int remaining = total - LEVEL_UP_REWARDS - BATTERY_REFILL_REWARDS - TIME_CHUNK_REWARDS - AREA_KEY_REWARDS;
    int maxClothing = min(remaining, COSTUME_POOL_SIZE);  // can't exceed pool size
    int clothingCount = RngRange(CLOTHING_REWARDS_MIN, maxClothing + 1);
    int setItemCount  = remaining - clothingCount;
    s_setItemCount = setItemCount; 

    char buf[128];
    sprintf_s(buf, "[CHECKS] Reward split: %d area keys, %d clothing, %d set items (seed %u)",
              AREA_KEY_REWARDS, clothingCount, setItemCount, s_seed);
    LogLine(buf);

    std::vector<Reward> clothing, setItems;
    for (int i = 0; i < clothingCount; i++)
        clothing.push_back({ RewardType::Clothing, 0 });
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

    // Place time chunks first (most important for beatability), then area keys
    PlaceEvenly(timeChunks);
    PlaceEvenly(areaKeys);
    PlaceEvenly(levelUps);
    PlaceEvenly(batteries);
    PlaceEvenly(clothing);
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

bool CheckSystem::IsSeedBeatable()
{
    if (!s_ready)
    {
        LogLine("[SEED VALIDATION] ERROR: System not ready");
        return false;
    }

    TimeManager::ChunkSize mode = TimeChunkReward::GetChunkMode();
    int totalChunks = TimeChunkReward::GetTotalChunks();

    LogLine("[SEED VALIDATION] === CHECKING SEED BEATABILITY ===");

    char buf[256];
    sprintf_s(buf, "[SEED VALIDATION] Mode: %s, Total chunks needed: %d",
              (mode == TimeManager::ChunkSize::SIX_HOURS) ? "6hr" : "12hr",
              totalChunks - 1);
    LogLine(buf);

    // Starting state: chunk 0 unlocked, Frank starts in Paradise Plaza
    int chunksUnlocked = 1;
    uint32_t currentMaxTime = TimeManager::GetChunk(mode, 0).endTick;

    bool zonesUnlocked[static_cast<int>(ZoneID::COUNT)] = {};
    zonesUnlocked[static_cast<int>(ZoneID::ParadisePlaza)] = true;

    std::unordered_set<uint64_t> usedChecks;

    // Each pass collects all currently reachable time chunks and area keys.
    // Repeat until no new progress can be made.
    bool anyProgress = true;
    while (anyProgress)
    {
        anyProgress = false;

        for (const auto& check : s_allChecks)
        {
            uint64_t ckey = CheckKey(check);
            if (usedChecks.count(ckey)) continue;

            uint32_t availableAt = CheckAvailability::GetEarliestTime(check);
            if (availableAt > currentMaxTime) continue;

            ZoneID zone = CheckAvailability::GetRequiredZone(check);
            if (zone != ZoneID::COUNT && !zonesUnlocked[static_cast<int>(zone)]) continue;

            Reward reward = GetRewardForCheck(check);

            if (reward.type == RewardType::TimeChunk)
            {
                usedChecks.insert(ckey);
                chunksUnlocked++;
                currentMaxTime = TimeManager::GetChunk(mode, chunksUnlocked - 1).endTick;
                anyProgress = true;

                sprintf_s(buf, "[SEED VALIDATION]   +TimeChunk -> %d/%d unlocked, limit now %s",
                          chunksUnlocked, totalChunks,
                          TimeManager::TicksToTimeString(currentMaxTime).c_str());
                LogLine(buf);
            }
            else if (reward.type == RewardType::AreaKey)
            {
                ZoneID grantedZone = static_cast<ZoneID>(reward.value);
                if (grantedZone < ZoneID::COUNT && !zonesUnlocked[static_cast<int>(grantedZone)])
                {
                    usedChecks.insert(ckey);
                    zonesUnlocked[static_cast<int>(grantedZone)] = true;
                    anyProgress = true;

                    sprintf_s(buf, "[SEED VALIDATION]   +AreaKey -> unlocked %s",
                              AreaKeySystem::GetZoneName(grantedZone));
                    LogLine(buf);
                }
            }
        }
    }

    if (chunksUnlocked >= totalChunks)
    {
        LogLine("[SEED VALIDATION] SEED IS BEATABLE");
        return true;
    }

    sprintf_s(buf, "[SEED VALIDATION] UNBEATABLE: stuck at %d/%d chunks", chunksUnlocked, totalChunks);
    LogLine(buf);
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
        
        if (IsSeedBeatable())
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
    fprintf(f, "  PROGRESSION LOGIC (ADDITIVE TIME CHUNKS)\n");
    fprintf(f, "═══════════════════════════════════════════════════════════\n\n");
    fprintf(f, "Note: Any time chunk reward unlocks the next time slot.\n");
    fprintf(f, "You need %d time chunks total to reach the end.\n\n", totalChunks - 1);
    
    // Track progression through time chunks with ADDITIVE logic
    uint32_t currentMaxTime = TimeManager::GetChunk(mode, 0).endTick;
    std::unordered_set<uint64_t> usedTimeChunks;
    int chunksUnlocked = 1;  // Start with chunk 0
    
    for (int chunkNum = 0; chunkNum < totalChunks; chunkNum++)
    {
        auto chunk = TimeManager::GetChunk(mode, chunkNum);
        
        fprintf(f, "─────────────────────────────────────────────────────────\n");
        fprintf(f, "CHUNK %d: %s\n", chunkNum, chunk.label.c_str());
        fprintf(f, "Time Period: %s - %s\n", 
                chunk.startTime.c_str(), chunk.endTime.c_str());
        fprintf(f, "─────────────────────────────────────────────────────────\n\n");
        
        if (chunkNum == 0)
        {
            fprintf(f, "This chunk is unlocked at game start.\n\n");
        }
        else
        {
            fprintf(f, "UNLOCK REQUIREMENT: Obtain ANY time chunk reward (need %d total chunks)\n\n", chunksUnlocked);
            
            // Find ANY available time chunk reward
            bool found = false;
            CheckId foundCheck = {CheckType::PPSticker, 0};
            Reward foundReward = {RewardType::None, 0};
            uint32_t foundAvailableAt = 0;
            
            for (const auto& check : s_allChecks)
            {
                Reward reward = GetRewardForCheck(check);
                
                // Is this ANY time chunk reward we haven't used yet?
                if (reward.type == RewardType::TimeChunk)
                {
                    uint64_t checkKey = ((uint64_t)check.type << 32) | check.id;
                    
                    // Skip if already used
                    if (usedTimeChunks.count(checkKey))
                        continue;
                    
                    uint32_t availableAt = CheckAvailability::GetEarliestTime(check);
                    
                    // Can we get this before current time limit?
                    if (availableAt <= currentMaxTime)
                    {
                        foundCheck = check;
                        foundReward = reward;
                        foundAvailableAt = availableAt;
                        found = true;
                        
                        // Mark as used for next iteration
                        usedTimeChunks.insert(checkKey);
                        chunksUnlocked++;
                        
                        // Update time limit
                        auto nextChunk = TimeManager::GetChunk(mode, chunksUnlocked - 1);
                        currentMaxTime = nextChunk.endTick;
                        
                        break;
                    }
                }
            }
            
            if (found)
            {
                std::string checkName = CheckAvailability::GetCheckName(foundCheck);
                
                fprintf(f, "  Reward Location: %s (Time Chunk %d)\n", 
                        checkName.c_str(), foundReward.value);
                fprintf(f, "  Available at: %s\n", 
                        TimeManager::TicksToTimeString(foundAvailableAt).c_str());
                fprintf(f, "  Previous time limit: %s\n",
                        TimeManager::TicksToTimeString(chunk.startTick).c_str());
                fprintf(f, "  New time limit: %s\n",
                        TimeManager::TicksToTimeString(currentMaxTime).c_str());
                fprintf(f, "  Status: ✓ OBTAINABLE\n\n");
            }
            else
            {
                fprintf(f, "  ERROR: No available time chunk reward found!\n");
                fprintf(f, "  Current time limit: %s\n",
                        TimeManager::TicksToTimeString(currentMaxTime).c_str());
                fprintf(f, "  Status: ✗ LOCKED (SEED INVALID)\n\n");
            }
        }
        
        // List available checks in this time period
        fprintf(f, "Checks Available During This Period:\n");
        
        int availableCount = 0;
        for (const auto& check : s_allChecks)
        {
            uint32_t availableAt = CheckAvailability::GetEarliestTime(check);
            
            // Is this check available during this chunk?
            if (availableAt >= chunk.startTick && availableAt < chunk.endTick)
            {
                Reward reward = GetRewardForCheck(check);
                
                // Get the readable name
                std::string checkName = CheckAvailability::GetCheckName(check);
                
                const char* rewardTypeName = "";
                switch (reward.type)
                {
                    case RewardType::LevelUp:       rewardTypeName = "Level Up"; break;
                    case RewardType::BatteryRefill: rewardTypeName = "Battery Refill"; break;
                    case RewardType::TimeChunk:
                        rewardTypeName = "Time Chunk";
                        fprintf(f, "  - %s → %s %d\n",
                                checkName.c_str(), rewardTypeName, reward.value);
                        availableCount++;
                        continue;
                    case RewardType::AreaKey:
                        rewardTypeName = "Area Key";
                        fprintf(f, "  - %s → %s (zone %d)\n",
                                checkName.c_str(), rewardTypeName, reward.value);
                        availableCount++;
                        continue;
                    case RewardType::SetItem:       rewardTypeName = "Set Item"; break;
                    default:                        rewardTypeName = "None"; break;
                }
                
                fprintf(f, "  - %s → %s\n", checkName.c_str(), rewardTypeName);
                availableCount++;
            }
        }
        
        if (availableCount == 0)
        {
            fprintf(f, "  (None)\n");
        }
        
        fprintf(f, "\nTotal: %d checks available\n\n", availableCount);
    }
    
    // Summary statistics
    fprintf(f, "═══════════════════════════════════════════════════════════\n");
    fprintf(f, "  REWARD DISTRIBUTION SUMMARY\n");
    fprintf(f, "═══════════════════════════════════════════════════════════\n\n");
    
    int levelUps = 0, batteries = 0, timeChunks = 0, setItems = 0, areaKeys = 0;
    for (const auto& pair : s_rewardMap)
    {
        switch (pair.second.type)
        {
            case RewardType::LevelUp:       levelUps++; break;
            case RewardType::BatteryRefill: batteries++; break;
            case RewardType::TimeChunk:     timeChunks++; break;
            case RewardType::SetItem:       setItems++; break;
            case RewardType::AreaKey:       areaKeys++; break;
            default: break;
        }
    }

    fprintf(f, "Level Ups: %d\n", levelUps);
    fprintf(f, "Battery Refills: %d\n", batteries);
    fprintf(f, "Time Chunks: %d\n", timeChunks);
    fprintf(f, "Area Keys: %d\n", areaKeys);
    fprintf(f, "Set Items: %d\n", setItems);
    fprintf(f, "\nTotal Rewards: %d\n", levelUps + batteries + timeChunks + areaKeys + setItems);
    
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
    FILE* f = nullptr;
    fopen_s(&f, SEED_FILE_PATH, "wb");
    if (f) { fwrite(&s_seed, sizeof(uint32_t), 1, f); fclose(f); }
}

bool CheckSystem::LoadSeedFromFile()
{
    FILE* f = nullptr;
    fopen_s(&f, SEED_FILE_PATH, "rb");
    if (!f) return false;
    uint32_t loaded = 0;
    bool ok = fread(&loaded, sizeof(uint32_t), 1, f) == 1;
    fclose(f);
    if (ok && loaded != 0) { s_seed = loaded; return true; }
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

static bool s_initialized = false;

void CheckSystem::Initialize()
{
    if (s_initialized)
        return;

    s_initialized = true;

    SaveStateManager::Initialize();
    
    // Set chunk mode BEFORE initializing TimeChunkReward
    #if USE_6_HOUR_CHUNKS
        TimeChunkReward::SetChunkMode(TimeManager::ChunkSize::SIX_HOURS);
    #else
        TimeChunkReward::SetChunkMode(TimeManager::ChunkSize::TWELVE_HOURS);
    #endif
    
    TimeChunkReward::Initialize();
    
    // Register all check ranges BEFORE building check list

    //Photosanity
        // PP Stickers 
        RegisterCheckRange(CheckType::PPSticker, 128, 227);

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

        // Psychopaths - register only what you have (no 0x6E1)
        RegisterCheckRange(CheckType::PsychopathPhoto, 0x6D4, 0x6DB);  // Kent through Jo
        RegisterCheckRange(CheckType::PsychopathPhoto, 0x6DC, 0x6DF);  // Convicts + Cletus
        RegisterCheckRange(CheckType::PsychopathPhoto, 0x6E3, 0x6E3);  // Carlito
        RegisterCheckRange(CheckType::PsychopathPhoto, 0x6E6, 0x6E9);  // Isabella + Hall Family (no 0x6E1)

    //Pickups
        // Clothing pickups — 66 world checks (achievement unlocks excluded)
        RegisterCheckRange(CheckType::Clothing,  0,  26);   // slot 0: cos000-cos026
        RegisterCheckRange(CheckType::Clothing, 28,  33);   // slot 0: cos028-cos033
        RegisterCheckRange(CheckType::Clothing, 38,  38);   // slot 0: cos038
        RegisterCheckRange(CheckType::Clothing, 40,  42);   // slot 0: cos040-cos042
        RegisterCheckRange(CheckType::Clothing, 100, 109);  // slot 1: cos100-cos109
        RegisterCheckRange(CheckType::Clothing, 202, 205);  // slot 2: cos202-cos205
        RegisterCheckRange(CheckType::Clothing, 207, 211);  // slot 2: cos207-cos211
        RegisterCheckRange(CheckType::Clothing, 214, 217);  // slot 2: cos214-cos217
        RegisterCheckRange(CheckType::Clothing, 219, 219);  // slot 2: cos219
        RegisterCheckRange(CheckType::Clothing, 301, 302);  // slot 3: cos301-cos302
        RegisterCheckRange(CheckType::Clothing, 304, 304);  // slot 3: cos304
        RegisterCheckRange(CheckType::Clothing, 306, 306);  // slot 3: cos306
        RegisterCheckRange(CheckType::Clothing, 310, 310);  // slot 3: cos310
        
    // Build the check list from registered ranges
    RebuildCheckList();
    
    // Initialize check availability database
    CheckAvailability::Initialize();
    
    // Load or generate seed
    if (!LoadSeedFromFile())
    {
        // No saved seed - generate a beatable one
        LogLine("[CHECKS] No saved seed found - generating new beatable seed");
        GenerateBeatableSeed();
    }
    else
    {
        // Seed loaded - MUST regenerate reward map for the loaded seed
        GenerateRewardMap();  // This uses s_seed that was loaded
        
        char buf[64];
        sprintf_s(buf, "[CHECKS] Loaded seed: %u", s_seed);
        LogLine(buf);
        
        // Validate the loaded seed
        if (!IsSeedBeatable())
        {
            LogLine("[CHECKS] ✗ Loaded seed is UNBEATABLE! Generating new one...");
            GenerateBeatableSeed();
        }
        else
        {
            LogLine("[CHECKS] ✓ Loaded seed is beatable!");
            WriteSeedAnalysisToFile();  // Write analysis for the loaded seed
        }
    }
    
    LoadCompletedChecks();
    
    s_ready = true;
}

void CheckSystem::SetSeed(uint32_t seed)
{
    if (seed == 0)
    {
        srand((unsigned int)time(nullptr));
        seed = (uint32_t)rand();
    }
    s_seed = seed;
    s_completed.clear();
    RebuildCheckList();
    GenerateRewardMap();
    SaveSeedToFile();
    s_ready = true;

    ResetRewardSlots();

    char buf[64];
    sprintf_s(buf, "[CHECKS] Seed set: %u", s_seed);
    LogLine(buf);
}

uint32_t CheckSystem::GetSeed()          { return s_seed; }
bool     CheckSystem::IsReady()          { return s_ready; }
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

void CheckSystem::CompleteCheck(CheckType type, uint32_t id)
{
    char buf[128];
    
    // Debug — log state at time of check
    sprintf_s(buf, "[CHECKS] CompleteCheck called. ready=%d total=%d completed=%d type=%u id=%u",
        s_ready, (int)s_allChecks.size(), (int)s_completed.size(),
        (uint32_t)type, id);
    LogLine(buf);

    if (!s_ready) return;

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