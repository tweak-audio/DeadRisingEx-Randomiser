#include "TimeManager.h"
#include "InputSystem.h"
#include "DeadRisingEx/Utilities/DebugLog.h"
#include <Windows.h>
#include <cstdio>

bool TimeManager::s_initialized = false;
bool TimeManager::s_timeFrozen = false;
uint32_t TimeManager::s_frozenTime = 0;

void TimeManager::Initialize()
{
    LogLine("[TimeManager] System initialized");
    LogLine("[TimeManager] Using same base as camera battery system");
    LogLine("[TimeManager] Game spans 72 hours: 12:00 PM Day 1 to 12:00 PM Day 4");
    s_initialized = true;
}

uint32_t* TimeManager::GetGameTimePointer()
{
    uintptr_t moduleBase = (uintptr_t)GetModuleHandleA(nullptr);
    if (!moduleBase)
    {
        LogLine("[TimeManager] ERROR: Could not get module base");
        return nullptr;
    }

    // Use the SAME pointer chain as the working camera battery code
    // [[[DeadRising.exe + 0x1CF2AA0] + 0x20DC0] + 0x198]
    
    uintptr_t* gameManager = (uintptr_t*)(moduleBase + GAME_MANAGER_OFFSET);
    if (!gameManager)
    {
        LogLine("[TimeManager] ERROR: gameManager address invalid");
        return nullptr;
    }
    
    if (!*gameManager)
    {
        // This is normal during loading - game manager not initialized yet
        return nullptr;
    }

    uintptr_t* playerPtr = (uintptr_t*)(*gameManager + GAME_STATE_PTR_OFFSET);
    if (!playerPtr)
    {
        LogLine("[TimeManager] ERROR: playerPtr address invalid");
        return nullptr;
    }
    
    if (!*playerPtr)
    {
        // This is normal before player spawns
        return nullptr;
    }

    // Time is at offset 0x198 in the player object
    uint32_t* timePtr = (uint32_t*)(*playerPtr + GAME_TIME_VALUE_OFFSET);
    
    return timePtr;
}

uint32_t TimeManager::GetGameTime()
{
    if (s_timeFrozen)
        return s_frozenTime;
    
    uint32_t* timePtr = GetGameTimePointer();
    if (!timePtr)
    {
        return 0;  // Don't spam logs, just return 0
    }
    
    return *timePtr;
}

void TimeManager::SetGameTime(uint32_t ticks)
{
    uint32_t* timePtr = GetGameTimePointer();
    if (!timePtr)
    {
        LogLine("[TimeManager] ERROR: Could not set game time - pointer invalid");
        return;
    }
    
    uint32_t oldTime = *timePtr;
    *timePtr = ticks;
    
    char buf[256];
    sprintf_s(buf, "[TimeManager] Time changed: %s -> %s", 
              TicksToTimeString(oldTime).c_str(), 
              TicksToTimeString(ticks).c_str());
    LogLine(buf);
}

std::string TimeManager::TicksToTimeString(uint32_t ticks)
{
    // Convert ticks to time (same logic as the C# code)
    uint32_t hours = (ticks / TICKS_PER_HOUR) % 24;
    uint32_t minutes = (ticks / TICKS_PER_MINUTE) % 60;
    uint32_t seconds = (ticks / TICKS_PER_SECOND) % 60;
    
    const char* ampm = "AM";
    if (hours >= 12)
    {
        ampm = "PM";
        hours %= 12;
    }
    if (hours == 0)
    {
        hours = 12;
    }
    
    char buf[32];
    sprintf_s(buf, "%02u:%02u:%02u %s", hours, minutes, seconds, ampm);
    return std::string(buf);
}

std::string TimeManager::GetTimeString()
{
    return TicksToTimeString(GetGameTime());
}

void TimeManager::FreezeTime(bool freeze)
{
    if (freeze && !s_timeFrozen)
    {
        s_frozenTime = GetGameTime();
        s_timeFrozen = true;
        
        char buf[128];
        sprintf_s(buf, "[TimeManager] Time frozen at %s", TicksToTimeString(s_frozenTime).c_str());
        LogLine(buf);
    }
    else if (!freeze && s_timeFrozen)
    {
        s_timeFrozen = false;
        LogLine("[TimeManager] Time unfrozen");
    }
}

bool TimeManager::IsTimeFrozen()
{
    return s_timeFrozen;
}

void TimeManager::AddHours(int hours)
{
    uint32_t currentTime = GetGameTime();
    if (currentTime == 0)
    {
        LogLine("[TimeManager] ERROR: Cannot add time - game time not available");
        return;
    }
    SetGameTime(currentTime + (hours * TICKS_PER_HOUR));
}

void TimeManager::AddMinutes(int minutes)
{
    uint32_t currentTime = GetGameTime();
    if (currentTime == 0)
    {
        LogLine("[TimeManager] ERROR: Cannot add time - game time not available");
        return;
    }
    SetGameTime(currentTime + (minutes * TICKS_PER_MINUTE));
}

void TimeManager::AddSeconds(int seconds)
{
    uint32_t currentTime = GetGameTime();
    if (currentTime == 0)
    {
        LogLine("[TimeManager] ERROR: Cannot add time - game time not available");
        return;
    }
    SetGameTime(currentTime + (seconds * TICKS_PER_SECOND));
}

void TimeManager::SetTime(int hour, int minute, int second, bool isPM)
{
    // Convert 12-hour format to 24-hour
    if (isPM && hour != 12)
        hour += 12;
    else if (!isPM && hour == 12)
        hour = 0;
    
    uint32_t ticks = (hour * TICKS_PER_HOUR) + 
                     (minute * TICKS_PER_MINUTE) + 
                     (second * TICKS_PER_SECOND);
    
    SetGameTime(ticks);
}

void TimeManager::PrintDebugInfo()
{
    LogLine("[TimeManager] === TIME DEBUG ===");
    
    uintptr_t moduleBase = (uintptr_t)GetModuleHandleA(nullptr);
    char buf[256];
    
    sprintf_s(buf, "[TimeManager] Module base: %p", (void*)moduleBase);
    LogLine(buf);
    
    uintptr_t* gameManager = (uintptr_t*)(moduleBase + GAME_MANAGER_OFFSET);
    sprintf_s(buf, "[TimeManager] Game manager address: %p", (void*)gameManager);
    LogLine(buf);
    
    if (!gameManager || !*gameManager)
    {
        LogLine("[TimeManager] Game manager is NULL - game not loaded yet?");
        return;
    }
    
    sprintf_s(buf, "[TimeManager] Game manager value: %p", (void*)*gameManager);
    LogLine(buf);
    
    uintptr_t* playerPtr = (uintptr_t*)(*gameManager + GAME_STATE_PTR_OFFSET);
    sprintf_s(buf, "[TimeManager] Player ptr address: %p", (void*)playerPtr);
    LogLine(buf);
    
    if (!playerPtr || !*playerPtr)
    {
        LogLine("[TimeManager] Player object is NULL - player not spawned?");
        return;
    }
    
    sprintf_s(buf, "[TimeManager] Player object: %p", (void*)*playerPtr);
    LogLine(buf);
    
    uint32_t* timePtr = (uint32_t*)(*playerPtr + GAME_TIME_VALUE_OFFSET);
    sprintf_s(buf, "[TimeManager] Time pointer: %p", (void*)timePtr);
    LogLine(buf);
    
    sprintf_s(buf, "[TimeManager] Current time (ticks): %u", *timePtr);
    LogLine(buf);
    
    sprintf_s(buf, "[TimeManager] Current time (string): %s", TicksToTimeString(*timePtr).c_str());
    LogLine(buf);
    
    sprintf_s(buf, "[TimeManager] Time frozen: %s", s_timeFrozen ? "YES" : "NO");
    LogLine(buf);
    
    LogLine("[TimeManager] === END DEBUG ===");
}

// ═══════════════════════════════════════════
//  TIME CHUNK SYSTEM IMPLEMENTATION
// ═══════════════════════════════════════════

TimeManager::TimeChunk TimeManager::GetCurrentChunk(ChunkSize size)
{
    uint32_t currentTime = GetGameTime();
    int chunkNum = GetCurrentChunkNumber(size);
    return GetChunk(size, chunkNum);
}

TimeManager::TimeChunk TimeManager::GetChunk(ChunkSize size, int chunkNumber)
{
    TimeChunk chunk;
    chunk.chunkNumber = chunkNumber;
    
    uint32_t chunkDuration;
    int totalChunks;
    
    if (size == ChunkSize::SIX_HOURS)
    {
        chunkDuration = 6 * TICKS_PER_HOUR;  // 648000 ticks
        totalChunks = 12;
    }
    else // TWELVE_HOURS
    {
        chunkDuration = 12 * TICKS_PER_HOUR; // 1296000 ticks
        totalChunks = 6;
    }
    
    // Clamp chunk number
    if (chunkNumber < 0) chunkNumber = 0;
    if (chunkNumber >= totalChunks) chunkNumber = totalChunks - 1;
    
    chunk.startTick = GAME_START_TICK + (chunkNumber * chunkDuration);
    chunk.endTick = chunk.startTick + chunkDuration;
    
    chunk.startTime = TicksToTimeString(chunk.startTick);
    chunk.endTime = TicksToTimeString(chunk.endTick);
    
    // Generate label based on chunk size
    if (size == ChunkSize::SIX_HOURS)
    {
        // Calculate hours since game start (12 PM Day 1)
        int hoursSinceStart = chunkNumber * 6;
        int hourOfDay = (12 + hoursSinceStart) % 24; // Start at noon (12)
        
        // Calculate which calendar day (accounting for starting at noon)
        // First 12 hours (chunks 0-1) = Day 1
        // Next 24 hours (chunks 2-5) = Day 2
        // etc.
        int dayNumber = 1 + ((hoursSinceStart + 12) / 24);
        
        // Determine period label
        const char* period;
        if (hourOfDay >= 12 && hourOfDay < 18)  period = "Noon-6PM";
        else if (hourOfDay >= 18 || hourOfDay < 0)  period = "6PM-Midnight"; // Cross midnight
        else if (hourOfDay >= 0 && hourOfDay < 6)   period = "Midnight-6AM";
        else if (hourOfDay >= 6 && hourOfDay < 12)  period = "6AM-Noon";
        else period = "Unknown";
        
        char labelBuf[64];
        sprintf_s(labelBuf, "Day %d %s", dayNumber, period);
        chunk.label = labelBuf;
    }
    else // TWELVE_HOURS
    {
        // Calculate which day we're on
        int hoursSinceStart = chunkNumber * 12;
        int dayNumber = (hoursSinceStart / 24) + 1;
        bool isDay = (chunkNumber % 2 == 0);  // Even chunks are day, odd are night
        
        char labelBuf[64];
        sprintf_s(labelBuf, "Day %d %s", dayNumber, isDay ? "Day" : "Night");
        chunk.label = labelBuf;
    }
    
    return chunk;
}

std::vector<TimeManager::TimeChunk> TimeManager::GetAllChunks(ChunkSize size)
{
    int totalChunks = (size == ChunkSize::SIX_HOURS) ? 12 : 6;
    std::vector<TimeChunk> chunks;
    chunks.reserve(totalChunks);
    
    for (int i = 0; i < totalChunks; i++)
    {
        chunks.push_back(GetChunk(size, i));
    }
    
    return chunks;
}

void TimeManager::JumpToChunk(ChunkSize size, int chunkNumber)
{
    TimeChunk chunk = GetChunk(size, chunkNumber);
    SetGameTime(chunk.startTick);
    
    char buf[128];
    sprintf_s(buf, "[TimeManager] Jumped to chunk %d: %s (%s)", 
              chunkNumber, chunk.label.c_str(), chunk.startTime.c_str());
    LogLine(buf);
}

int TimeManager::GetCurrentChunkNumber(ChunkSize size)
{
    uint32_t currentTime = GetGameTime();
    
    // Before game start
    if (currentTime < GAME_START_TICK)
        return 0;
    
    // After game end
    if (currentTime >= GAME_END_TICK)
        return (size == ChunkSize::SIX_HOURS) ? 11 : 5;
    
    uint32_t elapsed = currentTime - GAME_START_TICK;
    uint32_t chunkDuration = (size == ChunkSize::SIX_HOURS) ? 
                             (6 * TICKS_PER_HOUR) : (12 * TICKS_PER_HOUR);
    
    return (int)(elapsed / chunkDuration);
}