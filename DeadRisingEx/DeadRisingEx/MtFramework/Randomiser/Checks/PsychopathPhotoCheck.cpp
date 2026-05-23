#include "PsychopathPhotoCheck.h"
#include "CheckSystem.h"
#include "DeadRisingEx/MtFramework/Randomiser/InputSystem.h"

#include "DeadRisingEx/Utilities/DebugLog.h"
#include "MtFramework/Utils/Utilities.h"
#include <unordered_set>
#include <unordered_map>
#include <cstring>

// ─────────────────────────────────────────────
//  Psychopath ID mapping (em files to IDs)
// ─────────────────────────────────────────────

struct PsychopathMapping {
    const char* name;
    uint32_t id;
};

static const PsychopathMapping s_psychopathMappings[] = {
    {"em40", 0x6D0},  // SWAT (excluded)
    {"em41", 0x6D1},  // Brock (excluded)
    {"em43", 0x6D3},  // Cultist (excluded)
    {"em44", 0x6D4},  // Sean
    {"em45", 0x6D5},  // Kent
    {"em46", 0x6D6},  // Cliff
    {"em47", 0x6D7},  // Paul
    {"em48", 0x6D8},  // Adam
    {"em49", 0x6D9},  // Larry
    {"em4a", 0x6DA},  // Steven
    {"em4b", 0x6DB},  // Jo
    {"em4c", 0x6DC},  // Sam (Convict)
    {"em4d", 0x6DD},  // Miguel (Convict)
    {"em4e", 0x6DE},  // Reginald (Convict)
    {"em4f", 0x6DF},  // Cletus
    {"em50", 0x6E0},  // unused
    {"em53", 0x6E3},  // Carlito (Food Court)
    {"em56", 0x6E6},  // Isabella (on bike)
    {"em57", 0x6E7},  // Roger Hall
    {"em58", 0x6E8},  // Jack Hall
    {"em59", 0x6E9},  // Thomas Hall
    {"em5a", 0x6E4},  // helicopter (excluded)
};

static const int s_psychopathMappingCount = sizeof(s_psychopathMappings) / sizeof(PsychopathMapping);

// ─────────────────────────────────────────────
//  State tracking
// ─────────────────────────────────────────────

static std::unordered_set<uint32_t> s_knownPsychopaths;
static std::unordered_set<uint32_t> s_photographedPsychopaths;

// Mapping: NPC object pointer → psychopath ID
static std::unordered_map<void*, uint32_t> s_npcObjectToPsychopathId;

// Mapping: Runtime NPC ID (from objectPtr+0x108) → psychopath ID
static std::unordered_map<uint32_t, uint32_t> s_runtimeIdToPsychopathId;

// ─────────────────────────────────────────────
//  Helper functions
// ─────────────────────────────────────────────

bool PsychopathPhotoCheck::IsKnownPsychopath(uint32_t psychopathId)
{
    return s_knownPsychopaths.count(psychopathId) > 0;
}

uint32_t PsychopathPhotoCheck::GetPsychopathIdFromObject(void* npcObject)
{
    auto it = s_npcObjectToPsychopathId.find(npcObject);
    if (it != s_npcObjectToPsychopathId.end())
    {
        return it->second;
    }
    return 0;
}

uint32_t PsychopathPhotoCheck::GetPsychopathIdFromRuntimeId(uint32_t runtimeId)
{
    auto it = s_runtimeIdToPsychopathId.find(runtimeId);
    if (it != s_runtimeIdToPsychopathId.end())
    {
        return it->second;
    }
    return 0;
}

// ─────────────────────────────────────────────
//  Helper to find psychopath ID from path
// ─────────────────────────────────────────────

static uint32_t GetPsychopathIdFromPath(const char* path)
{
    // Expected format: "tool\\scoopList\\em48" or similar
    if (!path || strstr(path, "tool\\scoopList\\") != path)
        return 0;
    
    const char* name = path + strlen("tool\\scoopList\\");
    
    // Convert to lowercase for comparison
    char lowerName[32] = {0};
    for (int i = 0; i < 31 && name[i] != '\0'; i++)
    {
        char c = name[i];
        if (c >= 'A' && c <= 'Z')
            c = c - 'A' + 'a';
        lowerName[i] = c;
    }
    
    // Look up in mapping table
    for (int i = 0; i < s_psychopathMappingCount; i++)
    {
        if (strcmp(lowerName, s_psychopathMappings[i].name) == 0)
        {
            return s_psychopathMappings[i].id;
        }
    }
    
    return 0;
}

static uint32_t SafeReadRuntimeId(void* npcObject, bool& outValid)
{
    uint32_t runtimeId = 0;
    outValid = false;
    __try
    {
        runtimeId = *(uint32_t*)((uintptr_t)npcObject + 0x108);
        outValid = true;
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        outValid = false;
    }
    return runtimeId;
}

// ─────────────────────────────────────────────
//  Public registration function (called from survivor hook)
// ─────────────────────────────────────────────

void PsychopathPhotoCheck::RegisterPsychopathScoop(void* npcObject, const char* scoopPath)
{
    uint32_t psychopathId = GetPsychopathIdFromPath(scoopPath);

    if (psychopathId != 0)
    {
        bool isNew = s_knownPsychopaths.insert(psychopathId).second;

        if (isNew)
        {
            s_npcObjectToPsychopathId[npcObject] = psychopathId;

            bool runtimeIdValid = false;
            uint32_t runtimeId = SafeReadRuntimeId(npcObject, runtimeIdValid);

            if (runtimeIdValid)
            {
                s_runtimeIdToPsychopathId[runtimeId] = psychopathId;

                char buf[256];
                sprintf_s(buf, "[PSYCHOPATH SCOOP] Registered %s → psychopathId=0x%X, npcObject=%p, runtimeId=0x%X",
                    scoopPath, psychopathId, npcObject, runtimeId);
                LogLine(buf);
            }
            else
            {
                char buf[256];
                sprintf_s(buf, "[PSYCHOPATH SCOOP] Registered %s → psychopathId=0x%X, npcObject=%p (runtime ID not readable yet)",
                    scoopPath, psychopathId, npcObject);
                LogLine(buf);
            }
        }
    }
}

// ─────────────────────────────────────────────
//  Public callback
// ─────────────────────────────────────────────

void PsychopathPhotoCheck::OnPsychopathPhotographed(uint32_t psychopathId, int totalCollected, int totalPsychopaths)
{
    if (s_photographedPsychopaths.count(psychopathId) > 0)
        return;
    
    s_photographedPsychopaths.insert(psychopathId);
    
    int total = (int)s_photographedPsychopaths.size();
    bool isKnown = s_knownPsychopaths.count(psychopathId) > 0;
    
    char buf[256];
    sprintf_s(buf, "[PSYCHOPATH PHOTO] *** Photographed psychopath ID=0x%X | Total: %d / 14 | Known: %s ***",
        psychopathId, total, isKnown ? "YES" : "NO");
    LogLine(buf);

    CheckSystem::CompleteCheck(CheckType::PsychopathPhoto, psychopathId);
}

// ─────────────────────────────────────────────
//  Registration
// ─────────────────────────────────────────────

void PsychopathPhotoCheck::RegisterTypeInfo()
{
    // No hook needed - survivor hook already handles it
    LogLine("[PSYCHOPATH PHOTO] Using shared scoop registration hook");
}