

#pragma once

#include <Windows.h>
#include <stdint.h>

#define XP_OFFSET    0x50
#define LEVEL_OFFSET 0x68
#define MAX_LEVEL    50

template<typename T>
T Read(uint8_t* base, int offset)
{
    return *(T*)(base + offset);
}

template<typename T>
void Write(uint8_t* base, int offset, T value)
{
    *(T*)(base + offset) = value;
}

// Public API
void GrantLevels(int count);
void GrantLevelViaXP(int count);
void PrintPlayerStats();
void ProcessPendingRewards();

extern void* g_statsObject;
extern bool  g_statsResolved;

// Hook functions
void __fastcall Hook_SetPlayerLevel(void* playerObj, int level);
void __fastcall Hook_LevelUpCallback(void* param_1);
int  __fastcall Hook_XPAccumulator(void* param_1, unsigned int param_2, int param_3, int param_4, int param_5);

// Function pointers
extern void(__fastcall* SetPlayerLevel)(void* playerObj, int level);

typedef void(__fastcall* LevelUpCallback_t)(void* param_1);
extern LevelUpCallback_t originalLevelUpCallback;

typedef int(__fastcall* XPAccumulator_t)(void* param_1, unsigned int param_2, int param_3, int param_4, int param_5);
extern XPAccumulator_t originalXPAccumulator;