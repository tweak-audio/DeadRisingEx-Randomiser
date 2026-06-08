/*

*/

#include "FastFrank.h"
#include "InputSystem.h"
#include "detours.h"
#include "DeadRisingEx/Utilities/DebugLog.h"
#include "DeadRisingEx/MtFramework/Randomiser/RandomiserConfig.h"

// External references
extern void* uPlayerInstance;

// Fast Frank state

enum SpeedLevel {
    SLOW = 0,
    NORMAL = 1,
    FAST = 7,
    VERY_FAST = 10,
    MAX = 14
};

static bool s_fastFrankEnabled = true;
static int s_fastFrankSpeedLevel = SpeedLevel(MAX); 

// Original function pointer
static void* (*originalSetRunLevel)(void* thisptr, int level) = 
    (void* (*)(void*, int))GetModuleAddress(0x1401f8410);

// Hook implementation
void* FastFrank::Hook_SetRunLevel(void* thisptr, int level)
{
    char buf[128];
    sprintf_s(buf, "[FAST FRANK] SetRunLevel called: level=%d, fast_frank=%d", 
        level, s_fastFrankEnabled);
    LogLine(buf);
    
    // If fast_frank is enabled, force level to configured speed
    if (s_fastFrankEnabled)
    {
        level = s_fastFrankSpeedLevel;
        sprintf_s(buf, "[FAST FRANK] Forced level to %d", s_fastFrankSpeedLevel);
        LogLine(buf);
    }
    
    return originalSetRunLevel(thisptr, level);
}

void FastFrank::InitializeHooks()
{
    const RandomiserConfig& cfg = RandomiserConfig::Get();
    s_fastFrankEnabled    = cfg.fastFrank;
    s_fastFrankSpeedLevel = cfg.fastFrankSpeed;

    DetourAttach((void**)&originalSetRunLevel, Hook_SetRunLevel);
    LogLine("[FAST FRANK] Hooks initialized");
}

void FastFrank::SetEnabled(bool enabled)
{
    s_fastFrankEnabled = enabled;
    
    char buf[64];
    sprintf_s(buf, "[FAST FRANK] %s", enabled ? "ENABLED" : "DISABLED");
    LogLine(buf);
    
    // Immediately apply by calling the setter with current speed level
    if (enabled && uPlayerInstance)
    {
        LogLine("[FAST FRANK] Forcing immediate speed update via SetRunLevel");
        originalSetRunLevel(uPlayerInstance, s_fastFrankSpeedLevel);
    }
}

bool FastFrank::IsEnabled()
{
    return s_fastFrankEnabled;
}

void FastFrank::SetSpeedLevel(int speedLevel)
{
    if (speedLevel < 0 || speedLevel > 4)
    {
        LogLine("[FAST FRANK] Invalid speed level, must be 0-4");
        return;
    }
    
    s_fastFrankSpeedLevel = speedLevel;
    
    char buf[128];
    sprintf_s(buf, "[FAST FRANK] Speed level set to %d", speedLevel);
    LogLine(buf);
    
    // Apply immediately if enabled
    if (s_fastFrankEnabled && uPlayerInstance)
    {
        originalSetRunLevel(uPlayerInstance, speedLevel);
    }
}

int FastFrank::GetSpeedLevel()
{
    return s_fastFrankSpeedLevel;
}

void FastFrank::ApplyToStatsObject(void* statsObject)
{
    if (!s_fastFrankEnabled || !statsObject)
    {
        return;
    }
    
    __try
    {
        // Run level is at stats object + 0x78
        byte* runLevelPtr = (byte*)statsObject + 0x78;
        byte currentRunLevel = *runLevelPtr;
        
        char buf[128];
        sprintf_s(buf, "[FAST FRANK] Current run level: %d, setting to %d", 
            currentRunLevel, s_fastFrankSpeedLevel);
        LogLine(buf);
        
        *runLevelPtr = (byte)s_fastFrankSpeedLevel;
        
        sprintf_s(buf, "[FAST FRANK] Successfully set to %d", s_fastFrankSpeedLevel);
        LogLine(buf);
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        LogLine("[FAST FRANK] Exception while setting run level");
    }
}

void FastFrank::OnPlayerConstruct(void* playerInstance)
{
    if (!s_fastFrankEnabled)
        return;

    __try
    {
        byte* runLevelPtr = (byte*)playerInstance + 0x78;
        *runLevelPtr = (byte)s_fastFrankSpeedLevel;

        char buf[128];
        sprintf_s(buf, "[FAST FRANK] Set initial run level to %d in constructor",
            s_fastFrankSpeedLevel);
        LogLine(buf);
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        LogLine("[FAST FRANK] Exception setting run level in constructor");
    }
}