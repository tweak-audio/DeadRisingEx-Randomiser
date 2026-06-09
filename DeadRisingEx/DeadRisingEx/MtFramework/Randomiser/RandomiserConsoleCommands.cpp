#include "DeadRisingEx/MtFramework/Rendering/ImGui/ImGuiConsole.h"
#include "DeadRisingEx/MtFramework/Randomiser/Checks/CheckSystem.h"
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
//  Registration
// ─────────────────────────────────────────────────────────────────────────────

static const ConsoleCommandInfo g_RandomiserCommands[] =
{
    { L"randomiser_seed",   L"Reroll to a new random seed, or 'randomiser_seed <n>' to use a specific seed", Cmd_RandomiserSeed   },
    { L"randomiser_status", L"Print current seed and check progress",                                         Cmd_RandomiserStatus },
};

void RegisterRandomiserConsoleCommands()
{
    ImGuiConsole::Instance()->RegisterCommands(g_RandomiserCommands, ARRAYSIZE(g_RandomiserCommands));
}
