#include <windows.h>
#include "detours.h"
#include <cstdint>
#include "DeadRisingEx/Utilities/DebugLog.h"
#include "MtFramework/Game/sMain.h"
#include "CostumeCheck.h"
#include "DeadRisingEx/MtFramework/Randomiser/Checks/CheckSystem.h"
#include "DeadRisingEx/MtFramework/Randomiser/Checks/CheckAvailability.h"
#include "DeadRisingEx/MtFramework/Randomiser/AreaKeySystem.h"
#include "../InputSystem.h"
#include "../AreaTransitionHook.h"

namespace CostumeCheck
{
    // ═══════════════════════════════════════════
    //  VARIABLES
    // ═══════════════════════════════════════════

    void(__fastcall* originalCostumePickup)(void*, void*) =
        (void(__fastcall*)(void*, void*))GetModuleAddress(0x14005e4f0);

    static void* s_costumeStateMachineObj = nullptr;

    static const int s_achievementCostumes[] = {
        // Outfits (slot 0)
        27,   // Mega Man Torso
        32,   // Ammo Belt
        35,   // Arthur's Boxers
        36,   // Prisoner Outfit
        37,   // Mall Employee Uniform
        41,   // Special Forces Uniform
        42,   // Pro Wrestling Briefs
        // Shoes (slot 1)
        107,  // Special Forces Boots
        108,  // Mega Man Boots
        109,  // Pro Wrestling Boots
        // Head (slot 2)
        211,  // White Hat
        219,  // Cop Hat
        // Accessories (slot 3)
        307,  // Hockey Mask
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
        uint8_t  slot      = *(uint8_t*)((uintptr_t)costumeItem + 0x708);
        uint8_t  id        = *(uint8_t*)((uintptr_t)costumeItem + 0x70c);
        uint32_t costumeId = (uint32_t)(slot * 100 + id);
        ZoneID   zone      = AreaKeySystem::ZoneFromAreaId(AreaTransitionHook::GetCurrentAreaId());

        LogCostume(slot, id, (int)costumeId);

        if (!IsAchievementCostume((int)costumeId))
        {
            uint32_t checkId = 0;
            const std::vector<uint32_t>* checks = CheckAvailability::GetCostumeCheckIds(costumeId, zone);
            if (checks)
            {
                for (uint32_t cid : *checks)
                {
                    if (!CheckSystem::IsCompleted(CheckType::Costume, cid))
                    {
                        checkId = cid;
                        break;
                    }
                }
            }

            char buf[96];
            sprintf_s(buf, "[COSTUME] slot=%d id=%d costumeId=%d zone=%d checkId=%d",
                      slot, id, costumeId, (int)zone, checkId);
            LogLine(buf);

            if (checkId != 0)
                CheckSystem::CompleteCheck(CheckType::Costume, checkId);
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
        LogLine("[CostumeCheck] Hooks registered");
    }
}