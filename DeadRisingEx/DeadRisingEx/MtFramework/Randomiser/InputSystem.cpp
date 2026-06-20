
#include "InputSystem.h"
#include "AreaKeySystem.h"
#include "TimeManager.h"
#include "KeyItemCheck.h"
#include "Checks/CheckSystem.h"
#include "Rewards/LevelUpRewardSystem.h"
#include "Rewards/SetItemRewardSystem.h"
#include "Rewards/CameraRefillReward.h"
#include "Rewards/TimeChunkReward.h"
#include "Rewards/RewardNotif.h"
#include "DeadRisingEx/MtFramework/Randomiser/Rewards/CostumeRewardSystem.h"

#include "DeadRisingEx/MtFramework/Player/uPlayerImpl.h"
#include "DeadRisingEx/Utilities/DebugLog.h"
#include <Windows.h>
#include <stdio.h>

static HANDLE hRandomiserLog = INVALID_HANDLE_VALUE;
static bool   g_logShuttingDown = false;

void ShutdownRandomiserLog()
{
    g_logShuttingDown = true;
    if (hRandomiserLog != INVALID_HANDLE_VALUE)
    {
        CloseHandle(hRandomiserLog);
        hRandomiserLog = INVALID_HANDLE_VALUE;
    }
}

void InitRandomiserLog()
{
    CHAR sLogFilePath[MAX_PATH] = { 0 };
    snprintf(sLogFilePath, sizeof(sLogFilePath), "%s\\RandomiserDebug.txt", ModConfig::Instance()->GameDirectory);
    hRandomiserLog = CreateFileA(sLogFilePath, GENERIC_WRITE, FILE_SHARE_READ, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
}

void LogLine(const char* text)
{
    if (hRandomiserLog != INVALID_HANDLE_VALUE)
    {
        char lineBuf[2048];
        int len = _snprintf_s(lineBuf, sizeof(lineBuf) - 2, _TRUNCATE, "%s", text);
        if (len > 0)
        {
            lineBuf[len]     = '\r';
            lineBuf[len + 1] = '\n';
            DWORD bytesWritten = 0;
            WriteFile(hRandomiserLog, lineBuf, (DWORD)(len + 2), &bytesWritten, NULL);
            FlushFileBuffers(hRandomiserLog);
        }
    }

    if (!g_logShuttingDown)
        ImGuiConsole::Instance()->ConsolePrint(text);
}

// ─────────────────────────────────────────────
//  Game event queue — fired on game thread by HandleDebugInput
// ─────────────────────────────────────────────

static uint32_t s_pendingEventId = 0;

void QueueGameEvent(uint32_t eventId)
{
    s_pendingEventId = eventId;
}

// ─────────────────────────────────────────────
//  Player struct snapshot/diff — find unknown offsets
//  Bind TakePlayerSnapshot/DiffPlayerSnapshot to keys to use.
// ─────────────────────────────────────────────

#if 0
static constexpr int   SNAP_SIZE = 0x10000;
static uint8_t         s_playerSnap[SNAP_SIZE] = {};
static bool            s_snapTaken = false;

static void TakePlayerSnapshot()
{
    if (!uPlayerInstance) { LogLine("[SNAP] No player instance"); return; }
    memcpy(s_playerSnap, (uint8_t*)uPlayerInstance, SNAP_SIZE);
    s_snapTaken = true;
    LogLine("[SNAP] Snapshot taken");
}

static void DiffPlayerSnapshot()
{
    if (!uPlayerInstance) { LogLine("[SNAP] No player instance"); return; }
    if (!s_snapTaken)     { LogLine("[SNAP] No snapshot — press F4 first"); return; }

    const uint8_t* now = (uint8_t*)uPlayerInstance;

    // Build a change map first
    bool changed[SNAP_SIZE] = {};
    for (int i = 0; i < SNAP_SIZE; i++)
        changed[i] = (now[i] != s_playerSnap[i]);

    // Only report bytes that are isolated — no adjacent byte also changed.
    // This filters out floats/matrices/vectors which change 4+ bytes at once.
    int count = 0;
    char buf[64];
    for (int i = 0; i < SNAP_SIZE; i++)
    {
        if (!changed[i]) continue;
        bool neighbourChanged = (i > 0          && changed[i - 1])
                             || (i < SNAP_SIZE-1 && changed[i + 1]);
        if (neighbourChanged) continue;

        sprintf_s(buf, "[SNAP] +0x%X: 0x%02X -> 0x%02X", i, s_playerSnap[i], now[i]);
        LogLine(buf);
        count++;
    }
    sprintf_s(buf, "[SNAP] %d isolated byte(s) changed", count);
    LogLine(buf);
}
#endif

// ─────────────────────────────────────────────
//  Main input handler
// ─────────────────────────────────────────────

void HandleDebugInput()
{
    // Fire any event queued from the console (console runs on render thread, not game thread)
    if (s_pendingEventId != 0)
    {
        uint32_t id = s_pendingEventId;
        s_pendingEventId = 0;
        if (KeyItemCheck::originalGameEvent && KeyItemCheck::s_manager)
        {
            KeyItemCheck::originalGameEvent(KeyItemCheck::s_manager, id);
            char buf[64];
            sprintf_s(buf, "[EVENT] Fired queued event 0x%X", id);
            LogLine(buf);
        }
        else
        {
            LogLine("[EVENT] Could not fire queued event — manager not ready");
        }
    }

    // Enforce time gating every frame
    TimeChunkReward::EnforceTimeGate();

    // ═══════════════════════════════════════════
    //  INPUT CONTROLS
    // ═══════════════════════════════════════════
    
    if (GetAsyncKeyState('1') & 1)
    {

    }

    if (GetAsyncKeyState('2') & 1)
    {
        
    }

    if (GetAsyncKeyState('3') & 1)
    {
        
    }

    if (GetAsyncKeyState('4') & 1)
    {
        
    }
    
    if (GetAsyncKeyState('5') & 1)
    {
        
    }

    if (GetAsyncKeyState('6') & 1)
    {
        
    }

    if (GetAsyncKeyState('7') & 1)
    {
        
    }

    if (GetAsyncKeyState('8') & 1)
    {
        
    }

    if (GetAsyncKeyState('9') & 1)
    {
        
    }

    if (GetAsyncKeyState('0') & 1)
    {

    }

    // ═══════════════════════════════════════════
    //  ADDITIONAL SHORTCUTS (Optional)
    // ═══════════════════════════════════════════
/*  
    // F1 - Add 1 hour
    if (GetAsyncKeyState(VK_F1) & 1)
    {
        char buf[128];
        sprintf_s(buf, "[TIME] +1 hour (was: %s)", TimeManager::GetTimeString().c_str());
        LogLine(buf);
        TimeManager::AddHours(1);
        sprintf_s(buf, "[TIME] Now: %s", TimeManager::GetTimeString().c_str());
        LogLine(buf);
    }

    // F2 - Subtract 1 hour
    if (GetAsyncKeyState(VK_F2) & 1)
    {

    }

    // F3 - Set time to 12:00 PM (noon)
    if (GetAsyncKeyState(VK_F3) & 1)
    {
        LogLine("[DEBUG] Time system info:");
        TimeManager::PrintDebugInfo();
    }

    if (GetAsyncKeyState(VK_F4) & 1)


    if (GetAsyncKeyState(VK_F5) & 1)
    {
        for (int i = 0; i < static_cast<int>(ZoneID::COUNT); i++)
            AreaKeySystem::Get().GiveKey(static_cast<ZoneID>(i));
        LogLine("[DEBUG] All area keys granted");
    }

    if (GetAsyncKeyState(VK_F6) & 1)
    {
        TimeChunkReward::UnlockAllChunks();
        LogLine("[DEBUG] All time chunks unlocked");
    }
*/ 
}