#include "DeadRisingEx/MtFramework/Player/uPlayerImpl.h"
#include "DeadRisingEx/MtFramework/Randomiser/InputSystem.h"
#include "DeadRisingEx/MtFramework/Randomiser/Checks/CheckSystem.h"
#include "SetItemRewardSystem.h"

#include "MtFramework/Item/uItem.h"
#include "MtFramework/Item/sItemCtrl.h"
#include "MtFramework/Archive/sResource.h"
#include "MtFramework/MtObject.h"

#include <Windows.h>
#include <vector>
#include <cmath>
#include <string>

extern bool(__stdcall *sUnit_AddObject)(void *thisptr, DWORD Unk, void *pObject);

static const char* FAILED_REWARD_ITEMS = "DeadRisingEx_Failed_Items.txt";

// ─────────────────────────────────────────────
//  Item pool — curated list of spawnable items
// ─────────────────────────────────────────────

static const DWORD g_itemPool[] =
{
    4,      // Baseball Bat
    12,     // Beam Sword
    19,     // Cleaver
    20,     // Skateboard
    21,     // Toy Cube
    30,     // Water Gun
    32,     // Toy Laser Sword
    38,     // Snack
    //43,     // Well Done Steak
    44,     // Spoiled Meat
    52,     // Katana
    57,     // Potted Plant
    58,     // Mega Man Buster
    60,     // Shotgun
    71,     // Bowling Ball
    106,    // Mega Buster
    127,    // Small Chainsaw
    136,    // 2 x 4
    140,    // Wine
    159,    // Rock
    190,    // Molotov Cocktail
    218,    // Orange Juice
    228,    // Shampoo
    241,    // Rat Stick
    242,    // Rat Saucer
    302,    // Quick Step
    303,    // Randomizer
    304,    // Untouchable
    305,    // Spitfire
    306,    // Nectar
    307,    // Energizer
};

constexpr int ITEM_POOL_SIZE = sizeof(g_itemPool) / sizeof(g_itemPool[0]);

// ─────────────────────────────────────────────
//  Item name lookup
// ─────────────────────────────────────────────

const char* GetItemNameFromId(DWORD itemId)
{
    switch (itemId)
    {
        case 4:   return "Baseball Bat";
        case 12:  return "Beam Sword";
        case 19:  return "Cleaver";
        case 20:  return "Skateboard";
        case 21:  return "Toy Cube";
        case 30:  return "Water Gun";
        case 32:  return "Toy Laser Sword";
        case 38:  return "Snack";
        //case 43:  return "Well Done Steak";
        case 44:  return "Spoiled Meat";
        case 52:  return "Katana";
        case 57:  return "Potted Plant";
        case 58:  return "Mega Man Buster";
        case 60:  return "Shotgun";
        case 71:  return "Bowling Ball";
        case 106: return "Mega Buster";
        case 127: return "Small Chainsaw";
        case 136: return "2 x 4";
        case 140: return "Wine";
        case 159: return "Rock";
        case 190: return "Molotov Cocktail";
        case 218: return "Orange Juice";
        case 228: return "Shampoo";
        case 241: return "Rat Stick";
        case 242: return "Rat Saucer";
        case 302: return "Quick Step";
        case 303: return "Randomizer";
        case 304: return "Untouchable";
        case 305: return "Spitfire";
        case 306: return "Nectar";
        case 307: return "Energizer";
        default:  return "Unknown Item";
    }
}

// ─────────────────────────────────────────────
//  Slot state
// ─────────────────────────────────────────────

static std::vector<DWORD> g_rewardSlots;
static int   g_nextSlot          = 0;
static bool  g_slotsInitialized  = false;

// ─────────────────────────────────────────────
//  Seeded RNG — uses CheckSystem seed for determinism
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

void GenerateRewardSlots()
{
    if (g_slotsInitialized) return;

    uint32_t seed = CheckSystem::GetSeed();
    if (seed == 0)
        LogLine("[RewardSystem] Warning: seed is 0");

    int setItemCount = CheckSystem::GetSetItemCount();  // ← runtime count

    RngSeed(seed ^ 0xCAFEBABE);

    // Build shuffled pool tiled to fill all slots
    g_rewardSlots.resize(setItemCount);
    for (int i = 0; i < setItemCount; i++)
        g_rewardSlots[i] = g_itemPool[i % ITEM_POOL_SIZE];

    for (int i = setItemCount - 1; i > 0; i--)
    {
        int j         = RngRange(0, i + 1);
        DWORD tmp     = g_rewardSlots[i];
        g_rewardSlots[i] = g_rewardSlots[j];
        g_rewardSlots[j] = tmp;
    }

    g_slotsInitialized = true;
    g_nextSlot         = 0;

    char buf[64];
    sprintf_s(buf, "[RewardSystem] %d slots generated for seed %u", setItemCount, seed);
    LogLine(buf);
}

void ResetRewardSlots()
{
    g_slotsInitialized = false;
    g_nextSlot         = 0;
}

// ─────────────────────────────────────────────
//  Helpers
// ─────────────────────────────────────────────

static Vector4 GetPlayerSpawnPosition()
{
    if (!uPlayerInstance)
        return { 0, 0, 0, 1 };

    Vector4 pos = *(Vector4*)((uint8_t*)uPlayerInstance + 0x40);

    if (!std::isfinite(pos.x) || !std::isfinite(pos.y) || !std::isfinite(pos.z))
        return { 0, 0, 0, 1 };

    pos.y += 60.0f;
    return pos;
}

// ── Approach A (inactive): sItemCtrl::_SpawnItem ──────────────────────────
// Pre-loads the archive, delegates init to _SpawnItem (which calls
// SetupItemProperties internally), then releases the manual archive ref.
// Goes through the Hook_SpawnItem detour.
#if 0
static bool SpawnItemViaCtrl(DWORD itemId, const Vector4& pos)
{
    sItemCtrl* ctrl = sItemCtrl::Instance();
    if (!ctrl) return false;

    sResource* resMgr = sResource::Instance();
    if (!resMgr) return false;

    ItemInfoEntry* info = &uItem::ItemInfoTable[itemId];
    if (!info || !info->ArchivePath || !info->ArchivePath[0])
        return false;

    cResource* res = resMgr->LoadGameResource<cResource>(
        rArchive::DebugTypeInfo,
        info->ArchivePath,
        RLF_SYNCHRONOUS | RLF_LOAD_AS_ARCHIVE
    );

    if (!res) return false;

    uItem* item = sItemCtrl::_SpawnItem(ctrl, itemId);
    res->DecrementRefCount();

    if (!item) return false;

    Vector4* itemPos = (Vector4*)((uint8_t*)item + 0x40);
    if (itemPos) *itemPos = pos;

    return true;
}
#endif

// ── Approach B (active): DTI-based spawn (mirrors spawn_item console command) ──
// Parses the hex file ID from the archive path, resolves the uOmXX class name
// via MtDTI, creates the instance directly, then registers it with sUnit.
// Does NOT go through sItemCtrl or Hook_SpawnItem.
static bool SpawnItemAtPosition(DWORD itemId, const Vector4& pos)
{
    ItemInfoEntry* info = &uItem::ItemInfoTable[itemId];
    if (!info || !info->ArchivePath || !info->ArchivePath[0])
        return false;

    if (uItem::ItemProperties[itemId] == nullptr)
        return false;

    // Parse hex file ID from the tail of the archive path (e.g. "...\\om0004" → 0x04)
    std::string sItemPath = info->ArchivePath;
    int slashIdx = (int)sItemPath.find_last_of('\\');
    std::string sItemName = sItemPath.substr(slashIdx + 3);
    DWORD itemFileId = strtoul(sItemName.c_str(), nullptr, 16);

    sResource* resMgr = sResource::Instance();
    if (!resMgr) return false;

    cResource* res = resMgr->LoadGameResource<cResource>(
        rArchive::DebugTypeInfo,
        sItemPath.c_str(),
        RLF_SYNCHRONOUS | RLF_LOAD_AS_ARCHIVE
    );
    if (!res) return false;

    char sItemClassName[64];
    switch (itemId)
    {
    case 69:   // Queen bee — has a separate item DTI instance
    case 135:  // Sniper rifle — same
        snprintf(sItemClassName, sizeof(sItemClassName), "uOm%02x_1", itemFileId);
        break;
    default:
        snprintf(sItemClassName, sizeof(sItemClassName), "uOm%02x", itemFileId);
        break;
    }

    MtDTI* pItemDTI = MtDTI::FindDTIByName(sItemClassName, MtDTI::DefaultMtDTIParentObject);
    if (!pItemDTI) return false;

    uItem* pItem = pItemDTI->CreateInstance<uItem>();
    if (!pItem) return false;

    if (!pItem->SetupItemProperties()) return false;

    *(DWORD*)(((BYTE*)pItem) + 0x2F48) |= 0x1000;

    if (!sUnit_AddObject(*g_sUnitInstance, 9, pItem)) return false;

    *(Vector4*)(((BYTE*)pItem) + 0x40) = pos;

    return true;
}

static void LogFailedItem(DWORD itemId, int slotNumber)
{
    FILE* f = nullptr;
    
    // Open in APPEND mode ("a") so we don't overwrite previous failures
    // "a" = append text mode (creates file if it doesn't exist)
    fopen_s(&f, FAILED_REWARD_ITEMS, "a");
    if (!f) return;
    
    // Get item name from the table if available
    ItemInfoEntry* info = &uItem::ItemInfoTable[itemId];
    const char* itemName = "Unknown";
    if (info && info->ArchivePath)
        itemName = info->ArchivePath;
    
    // Write a line with: timestamp, slot number, item ID, and item name
    // Format: "Slot 5: Item 181 (Cardboard Box) - Failed to spawn\n"
    fprintf(f, "Slot %d: Item %u (%s) - Failed to spawn\n", 
            slotNumber, itemId, itemName);
    
    fclose(f);
}

// ─────────────────────────────────────────────
//  Public API
// ─────────────────────────────────────────────

int SpawnNextRewardSlotNearPlayer()
{
    if (!g_slotsInitialized)
        GenerateRewardSlots();

    if (g_nextSlot >= (int)g_rewardSlots.size())
    {
        LogLine("[RewardSystem] No more slots to spawn");
        return -1;
    }

    DWORD   itemId  = g_rewardSlots[g_nextSlot];
    Vector4 pos     = GetPlayerSpawnPosition();

    if (SpawnItemAtPosition(itemId, pos))
    {
        char buf[64];
        sprintf_s(buf, "[RewardSystem] Spawned slot %d item=%d", g_nextSlot, itemId);
        LogLine(buf);
        g_nextSlot++;
        return (int)itemId;
    }
    else
    {
        char buf[64];
        sprintf_s(buf, "[RewardSystem] Slot %d item=%d failed to spawn", g_nextSlot, itemId);
        LogLine(buf);
        LogFailedItem(itemId, g_nextSlot);
        g_nextSlot++;
        return (int)itemId; // still return the ID so the notification shows
    }
}