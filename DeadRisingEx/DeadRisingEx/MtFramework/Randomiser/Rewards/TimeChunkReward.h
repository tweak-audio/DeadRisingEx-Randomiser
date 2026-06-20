#pragma once
#include "DeadRisingEx/MtFramework/Randomiser/Checks/CheckSystem.h"
#include "DeadRisingEx/MtFramework/Randomiser/TimeManager.h"

class TimeChunkReward
{
public:
    static void Initialize();
    
    // Set which chunk mode to use (must be called before Initialize or after Reset)
    static void SetChunkMode(TimeManager::ChunkSize mode);

    static TimeManager::ChunkSize GetChunkMode();
    
    // Called when a time chunk reward is received
    static void GrantTimeChunk(int chunkNumber);
    
    // Check if a specific chunk has been unlocked
    static bool IsChunkUnlocked(int chunkNumber);
    
    // Get the furthest unlocked chunk
    static int GetFurthestUnlockedChunk();
    
    // Auto-advance time to the furthest unlocked chunk
    static void AdvanceToFurthestUnlocked();
    
    // Get info about current time restrictions
    static std::string GetTimeStatus();
    
    // Check if current time is beyond unlocked chunks (called every frame)
    static void EnforceTimeGate();
    
    // Debug: unlock all chunks
    static void UnlockAllChunks();
    
    // Debug: reset all chunks
    static void ResetAllChunks();
    
    // Enable/disable time gating (for testing)
    static void SetTimeGatingEnabled(bool enabled);
    static bool IsTimeGatingEnabled();
    
    // Get total number of chunks for current mode (for reward pool calculation)
    static int GetTotalChunks();
    static int GetUnlockedChunkCount();

    // Get hours added per chunk for current mode (6 or 12)
    static int GetHoursPerChunk();

private:
    static TimeManager::ChunkSize s_chunkMode;
    static bool s_chunksUnlocked[12];  // Max 12 chunks (for 6-hour mode)
    static bool s_initialized;
    static bool s_timeGatingEnabled;
    static bool s_hasWarnedPlayer;
    
    static void SaveUnlockedChunks();
    static void LoadUnlockedChunksWithoutMode();
    static uint32_t GetMaxAllowedTime();
};