
#pragma once
#include <cstdint>

class uPPStickerImpl
{
public:
    static void RegisterTypeInfo();
    static void OnStickerCollected(void* stickerObject, uint32_t itemPhotoId,
                                   int totalCollected, int totalStickers);
    static bool IsPlayerActive();
};

constexpr int TOTAL_PPSTICKERS = 100;