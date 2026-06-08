
#include "InputSystem.h"
#include "AreaKeySystem.h"
#include "TimeManager.h"
#include "Checks/CheckSystem.h"
#include "Rewards/LevelUpRewardSystem.h"
#include "Rewards/SetItemRewardSystem.h"
#include "Rewards/CameraRefillReward.h"
#include "Rewards/TimeChunkReward.h"
#include "Rewards/RewardNotif.h"
#include "DeadRisingEx/MtFramework/Randomiser/Rewards/ClothingRewardSystem.h"

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
        for (int i = 0; i < static_cast<int>(ZoneID::COUNT); i++)
            AreaKeySystem::Get().GiveKey(static_cast<ZoneID>(i));
        LogLine("[DEBUG] All area keys granted");
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
        char buf[128];
        sprintf_s(buf, "[TIME] -1 hour (was: %s)", TimeManager::GetTimeString().c_str());
        LogLine(buf);
        //TimeManager::AddHours(-1);
        sprintf_s(buf, "[TIME] Now: %s", TimeManager::GetTimeString().c_str());
        LogLine(buf);
    }

    // F3 - Set time to 12:00 PM (noon)
    if (GetAsyncKeyState(VK_F3) & 1)
    {
        LogLine("[DEBUG] Time system info:");
        TimeManager::PrintDebugInfo();
    }

    if (GetAsyncKeyState(VK_F4) & 1)
    {
        
    }

    if (GetAsyncKeyState(VK_F5) & 1)
    {

    }
}