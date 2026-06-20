#pragma once
#include <cstdint>

namespace RandomiserSave
{
    void        Load();
    void        Save();

    uint32_t    GetSeed();
    void        SetSeed(uint32_t seed);

    const bool* GetChunksUnlocked();
    void        SetChunksUnlocked(const bool chunks[12]);
}
