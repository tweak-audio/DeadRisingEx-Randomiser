#include "CaseCheck.h"
#include "../InputSystem.h"
#include "DeadRisingEx/Utilities/DebugLog.h"
#include "MtFramework/Game/sMain.h"  // for GetModuleAddress
#include <windows.h>
#include <cstdint>
#include <cstdio>

// ─────────────────────────────────────────────
//  Pointer chain (translated from DR_RTM US)
//  Pointer("DeadRising.exe", 26496472, 134592) + 336
//
//  26496472 dec = 0x194A158
//  So static address = 0x140000000 + 0x194A158 = 0x14194A158
//  Deref → add 0x20DC0 (134592) → deref → add 0x150 (336)
// ─────────────────────────────────────────────

static const uintptr_t PROGRESS_STATIC_ADDR = 0x14194A158;
static const uintptr_t DEREF_OFFSET         = 0x20DC0;   // 134592
static const uintptr_t PROG_OFFSET          = 0x150;     // 336

static uint32_t s_lastProgress = 0xFFFFFFFF;

static uint32_t ReadCampaignProgress()
{
    // Step 1: get the relocated static address
    uintptr_t* p1 = reinterpret_cast<uintptr_t*>(GetModuleAddress((void*)PROGRESS_STATIC_ADDR));
    if (!p1 || !*p1) return 0;

    // Step 2: dereference and add second offset
    uintptr_t* p2 = reinterpret_cast<uintptr_t*>(*p1 + DEREF_OFFSET);
    if (!p2 || !*p2) return 0;

    // Step 3: read the uint32 at the final offset
    return *reinterpret_cast<uint32_t*>(*p2 + PROG_OFFSET);
}

void CaseCheck::Initialize()
{
    s_lastProgress = 0xFFFFFFFF;
    LogLine("[CaseCheck] Initialized");
}

void CaseCheck::OnGameStateTick()
{
    uint32_t progress = ReadCampaignProgress();
    if (progress == 0) return;

    if (progress != s_lastProgress)
    {
        char buf[128];
        sprintf_s(buf, "[CaseCheck] campaignProgress: %u -> %u", s_lastProgress, progress);
        LogLine(buf);
        s_lastProgress = progress;
    }
}