/*

*/

#pragma once
#include "DeadRisingEx.h"

class FastFrank
{
public:
    // Enable/disable Fast Frank
    static void SetEnabled(bool enabled);
    static bool IsEnabled();
    
    // Set speed level (0-4: 0=speed 4, 1=speed 7, 2=speed 10, 3=speed 12, 4=speed 14)
    static void SetSpeedLevel(int speedLevel);
    static int GetSpeedLevel();
    
    // Apply Fast Frank to player stats object
    static void ApplyToStatsObject(void* statsObject);
    
    // Called during player construction to set initial speed
    static void OnPlayerConstruct(void* playerInstance);
    
    // Hook initialization
    static void InitializeHooks();
    
    // Hook functions (public so they can be registered)
    static void* Hook_SetRunLevel(void* thisptr, int level);
};