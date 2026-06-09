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
        char buf[256];

        void* obj = CostumeCheck::GetStateMachineObj();
        sprintf_s(buf, "[COSTUME REWARD] GiveCostume slot=%d id=%d obj=%p", slot, costumeId, obj);
        LogLine(buf);

        if (obj == nullptr)
        {
            LogLine("[COSTUME REWARD] No state machine obj — aborting");
            return;
        }

        uintptr_t costumeState = (uintptr_t)obj;
        uintptr_t writeAddr    = costumeState + 0x14a4 + slot * 4;
        sprintf_s(buf, "[COSTUME REWARD] Writing id=%d to addr=%p (obj+0x%X)",
                  costumeId, (void*)writeAddr, (uint32_t)(0x14a4 + slot * 4));
        LogLine(buf);

        *(uint32_t*)writeAddr = costumeId;
        LogLine("[COSTUME REWARD] Memory write OK");

        sprintf_s(buf, "[COSTUME REWARD] Calling ApplyCostumeSlot(obj=%p, slot=%d)", obj, slot);
        LogLine(buf);

        ApplyCostumeSlot(obj, slot);
        LogLine("[COSTUME REWARD] ApplyCostumeSlot returned");

        sprintf_s(buf, "[COSTUME REWARD] Gave slot=%d id=%d", slot, costumeId);
        LogLine(buf);
    }
}