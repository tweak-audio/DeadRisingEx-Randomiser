#include "CheckSystem.h"
#include "CheckAvailability.h"
#include "SurvivorPhotoCheck.h"
#include "DeadRisingEx/MtFramework/Randomiser/InputSystem.h"
#include "DeadRisingEx/MtFramework/Randomiser/Rewards/SetItemRewardSystem.h"
#include "DeadRisingEx/MtFramework/Randomiser/Rewards/TimeChunkReward.h"
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

    std::vector<Reward> pool;
    pool.reserve(total);

    // Add rewards
    for (int i = 0; i < LEVEL_UP_REWARDS; i++)
        pool.push_back({ RewardType::LevelUp, 0 });
    for (int i = 0; i < BATTERY_REFILL_REWARDS; i++)
        pool.push_back({ RewardType::BatteryRefill, 0 });
    
    // Add time chunk rewards (mode-agnostic)
    for (int i = 0; i < TIME_CHUNK_REWARDS; i++)
        pool.push_back({ RewardType::TimeChunk, i + 1 });  // chunks 1-11 or 1-5
    
    // Fill remaining with SetItem
    while ((int)pool.size() < total)
        pool.push_back({ RewardType::SetItem, 0 });
    pool.resize(total);

    // Fisher-Yates shuffle
    RngSeed(s_seed);
    for (int i = total - 1; i > 0; i--)
    {
        int j = RngRange(0, i + 1);
        Reward tmp = pool[i];
        pool[i] = pool[j];
        pool[j] = tmp;
    }

    s_rewardMap.clear();
    for (int i = 0; i < total; i++)
        s_rewardMap[s_allChecks[i]] = pool[i];
}

bool CheckSystem::IsSeedBeatable()
{
    if (!s_ready)
    {
        LogLine("[SEED VALIDATION] ERROR: System not ready");
        return false;
    }
    
    //CheckAvailability::Initialize();
    
    TimeManager::ChunkSize mode = TimeChunkReward::GetChunkMode();
    int totalChunks = TimeChunkReward::GetTotalChunks();
    
    LogLine("[SEED VALIDATION] === CHECKING SEED BEATABILITY (ADDITIVE MODE) ===");
    
    char buf[256];
    sprintf_s(buf, "[SEED VALIDATION] Mode: %s, Total chunks needed: %d", 
              (mode == TimeManager::ChunkSize::SIX_HOURS) ? "6hr" : "12hr",
              totalChunks - 1);  // -1 because chunk 0 is free
    LogLine(buf);
    
    // Start with chunk 0 unlocked (1 chunk)
    int chunksUnlocked = 1;
    uint32_t currentMaxTime = TimeManager::GetChunk(mode, 0).endTick;
    
    // Track which time chunk rewards we've already used
    std::unordered_set<uint64_t> usedChunks;
    
    // Keep unlocking chunks until we can't find any more
    while (chunksUnlocked < totalChunks)
    {
        sprintf_s(buf, "[SEED VALIDATION] Currently have %d chunks unlocked, need %d total...", 
                  chunksUnlocked, totalChunks);
        LogLine(buf);
        
        // Find ANY time chunk reward that's obtainable before current time limit
        bool foundUnlock = false;
        
        for (const auto& check : s_allChecks)
        {
            Reward reward = GetRewardForCheck(check);
            
            // Is this ANY time chunk reward that we haven't used yet?
            if (reward.type == RewardType::TimeChunk)
            {
                uint64_t checkKey = CheckKey(check);
                
                // Skip if we already used this check
                if (usedChunks.count(checkKey))
                    continue;
                
                uint32_t availableAt = CheckAvailability::GetEarliestTime(check);
                
                // Can we get this check before the current time gate?
                if (availableAt <= currentMaxTime)
                {
                    sprintf_s(buf, "[SEED VALIDATION]   Found time chunk %d at check [%u:%u], available at %s",
                              reward.value, (uint32_t)check.type, check.id,
                              TimeManager::TicksToTimeString(availableAt).c_str());
                    LogLine(buf);
                    
                    foundUnlock = true;
                    
                    // Mark this check as used
                    usedChunks.insert(checkKey);
                    chunksUnlocked++;
                    
                    // Expand our time window to the new limit
                    auto nextLimitChunk = TimeManager::GetChunk(mode, chunksUnlocked - 1);
                    currentMaxTime = nextLimitChunk.endTick;
                    
                    sprintf_s(buf, "[SEED VALIDATION]   ✓ UNLOCKABLE! Now have %d chunks. New time limit: %s",
                              chunksUnlocked, TimeManager::TicksToTimeString(currentMaxTime).c_str());
                    LogLine(buf);
                    
                    break;  // Break inner loop, continue while loop to search again
                }
            }
        }
        
        // If we couldn't find ANY unlockable chunk, seed is unbeatable
        if (!foundUnlock)
        {
            sprintf_s(buf, "[SEED VALIDATION] ✗✗✗ UNBEATABLE: Cannot unlock enough chunks (stuck at %d/%d) ✗✗✗", 
                      chunksUnlocked, totalChunks);
            LogLine(buf);
            return false;
        }
    }
    
    LogLine("[SEED VALIDATION] ✓✓✓ SEED IS BEATABLE ✓✓✓");
    return true;
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
    
    int levelUps = 0, batteries = 0, timeChunks = 0, setItems = 0;
    for (const auto& pair : s_rewardMap)
    {
        switch (pair.second.type)
        {
            case RewardType::LevelUp:       levelUps++; break;
            case RewardType::BatteryRefill: batteries++; break;
            case RewardType::TimeChunk:     timeChunks++; break;
            case RewardType::SetItem:       setItems++; break;
            default: break;
        }
    }
    
    fprintf(f, "Level Ups: %d\n", levelUps);
    fprintf(f, "Battery Refills: %d\n", batteries);
    fprintf(f, "Time Chunks: %d\n", timeChunks);
    fprintf(f, "Set Items: %d\n", setItems);
    fprintf(f, "\nTotal Rewards: %d\n", levelUps + batteries + timeChunks + setItems);
    
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
    // PP Stickers (no issues)
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