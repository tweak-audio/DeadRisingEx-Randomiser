#include "CameraRefillReward.h"
#include "DeadRisingEx/MtFramework/Player/uPlayerImpl.h"
#include "DeadRisingEx/MtFramework/Randomiser/InputSystem.h"
#include "DeadRisingEx/Utilities/DebugLog.h"
#include <cstdint>

// Confirmed from reverse engineering in Ghidra:
// Static base: DeadRising.exe + 0x1CF2AA0 (DAT_141cf2aa0)
// First pointer: [base + 0x20DC0] -> Player Object
// Second offset: [player + 0x9340] -> Camera Battery (int32)

static constexpr uintptr_t GAME_MANAGER_OFFSET = 0x1CF2AA0;  // Static address in module
static constexpr uintptr_t PLAYER_PTR_OFFSET = 0x20DC0;      // Offset to player pointer
static constexpr uintptr_t CAMERA_BATTERY_OFFSET = 0x9340;   // Offset to battery in player object
static constexpr int MAX_BATTERY = 30;

void CameraRefillReward::Initialize()
{
    LogLine("[CameraRefill] System initialized");
    LogLine("[CameraRefill] Pointer chain: [[[DeadRising.exe+1CF2AA0]+20DC0]+9340]");
    LogLine("[CameraRefill] Max battery: 30");
}

static int* GetBatteryPointer()
{
    // Get module base address (DeadRising.exe)
    uintptr_t moduleBase = (uintptr_t)GetModuleHandleA(nullptr);
    if (!moduleBase)
    {
        LogLine("[CameraRefill] ERROR: Could not get module base");
        return nullptr;
    }

    // Step 1: Read game manager static address
    uintptr_t* gameManager = (uintptr_t*)(moduleBase + GAME_MANAGER_OFFSET);
    if (!gameManager || !*gameManager)
    {
        LogLine("[CameraRefill] ERROR: Game manager pointer is NULL");
        return nullptr;
    }

    // Step 2: Read player object pointer
    uintptr_t* playerPtr = (uintptr_t*)(*gameManager + PLAYER_PTR_OFFSET);
    if (!playerPtr || !*playerPtr)
    {
        LogLine("[CameraRefill] ERROR: Player object pointer is NULL");
        return nullptr;
    }

    // Step 3: Get battery address
    int* battery = (int*)(*playerPtr + CAMERA_BATTERY_OFFSET);
    
    return battery;
}

void CameraRefillReward::RefillFilm(int amount)
{
    LogLine("[CameraRefill] RefillFilm called");

    int* battery = GetBatteryPointer();
    if (!battery)
    {
        LogLine("[CameraRefill] ERROR: battery pointer invalid");
        return;
    }

    int oldValue = *battery;
    *battery = MAX_BATTERY;

    char buf[128];
    sprintf_s(buf, "[CameraRefill] Battery refilled: %d -> %d", oldValue, MAX_BATTERY);
    LogLine(buf);
}

void CameraRefillReward::SetFilmCount(int count)
{
    LogLine("[CameraRefill] SetFilmCount called");

    int* battery = GetBatteryPointer();
    if (!battery)
    {
        LogLine("[CameraRefill] ERROR: battery pointer invalid");
        return;
    }

    if (count > MAX_BATTERY) count = MAX_BATTERY;
    if (count < 0) count = 0;

    int old = *battery;
    *battery = count;

    char buf[128];
    sprintf_s(buf, "[CameraRefill] Battery set: %d -> %d", old, count);
    LogLine(buf);
}

int CameraRefillReward::GetCurrentFilmCount()
{
    int* battery = GetBatteryPointer();
    if (!battery)
    {
        LogLine("[CameraRefill] ERROR: Could not read battery");
        return -1;
    }

    return *battery;
}


//-------Debugging-------
void CameraRefillReward::ScanPlayerForCamera()
{
    LogLine("[CameraRefill] === BATTERY DEBUG ===");

    uintptr_t moduleBase = (uintptr_t)GetModuleHandleA(nullptr);
    char buf[256];
    
    sprintf_s(buf, "[CameraRefill] Module base: %p", (void*)moduleBase);
    LogLine(buf);

    uintptr_t* gameManager = (uintptr_t*)(moduleBase + GAME_MANAGER_OFFSET);
    sprintf_s(buf, "[CameraRefill] Game manager address: %p", (void*)gameManager);
    LogLine(buf);

    if (!gameManager || !*gameManager)
    {
        LogLine("[CameraRefill] Game manager is NULL - game not loaded yet?");
        return;
    }

    sprintf_s(buf, "[CameraRefill] Game manager value: %p", (void*)*gameManager);
    LogLine(buf);

    uintptr_t* playerPtr = (uintptr_t*)(*gameManager + PLAYER_PTR_OFFSET);
    sprintf_s(buf, "[CameraRefill] Player ptr address: %p", (void*)playerPtr);
    LogLine(buf);

    if (!playerPtr || !*playerPtr)
    {
        LogLine("[CameraRefill] Player object is NULL - player not spawned?");
        return;
    }

    sprintf_s(buf, "[CameraRefill] Player object: %p", (void*)*playerPtr);
    LogLine(buf);

    int* battery = (int*)(*playerPtr + CAMERA_BATTERY_OFFSET);
    sprintf_s(buf, "[CameraRefill] Battery address: %p", (void*)battery);
    LogLine(buf);

    sprintf_s(buf, "[CameraRefill] Battery value: %d", *battery);
    LogLine(buf);

    LogLine("[CameraRefill] === END DEBUG ===");
}