
#include "DeadRisingEx/MtFramework/Randomiser/InputSystem.h"
#include "DeadRisingEx/MtFramework/Player/uPlayerImpl.h"
#include "DeadRisingEx/MtFramework/Randomiser/Checks/ChecksManager.h"
#include "TimeChunkReward.h"
#include "DeadRisingEx/Utilities/DebugLog.h"
#include <stdio.h>

bool TimeChunkReward::s_initialized = false;
TimeManager::ChunkSize TimeChunkReward::s_chunkMode = TimeManager::ChunkSize::SIX_HOURS;  // Default to 6-hour chunks
bool TimeChunkReward::s_chunksUnlocked[12] = {};
bool TimeChunkReward::s_timeGatingEnabled = true;
bool TimeChunkReward::s_hasWarnedPlayer = false;
static bool s_hasKilledForTimeGate = false; 

static const char* TIME_CHUNKS_SAVE_FILE = "DeadRisingEx_TimeChunks.dat";

void TimeChunkReward::Initialize()
{
    if (s_initialized)
        return;
    
    LogLine("[TIME GATE] Initializing time chunk reward system");
    
    // Set mode based on CheckSystem constant BEFORE loading save
    s_chunkMode = USE_6_HOUR_CHUNKS ? TimeManager::ChunkSize::SIX_HOURS 
                                    : TimeManager::ChunkSize::TWELVE_HOURS;
    
    // Always unlock the first chunk so game can start
    s_chunksUnlocked[0] = true;
    
    // Load unlocked chunks (but mode is already set above)
    LoadUnlockedChunksWithoutMode();  // New function that doesn't load mode
    
    s_initialized = true;
    s_timeGatingEnabled = true;
    
    char buf[128];
    sprintf_s(buf, "[TIME GATE] System initialized - Mode: %s chunks, Time gating %s", 
              (s_chunkMode == TimeManager::ChunkSize::SIX_HOURS) ? "6-hour" : "12-hour",
              s_timeGatingEnabled ? "ENABLED" : "DISABLED");
    LogLine(buf);
}

void TimeChunkReward::LoadUnlockedChunksWithoutMode()
{
    FILE* f = nullptr;
    fopen_s(&f, TIME_CHUNKS_SAVE_FILE, "rb");
    if (!f) return;
    
    // Skip the saved mode (read but ignore it)
    int savedMode = 6;
    fread(&savedMode, sizeof(int), 1, f);
    // DON'T apply it - we already set s_chunkMode from USE_6_HOUR_CHUNKS
    
    // Load unlocked chunks
    fread(s_chunksUnlocked, sizeof(bool), 12, f);
    
    // Load gating setting
    fread(&s_timeGatingEnabled, sizeof(bool), 1, f);
    
    fclose(f);
    
    LogLine("[TIME GATE] Loaded unlocked chunks from save (mode from code)");
}

void TimeChunkReward::SetChunkMode(TimeManager::ChunkSize mode)
{
    if (s_initialized)
    {
        LogLine("[TIME GATE] WARNING: Cannot change chunk mode after initialization. Call ResetAllChunks() first.");
        return;
    }
    
    s_chunkMode = mode;
    
    char buf[128];
    sprintf_s(buf, "[TIME GATE] Chunk mode set to: %s", 
              (mode == TimeManager::ChunkSize::SIX_HOURS) ? "6-hour (12 chunks)" : "12-hour (6 chunks)");
    LogLine(buf);
}

TimeManager::ChunkSize TimeChunkReward::GetChunkMode()
{
    return s_chunkMode;
}

int TimeChunkReward::GetTotalChunks()
{
    return (s_chunkMode == TimeManager::ChunkSize::SIX_HOURS) ? 12 : 6;
}

int TimeChunkReward::GetUnlockedChunkCount()
{
    int count = 0;
    int maxChunks = GetTotalChunks();
    for (int i = 0; i < maxChunks; i++)
    {
        if (s_chunksUnlocked[i])
            count++;
    }
    return count;
}

void TimeChunkReward::GrantTimeChunk(int chunkNumber)
{
    int maxChunks = GetTotalChunks();
    
    if (chunkNumber < 0 || chunkNumber >= maxChunks)
    {
        char buf[128];
        sprintf_s(buf, "[TIME GATE] ERROR: Invalid chunk number: %d (max: %d)", chunkNumber, maxChunks - 1);
        LogLine(buf);
        return;
    }
    
    if (s_chunksUnlocked[chunkNumber])
    {
        char buf[128];
        sprintf_s(buf, "[TIME GATE] Chunk %d already unlocked", chunkNumber);
        LogLine(buf);
        return;
    }
    
    s_chunksUnlocked[chunkNumber] = true;
    
    auto chunk = TimeManager::GetChunk(s_chunkMode, chunkNumber);
    
    // Calculate new time limit based on total unlocked count
    int totalUnlocked = GetUnlockedChunkCount();
    auto newLimitChunk = TimeManager::GetChunk(s_chunkMode, totalUnlocked - 1);
    
    char buf[256];
    sprintf_s(buf, "[TIME GATE] ★ Unlocked chunk %d: %s (%s to %s)", 
              chunkNumber, chunk.label.c_str(), 
              chunk.startTime.c_str(), chunk.endTime.c_str());
    LogLine(buf);
    
    sprintf_s(buf, "[TIME GATE] Total unlocked: %d/%d | New time limit: %s", 
              totalUnlocked, maxChunks, newLimitChunk.endTime.c_str());
    LogLine(buf);
    
    SaveUnlockedChunks();
    s_hasWarnedPlayer = false;  // Reset warning flag when new chunk unlocked
    s_hasKilledForTimeGate = false;  // Reset kill flag so they can continue
}

bool TimeChunkReward::IsChunkUnlocked(int chunkNumber)
{
    int maxChunks = GetTotalChunks();
    if (chunkNumber < 0 || chunkNumber >= maxChunks) 
        return false;
    return s_chunksUnlocked[chunkNumber];
}

int TimeChunkReward::GetFurthestUnlockedChunk()
{
    int maxChunks = GetTotalChunks();
    for (int i = maxChunks - 1; i >= 0; i--)
    {
        if (s_chunksUnlocked[i])
            return i;
    }
    return 0;
}

uint32_t TimeChunkReward::GetMaxAllowedTime()
{
    // Count how many chunks are unlocked
    int unlockedCount = GetUnlockedChunkCount();
    
    // Each unlocked chunk gives you access to that chunk's time period
    // Time limit = end of the Nth chunk (where N = number unlocked)
    if (unlockedCount == 0)
        unlockedCount = 1;  // Always have at least chunk 0
    
    auto chunk = TimeManager::GetChunk(s_chunkMode, unlockedCount - 1);
    
    // Return the END tick of that chunk
    return chunk.endTick;
}

void TimeChunkReward::EnforceTimeGate()
{
    if (!s_initialized || !s_timeGatingEnabled)
        return;
    
    uint32_t currentTime = TimeManager::GetGameTime();
    if (currentTime == 0)  // Game not loaded yet
        return;
    
    uint32_t maxAllowedTime = GetMaxAllowedTime();
    
    // If time has passed beyond the allowed limit, kill player
    if (currentTime > maxAllowedTime)
    {
        // Only kill once per time gate violation
        if (!s_hasKilledForTimeGate)
        {
            int unlockedCount = GetUnlockedChunkCount();
            auto limitChunk = TimeManager::GetChunk(s_chunkMode, unlockedCount - 1);
            
            char buf[256];
            sprintf_s(buf, "[TIME GATE] ⚠ TIME LIMIT EXCEEDED! You have %d/%d chunks unlocked", 
                      unlockedCount, GetTotalChunks());
            LogLine(buf);
            sprintf_s(buf, "[TIME GATE] Current limit: %s (end of chunk %d)", 
                      limitChunk.endTime.c_str(), unlockedCount - 1);
            LogLine(buf);
            LogLine("[TIME GATE] Unlock more time chunks to continue! Game Over.");
            
            uPlayerImpl::KillPlayer();
            
            s_hasKilledForTimeGate = true;  // Prevent killing every frame
        }
    }
    else
    {
        // Time is within allowed range - reset the flag so we can kill again if they exceed it later
        s_hasKilledForTimeGate = false;
    }
}

void TimeChunkReward::AdvanceToFurthestUnlocked()
{
    int unlockedCount = GetUnlockedChunkCount();
    if (unlockedCount == 0)
        unlockedCount = 1;
    
    TimeManager::JumpToChunk(s_chunkMode, unlockedCount - 1);
    
    auto chunk = TimeManager::GetChunk(s_chunkMode, unlockedCount - 1);
    char buf[128];
    sprintf_s(buf, "[TIME GATE] Advanced to current time limit (chunk %d): %s", 
              unlockedCount - 1, chunk.label.c_str());
    LogLine(buf);
}

std::string TimeChunkReward::GetTimeStatus()
{
    int unlockedCount = GetUnlockedChunkCount();
    int total = GetTotalChunks();
    auto limitChunk = TimeManager::GetChunk(s_chunkMode, unlockedCount - 1);
    
    char buf[256];
    sprintf_s(buf, "Mode: %s | Unlocked: %d/%d | Time limit: %s | Gating: %s",
              (s_chunkMode == TimeManager::ChunkSize::SIX_HOURS) ? "6hr" : "12hr",
              unlockedCount, total,
              limitChunk.endTime.c_str(),
              s_timeGatingEnabled ? "ON" : "OFF");
    
    return std::string(buf);
}

void TimeChunkReward::SetTimeGatingEnabled(bool enabled)
{
    s_timeGatingEnabled = enabled;
    
    char buf[128];
    sprintf_s(buf, "[TIME GATE] Time gating %s", enabled ? "ENABLED" : "DISABLED");
    LogLine(buf);
    
    if (!enabled)
    {
        LogLine("[TIME GATE] Time is now free-flowing (debug mode)");
    }
}

bool TimeChunkReward::IsTimeGatingEnabled()
{
    return s_timeGatingEnabled;
}

void TimeChunkReward::UnlockAllChunks()
{
    int maxChunks = GetTotalChunks();
    for (int i = 0; i < maxChunks; i++)
        s_chunksUnlocked[i] = true;
    
    SaveUnlockedChunks();
    s_hasWarnedPlayer = false;
    LogLine("[TIME GATE] All chunks unlocked!");
}

void TimeChunkReward::ResetAllChunks()
{
    for (int i = 0; i < 12; i++)
        s_chunksUnlocked[i] = false;
    
    // Always keep first chunk unlocked
    s_chunksUnlocked[0] = true;
    
    SaveUnlockedChunks();
    s_hasWarnedPlayer = false;
    s_initialized = false;  // Allow mode change after reset
    
    LogLine("[TIME GATE] All chunks reset (except chunk 0). You can now change chunk mode.");
}

void TimeChunkReward::SaveUnlockedChunks()
{
    FILE* f = nullptr;
    fopen_s(&f, TIME_CHUNKS_SAVE_FILE, "wb");
    if (!f) return;
    
    // Save the mode
    int modeValue = (s_chunkMode == TimeManager::ChunkSize::SIX_HOURS) ? 6 : 12;
    fwrite(&modeValue, sizeof(int), 1, f);
    
    // Save unlocked chunks
    fwrite(s_chunksUnlocked, sizeof(bool), 12, f);
    
    // Save gating setting
    fwrite(&s_timeGatingEnabled, sizeof(bool), 1, f);
    
    fclose(f);
}