#include "LevelUpRewardSystem.h"
#include "DeadRisingEx/MtFramework/Player/uPlayerImpl.h"
#include "DeadRisingEx/MtFramework/Randomiser/InputSystem.h"

#include "detours.h"
#include "DeadRisingEx/Utilities/DebugLog.h"
#include <stdio.h>
#include <Windows.h>
#include <vector>

// ─────────────────────────────────────────────
//  Globals
// ─────────────────────────────────────────────

void* g_statsObject    = nullptr;
bool  g_statsResolved  = false;
bool  g_blockLevelUps  = true;
bool  g_forcingLevelUp = false;

static std::vector<int> s_pendingRewards;

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
    static bool s_processing = false;
    if (s_processing || s_pendingRewards.empty() || !g_statsResolved)
        return;

    s_processing = true;

    char buf[64];
    sprintf_s(buf, "[LEVEL] Processing %d pending rewards", (int)s_pendingRewards.size());
    LogLine(buf);

    std::vector<int> toProcess = s_pendingRewards;
    s_pendingRewards.clear();

    for (int count : toProcess)
        GrantLevelViaXP(count);

    s_processing = false;
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
    }

    uint8_t* base   = (uint8_t*)param_1;
    int levelBefore = Read<int>(base, LEVEL_OFFSET);

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

    ProcessPendingRewards();

    return result;
}

// ─────────────────────────────────────────────
//  Hook — Level up callback
// ─────────────────────────────────────────────

void __fastcall Hook_LevelUpCallback(void* param_1)
{
    if (param_1 != nullptr && !g_statsResolved)
    {
        g_statsObject   = param_1;
        g_statsResolved = true;

        char buf[128];
        sprintf_s(buf, sizeof(buf), "[HOOK] Stats captured via callback: %p", param_1);
        LogLine(buf);

        uPlayerImpl::SetFastFrank(true);
        originalLevelUpCallback(param_1);
        uPlayerImpl::ApplyFastFrank(param_1);
        ProcessPendingRewards();
        return;
    }

    if (g_blockLevelUps && !g_forcingLevelUp)
    {
        LogLine("[BLOCK] LevelUpCallback blocked");
        return;
    }

    originalLevelUpCallback(param_1);
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
//  GrantLevelViaXP
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

    if (current >= MAX_LEVEL - 1)
    {
        LogLine("[WARNING] At or near max level, cannot grant more levels");
        return;
    }

    int currentXP = Read<int>(base, XP_OFFSET);
    int threshold = XP_THRESHOLDS[current + 1];
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

    if (count > 1 && after < target && after > current)
        GrantLevelViaXP(count - 1);
}

// ─────────────────────────────────────────────
//  GrantLevels — public API
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