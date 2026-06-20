#include "KeyItemCheck.h"
#include <windows.h>
#include <detours.h>
#include <cstdint>
#include <stdio.h>
#include "Misc/AsmHelpers.h"
#include "InputSystem.h"
#include "Checks/CheckSystem.h"

namespace KeyItemCheck
{
    // Event ID → check ID mapping.
    // Fill in event IDs for uOm0081/0084/00de once confirmed in-game.
    const KeyEventEntry kKeyEvents[] =
    {
        { 0x822, 0 },   // uOm0028 — MT key
        // { 0xTBD, 1 },  // uOm0081
        // { 0xTBD, 2 },  // uOm0084
        // { 0xTBD, 3 },  // uOm00de
    };
    const int kKeyEventCount = sizeof(kKeyEvents) / sizeof(kKeyEvents[0]);

    // Exposed for KeyItemReward.cpp
    void (__stdcall* originalGameEvent)(int64_t, uint32_t) = nullptr;
    int64_t s_manager = 0;

    constexpr uintptr_t ADDR_GAME_EVENT = 0x140014500;

    static bool IsKeyEvent(uint32_t event_id)
    {
        for (int i = 0; i < kKeyEventCount; i++)
            if (kKeyEvents[i].eventId == event_id) return true;
        return false;
    }

    static void __stdcall Hook_GameEvent(int64_t manager, uint32_t event_id)
    {
        // Capture manager on first call for use by KeyItemReward
        if (s_manager == 0)
            s_manager = manager;

        if (IsKeyEvent(event_id))
        {
            char buf[64];
            sprintf_s(buf, "[KEYITEM] event=0x%X", event_id);
            LogLine(buf);

            for (int i = 0; i < kKeyEventCount; i++)
            {
                if (kKeyEvents[i].eventId == event_id)
                {
                    CheckSystem::CompleteCheck(CheckType::KeyItem, kKeyEvents[i].checkId);
                    break;
                }
            }
            return;
        }

        originalGameEvent(manager, event_id);
    }

    void Install()
    {
        originalGameEvent = (void(__stdcall*)(int64_t, uint32_t))GetModuleAddress(ADDR_GAME_EVENT);

        DetourTransactionBegin();
        DetourUpdateThread(GetCurrentThread());
        DetourAttach((void**)&originalGameEvent, (void*)Hook_GameEvent);
        LONG err = DetourTransactionCommit();

        if (err != NO_ERROR)
        {
            char buf[64];
            sprintf_s(buf, "[KEYITEM] ERROR: DetourTransactionCommit failed (%ld)", err);
            LogLine(buf);
        }
        else
        {
            LogLine("[KEYITEM] Hook installed on FUN_140014500");
        }
    }
}
