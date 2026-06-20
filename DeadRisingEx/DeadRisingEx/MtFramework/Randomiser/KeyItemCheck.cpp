#include "KeyItemCheck.h"
#include <windows.h>
#include <detours.h>
#include <cstdint>
#include <stdio.h>
#include <unordered_set>
#include "Misc/AsmHelpers.h"
#include "InputSystem.h"
#include "Checks/CheckSystem.h"

namespace KeyItemCheck
{
    // Event ID → check ID mapping.
    // Fill in event IDs for uOm0081/0084/00de once confirmed in-game.
    const KeyEventEntry kKeyEvents[] =
    {
        { 0x822, 0 },   // uOm0028 — MT key (confirmed)
        //{ 0x883, 0 },   // Radio and map
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

    static std::unordered_set<uint32_t> s_seenEvents;

    static void __stdcall Hook_GameEvent(int64_t manager, uint32_t event_id)
    {
        // Capture first non-zero manager for use by KeyItemReward / fire_event.
        // Many events fire with manager=0 (non-item events), so only store non-zero.
        if (s_manager == 0 && manager != 0)
            s_manager = manager;

        if (s_seenEvents.find(event_id) == s_seenEvents.end())
        {
            s_seenEvents.insert(event_id);
            char buf[64];
            sprintf_s(buf, "[KEYITEM] event=0x%X", event_id);
            LogLine(buf);
        }

        // Key items are reward-only — suppress physical pickup so the MT key
        // can only be received as a randomiser reward via KeyItemReward.
        if (IsKeyEvent(event_id))
            return;

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
