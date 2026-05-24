#include "ClothingReward.h"
#include "ClothingRewardSystem.h"
#include "DeadRisingEx/MtFramework/Randomiser/Checks/CheckSystem.h"
#include "DeadRisingEx/MtFramework/Randomiser/Rewards/ClothingReward.h"
#include "DeadRisingEx/Utilities/DebugLog.h"
#include "../InputSystem.h"

// ─────────────────────────────────────────────
//  Costume pool entry
// ─────────────────────────────────────────────

struct CostumeEntry
{
    uint8_t slot;
    uint8_t id;
    const char* name;
};

// ─────────────────────────────────────────────
//  Full costume pool (checks + cut content)
// ─────────────────────────────────────────────

static const CostumeEntry g_costumePool[] =
{
    // Slot 0 - Outfit
    { 0,  1,  "Top 01" },
    { 0,  2,  "Top 02" },
    { 0,  3,  "Top 03" },
    { 0,  4,  "Top 04" },
    { 0,  5,  "Top 05" },
    { 0,  6,  "Top 06" },
    { 0,  7,  "Top 07" },
    { 0,  8,  "Top 08" },
    { 0,  9,  "Top 09" },
    { 0, 10,  "Top 10" },
    { 0, 11,  "Top 11" },
    { 0, 12,  "Top 12" },
    { 0, 13,  "Top 13" },
    { 0, 14,  "Top 14" },
    { 0, 15,  "Top 15" },
    { 0, 16,  "Top 16" },
    { 0, 17,  "Top 17" },
    { 0, 18,  "Top 18" },
    { 0, 19,  "Top 19" },
    { 0, 20,  "Top 20" },
    { 0, 21,  "Top 21" },
    { 0, 22,  "Top 22" },
    { 0, 23,  "Top 23" },
    { 0, 24,  "Top 24" },
    { 0, 25,  "Top 25" },
    { 0, 26,  "Top 26" },
    { 0, 27,  "Top 27" },  // cut?
    { 0, 28,  "Top 28" },
    { 0, 29,  "Top 29" },
    { 0, 30,  "Top 30" },
    { 0, 31,  "Top 31" },
    { 0, 32,  "Top 32" },
    { 0, 33,  "Top 33" },
    { 0, 34,  "Top 34" },  // cut?
    { 0, 35,  "Top 35" },  // cut?
    { 0, 36,  "Top 36" },  // cut?
    { 0, 37,  "Top 37" },  // cut?
    { 0, 38,  "Top 38" },
    { 0, 39,  "Top 39" },  // cut?
    { 0, 40,  "Top 40" },
    { 0, 41,  "Top 41" },
    { 0, 42,  "Top 42" },

    // Slot 1 - Shoes
    { 1,  0,  "Shoes 00" },
    { 1,  1,  "Shoes 01" },
    { 1,  2,  "Shoes 02" },
    { 1,  3,  "Shoes 03" },
    { 1,  4,  "Shoes 04" },
    { 1,  5,  "Shoes 05" },
    { 1,  6,  "Shoes 06" },
    { 1,  7,  "Shoes 07" },
    { 1,  8,  "Shoes 08" },
    { 1,  9,  "Shoes 09" },

    // Slot 2 - Unknown
    { 2,  1,  "Slot2 01" },
    { 2,  2,  "Slot2 02" },
    { 2,  3,  "Slot2 03" },
    { 2,  4,  "Slot2 04" },
    { 2,  5,  "Slot2 05" },
    { 2,  7,  "Slot2 07" },
    { 2,  8,  "Slot2 08" },
    { 2,  9,  "Slot2 09" },
    { 2, 10,  "Slot2 10" },
    { 2, 11,  "Slot2 11" },
    { 2, 14,  "Slot2 14" },
    { 2, 15,  "Slot2 15" },
    { 2, 16,  "Slot2 16" },
    { 2, 17,  "Slot2 17" },
    { 2, 19,  "Slot2 19" },

    // Slot 3 - Accessories
    { 3,  1,  "Accessory 01" },
    { 3,  2,  "Accessory 02" },
    { 3,  3,  "Accessory 03" },  // cut?
    { 3,  4,  "Accessory 04" },
    { 3,  5,  "Accessory 05" },  // cut?
    { 3,  6,  "Accessory 06" },
    { 3,  7,  "Accessory 07" },  // cut?
    { 3, 10,  "Accessory 10" },

    // Slot 4 - Watch
    //{ 4,  1,  "Slot4 01" },

    // Slot 5 - Camera
    //{ 5,  1,  "Slot5 01" },
};

// ─────────────────────────────────────────────
//  Slot state
// ─────────────────────────────────────────────

static const int MAX_CLOTHING_REWARDS = COSTUME_POOL_SIZE;
static CostumeEntry g_rewardSlots[COSTUME_POOL_SIZE] = {};
static int   g_nextSlot         = 0;
static bool  g_slotsInitialized = false;

// ─────────────────────────────────────────────
//  Seeded RNG
// ─────────────────────────────────────────────

static uint32_t s_rngState = 0;

static void     RngSeed(uint32_t seed) { s_rngState = seed ^ 0xDEADBEEF; }
static uint32_t RngNext()
{
    s_rngState = s_rngState * 1664525u + 1013904223u;
    return s_rngState;
}
static int RngRange(int min, int max)
{
    return min + (int)(RngNext() % (uint32_t)(max - min));
}

// ─────────────────────────────────────────────
//  Slot generation
// ─────────────────────────────────────────────

void GenerateClothingRewardSlots()
{
    if (g_slotsInitialized) return;

    uint32_t seed = CheckSystem::GetSeed();
    if (seed == 0)
        LogLine("[ClothingRewardSystem] Warning: seed is 0");

    // Different RNG offset from item system
    RngSeed(seed ^ 0xC0FFEE00);

    // Copy pool and Fisher-Yates shuffle
    for (int i = 0; i < COSTUME_POOL_SIZE; i++)
        g_rewardSlots[i] = g_costumePool[i];

    for (int i = COSTUME_POOL_SIZE - 1; i > 0; i--)
    {
        int j = RngRange(0, i + 1);
        CostumeEntry tmp  = g_rewardSlots[i];
        g_rewardSlots[i]  = g_rewardSlots[j];
        g_rewardSlots[j]  = tmp;
    }

    g_slotsInitialized = true;
    g_nextSlot         = 0;

    char buf[64];
    sprintf_s(buf, "[ClothingRewardSystem] %d slots generated for seed %u",
              COSTUME_POOL_SIZE, seed);
    LogLine(buf);
}

void ResetClothingRewardSlots()
{
    g_slotsInitialized = false;
    g_nextSlot         = 0;
}

// ─────────────────────────────────────────────
//  Public API
// ─────────────────────────────────────────────

ClothingRewardResult GiveNextClothingReward()
{
    if (!g_slotsInitialized)
        GenerateClothingRewardSlots();

    if (g_nextSlot >= COSTUME_POOL_SIZE)
    {
        LogLine("[ClothingRewardSystem] No more clothing rewards");
        return { -1, nullptr };
    }

    const CostumeEntry& entry = g_rewardSlots[g_nextSlot];
    ClothingReward::GiveCostume(entry.slot, entry.id);

    char buf[128];
    sprintf_s(buf, "[ClothingRewardSystem] Gave slot %d: %s (slot=%d id=%d)",
              g_nextSlot, entry.name, entry.slot, entry.id);
    LogLine(buf);

    g_nextSlot++;
    return { entry.slot * 100 + entry.id, entry.name };
}