#include "DeadRisingEx/MtFramework/Rendering/ImGui/ImGuiConsole.h"
#include "DeadRisingEx/MtFramework/Randomiser/Checks/CheckSystem.h"
#include "DeadRisingEx/MtFramework/Randomiser/TimeManager.h"
#include "DeadRisingEx/MtFramework/Randomiser/KeyItemCheck.h"
#include "DeadRisingEx/MtFramework/Randomiser/InputSystem.h"
#include <stdio.h>

// ─────────────────────────────────────────────────────────────────────────────
//  randomiser_seed [n]
//
//  No argument  → find a new guaranteed-beatable random seed (same as startup).
//  With argument → force that exact seed (warns if unbeatable).
// ─────────────────────────────────────────────────────────────────────────────

static __int64 Cmd_RandomiserSeed(WCHAR** argv, int argc)
{
    if (!CheckSystem::IsReady())
    {
        ImGuiConsole::Instance()->ConsolePrint(L"[RANDOMISER] Not ready yet — wait for initialisation to complete.\n");
        return 0;
    }

    if (argc >= 1)
    {
        uint32_t seed = (uint32_t)_wtoi(argv[0]);
        if (seed == 0)
        {
            ImGuiConsole::Instance()->ConsolePrint(L"[RANDOMISER] Seed must be non-zero. Use no argument to reroll randomly.\n");
            return 0;
        }

        CheckSystem::SetSeed(seed);

        wchar_t buf[128];
        if (!CheckSystem::IsSeedBeatable(/*quiet=*/true))
        {
            swprintf_s(buf, L"[RANDOMISER] Seed %u set. WARNING: this seed may be unbeatable!\n", seed);
        }
        else
        {
            swprintf_s(buf, L"[RANDOMISER] Seed %u set.\n", seed);
        }
        ImGuiConsole::Instance()->ConsolePrint(buf);
    }
    else
    {
        ImGuiConsole::Instance()->ConsolePrint(L"[RANDOMISER] Searching for beatable seed...\n");
        CheckSystem::GenerateBeatableSeed();

        wchar_t buf[64];
        swprintf_s(buf, L"[RANDOMISER] Seed set to: %u\n", CheckSystem::GetSeed());
        ImGuiConsole::Instance()->ConsolePrint(buf);
    }

    return 0;
}

// ─────────────────────────────────────────────────────────────────────────────
//  randomiser_status
//
//  Prints the current seed, check progress, and config snapshot.
// ─────────────────────────────────────────────────────────────────────────────

static __int64 Cmd_RandomiserStatus(WCHAR** argv, int argc)
{
    if (!CheckSystem::IsReady())
    {
        ImGuiConsole::Instance()->ConsolePrint(L"[RANDOMISER] Not initialised yet.\n");
        return 0;
    }

    wchar_t buf[256];
    swprintf_s(buf, L"[RANDOMISER] Seed: %u | Checks: %d / %d\n",
        CheckSystem::GetSeed(),
        CheckSystem::GetCompletedChecks(),
        CheckSystem::GetTotalChecks());
    ImGuiConsole::Instance()->ConsolePrint(buf);
    return 0;
}

// ─────────────────────────────────────────────────────────────────────────────
//  time_speed [0.5 | 1 | 2]
//
//  No argument  → print current speed multiplier.
//  0.5          → half speed.
//  1 / 1.0      → normal speed.
//  2 / 2.0      → double speed.
// ─────────────────────────────────────────────────────────────────────────────

static __int64 Cmd_TimeSpeed(WCHAR** argv, int argc)
{
    if (argc < 1)
    {
        wchar_t buf[64];
        swprintf_s(buf, L"[TIME] Current speed: %.2fx\n", TimeManager::GetSpeedMultiplier());
        ImGuiConsole::Instance()->ConsolePrint(buf);
        return 0;
    }

    float mult = (float)_wtof(argv[0]);
    if (mult <= 0.0f)
    {
        ImGuiConsole::Instance()->ConsolePrint(L"[TIME] Speed must be greater than 0. Try 0.5, 1, or 2.\n");
        return 0;
    }

    TimeManager::SetSpeedMultiplier(mult);

    wchar_t buf[64];
    swprintf_s(buf, L"[TIME] Speed set to %.2fx\n", mult);
    ImGuiConsole::Instance()->ConsolePrint(buf);
    return 0;
}

// ─────────────────────────────────────────────────────────────────────────────
//  fire_event <hex_id>
//
//  Fires any game event by hex ID directly through the event handler.
//  Useful for testing what a given event ID does in-game.
//  Example: fire_event 0x2E6
// ─────────────────────────────────────────────────────────────────────────────

static __int64 Cmd_FireEvent(WCHAR** argv, int argc)
{
    if (argc < 1)
    {
        ImGuiConsole::Instance()->ConsolePrint(L"[EVENT] Usage: fire_event <hex_id>  e.g. fire_event 0x2E6\n");
        return 0;
    }

    uint32_t eventId = (uint32_t)wcstoul(argv[0], nullptr, 0);

    QueueGameEvent(eventId);

    wchar_t buf[64];
    swprintf_s(buf, L"[EVENT] Queued event 0x%X — will fire next game tick\n", eventId);
    ImGuiConsole::Instance()->ConsolePrint(buf);
    return 0;
}

// ─────────────────────────────────────────────────────────────────────────────
//  Registration
// ─────────────────────────────────────────────────────────────────────────────

static const ConsoleCommandInfo g_RandomiserCommands[] =
{
    { L"randomiser_seed",   L"Reroll to a new random seed, or 'randomiser_seed <n>' to use a specific seed", Cmd_RandomiserSeed   },
    { L"randomiser_status", L"Print current seed and check progress",                                         Cmd_RandomiserStatus },
    { L"time_speed",        L"Set game time speed: 0.5 (half), 1 (normal), 2 (double). No arg = show current", Cmd_TimeSpeed     },
    { L"fire_event",        L"Fire a game event by hex ID for testing. Usage: fire_event <hex_id>",            Cmd_FireEvent        },
};

void RegisterRandomiserConsoleCommands()
{
    ImGuiConsole::Instance()->RegisterCommands(g_RandomiserCommands, ARRAYSIZE(g_RandomiserCommands));
}
