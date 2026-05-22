#include "LevelUpRewardSystem.h"
#include "RewardNotif.h"
#include "DeadRisingEx/MtFramework/Player/uPlayerImpl.h"
#include "DeadRisingEx/MtFramework/Randomiser/InputSystem.h"

#include "detours.h"
#include "DeadRisingEx/Utilities/DebugLog.h"
#include "MtFramework/Game/sMain.h"
#include <stdio.h>
#include <Windows.h>
#include <vector>

// ─────────────────────────────────────────────
//  Globals
// ─────────────────────────────────────────────

void* g_statsObject        = nullptr;
bool  g_statsResolved      = false;
bool  g_blockLevelUps      = true;
bool  g_forcingLevelUp     = false;
bool  g_primedFromCallback = false;

static std::vector<int> s_pendingRewards;  // ← ADD THIS

// ─────────────────────────────────────────────
//  Function pointers
// ─────────────────────────────────────────────

void(__fastcall* SetPlayerLevel)(void* playerObj, int level) =
    (void(__fastcall*)(void*, int))GetModuleAddress(0x1400971C0);

XPAccumulator_t originalXPAccumulator =
    (XPAccumulator_t)GetModuleAddress(0x1400975B0);

LevelUpCallback_t originalLevelUpCallback =
    (LevelUpCallback_t)GetModuleAddress(0x140097240);

// ─────────────────────────────────────────────
//  XP thresholds
// ─────────────────────────────────────────────

static const int XP_THRESHOLDS[MAX_LEVEL + 1] =
{
    0,        // Index 0 (unused)
    0,        // Level 1
    20000,    // Level 2
    40000,    // Level 3
    60000,    // Level 4
    80000,    // Level 5
    110000,   // Level 6
    140000,   // Level 7
    170000,   // Level 8
    200000,   // Level 9
    230000,   // Level 10
    280000,   // Level 11
    330000,   // Level 12
    380000,   // Level 13
    430000,   // Level 14
    480000,   // Level 15
    550000,   // Level 16
    620000,   // Level 17
    690000,   // Level 18
    760000,   // Level 19
    830000,   // Level 20
    920000,   // Level 21
    1010000,  // Level 22
    1100000,  // Level 23
    1190000,  // Level 24
    1280000,  // Level 25
    1380000,  // Level 26
    1480000,  // Level 27
    1580000,  // Level 28
    1680000,  // Level 29
    1780000,  // Level 30
    1910000,  // Level 31
    2040000,  // Level 32
    2170000,  // Level 33
    2300000,  // Level 34
    2430000,  // Level 35
    2580000,  // Level 36
    2730000,  // Level 37
    2880000,  // Level 38
    3030000,  // Level 39
    3180000,  // Level 40
    3350000,  // Level 41
    3520000,  // Level 42
    3690000,  // Level 43
    3860000,  // Level 44
    4030000,  // Level 45
    4210000,  // Level 46
    4390000,  // Level 47
    4570000,  // Level 48
    4750000,  // Level 49
    5000000   // Level 50
};

// ─────────────────────────────────────────────
//  Stats object resolution
// ─────────────────────────────────────────────

bool TryResolveStatsObject()
{
    if (g_statsResolved) return true;
    if (!uPlayerInstance) return false;

    __try
    {
        for (int offset = 0; offset < 0x300; offset += 8)
        {
            void* candidate = *(void**)((uint8_t*)uPlayerInstance + offset);

            if ((uintptr_t)candidate < 0x10000 ||
                (uintptr_t)candidate > 0x7FFFFFFFFFFF)
                continue;

            int val = *(int*)((uint8_t*)candidate + LEVEL_OFFSET);

            if (val >= 1 && val <= MAX_LEVEL)
            {
                g_statsObject   = candidate;
                g_statsResolved = true;

                char buf[128];
                sprintf_s(buf, sizeof(buf),
                    "[HOOK] Stats resolved from player: offset=0x%X level=%d ptr=%p",
                    offset, val, g_statsObject);
                LogLine(buf);
                return true;
            }
        }
    }
    __except(EXCEPTION_EXECUTE_HANDLER) {}

    return false;
}

// ─────────────────────────────────────────────
//  Hook — SetPlayerLevel
// ─────────────────────────────────────────────

void __fastcall Hook_SetPlayerLevel(void* playerObj, int level)
{
    g_statsObject   = playerObj;
    g_statsResolved = true;

    if (g_blockLevelUps)
    {
        LogLine("[BLOCK] Level change prevented");
        return;
    }

    SetPlayerLevel(playerObj, level);
}

// ─────────────────────────────────────────────
//  Pending reward processing
// ─────────────────────────────────────────────

void ProcessPendingRewards()
{
    if (s_pendingRewards.empty() || !g_statsResolved)
        return;
    
    char buf[64];
    sprintf_s(buf, "[LEVEL] Processing %d pending rewards", (int)s_pendingRewards.size());
    LogLine(buf);
    
    for (int count : s_pendingRewards)
    {
        GrantLevelViaXP(count);
    }
    
    s_pendingRewards.clear();
}

// ─────────────────────────────────────────────
//  Hook — XP accumulator
// ─────────────────────────────────────────────

int __fastcall Hook_XPAccumulator(void* param_1, unsigned int param_2, int param_3, int param_4, int param_5)
{
    if (!param_1)
        return originalXPAccumulator(param_1, param_2, param_3, param_4, param_5);

    if (!g_statsResolved)
    {
        g_statsObject   = param_1;
        g_statsResolved = true;

        char buf[128];
        sprintf_s(buf, sizeof(buf), "[HOOK] Stats resolved via XP accumulator: %p", param_1);
        LogLine(buf);
        
        // Process any rewards that were queued before stats were ready
        ProcessPendingRewards(); 
    }

    uint8_t* base   = (uint8_t*)param_1;
    int levelBefore = Read<int>(base, LEVEL_OFFSET);

    // Block natural XP level ups unless we are forcing one
    if (g_blockLevelUps && !g_forcingLevelUp &&
        levelBefore >= 1 && levelBefore < MAX_LEVEL)
    {
        int xp        = Read<int>(base, XP_OFFSET);
        int threshold = XP_THRESHOLDS[levelBefore + 1];

        if (xp + param_3 >= threshold)
            param_3 = max(0, (threshold - 1) - xp);
    }

    int result     = originalXPAccumulator(param_1, param_2, param_3, param_4, param_5);
    int levelAfter = Read<int>(base, LEVEL_OFFSET);

    if (g_blockLevelUps && !g_forcingLevelUp && levelAfter != levelBefore)
    {
        Write<int>(base, LEVEL_OFFSET, levelBefore);

        char buf[128];
        sprintf_s(buf, sizeof(buf), "[BLOCK] Level rollback: %d -> %d", levelAfter, levelBefore);
        LogLine(buf);
    }

    return result;
}

// ─────────────────────────────────────────────
//  Hook — Level up callback
// ─────────────────────────────────────────────

void __fastcall Hook_LevelUpCallback(void* param_1)
{
    if (param_1 != nullptr && !g_primedFromCallback)
    {
        g_statsObject        = param_1;
        g_statsResolved      = true;
        g_primedFromCallback = true;

        char buf[128];
        sprintf_s(buf, sizeof(buf), "[HOOK] Stats captured via callback: %p", param_1);
        LogLine(buf);
        
        // Enable fast_frank on first stats capture
        uPlayerImpl::SetFastFrank(true);

        originalLevelUpCallback(param_1);
        
        // Apply fast frank AFTER callback, passing the stats object
        uPlayerImpl::ApplyFastFrank(param_1);
        
        // Process pending rewards after first callback
        ProcessPendingRewards();  // ← ADD THIS TOO
        return;
    }

    if (g_blockLevelUps && !g_forcingLevelUp)
    {
        LogLine("[BLOCK] LevelUpCallback blocked");
        return;
    }

    originalLevelUpCallback(param_1);
    
    // Apply fast frank after level up, passing param_1 as stats object
    uPlayerImpl::ApplyFastFrank(param_1);
}

// ─────────────────────────────────────────────
//  Debug helpers
// ─────────────────────────────────────────────

void PrintPlayerStats()
{
    if (!g_statsResolved)
    {
        LogLine("[ERROR] Stats not resolved yet");
        return;
    }

    uint8_t* base = (uint8_t*)g_statsObject;
    int level     = Read<int>(base, LEVEL_OFFSET);
    int xp        = Read<int>(base, XP_OFFSET);

    char buf[256];
    sprintf_s(buf, sizeof(buf), "Level=%d XP=%d ptr=%p", level, xp, g_statsObject);
    LogLine(buf);
}

// ─────────────────────────────────────────────
//  ForceLevelUp — debug only, uses SetPlayerLevel
// ─────────────────────────────────────────────

void ForceLevelUp()
{
    if (!g_statsResolved)
    {
        LogLine("[ERROR] Stats not resolved");
        return;
    }

    uint8_t* base = (uint8_t*)g_statsObject;
    int current   = Read<int>(base, LEVEL_OFFSET);

    if (current < 1 || current >= MAX_LEVEL)
    {
        LogLine("[ERROR] Level sanity check failed");
        return;
    }

    char buf[128];
    sprintf_s(buf, sizeof(buf), "[DEBUG] ForceLevelUp: %d -> %d", current, current + 1);
    LogLine(buf);

    g_forcingLevelUp = true;
    g_blockLevelUps  = false;
    SetPlayerLevel(g_statsObject, current + 1);
    g_blockLevelUps  = true;
    g_forcingLevelUp = false;

    int after = Read<int>(base, LEVEL_OFFSET);
    sprintf_s(buf, sizeof(buf), "[DEBUG] ForceLevelUp after: level=%d", after);
    LogLine(buf);
}

// ─────────────────────────────────────────────
//  GrantLevelViaXP — natural level up via XP injection
// ─────────────────────────────────────────────

void GrantLevelViaXP(int count)
{
    if (!g_statsResolved)
    {
        LogLine("[ERROR] Stats not resolved");
        return;
    }

    uint8_t* base = (uint8_t*)g_statsObject;
    int current   = Read<int>(base, LEVEL_OFFSET);
    int target    = min(current + count, MAX_LEVEL);

    if (current >= MAX_LEVEL)
    {
        LogLine("[INFO] Already at max level");
        return;
    }

    // FIX: Make sure we don't go out of bounds on the threshold array
    if (current >= MAX_LEVEL - 1)
    {
        LogLine("[WARNING] At or near max level, cannot grant more levels");
        return;
    }

    int currentXP = Read<int>(base, XP_OFFSET);
    int threshold = XP_THRESHOLDS[current + 1];  // threshold TO REACH next level
    int xpNeeded  = (threshold - currentXP) + 1;

    if (xpNeeded <= 0)
        xpNeeded = 1;

    char buf[128];
    sprintf_s(buf, "[LEVEL] Injecting %d XP (level %d -> %d)", xpNeeded, current, current + 1);
    LogLine(buf);

    g_blockLevelUps  = false;
    g_forcingLevelUp = true;

    originalXPAccumulator(g_statsObject, 1, xpNeeded, 0, 0);

    g_blockLevelUps  = true;
    g_forcingLevelUp = false;

    int after = Read<int>(base, LEVEL_OFFSET);
    sprintf_s(buf, "[LEVEL] Level after XP injection: %d", after);
    LogLine(buf);

    // Recurse for remaining levels
    if (count > 1 && after < target && after > current)  // ← Add check that level actually increased
        GrantLevelViaXP(count - 1);
}

// ─────────────────────────────────────────────
//  GrantLevels — public API used by CheckSystem
// ─────────────────────────────────────────────

void GrantLevels(int count)
{
    if (!g_statsResolved)
    {
        char buf[64];
        sprintf_s(buf, "[LEVEL] Stats not ready, queueing %d level reward", count);
        LogLine(buf);
        s_pendingRewards.push_back(count);
        return;
    }
    
    GrantLevelViaXP(count);
}