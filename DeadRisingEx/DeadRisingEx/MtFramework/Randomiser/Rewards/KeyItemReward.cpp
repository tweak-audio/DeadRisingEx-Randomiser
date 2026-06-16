#include "KeyItemReward.h"
#include "../KeyItemCheck.h"
#include "DeadRisingEx/MtFramework/Randomiser/InputSystem.h"
#include <stdio.h>

namespace KeyItemReward
{
    struct KeyInfo { uint32_t keyId; uint32_t eventId; const char* name; };
    static constexpr KeyInfo kKeyInfo[] =
    {
        { 0, 0x822, "MT Key" },     // uOm0028
        // { 1, 0xTBD, "Key uOm0081" },
        // { 2, 0xTBD, "Key uOm0084" },
        // { 3, 0xTBD, "Key uOm00de" },
    };

    const char* GrantKeyItem(uint32_t keyId)
    {
        for (const auto& info : kKeyInfo)
        {
            if (info.keyId != keyId) continue;

            if (KeyItemCheck::originalGameEvent && KeyItemCheck::s_manager)
            {
                KeyItemCheck::originalGameEvent(KeyItemCheck::s_manager, info.eventId);

                char buf[64];
                sprintf_s(buf, "[KEYITEM] Granted %s (event=0x%X)", info.name, info.eventId);
                LogLine(buf);
            }
            else
            {
                LogLine("[KEYITEM] WARNING: GrantKeyItem called before hook fired");
            }

            return info.name;
        }

        char buf[64];
        sprintf_s(buf, "[KEYITEM] WARNING: Unknown keyId %u", keyId);
        LogLine(buf);
        return nullptr;
    }
}
