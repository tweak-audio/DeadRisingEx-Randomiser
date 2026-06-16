#pragma once
#include <cstdint>

// uOm0028 (MT key), uOm0081, uOm0084, uOm00de
constexpr int TOTAL_KEY_ITEMS = 4;

namespace KeyItemCheck
{
    struct KeyEventEntry { uint32_t eventId; uint32_t checkId; };
    extern const KeyEventEntry kKeyEvents[];
    extern const int           kKeyEventCount;

    // Set once the hook fires — needed by KeyItemReward to call the original
    extern void (__stdcall* originalGameEvent)(int64_t, uint32_t);
    extern int64_t s_manager;

    void Install();
}
