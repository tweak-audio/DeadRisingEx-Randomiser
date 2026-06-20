#include "CostumeReward.h"
#include "CostumeRewardSystem.h"
#include "DeadRisingEx/MtFramework/Randomiser/Checks/CheckSystem.h"
#include "DeadRisingEx/MtFramework/Randomiser/Checks/ChecksManager.h"
#include "DeadRisingEx/MtFramework/Randomiser/Rewards/CostumeReward.h"
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
    { 0,  0,  "	Shirtless with Blue Boxers and Barefoot" },
    { 0,  1,  "Frank's Default Clothing" },
    { 0,  2,  "Brown Jacket with Fur Trim Tan Shirt and Black Pants" },
    { 0,  3,  "	Black and White Sleeveless Sports Top with Black and Grey Shorts" },
    { 0,  4,  "Red and White Sleeveless Shirt with Red and Black Checkered Shorts" },
    { 0,  5,  "White Dress Shirt, Black Tie, and Grey Dress Pants" },
    { 0,  6,  "Black and Brown Checkered Dress Shirt, Black Tie, and Striped Pants" },
    { 0,  7,  "Grey Business Suit with Striped Tie" },
    { 0,  8,  "White Business Suit and Striped Tie" },
    { 0,  9,  "Yellow Suit with Yellow Striped Tie" },
    { 0, 10,  "White Skirt" },
    { 0, 11,  "Black Skirt" },
    { 0, 12,  "Pink Skirt" },
    { 0, 13,  "Purple Dress" },
    { 0, 14,  "White Teddy" },
    { 0, 15,  "Black Sundress with Red Roses" },
    { 0, 16,  "Blue and White Flowery Dress" },
    { 0, 17,  "Red and Grey Ratman T-shirt with Brown Shorts" },
    { 0, 18,  "Green Ratman T-shirt and Blue Jean Shorts" },
    { 0, 19,  "Pink and Black Striped T-shirt with Pink Jean Shorts" },
    { 0, 20,  "Blue T-shirt with White Stars and Red Shorts" },
    { 0, 21,  "Miami Nights Outfit" },
    { 0, 22,  "Casual Outfit" },
    { 0, 23,  "USA Track Outfit" },
    { 0, 24,  "Black and White Spandex Track Suit" },
    { 0, 25,  "Tan Camouflage Vest with Dark Brown Shorts" },
    { 0, 26,  "Blue Vest with Tan Shorts" },
    { 0, 27,  "Mega Man Torso" }, 
    { 0, 28,  "Weekender Outfit" }, //test, may need removing
    { 0, 29,  "Man in Black Outfit" }, //test, may need removing
    { 0, 30,  "Strike Outfit" },
    { 0, 31,  "Accountant Outfit" },
    { 0, 32,  "Ammo Belt" },
    { 0, 33,  "Pink Paparazzi Outfit" },
    { 0, 34,  "Grandpa Outfit" },  
    { 0, 35,  "Arthur's Boxers" },  
    { 0, 36,  "Prisoner Outfit" }, 
    { 0, 37,  "Mall Employee Uniform" }, 
    { 0, 38,  "Burgundy Wine Outfit" },
    { 0, 39,  "Cold Hearted Snake Outfit" },  
    { 0, 40,  "Pure White Outfit" },
    { 0, 41,  "Special Forces Uniform" },
    { 0, 42,  "Pro Wrestling Briefs" },

    // Slot 1 - Shoes
    { 1,  0,  "Bare Feet" },
    { 1,  1,  "Frank's Default Shoes" },
    { 1,  2,  "White Dress Shoes" },
    { 1,  3,  "Black Dress Shoes with Purple Socks" },
    { 1,  4,  "Red and Black Running Shoes" },
    { 1,  5,  "White and Red Lowtops with Soccer Socks" },
    { 1,  6,  "Orange Lowtops with White Pearl Anklet" },
    { 1,  7,  "Special Forces Boots" },
    { 1,  8,  "Mega Man Boots" },
    { 1,  9,  "Pro Wrestling Boots" },

    // Slot 2 - Head wear
    { 2,  0,  "Frank's Default Face" }, //maybe remove?
    { 2,  1,  "Brown Hair Dye" },
    { 2,  2,  "Mega Man Helmet" },
    { 2,  3,  "Grey Hair Dye" },
    { 2,  4,  "Light Brown Hair Dye" },
    { 2,  5,  "Red Hair Dye" },
    { 2,  7,  "Black Baseball Cap" },
    { 2,  8,  "Blue and White Baseball Cap" },
    { 2,  9,  "Tan Fedora" },
    { 2, 10,  "Black Fedora" },
    { 2, 11,  "White Hat" },
    { 2, 14,  "Ghoul Mask" },
    { 2, 15,  "Horse Mask" },
    { 2, 16,  "Teddy Bear Mask" },
    { 2, 17,  "Servbot Mask" },
    { 2, 19,  "Cop Hat" },

    // Slot 3 - Accessories
    { 3,  1,  "Grey Sunglasses" },
    { 3,  2,  "Silver Wire-Frame Dark-tinted Glasses" },
    { 3,  3,  "Grey Rimless Wire-Frame Glasses" },  
    { 3,  4,  "Red Armless Sunglasses" },
    { 3,  5,  "Silver Rimless Wire-Frame Glasses" },  
    { 3,  6,  "Black and Orange Sunglasses" },
    { 3,  7,  "Hockey Mask" },  
    { 3, 10,  "Round Shades Outfit" },

    // Slot 4 - Watch
    //{ 4,  1,  "Slot4 01" },

    // Slot 5 - Camera
    //{ 5,  1,  "Slot5 01" },
};

// ─────────────────────────────────────────────
//  Slot state
// ─────────────────────────────────────────────

static const int MAX_COSTUME_REWARDS = COSTUME_POOL_SIZE;
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

void GenerateCostumeRewardSlots()
{
    if (g_slotsInitialized) return;

    uint32_t seed = CheckSystem::GetSeed();
    if (seed == 0)
        LogLine("[CostumeRewardSystem] Warning: seed is 0");

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
    sprintf_s(buf, "[CostumeRewardSystem] %d slots generated for seed %u",
              COSTUME_POOL_SIZE, seed);
    LogLine(buf);
}

void ResetCostumeRewardSlots()
{
    g_slotsInitialized = false;
    g_nextSlot         = 0;
}

// ─────────────────────────────────────────────
//  Public API
// ─────────────────────────────────────────────

CostumeRewardResult GiveNextCostumeReward()
{
    if (!g_slotsInitialized)
        GenerateCostumeRewardSlots();

    if (g_nextSlot >= COSTUME_POOL_SIZE)
    {
        LogLine("[CostumeRewardSystem] No more costume rewards");
        return { -1, nullptr };
    }

    const CostumeEntry& entry = g_rewardSlots[g_nextSlot];
    CostumeReward::GiveCostume(entry.slot, entry.id);

    SaveStateManager::SetRewardCostume(entry.slot, entry.id);

    char buf[128];
    sprintf_s(buf, "[CostumeRewardSystem] Gave slot %d: %s (slot=%d id=%d)",
              g_nextSlot, entry.name, entry.slot, entry.id);
    LogLine(buf);

    g_nextSlot++;
    return { entry.slot * 100 + entry.id, entry.name };
}

void ApplyRandomStartingOutfit()
{
    LogLine("[OUTFIT] ApplyRandomStartingOutfit enter");

    uint32_t seed = CheckSystem::GetSeed();
    if (seed == 0)
    {
        LogLine("[OUTFIT] Seed is 0 — aborting");
        return;
    }

    RngSeed(seed ^ 0xF0FFFFFF);

    // Use fixed-size C arrays — avoids heap allocation inside a game hook,
    // which can crash if the game's allocator is in a bad state at this point.
    // Max per slot: slot0=42, slot1=10, slot2=15, slot3=8 — 64 is safe headroom.
    uint8_t slotIds[6][64] = {};
    int     slotCounts[6]  = {};

    for (int i = 0; i < COSTUME_POOL_SIZE; i++)
    {
        uint8_t slot = g_costumePool[i].slot;
        if (slot < 6 && slotCounts[slot] < 64)
            slotIds[slot][slotCounts[slot]++] = g_costumePool[i].id;
    }

    for (int slot = 0; slot < 6; slot++)
    {
        if (slotCounts[slot] == 0)
            continue;

        int pick = RngRange(0, slotCounts[slot] + 1);
        if (pick == 0)
            continue;

        uint8_t id = slotIds[slot][pick - 1];

        char buf[64];
        sprintf_s(buf, "[OUTFIT] Slot %d: id=%d", slot, id);
        LogLine(buf);

        CostumeReward::GiveCostume((uint8_t)slot, id);
    }

    LogLine("[OUTFIT] ApplyRandomStartingOutfit done");

    ReapplyRewardedCostumes();
}

void ReapplyRewardedCostumes()
{
    bool any = false;
    for (int slot = 0; slot < 6; slot++)
    {
        if (!SaveStateManager::HasRewardCostume(slot)) continue;
        uint8_t id = SaveStateManager::GetRewardCostumeId(slot);
        CostumeReward::GiveCostume((uint8_t)slot, id);
        char buf[96];
        sprintf_s(buf, "[CostumeRewardSystem] Reapplied reward costume slot=%d id=%d", slot, id);
        LogLine(buf);
        any = true;
    }
    if (!any)
        LogLine("[CostumeRewardSystem] No reward costumes to reapply");
}