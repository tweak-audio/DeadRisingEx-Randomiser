#include "AreaTransitionHook.h"
#include "AreaKeySystem.h"
#include "Misc/AsmHelpers.h"
#include "DeadRisingEx/Utilities/DebugLog.h"
#include "InputSystem.h"
#include <MtFramework/Game/sSnatcherMain.h>
#include <detours.h>
#include <stdio.h>

AreaTransitionHook::FnAreaHitProcess  AreaTransitionHook::s_originalAreaHitProcess  = nullptr;
AreaTransitionHook::FnAreaChange      AreaTransitionHook::s_originalAreaChange      = nullptr;
AreaTransitionHook::FnTransitionStart AreaTransitionHook::s_originalTransitionStart = nullptr;

static uint32_t s_previousArea = 0;

static void LogTransition(const char* msg)
{
    static char path[MAX_PATH] = { 0 };
    if (path[0] == 0)
    {
        GetModuleFileNameA(NULL, path, MAX_PATH);
        char* lastSlash = strrchr(path, '\\');
        if (lastSlash) *(lastSlash + 1) = 0;
        strcat_s(path, MAX_PATH, "transition_log.txt");
    }
    FILE* f = fopen(path, "a");
    if (f)
    {
        SYSTEMTIME t;
        GetLocalTime(&t);
        fprintf(f, "[%02d:%02d:%02d] %s\n", t.wHour, t.wMinute, t.wSecond, msg);
        fclose(f);
    }
}

void AreaTransitionHook::Install()
{
    s_originalAreaHitProcess  = reinterpret_cast<FnAreaHitProcess> (GetModuleAddress(0x14008fb70));
    s_originalAreaChange      = reinterpret_cast<FnAreaChange>     (GetModuleAddress(0x1400628f0));
    s_originalTransitionStart = reinterpret_cast<FnTransitionStart>(GetModuleAddress(0x140209d00));

    DetourTransactionBegin();
    DetourUpdateThread(GetCurrentThread());
    DetourAttach(&(PVOID&)s_originalAreaHitProcess,  Hook_AreaHitProcess);
    DetourAttach(&(PVOID&)s_originalAreaChange,      Hook_AreaChange);
    DetourAttach(&(PVOID&)s_originalTransitionStart, Hook_TransitionStart);
    LONG err = DetourTransactionCommit();

    if (err != NO_ERROR)
    {
        char buf[64];
        snprintf(buf, sizeof(buf), "AreaTransitionHook: Install failed %d\n", err);
        LogTransition(buf);
    }
    else
    {
        LogTransition("AreaTransitionHook: Installed successfully\n");
    }
}

// ─── Hook: AreaHitProcess (FUN_14008fb70) ────────────────────────────────────

void __fastcall AreaTransitionHook::Hook_AreaHitProcess(
    int64_t param_1, uint16_t param_2, int16_t param_3, uint32_t param_4)
{
    uintptr_t disabledActor = 0;

    if (AreaKeySystem::Get().IsIntroComplete() && param_3 > 0)
    {
        uint32_t dest = static_cast<uint16_t>(param_3);
        ZoneID zone = AreaKeySystem::ZoneFromAreaId(dest);
        if (zone != ZoneID::COUNT && !AreaKeySystem::Get().HasKey(dest))
        {
            int count = *(int*)(param_1 + 0x48);
            uintptr_t actorList = *(uintptr_t*)(param_1 + 0x58);
            for (int i = 0; i < count; i++)
            {
                uintptr_t actor = *(uintptr_t*)(actorList + i * 8);
                if (actor &&
                    *(uint32_t*)(actor + 0x44) == 4 &&
                    *(uint32_t*)(actor + 0x14) == dest)
                {
                    *(uint32_t*)(actor + 0x44) = 9;
                    disabledActor = actor;
                    break;
                }
            }
        }
    }

    s_originalAreaHitProcess(param_1, param_2, param_3, param_4);

    if (disabledActor)
        *(uint32_t*)(disabledActor + 0x44) = 4;
}

// ─── Hook: TransitionStart (FUN_140209d00) ───────────────────────────────────

void __fastcall AreaTransitionHook::Hook_TransitionStart(int64_t param_1)
{
    if (AreaKeySystem::Get().IsIntroComplete())
    {
        uintptr_t areaMgrPtr = reinterpret_cast<uintptr_t>(GetModuleAddress(0x141945f70));
        uintptr_t areaMgr    = areaMgrPtr ? *reinterpret_cast<uintptr_t*>(areaMgrPtr) : 0;
        if (areaMgr)
        {
            uint32_t dest = *(uint32_t*)(areaMgr + 0x48);
            ZoneID zone = AreaKeySystem::ZoneFromAreaId(dest);
            if (zone != ZoneID::COUNT && !AreaKeySystem::Get().HasKey(dest))
                return;
        }
    }
    s_originalTransitionStart(param_1);
}

// ─── Hook: AreaChange (FUN_1400628f0) ────────────────────────────────────────

void __fastcall AreaTransitionHook::Hook_AreaChange(int64_t param_1, uint32_t areaId)
{
    if (areaId == 0xffffffff)
    {
        s_originalAreaChange(param_1, areaId);
        return;
    }

    if (!AreaKeySystem::Get().IsIntroComplete())
    {
        if (s_previousArea == 0x216 && areaId == 0x200)
        {
            AreaKeySystem::Get().SetIntroComplete(true);
            AreaKeySystem::Get().GiveKey(ZoneID::ParadisePlaza);
            LogTransition("INTRO COMPLETE: key system now active, Paradise Plaza unlocked");
        }
        s_previousArea = areaId;
        s_originalAreaChange(param_1, areaId);
        return;
    }

    if (!AreaKeySystem::Get().HasKey(areaId))
    {
        char buf[64];
        snprintf(buf, sizeof(buf), "Hook_AreaChange: BLOCKED 0x%x -> reloading 0x%x",
            areaId, s_previousArea);
        LogTransition(buf);

        s_originalAreaChange(param_1, s_previousArea);

        // Only attempt reload after intro — sSnatcherMain may not be ready during initial load
        if (AreaKeySystem::Get().IsIntroComplete())
        {
            BYTE* psSnatcherMain = (BYTE*)sSnatcherMain::Instance();
            if (psSnatcherMain)
            {
                BYTE* pcGametaskMain = *(BYTE**)(psSnatcherMain + 0x20DC0);
                if (pcGametaskMain)
                {
                    *(WORD*) (pcGametaskMain + 0x1B8) = (WORD)s_previousArea;
                    *(DWORD*)(pcGametaskMain + 0x1E0) = 0;
                }
            }
        }

        return;
    }

    s_previousArea = areaId;
    s_originalAreaChange(param_1, areaId);
}