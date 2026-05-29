/*

*/

#pragma once
#include "DeadRisingEx.h"
#include <MtFramework/MtObject.h>

extern void **g_sUnitInstance;
extern void *uPlayerInstance;
extern void* g_RealGameStateManager;

class uPlayerImpl
{
protected:

public:
    static void RegisterTypeInfo();

    // Fast Frank wrapper functions (delegates to FastFrank class)
    static void SetFastFrank(bool enabled);
    static bool IsFastFrankEnabled();
    static void ApplyFastFrank(void* statsObject);
    static void SetFastFrankSpeed(int speedLevel); // 0-4: (0=speed 4, 1=speed 7, 2=speed 10, 3=speed 12, 4=speed 14)
    
    // Check if player exists (in-game)
    static bool IsPlayerActive()
    {
        return uPlayerInstance != nullptr;
    }
    
    // ═══════════════════════════════════════════
    //  HEALTH MANAGEMENT
    // ═══════════════════════════════════════════
    
    // Kill the player instantly
    static void KillPlayer();
    static void TestDamageSimulation();
    
    // Set player health to a specific value
    static void SetHealth(int health);
    
    // Get current player health
    static int GetHealth();
    
    // Check if player is alive (health > 0)
    static bool IsAlive();
    
    // Debug: print health information
    static void PrintHealthDebug();
};