#include "RandomiserConfig.h"
#include <Windows.h>
#include <stdio.h>

// ─────────────────────────────────────────────────────────────────────────────
//  Helpers
// ─────────────────────────────────────────────────────────────────────────────

static bool ReadBool(LPCSTR section, LPCSTR key, bool defaultVal, LPCSTR path)
{
    CHAR buf[16] = {};
    GetPrivateProfileStringA(section, key, defaultVal ? "true" : "false",
        buf, sizeof(buf) - 1, path);
    return _stricmp(buf, "true") == 0;
}

static int ReadFastFrankSpeed(LPCSTR path)
{
    CHAR buf[16] = {};
    GetPrivateProfileStringA("Checks", "FastFrankSpeed", "Max", buf, sizeof(buf) - 1, path);
    if (_stricmp(buf, "Normal")   == 0) return 1;
    if (_stricmp(buf, "Fast")     == 0) return 7;
    if (_stricmp(buf, "VeryFast") == 0) return 10;
    return 14; // Max (default)
}

static uint32_t ReadUInt32(LPCSTR section, LPCSTR key, uint32_t defaultVal, LPCSTR path)
{
    CHAR buf[16] = {};
    char defStr[16];
    sprintf_s(defStr, "%u", defaultVal);
    GetPrivateProfileStringA(section, key, defStr, buf, sizeof(buf) - 1, path);
    return (uint32_t)strtoul(buf, nullptr, 10);
}

// ─────────────────────────────────────────────────────────────────────────────
//  Singleton
// ─────────────────────────────────────────────────────────────────────────────

RandomiserConfig& RandomiserConfig::Get()
{
    static RandomiserConfig s_instance;
    static bool             s_loaded = false;
    if (!s_loaded)
    {
        char dir[MAX_PATH] = {};
        GetCurrentDirectoryA(sizeof(dir), dir);
        s_instance.Load(dir);
        s_loaded = true;
    }
    return s_instance;
}

// ─────────────────────────────────────────────────────────────────────────────
//  Load
// ─────────────────────────────────────────────────────────────────────────────

void RandomiserConfig::Load(const std::string& gameDir)
{
    std::string path = gameDir + "\\randomiser_config.ini";
    LPCSTR p = path.c_str();

    // [PhotoSanity]
    photoSanity           = ReadBool("PhotoSanity", "PhotoSanity",          true, p);
    ppStickerChecks       = ReadBool("PhotoSanity", "PPStickerChecks",       true, p);
    survivorPhotoChecks   = ReadBool("PhotoSanity", "SurvivorPhotoChecks",   true, p);
    psychopathPhotoChecks = ReadBool("PhotoSanity", "PsychopathPhotoChecks", true, p);

    // [Checks]
    clothingChecks = ReadBool("Checks", "ClothingChecks",  true, p);
    areaKeyRewards = ReadBool("Checks", "AreaKeyRewards",  true, p);
    timeChunks     = ReadBool("Checks", "TimeChunks",      true, p);
    sixHourChunks  = ReadBool("Checks", "SixHourChunks",   true, p);
    fastFrank      = ReadBool("Checks", "FastFrank",        true, p);
    fastFrankSpeed = ReadFastFrankSpeed(p);

    randomiseStartingOutfit = ReadBool("Checks", "RandomiseStartingOutfit", true, p);

    // [Seed]
    customSeed = ReadUInt32("Seed", "Seed", 0, p);
}
