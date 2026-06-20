
#include "InputSystem.h"
#include "AreaKeySystem.h"
#include "TimeManager.h"
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

extern bool g_statsResolved;

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
        DWORD bytesWritten = 0;
        WriteFile(hRandomiserLog, text, lstrlenA(text), &bytesWritten, NULL);
        WriteFile(hRandomiserLog, "\r\n", 2, &bytesWritten, NULL);
        FlushFileBuffers(hRandomiserLog);
    }

    if (!g_logShuttingDown)
        ImGuiConsole::Instance()->ConsolePrint(text);
}

// ─────────────────────────────────────────────
//  Player struct snapshot/diff — find unknown offsets
//  F4 = snapshot, F5 = diff and log all changed bytes
// ─────────────────────────────────────────────

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

// ─────────────────────────────────────────────
//  Seed input state
// ─────────────────────────────────────────────

static bool     s_enteringSeed  = false;
static char     s_seedBuffer[16] = {};
static int      s_seedLen        = 0;

static void BeginSeedEntry()
{
    s_enteringSeed = true;
    s_seedLen      = 0;
    memset(s_seedBuffer, 0, sizeof(s_seedBuffer));
    LogLine("[SEED] Type a number then press ENTER. ESC to cancel.");
}

static void HandleSeedEntry()
{
    // Number keys 0-9
    for (int k = '0'; k <= '9'; k++)
    {
        if ((GetAsyncKeyState(k) & 1) && s_seedLen < 9)
        {
            s_seedBuffer[s_seedLen++] = (char)k;
            s_seedBuffer[s_seedLen]   = '\0';

            char buf[32];
            sprintf_s(buf, "[SEED] > %s", s_seedBuffer);
            LogLine(buf);
        }
    }

    // ENTER — confirm seed
    if (GetAsyncKeyState(VK_RETURN) & 1)
    {
        s_enteringSeed = false;

        if (s_seedLen == 0)
        {
            LogLine("[SEED] No input — generating random seed.");
            CheckSystem::SetSeed(0);
        }
        else
        {
            uint32_t seed = (uint32_t)atoi(s_seedBuffer);
            CheckSystem::SetSeed(seed);

            char buf[64];
            sprintf_s(buf, "[SEED] Seed confirmed: %u", seed);
            LogLine(buf);
        }
    }

    // ESC — cancel
    if (GetAsyncKeyState(VK_ESCAPE) & 1)
    {
        s_enteringSeed = false;
        LogLine("[SEED] Seed entry cancelled.");
    }
}

// ─────────────────────────────────────────────
//  Main input handler
// ─────────────────────────────────────────────

void HandleDebugInput()
{

    // Enforce time gating every frame
    TimeChunkReward::EnforceTimeGate();

    // If player is typing a seed, handle that exclusively
    if (s_enteringSeed)
    {
        HandleSeedEntry();
        return;
    }

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
    
    // F1 - Add 1 hour
    if (GetAsyncKeyState(VK_F1) & 1)
    {
        char buf[128];
        sprintf_s(buf, "[TIME] +1 hour (was: %s)", TimeManager::GetTimeString().c_str());
        LogLine(buf);
        //TimeManager::AddHours(1);
        sprintf_s(buf, "[TIME] Now: %s", TimeManager::GetTimeString().c_str());
        LogLine(buf);
    }

    // F2 - Subtract 1 hour
    if (GetAsyncKeyState(VK_F2) & 1)
    {
        if (uPlayerInstance)
        {
            // Test both consistent candidates — set them to what they become after pickup
            uint8_t* b237D = (uint8_t*)uPlayerInstance + 0x237D;
            uint8_t* b1500 = (uint8_t*)uPlayerInstance + 0x1500;
            *b237D &= ~0x08;
            *b1500  = 0x04;
            char buf[96];
            sprintf_s(buf, "[TEST] 0x237D=0x%02X  0x1500=0x%02X", *b237D, *b1500);
            LogLine(buf);
        }
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

}