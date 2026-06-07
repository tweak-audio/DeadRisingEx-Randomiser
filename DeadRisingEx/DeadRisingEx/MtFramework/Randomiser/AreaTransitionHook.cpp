#include "AreaTransitionHook.h"
#include "AreaKeySystem.h"
#include "Misc/AsmHelpers.h"
#include "DeadRisingEx/Utilities/DebugLog.h"
#include "InputSystem.h"
#include "Checks/SurvivorPhotoCheck.h"
#include "Checks/PsychopathPhotoCheck.h"
#include <MtFramework/Game/sSnatcherMain.h>
#include <detours.h>
#include <stdio.h>

static void NotifyAreaTransition()
{
    SurvivorPhotoCheck::OnAreaTransition();
    PsychopathPhotoCheck::OnAreaTransition();
}

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

uint32_t AreaTransitionHook::GetCurrentAreaId()
{
    return s_previousArea;
}

void AreaTransitionHook::Install()
{
    s_originalAreaChange      = reinterpret_cast<FnAreaChange>     (GetModuleAddress(0x1400628f0));
    s_originalTransitionStart = reinterpret_cast<FnTransitionStart>(GetModuleAddress(0x140209d00));

    DetourTransactionBegin();
    DetourUpdateThread(GetCurrentThread());
    // FUN_14008fb70 (AreaHitProcess) intentionally not hooked — it uses non-standard
    // implicit register params (unaff_RBX, unaff_EDI) that our declared signature
    // can't match, causing Detours to corrupt FUN_1401fee91's register state on return.
    DetourAttach(&(PVOID&)s_originalAreaChange,      Hook_AreaChange);
    DetourAttach(&(PVOID&)s_originalTransitionStart, Hook_TransitionStart);
    LONG err = DetourTransactionCommit();

    if (err != NO_ERROR)
    {
        char buf[64];
        snprintf(buf, sizeof(buf), "AreaTransitionHook: Install failed %d\n", err);
        LogLine(buf);
    }
    else
    {
        LogLine("AreaTransitionHook: Installed successfully\n");
    }
}

// ─── Hook: TransitionStart (FUN_140209d00) ───────────────────────────────────
// Belt-and-suspenders block. Zeroing areaMgr+0x38 prevents FUN_140001990 from
// advancing the load state machine in FUN_140204449 after we return.
// Note: for area-hit door transitions this may not fire — see Hook_AreaChange.

void __fastcall AreaTransitionHook::Hook_TransitionStart(int64_t param_1)
{
    { static int n = 0; if (++n <= 3) LogLine("TS"); }

    if (AreaKeySystem::Get().IsIntroComplete())
    {
        uintptr_t areaMgrPtr = reinterpret_cast<uintptr_t>(GetModuleAddress(0x141945f70));
        uintptr_t areaMgr    = areaMgrPtr ? *reinterpret_cast<uintptr_t*>(areaMgrPtr) : 0;
        if (areaMgr)
        {
            uint32_t dest = *(uint32_t*)(areaMgr + 0x48);
            ZoneID zone = AreaKeySystem::ZoneFromAreaId(dest);
            if (zone != ZoneID::COUNT && !AreaKeySystem::Get().HasKey(dest))
            {
                *(uintptr_t*)(areaMgr + 0x38) = 0;
                char buf[64];
                snprintf(buf, sizeof(buf), "Hook_TransitionStart: BLOCKED 0x%x", dest);
                LogLine(buf);
                return;
            }
        }
    }
    s_originalTransitionStart(param_1);
}

// ─── Hook: AreaChange (FUN_1400628f0) ────────────────────────────────────────
// Primary block — fires after assets load. Commits the locked zone (consistent
// with what's in memory), then reloads s_previousArea via cGametaskMain.
// DWORD write for +0x1B8 avoids upper-16-bit garbage from repeated calls.

static bool s_reloadInProgress = false;

void __fastcall AreaTransitionHook::Hook_AreaChange(int64_t param_1, uint32_t areaId)
{
    { static int n = 0; if (++n <= 5) { char b[24]; snprintf(b,sizeof(b),"AC %x",areaId); LogTransition(b); } }

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
            LogLine("INTRO COMPLETE: key system now active, Paradise Plaza unlocked");
        }
        NotifyAreaTransition();
        s_previousArea = areaId;
        s_originalAreaChange(param_1, areaId);
        LogLine("AC_done");
        return;
    }

    if (AreaKeySystem::Get().HasKey(areaId))
    {
        s_reloadInProgress = false;
        NotifyAreaTransition();
        s_previousArea = areaId;
        s_originalAreaChange(param_1, areaId);
        return;
    }

    // Locked zone
    if (s_reloadInProgress)
    {
        s_originalAreaChange(param_1, s_previousArea);
        return;
    }

    uint32_t fallback = AreaKeySystem::GetBlockFallback(s_previousArea, areaId);

    // Cooldown: if the same (from→to) transition re-fires within 2 seconds
    // (e.g. vehicle momentum carrying Frank back into the same locked entrance),
    // redirect without reload — fallback area is already loaded.
    static uint32_t s_cooldownFrom = 0;
    static uint32_t s_cooldownTo   = 0;
    static DWORD    s_cooldownEnd  = 0;

    bool inCooldown = (s_previousArea == s_cooldownFrom &&
                       areaId         == s_cooldownTo   &&
                       GetTickCount() < s_cooldownEnd);

    if (inCooldown)
    {
        s_originalAreaChange(param_1, fallback);
        return;
    }

    char buf[64];
    snprintf(buf, sizeof(buf), "Hook_AreaChange: BLOCKED 0x%x -> reloading 0x%x",
        areaId, fallback);
    LogLine(buf);

    s_cooldownFrom = s_previousArea;
    s_cooldownTo   = areaId;
    s_cooldownEnd  = GetTickCount() + 2000;

    s_originalAreaChange(param_1, areaId);
    s_reloadInProgress = true;

    BYTE* psSnatcherMain = (BYTE*)sSnatcherMain::Instance();
    if (psSnatcherMain)
    {
        BYTE* pcGametaskMain = *(BYTE**)(psSnatcherMain + 0x20DC0);
        if (pcGametaskMain)
        {
            *(DWORD*)(pcGametaskMain + 0x1B8) = fallback;
            *(DWORD*)(pcGametaskMain + 0x1E0) = 0;
        }
    }
}
