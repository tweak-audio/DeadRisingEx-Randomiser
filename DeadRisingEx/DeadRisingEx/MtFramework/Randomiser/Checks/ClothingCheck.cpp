#include <windows.h>
#include "detours.h"
#include <cstdint>
#include "DeadRisingEx/Utilities/DebugLog.h"
#include "MtFramework/Game/sMain.h"
#include "ClothingCheck.h"
#include "../InputSystem.h"

namespace ClothingCheck
{
    // ═══════════════════════════════════════════
    //  VARIABLES
    // ═══════════════════════════════════════════

    void(__fastcall* originalCostumePickup)(void*, void*) =
        (void(__fastcall*)(void*, void*))GetModuleAddress(0x14005e4f0);

    static void* s_costumeStateMachineObj = nullptr;

    // ═══════════════════════════════════════════
    //  GETTERS / SETTERS
    // ═══════════════════════════════════════════

    void* GetStateMachineObj()         { return s_costumeStateMachineObj; }
    void  SetStateMachineObj(void* obj) { s_costumeStateMachineObj = obj; }

    // ═══════════════════════════════════════════
    //  HOOKS
    // ═══════════════════════════════════════════

    void __fastcall Hook_CostumePickup(void* player, void* costumeItem)
    {
        uint8_t slotIndex = *(uint8_t*)((uintptr_t)costumeItem + 0x708);
        uint8_t costumeId = *(uint8_t*)((uintptr_t)costumeItem + 0x70c);

        char buf[128];
        sprintf_s(buf, "[COSTUME CHECK] slot=%d id=%d", slotIndex, costumeId);
        LogLine(buf);

        // TODO: fire randomiser check with (slotIndex * 100 + costumeId) as check ID

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