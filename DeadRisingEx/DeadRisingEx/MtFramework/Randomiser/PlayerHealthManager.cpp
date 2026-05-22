#include "../uPlayerImpl.h"
#include "DeadRisingEx/Utilities/DebugLog.h"
#include "InputSystem.h"
#include <cstdint>

// ═══════════════════════════════════════════
//  EXTERNAL REFERENCES
// ═══════════════════════════════════════════
extern void* uPlayerInstance;
extern void* g_RealGameStateManager;

// ═══════════════════════════════════════════
//  OFFSETS
// ═══════════════════════════════════════════
static constexpr uintptr_t PLAYER_HEALTH_OFFSET     = 0x12EC;
static constexpr uintptr_t PLAYER_MAX_HEALTH_OFFSET = 0x12E8;
static constexpr uintptr_t PLAYER_STATE_OFFSET      = 0x24;
static constexpr uintptr_t PLAYER_DEATH_FLAG_OFFSET = 0x16DA;

static constexpr uintptr_t GAME_STATE_OFFSET     = 0x38;
static constexpr uintptr_t GAME_SUBSTATE_OFFSET  = 0x3A;
static constexpr uint8_t   GAME_STATE_GAME_OVER  = 0x12;

// ═══════════════════════════════════════════
//  PLAYER STATES
// ═══════════════════════════════════════════
enum class PlayerState : uint8_t
{
    Alive    = 0,
    Unknown  = 1,
    Dead     = 2,
    GameOver = 3,
};

// ═══════════════════════════════════════════
//  DEATH TRIGGER (external)
// ═══════════════════════════════════════════
typedef void (__fastcall* DeathTriggerFunc)(void* player, void* attacker);
static DeathTriggerFunc TriggerDeath = 
    (DeathTriggerFunc)GetModuleAddress(0x14027cf50);

// ═══════════════════════════════════════════
//  HEALTH MANAGEMENT
// ═══════════════════════════════════════════

void uPlayerImpl::KillPlayer()
{
    if (!uPlayerInstance)
    {
        LogLine("[KILL] ERROR: No player instance");
        return;
    }
    
    if (!g_RealGameStateManager)
    {
        LogLine("[KILL] ERROR: Game state manager not captured yet");
        return;
    }

    char buf[256];
    LogLine("[KILL] === TRIGGERING GAME OVER ===");
    
    // Set player to dead state
    *(int*)((uintptr_t)uPlayerInstance + PLAYER_HEALTH_OFFSET) = 0;
    *(uint8_t*)((uintptr_t)uPlayerInstance + PLAYER_STATE_OFFSET) = (uint8_t)PlayerState::Dead;
    *(uint8_t*)((uintptr_t)uPlayerInstance + PLAYER_DEATH_FLAG_OFFSET) = 1;
    
    LogLine("[KILL] Player set to dead state (HP=0, State=Dead, Flag=1)");
    
    // Trigger game-over menu via game state
    uint8_t* statePtr = (uint8_t*)((uintptr_t)g_RealGameStateManager + GAME_STATE_OFFSET);
    uint8_t oldState = *statePtr;
    *statePtr = GAME_STATE_GAME_OVER;
    
    // Reset substate
    *(uint8_t*)((uintptr_t)g_RealGameStateManager + GAME_SUBSTATE_OFFSET) = 0;
    
    sprintf_s(buf, "[KILL] Game state set to 0x12 (was 0x%X)", oldState);
    LogLine(buf);
    LogLine("[KILL] === COMPLETE ===");
}

void uPlayerImpl::SetHealth(int health)
{
    if (!uPlayerInstance)
    {
        LogLine("[HEALTH] ERROR: No player instance");
        return;
    }

    int* healthPtr = (int*)((uintptr_t)uPlayerInstance + PLAYER_HEALTH_OFFSET);
    int oldHealth = *healthPtr;
    *healthPtr = health;

    char buf[128];
    sprintf_s(buf, "[HEALTH] Changed: %d -> %d", oldHealth, health);
    LogLine(buf);
    
    // Trigger death if health dropped to 0
    if (health <= 0 && oldHealth > 0)
    {
        LogLine("[HEALTH] Reached zero - triggering death");
        __try
        {
            TriggerDeath(uPlayerInstance, nullptr);
        }
        __except(EXCEPTION_EXECUTE_HANDLER)
        {
            LogLine("[HEALTH] ERROR: Death trigger crashed");
        }
    }
}

int uPlayerImpl::GetHealth()
{
    if (!uPlayerInstance)
        return -1;

    return *(int*)((uintptr_t)uPlayerInstance + PLAYER_HEALTH_OFFSET);
}

bool uPlayerImpl::IsAlive()
{
    if (!uPlayerInstance)
        return false;

    uint8_t state = *(uint8_t*)((uintptr_t)uPlayerInstance + PLAYER_STATE_OFFSET);
    int health = GetHealth();

    return (health > 0) && (state < (uint8_t)PlayerState::Dead);
}

void uPlayerImpl::PrintHealthDebug()
{
    LogLine("[DEBUG] === PLAYER STATE ===");

    if (!uPlayerInstance)
    {
        LogLine("[DEBUG] Player instance is NULL");
        return;
    }

    char buf[256];
    sprintf_s(buf, "[DEBUG] Instance: %p", uPlayerInstance);
    LogLine(buf);

    sprintf_s(buf, "[DEBUG] Health: %d / %d",
        *(int*)((uintptr_t)uPlayerInstance + PLAYER_HEALTH_OFFSET),
        *(int*)((uintptr_t)uPlayerInstance + PLAYER_MAX_HEALTH_OFFSET));
    LogLine(buf);

    uint8_t state = *(uint8_t*)((uintptr_t)uPlayerInstance + PLAYER_STATE_OFFSET);
    sprintf_s(buf, "[DEBUG] State: %d (0=Alive, 2=Dead, 3=GameOver)", state);
    LogLine(buf);

    uint8_t deathFlag = *(uint8_t*)((uintptr_t)uPlayerInstance + PLAYER_DEATH_FLAG_OFFSET);
    sprintf_s(buf, "[DEBUG] Death flag: %d", deathFlag);
    LogLine(buf);

    sprintf_s(buf, "[DEBUG] Status: %s", IsAlive() ? "ALIVE" : "DEAD");
    LogLine(buf);

    LogLine("[DEBUG] === END ===");
}

void uPlayerImpl::TestDamageSimulation()
{
    LogLine("[TEST] === DAMAGE SIMULATION ===");
    
    if (!uPlayerInstance)
    {
        LogLine("[TEST] No player instance");
        return;
    }

    int healthBefore = GetHealth();
    char buf[128];
    sprintf_s(buf, "[TEST] Health before: %d", healthBefore);
    LogLine(buf);

    LogLine("[TEST] Calling death trigger...");
    
    __try
    {
        TriggerDeath(uPlayerInstance, nullptr);
        LogLine("[TEST] Death trigger succeeded");
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        LogLine("[TEST] Death trigger crashed");
    }

    int healthAfter = GetHealth();
    uint8_t state = *(uint8_t*)((uintptr_t)uPlayerInstance + PLAYER_STATE_OFFSET);
    sprintf_s(buf, "[TEST] After: HP=%d, State=%d", healthAfter, state);
    LogLine(buf);

    LogLine("[TEST] === END ===");
}