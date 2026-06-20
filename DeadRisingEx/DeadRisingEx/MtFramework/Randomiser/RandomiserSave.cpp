#include "RandomiserSave.h"
#include <stdio.h>
#include <string.h>

static const char* SAVE_FILE = "DeadRisingEx_Randomiser_Save.dat";

struct SaveData
{
    uint32_t seed           = 0;
    bool     chunksUnlocked[12] = {};
};

static SaveData s_data;

void RandomiserSave::Load()
{
    FILE* f = nullptr;
    fopen_s(&f, SAVE_FILE, "rb");
    if (!f) return;
    fread(&s_data, sizeof(SaveData), 1, f);
    fclose(f);
}

void RandomiserSave::Save()
{
    FILE* f = nullptr;
    fopen_s(&f, SAVE_FILE, "wb");
    if (!f) return;
    fwrite(&s_data, sizeof(SaveData), 1, f);
    fclose(f);
}

uint32_t RandomiserSave::GetSeed()
{
    return s_data.seed;
}

void RandomiserSave::SetSeed(uint32_t seed)
{
    s_data.seed = seed;
    Save();
}

const bool* RandomiserSave::GetChunksUnlocked()
{
    return s_data.chunksUnlocked;
}

void RandomiserSave::SetChunksUnlocked(const bool chunks[12])
{
    memcpy(s_data.chunksUnlocked, chunks, 12 * sizeof(bool));
    Save();
}
