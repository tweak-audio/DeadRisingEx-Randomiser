#pragma once
#include <cstdint>
#include <string>
#include <vector>

class TimeManager
{
public:
    static void Initialize();
    
    // Read current game time (in ticks, 30 ticks = 1 second)
    static uint32_t GetGameTime();
    
    // Set game time (in ticks)
    static void SetGameTime(uint32_t ticks);
    
    // Convert ticks to readable time string (e.g., "12:00:00 PM")
    static std::string GetTimeString();
    static std::string TicksToTimeString(uint32_t ticks);
    
    // Freeze/unfreeze time
    static void FreezeTime(bool freeze);
    static bool IsTimeFrozen();
    
    // Add/subtract time
    static void AddHours(int hours);
    static void AddMinutes(int minutes);
    static void AddSeconds(int seconds);
    
    // Set specific time
    static void SetTime(int hour, int minute, int second, bool isPM);
    
    // Useful time constants
    static constexpr uint32_t TICKS_PER_SECOND = 30;
    static constexpr uint32_t TICKS_PER_MINUTE = 1800;  // 30 * 60
    static constexpr uint32_t TICKS_PER_HOUR = 108000;  // 30 * 60 * 60
    
    // ═══════════════════════════════════════════
    //  TIME CHUNK SYSTEM
    // ═══════════════════════════════════════════
    
    enum class ChunkSize
    {
        SIX_HOURS,      // 12 chunks total
        TWELVE_HOURS    // 6 chunks total
    };
    
    struct TimeChunk
    {
        int chunkNumber;        // 0-based index
        uint32_t startTick;     // Start time in ticks
        uint32_t endTick;       // End time in ticks
        std::string startTime;  // e.g., "12:00 PM"
        std::string endTime;    // e.g., "06:00 PM"
        std::string label;      // e.g., "Day 1 Noon-6PM"
    };
    
    // Get current chunk based on game time
    static TimeChunk GetCurrentChunk(ChunkSize size);
    
    // Get a specific chunk by number
    static TimeChunk GetChunk(ChunkSize size, int chunkNumber);
    
    // Get all chunks
    static std::vector<TimeChunk> GetAllChunks(ChunkSize size);
    
    // Jump to start of a specific chunk
    static void JumpToChunk(ChunkSize size, int chunkNumber);
    
    // Get which chunk number we're currently in
    static int GetCurrentChunkNumber(ChunkSize size);
    
    // Time constants for the 72-hour game (Sept 19 12PM to Sept 22 12PM)
    static constexpr uint32_t GAME_START_TICK = 3888000;   // Day 1, 12:00 PM (noon)
    static constexpr uint32_t GAME_END_TICK = 11664000;    // Day 4, 12:00 PM (noon) - 72 hours later
    static constexpr uint32_t TOTAL_GAME_TICKS = GAME_END_TICK - GAME_START_TICK; // 7776000 ticks = 72 hours
    
    // Debug
    static void PrintDebugInfo();

private:
    static uint32_t* GetGameTimePointer();
    
    // Use the SAME base as camera battery - we know this works!
    static constexpr uintptr_t GAME_MANAGER_OFFSET = 0x1CF2AA0;  // Same as camera
    static constexpr uintptr_t GAME_STATE_PTR_OFFSET = 0x20DC0;  // Same as camera  
    static constexpr uintptr_t GAME_TIME_VALUE_OFFSET = 0x198;   // Time offset in player object
    
    static bool s_initialized;
    static bool s_timeFrozen;
    static uint32_t s_frozenTime;
};