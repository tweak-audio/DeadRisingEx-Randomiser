#include "CostumeReward.h"
#include "DeadRisingEx/MtFramework/Randomiser/Checks/CostumeCheck.h"
#include "../InputSystem.h"
#include <windows.h>
#include <cstdint>
#include "DeadRisingEx/Utilities/DebugLog.h"
#include "MtFramework/Game/sMain.h"

namespace CostumeReward
{
    // ═══════════════════════════════════════════
    //  FUNCTION POINTERS
    // ═══════════════════════════════════════════

    typedef void(__fastcall* ApplyCostumeSlotFunc)(void*, uint8_t);
    ApplyCostumeSlotFunc ApplyCostumeSlot =
        (ApplyCostumeSlotFunc)GetModuleAddress(0x1401f7b80);

    // ═══════════════════════════════════════════
    //  PUBLIC API
    // ═══════════════════════════════════════════

    void GiveCostume(uint8_t slot, uint8_t costumeId)
    {
        void* obj = CostumeCheck::GetStateMachineObj();
        if (obj == nullptr)
        {
            LogLine("[COSTUME REWARD] No state machine obj — aborting");
            return;
        }

        uintptr_t writeAddr = (uintptr_t)obj + 0x14a4 + slot * 4;
        *(uint32_t*)writeAddr = costumeId;
        ApplyCostumeSlot(obj, slot);
    }
}