#include "SurvivorPhotoCheck.h"
#include "PsychopathPhotoCheck.h"
#include "PPStickerCheck.h"
#include "ClothingCheck.h"

#pragma once
#include <cstdint>
#include <functional>
#include <string> 

// ─────────────────────────────────────────────
//  Randomizer settings — edit these to tune the run
// ─────────────────────────────────────────────

// At the top, choose your mode:
constexpr bool USE_6_HOUR_CHUNKS = true;  // Set to false for 12-hour chunks

constexpr int TIME_CHUNK_REWARDS = USE_6_HOUR_CHUNKS ? 11 : 5;  // 11 for 6hr, 5 for 12hr
constexpr int LEVEL_UP_REWARDS = 49;
constexpr int BATTERY_REFILL_REWARDS = 10;
constexpr int AREA_KEY_REWARDS = 11;  // One per zone (COUNT), excluding ParadisePlaza (auto-given)
constexpr int CLOTHING_REWARDS_MIN = 30;

constexpr int TOTAL_CHECKS = TOTAL_PPSTICKERS + TOTAL_SURVIVORS + TOTAL_PSYCHOPATHS + TOTAL_COSTUMES;

// ─────────────────────────────────────────────
//  Check types
// ─────────────────────────────────────────────
enum class CheckType : uint32_t
{
    PPSticker           = 0,
    SurvivorPhoto       = 1,
    SurvivorJoin        = 2,
    PsychopathPhoto     = 3,
    Clothing            = 4,
};

// ─────────────────────────────────────────────
//  Reward types 
// ─────────────────────────────────────────────
enum class RewardType : uint32_t
{
    None            = 0,
    SetItem         = 1,
    LevelUp         = 2,
    BatteryRefill   = 3,
    TimeChunk       = 4,  // Now unified - mode determined at runtime
    Clothing        = 5,
    AreaKey         = 6,  // value = ZoneID index
};
// ─────────────────────────────────────────────
//  Reward descriptor
// ─────────────────────────────────────────────
struct Reward
{
    RewardType  type  = RewardType::None;
    int         value = 0;   // For TimeChunks: which chunk number (1-11 for 6hr, 1-5 for 12hr)
                             // For LevelUp: number of levels (if used)
                             // For others: unused/ignored
};

// ─────────────────────────────────────────────
//  Check ID
// ─────────────────────────────────────────────
struct CheckId
{
    CheckType   type;
    uint32_t    id;

    bool operator==(const CheckId& other) const
    {
        return type == other.type && id == other.id;
    }
};

struct CheckIdHash
{
    size_t operator()(const CheckId& c) const
    {
        return std::hash<uint64_t>()(((uint64_t)c.type << 32) | c.id);
    }
};

using CheckCallback = std::function<void(CheckId, Reward)>;

// ─────────────────────────────────────────────
//  CheckSystem state machine
// ─────────────────────────────────────────────

enum class CheckSystemState : uint8_t
{
    UNINITIALIZED = 0,  // Before Initialize()
    BOOTSTRAPPING = 1,  // Inside Initialize(), subsystems loading
    SEED_READY    = 2,  // Seed + reward map built, draining queued events
    READY         = 3,  // Fully live — CompleteCheck dispatches immediately
    RESEEDING     = 4,  // SetSeed() mid-game, reward map being rebuilt
};

// ─────────────────────────────────────────────
//  CheckSystem
// ─────────────────────────────────────────────

class CheckSystem
{
public:
    static void               Initialize();
    static void               SetSeed(uint32_t seed);
    static uint32_t           GetSeed();
    static bool               IsReady();
    static CheckSystemState   GetState();

    static void RegisterCheckRange(CheckType type, uint32_t idMin, uint32_t idMax);
    static void CompleteCheck(CheckType type, uint32_t id);
    static void OnCheckCompleted(CheckCallback callback);

    static int GetSetItemCount();

    static int  GetTotalChecks();
    static int  GetCompletedChecks();

    static bool IsSeedBeatable(bool quiet = false);
    static void GenerateBeatableSeed();
    static void DumpSeedAnalysis();  // Debug helper

private:
    static void     GenerateRewardMap();
    static void     SaveSeedToFile();
    static bool     LoadSeedFromFile();
    static void     LoadCompletedChecks();  
    static Reward   GetRewardForCheck(CheckId check);
    static void WriteSeedAnalysisToFile();
    static std::string GetCurrentTimeString();
};