#include "ClothingReward.h"
#include "DeadRisingEx/MtFramework/Randomiser/Checks/ClothingCheck.h"
#include "../InputSystem.h"
#include <windows.h>
#include <cstdint>
#include "DeadRisingEx/Utilities/DebugLog.h"
#include "MtFramework/Game/sMain.h"

namespace ClothingReward
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
        void* obj = ClothingCheck::GetStateMachineObj();
        if (obj == nullptr)
        {
            LogLine("[CLOTHING REWARD] No state machine obj yet - pick up a costume first");
            return;
        }

        uintptr_t costumeState = (uintptr_t)obj;
        *(uint32_t*)(costumeState + 0x14a4 + slot * 4) = costumeId;
        ApplyCostumeSlot(obj, slot);

        char buf[128];
        sprintf_s(buf, "[CLOTHING REWARD] Gave slot=%d id=%d", slot, costumeId);
        LogLine(buf);
    }
}