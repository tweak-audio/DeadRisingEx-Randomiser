#include <windows.h>
#include "detours.h"
#include <cstdint>
#include "DeadRisingEx/Utilities/DebugLog.h"
#include "MtFramework/Game/sMain.h"
#include "ClothingCheck.h"
#include "DeadRisingEx/MtFramework/Randomiser/Checks/CheckSystem.h"
#include "../InputSystem.h"
#include "../AreaTransitionHook.h"

namespace ClothingCheck
{
    // ═══════════════════════════════════════════
    //  VARIABLES
    // ═══════════════════════════════════════════

    void(__fastcall* originalCostumePickup)(void*, void*) =
        (void(__fastcall*)(void*, void*))GetModuleAddress(0x14005e4f0);

    static void* s_costumeStateMachineObj = nullptr;

    static const int s_achievementCostumes[] = {
        27, 34, 35, 36, 37, 39,
        303, 305, 307
    };

    static bool IsAchievementCostume(int checkId)
    {
        for (int id : s_achievementCostumes)
            if (id == checkId) return true;
        return false;
    }

    // ═══════════════════════════════════════════
    //  COSTUME LOG
    // ═══════════════════════════════════════════

    static void LogCostume(uint8_t slot, uint8_t id, int checkId)
    {
        static bool s_headerWritten = false;

        FILE* f = nullptr;
        fopen_s(&f, "costume_log.txt", "a");
        if (!f) return;

        if (!s_headerWritten)
        {
            fprintf(f, "%-10s %-10s %-10s %-10s\n", "checkId", "slot", "id", "areaId");
            fprintf(f, "---------- ---------- ---------- ----------\n");
            s_headerWritten = true;
        }

        uint32_t areaId = AreaTransitionHook::GetCurrentAreaId();
        fprintf(f, "%-10d %-10d %-10d 0x%-8X\n", checkId, slot, id, areaId);
        fclose(f);
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

        LogCostume(slot, id, checkId);

        if (!IsAchievementCostume(checkId))
        {
            char buf[64];
            sprintf_s(buf, "[COSTUME] slot=%d id=%d checkId=%d", slot, id, checkId);
            LogLine(buf);

            CheckSystem::CompleteCheck(CheckType::Clothing, (uint32_t)checkId);
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