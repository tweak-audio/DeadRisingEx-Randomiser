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
    
    Original_GameStateTick(gameStateManager);
    //CaseCheck, not quite working yet
    //CaseCheck::OnGameStateTick();
}

void* __stdcall Hook_uPlayer_ctor(void* thisptr)
{
    uPlayerInstance = thisptr;
    void* result = uPlayer_ctor(thisptr);

    //Apply Fast Frank
    FastFrank::OnPlayerConstruct(thisptr);

    //Initialise costume statew# machine obj
    ClothingCheck::SetStateMachineObj(thisptr);

    //Randomise starting outfit
    if (RANDOMISE_STARTING_OUTFIT)
        ApplyRandomStartingOutfit();
    
    char buf[128];
    sprintf_s(buf, "[HOOK] uPlayer constructed at %p", uPlayerInstance);
    LogLine(buf);
    
    return result;
}

void __fastcall Hook_DeathTrigger(void* player, void* attacker)
{
    char buf[256];
    sprintf_s(buf, "[DEATH] Triggered - player=%p, attacker=%p", player, attacker);
    LogLine(buf);
    
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

