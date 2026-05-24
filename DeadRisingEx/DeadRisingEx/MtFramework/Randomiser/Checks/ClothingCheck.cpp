#include <windows.h>
#include "detours.h"
#include <cstdint>
#include "DeadRisingEx/Utilities/DebugLog.h"
#include "MtFramework/Game/sMain.h"
#include "ClothingCheck.h"
#include "DeadRisingEx/MtFramework/Randomiser/Checks/CheckSystem.h"
#include "../InputSystem.h"

namespace ClothingCheck
{
    // ═══════════════════════════════════════════
    //  VARIABLES
    // ═══════════════════════════════════════════

    void(__fastcall* originalCostumePickup)(void*, void*) =
        (void(__fastcall*)(void*, void*))GetModuleAddress(0x14005e4f0);

    static void* s_costumeStateMachineObj = nullptr;

    // ── Achievement-only costumes (rewards only, never checks) ──
    static const int s_achievementCostumes[] = {
        27, 34, 35, 36, 37, 39,  // slot 0
        303, 305, 307             // slot 3
    };

    static bool IsAchievementCostume(int checkId)
    {
        for (int id : s_achievementCostumes)
            if (id == checkId) return true;
        return false;
    }

    // ═══════════════════════════════════════════
    //  GETTERS / SETTERS
    // ═══════════════════════════════════════════

    void* GetStateMachineObj()          { return s_costumeStateMachineObj; }
    void  SetStateMachineObj(void* obj) { s_costumeStateMachineObj = obj;  }

    // ═══════════════════════════════════════════
    //  HOOKS
    // ═══════════════════════════════════════════

    void __fastcall Hook_CostumePickup(void* player, void* costumeItem)
    {
        uint8_t slot    = *(uint8_t*)((uintptr_t)costumeItem + 0x708);
        uint8_t id      = *(uint8_t*)((uintptr_t)costumeItem + 0x70c);
        int     checkId = slot * 100 + id;

        if (!IsAchievementCostume(checkId))
        {
            char buf[64];
            sprintf_s(buf, "[COSTUME] Firing check %d", checkId);
            LogLine(buf);

            CheckSystem::CompleteCheck(CheckType::Clothing, (uint32_t)checkId);

            // Don't call originalCostumePickup — costume change is handled by reward system only
            return;
        }

        originalCostumePickup(player, costumeItem);
    }

    // ═══════════════════════════════════════════
    //  REGISTRATION
    // ═══════════════════════════════════════════

    void InitializeHooks()
    {
        DetourAttach((void**)&originalCostumePickup, Hook_CostumePickup);
        LogLine("[ClothingCheck] Hooks registered");
    }
}