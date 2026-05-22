#include "PPStickerCheck.h"
#include "CheckSystem.h"
#include "DeadRisingEx/MtFramework/Randomiser/InputSystem.h"
#include "MtFramework/Utils/Utilities.h"
#include <detours.h>

// ─────────────────────────────────────────────
//  Addresses
// ─────────────────────────────────────────────

constexpr uintptr_t ADDR_GAME_MANAGER    = 0x141CF2AA0;
constexpr uintptr_t ADDR_STICKER_UPDATE  = 0x14019AD20;

// GameManager offsets
constexpr uintptr_t OFFSET_SAVE_DATA_PTR = 0x20DD8;
constexpr uintptr_t OFFSET_STICKER_FLAGS = 0x1BB30;

// Sticker object offsets
constexpr uintptr_t OFFSET_STATE         = 0x24;
constexpr uintptr_t OFFSET_PHOTO_ID      = 0x13E0;

// Sticker photoId range (confirmed 128-227, 100 total)
constexpr uint32_t STICKER_ID_MIN        = 128;
constexpr uint32_t STICKER_ID_MAX        = 227;
constexpr int      STICKER_TOTAL         = 100;

// ─────────────────────────────────────────────
//  Hook pointer
// ─────────────────────────────────────────────

using FnStickerUpdate = void(__stdcall*)(int64_t* param_1);
FnStickerUpdate original_StickerUpdate = nullptr;

// ─────────────────────────────────────────────
//  Helpers
// ─────────────────────────────────────────────

static uint32_t* GetStickerFlagsArray()
{
    void** g_pGameManager = (void**)GetModuleAddress(ADDR_GAME_MANAGER);
    if (!g_pGameManager || !*g_pGameManager) return nullptr;

    uintptr_t gm  = (uintptr_t)*g_pGameManager;
    uintptr_t sav = *(uintptr_t*)(gm + OFFSET_SAVE_DATA_PTR);
    if (!sav) return nullptr;

    return (uint32_t*)(sav + OFFSET_STICKER_FLAGS);
}

static int CountCollectedStickers()
{
    uint32_t* flags = GetStickerFlagsArray();
    if (!flags) return 0;

    int count = 0;
    for (uint32_t i = STICKER_ID_MIN; i <= STICKER_ID_MAX; i++)
        if (flags[i] >= 1) count++;

    return count;
}

// ─────────────────────────────────────────────
//  Hook — Update (fires per frame, detects collection)
// ─────────────────────────────────────────────

void __stdcall Hook_StickerUpdate(int64_t* param_1)
{
    BYTE stateBefore = *(BYTE*)((uintptr_t)param_1 + OFFSET_STATE);

    original_StickerUpdate(param_1);

    BYTE stateAfter = *(BYTE*)((uintptr_t)param_1 + OFFSET_STATE);

    if (stateBefore == 0 && stateAfter == 1)
    {
        uint32_t photoId  = *(uint32_t*)((uintptr_t)param_1 + OFFSET_PHOTO_ID);
        int      collected = CountCollectedStickers();

        uPPStickerImpl::OnStickerCollected((void*)param_1, photoId, collected, STICKER_TOTAL);
    }
}

// ─────────────────────────────────────────────
//  Callback
// ─────────────────────────────────────────────

void uPPStickerImpl::OnStickerCollected(void* stickerObject, uint32_t itemPhotoId,
                                         int totalCollected, int totalStickers)
{
    char buf[128];
    sprintf_s(buf, "[STICKER] Collected PhotoId=%u | Progress: %d / %d",
        itemPhotoId, totalCollected, totalStickers);
    LogLine(buf);

    CheckSystem::CompleteCheck(CheckType::PPSticker, itemPhotoId);
}

// ─────────────────────────────────────────────
//  Registration
// ─────────────────────────────────────────────

void uPPStickerImpl::RegisterTypeInfo()
{
    original_StickerUpdate = (FnStickerUpdate)GetModuleAddress(ADDR_STICKER_UPDATE);
    DetourAttach((void**)&original_StickerUpdate, Hook_StickerUpdate);
}