#include "uPlayerImpl.h"
#include "detours.h"
#include <locale>
#include <codecvt>
#include <cstdint>
#include <MtFramework/Archive/sResource.h>
#include "DeadRisingEx/Utilities/DebugLog.h"
#include "MtFramework/Game/sMain.h"

#include "DeadRisingEx/MtFramework/Randomiser/Rewards/LevelUpRewardSystem.h"
#include "DeadRisingEx/MtFramework/Randomiser/Rewards/ClothingRewardSystem.h"
#include "DeadRisingEx/MtFramework/Randomiser/Checks/ClothingCheck.h"
#include "DeadRisingEx/MtFramework/Randomiser/Checks/CaseCheck.h"
#include "DeadRisingEx/MtFramework/Randomiser/InputSystem.h"
#include "DeadRisingEx/MtFramework/Randomiser/FastFrank.h"
#include "DeadRisingEx/MtFramework/Randomiser/RandomiserConfig.h"

// ═══════════════════════════════════════════
//  GLOBAL POINTERS
// ═══════════════════════════════════════════
void** g_sUnitInstance     = (void**)GetModuleAddress(0x141CF2620);
void** g_sItemCtrlInstance = (void**)GetModuleAddress(0x1419462A0);

void* uPlayerInstance = nullptr;
void* g_RealGameStateManager = nullptr;

// ═══════════════════════════════════════════
//  OFFSETS
// ═══════════════════════════════════════════
static constexpr uintptr_t GAME_STATE_OFFSET = 0x38;

// ═══════════════════════════════════════════
//  FUNCTION POINTERS
// ═══════════════════════════════════════════
typedef void (__fastcall* DeathTriggerFunc)(void* player, void* attacker);
typedef void (__fastcall* GameStateTickFunc)(void*);

void* (__stdcall* uPlayer_ctor)(void* thisptr) = 
    (void* (__stdcall*)(void*))GetModuleAddress(0x1401306E0);

static DeathTriggerFunc TriggerDeath = 
    (DeathTriggerFunc)GetModuleAddress(0x14027cf50);

GameStateTickFunc Original_GameStateTick = nullptr;
static bool g_pendingStartingOutfit = false;

// ═══════════════════════════════════════════
//  HOOKS
// ═══════════════════════════════════════════

void __fastcall Hook_GameStateTick(void* gameStateManager)
{
    if (!g_RealGameStateManager)
    {
        g_RealGameStateManager = gameStateManager;

        char buf[256];
        sprintf_s(buf, "[TICK] Captured game state manager: %p", gameStateManager);
        LogLine(buf);

        uint8_t state = *(uint8_t*)((uintptr_t)gameStateManager + GAME_STATE_OFFSET);
        sprintf_s(buf, "[TICK] State at +0x38: 0x%X", state);
        LogLine(buf);
    }

    // Defer ApplyRandomStartingOutfit until the game leaves loading state (0x0).
    // ApplyCostumeSlot crashes when called while the game state manager is still
    // at 0 — the costume system isn't live until the game advances past loading.
    if (g_pendingStartingOutfit && uPlayerInstance && g_RealGameStateManager)
    {
        uint8_t state = *(uint8_t*)((uintptr_t)g_RealGameStateManager + GAME_STATE_OFFSET);

        // Log state transitions while waiting (first 5 ticks + every time it changes)
        static uint8_t s_lastOutfitWaitState = 0xFF;
        static int s_outfitWaitTicks = 0;
        s_outfitWaitTicks++;
        if (state != s_lastOutfitWaitState || s_outfitWaitTicks <= 5)
        {
            char buf[128];
            sprintf_s(buf, "[TICK] Outfit pending — state=0x%X (tick %d)", state, s_outfitWaitTicks);
            LogLine(buf);
            s_lastOutfitWaitState = state;
        }

        if (state != 0)
        {
            char buf[128];
            sprintf_s(buf, "[TICK] Outfit state cleared — firing ApplyRandomStartingOutfit (state=0x%X)", state);
            LogLine(buf);
            g_pendingStartingOutfit = false;
            ApplyRandomStartingOutfit();
            LogLine("[TICK] ApplyRandomStartingOutfit returned");
        }
    }

    Original_GameStateTick(gameStateManager);
    //CaseCheck, not quite working yet
    //CaseCheck::OnGameStateTick();
}

void* __stdcall Hook_uPlayer_ctor(void* thisptr)
{
    char buf[128];
    sprintf_s(buf, "[HOOK] uPlayer ctor enter — thisptr=%p", thisptr);
    LogLine(buf);

    void* result = uPlayer_ctor(thisptr);
    uPlayerInstance = thisptr;

    sprintf_s(buf, "[HOOK] uPlayer ctor returned — result=%p, uPlayerInstance=%p", result, uPlayerInstance);
    LogLine(buf);

    FastFrank::OnPlayerConstruct(thisptr);

    sprintf_s(buf, "[HOOK] Calling SetStateMachineObj(%p)", thisptr);
    LogLine(buf);
    ClothingCheck::SetStateMachineObj(thisptr);
    LogLine("[HOOK] SetStateMachineObj done");

    if (RandomiserConfig::Get().randomiseStartingOutfit)
    {
        g_pendingStartingOutfit = true;
        LogLine("[HOOK] g_pendingStartingOutfit = true");
    }
    else
    {
        LogLine("[HOOK] randomiseStartingOutfit is false — outfit skipped");
    }

    return result;
}

void __fastcall Hook_DeathTrigger(void* player, void* attacker)
{
    char buf[256];
    sprintf_s(buf, "[DEATH] Triggered - player=%p, attacker=%p", player, attacker);
    //LogLine(buf);
    
    TriggerDeath(player, attacker);
    
    //LogLine("[DEATH] Trigger complete");
}

// ═══════════════════════════════════════════
//  REGISTRATION
// ═══════════════════════════════════════════

void uPlayerImpl::RegisterTypeInfo()
{
    DetourAttach((void**)&uPlayer_ctor, Hook_uPlayer_ctor);
    DetourAttach((void**)&TriggerDeath, Hook_DeathTrigger);
    DetourAttach((void**)&SetPlayerLevel, Hook_SetPlayerLevel);
    DetourAttach((void**)&originalXPAccumulator, Hook_XPAccumulator);
    DetourAttach((void**)&originalLevelUpCallback, Hook_LevelUpCallback);
    ClothingCheck::InitializeHooks();
    
    
    // Hook game state tick to capture the game state manager
    Original_GameStateTick = (GameStateTickFunc)GetModuleAddress(0x140204350);
    DetourAttach((void**)&Original_GameStateTick, Hook_GameStateTick);
    
    LogLine("[uPlayer] All hooks registered");
    
    FastFrank::InitializeHooks();
}

// ═══════════════════════════════════════════
//  FAST FRANK WRAPPERS
// ═══════════════════════════════════════════

void uPlayerImpl::SetFastFrank(bool enabled)        { FastFrank::SetEnabled(enabled); }
bool uPlayerImpl::IsFastFrankEnabled()              { return FastFrank::IsEnabled(); }
void uPlayerImpl::ApplyFastFrank(void* statsObject) { FastFrank::ApplyToStatsObject(statsObject); }
void uPlayerImpl::SetFastFrankSpeed(int speedLevel) { FastFrank::SetSpeedLevel(speedLevel); }

