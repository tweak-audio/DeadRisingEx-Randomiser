#pragma once
#include <string>
#include <cstdint>

class RandomiserConfig
{
public:
    static RandomiserConfig& Get();
    void Load(const std::string& gameDir);

    // ── PhotoSanity master + per-type toggles ──────────────────────────────
    // photoSanity=true  forces all photo check types ON (overrides type toggles below)
    // photoSanity=false lets the individual type toggles decide
    bool photoSanity          = true;
    bool ppStickerChecks      = true;
    bool survivorPhotoChecks  = true;
    bool psychopathPhotoChecks= true;

    bool EffectivePPSticker()       const { return photoSanity || ppStickerChecks; }
    bool EffectiveSurvivorPhoto()   const { return photoSanity || survivorPhotoChecks; }
    bool EffectivePsychopathPhoto() const { return photoSanity || psychopathPhotoChecks; }

    // ── Non-photo check type toggles ───────────────────────────────────────
    bool costumeChecks   = true;
    bool areaKeyRewards  = true;  // false = zones open from start, no AreaKey rewards
    bool keyItemRewards  = true;  // false = key item pickups pass through to vanilla (no randomisation)
    bool timeChunks      = true;  // false = no time chunk rewards, time gating disabled
    bool sixHourChunks   = true;  // true = 6-hour mode (11 chunks), false = 12-hour mode (5 chunks)
    bool fastFrank       = true;  // false = normal Frank speed
    int  fastFrankSpeed  = 14;   // raw run-level value: Normal=1, Fast=7, VeryFast=10, Max=14

    bool randomiseStartingOutfit = true;  // false = Frank keeps default outfit at game start

    // ── Seed (0 = generate random) ─────────────────────────────────────────
    uint32_t customSeed = 0;

private:
    RandomiserConfig() = default;
};
