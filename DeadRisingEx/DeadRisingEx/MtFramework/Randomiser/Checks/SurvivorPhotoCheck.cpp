#include "SurvivorPhotoCheck.h"
#include "PsychopathPhotoCheck.h" 
#include "CheckSystem.h"
#include "../InputSystem.h"

#include "DeadRisingEx/Utilities/DebugLog.h"
#include "MtFramework/Utils/Utilities.h"
#include <detours.h>
#include <unordered_set>
#include <unordered_map>

// ─────────────────────────────────────────────
//  Addresses
// ─────────────────────────────────────────────

constexpr uintptr_t ADDR_SCOOP_SETUP = 0x14014d750;

// ─────────────────────────────────────────────
//  Hook pointer
// ─────────────────────────────────────────────

using FnScoopSetup = uint64_t(__stdcall*)(int64_t param_1, const char* param_2);
static FnScoopSetup s_OrigScoopSetup = nullptr;  // Changed to static and renamed

// ─────────────────────────────────────────────
//  State tracking
// ─────────────────────────────────────────────

static std::unordered_set<uint32_t> s_knownSurvivors;
static std::unordered_set<uint32_t> s_photographedSurvivors;

// Mapping: NPC object pointer → survivor ID
static std::unordered_map<void*, uint32_t> s_npcObjectToSurvivorId;

// Mapping: Runtime NPC ID (from objectPtr+0x108) → survivor ID
static std::unordered_map<uint32_t, uint32_t> s_runtimeIdToSurvivorId;

// Mapping: Photo subject ID (low numbers 1-100) → survivor ID (0x4D0-0x56C)
static std::unordered_map<uint16_t, uint32_t> s_photoIdToSurvivorId;

// ─────────────────────────────────────────────
//  Helper functions
// ─────────────────────────────────────────────

bool SurvivorPhotoCheck::IsKnownSurvivor(uint32_t survivorId)
{
    return s_knownSurvivors.count(survivorId) > 0;
}

uint32_t SurvivorPhotoCheck::GetSurvivorIdFromObject(void* npcObject)
{
    auto it = s_npcObjectToSurvivorId.find(npcObject);
    if (it != s_npcObjectToSurvivorId.end())
    {
        return it->second;
    }
    return 0;
}

uint32_t SurvivorPhotoCheck::GetSurvivorIdFromPhotoId(uint16_t photoId)
{
    auto it = s_photoIdToSurvivorId.find(photoId);
    return (it != s_photoIdToSurvivorId.end()) ? it->second : 0;
}

static bool TryReadPhotoId(void* npcObject, uint16_t& outPhotoId)
{
    __try
    {
        void* component = *(void**)((uintptr_t)npcObject + 0x1010);
        if (component != nullptr)
        {
            outPhotoId = *(uint16_t*)((uintptr_t)component + 0x6e);
            
            if (outPhotoId > 0 && outPhotoId < 1000)
            {
                return true;
            }
        }
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
    }
    
    return false;
}

static bool TryReadRuntimeId(void* npcObject, uint32_t& outRuntimeId)
{
    __try
    {
        outRuntimeId = *(uint32_t*)((uintptr_t)npcObject + 0x108);
        return true;
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
    }
    
    return false;
}

void SurvivorPhotoCheck::UpdateRuntimeMappings()
{
    for (auto& pair : s_npcObjectToSurvivorId)
    {
        void* npcObject = pair.first;
        uint32_t survivorId = pair.second;
        
        uint32_t runtimeId = 0;
        if (TryReadRuntimeId(npcObject, runtimeId) && runtimeId != 0)
        {
            s_runtimeIdToSurvivorId[runtimeId] = survivorId;
            
            char buf[256];
            sprintf_s(buf, "[SURVIVOR PHOTO] Updated runtime mapping: runtimeId=0x%X → survivorId=0x%X", 
                runtimeId, survivorId);
            LogLine(buf);
        }
    }
}

// ─────────────────────────────────────────────
//  Public function for shared hook
// ─────────────────────────────────────────────

void SurvivorPhotoCheck::RegisterScoopFromHook(void* npcObject, const char* scoopPath)
{
    // Handle survivors (npc files)
    if (scoopPath && strstr(scoopPath, "tool\\scoopList\\npc") == scoopPath)
    {
        const char* npcNum = scoopPath + strlen("tool\\scoopList\\npc");
        uint32_t npcIndex  = (uint32_t)strtol(npcNum, nullptr, 16);
        uint32_t survivorId = 0x4D0 + npcIndex;

        bool isNew = s_knownSurvivors.insert(survivorId).second;
        
        if (isNew)
        {
            s_npcObjectToSurvivorId[npcObject] = survivorId;
            
            uint32_t runtimeId = 0;
            uint16_t photoId = 0;
            bool runtimeIdValid = TryReadRuntimeId(npcObject, runtimeId);
            bool photoIdValid = TryReadPhotoId(npcObject, photoId);
            
            if (photoIdValid)
            {
                s_photoIdToSurvivorId[photoId] = survivorId;
            }
            
            char buf[512];
            if (runtimeIdValid && photoIdValid)
            {
                sprintf_s(buf, "[NPC SCOOP] Registered %s → survivorId=0x%X, photoId=0x%04X, runtimeId=0x%X, obj=%p", 
                    scoopPath, survivorId, photoId, runtimeId, npcObject);
            }
            else if (runtimeIdValid)
            {
                sprintf_s(buf, "[NPC SCOOP] Registered %s → survivorId=0x%X, runtimeId=0x%X, obj=%p (photo ID not readable)", 
                    scoopPath, survivorId, runtimeId, npcObject);
            }
            else
            {
                sprintf_s(buf, "[NPC SCOOP] Registered %s → survivorId=0x%X, obj=%p (runtime/photo ID not readable yet)", 
                    scoopPath, survivorId, npcObject);
            }
            LogLine(buf);
            
            if (runtimeIdValid)
            {
                s_runtimeIdToSurvivorId[runtimeId] = survivorId;
            }
        }
    }
    // Handle psychopaths (em files)
    else if (scoopPath && strstr(scoopPath, "tool\\scoopList\\em") == scoopPath)
    {
        const char* emNum = scoopPath + strlen("tool\\scoopList\\em");

        // Skip if it's not a pure hex number (e.g., em08p, em08b have letters after numbers)
        // Only process pure hex like em40, em4e, etc.
        bool isPureHex = true;
        for (int i = 0; i < 2 && emNum[i] != '\0'; i++)
        {
            char c = emNum[i];
            if (!((c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F')))
            {
                isPureHex = false;
                break;
            }
        }
        
        // Also check if there's a third character that's a letter (like 'p' or 'b')
        if (emNum[2] != '\0' && emNum[2] != '\\')
        {
            isPureHex = false;
        }
        
        if (!isPureHex)
        {
            return; // Skip zombie variants like em08p, em08b
        }
        
        uint32_t emHexValue = (uint32_t)strtol(emNum, nullptr, 16);
        
        if (emHexValue == 0x00)
        {
            return; // Skip em00
        }
        
        uint16_t photoId = 0;
        bool photoIdValid = TryReadPhotoId(npcObject, photoId);
        
        uint32_t psychopathId = 0;
        
        // Special handling for Convicts (em4c, em4d, em4e)
        if (emHexValue == 0x4C || emHexValue == 0x4D || emHexValue == 0x4E)
        {
            if (photoIdValid)
            {
                static std::unordered_map<uint16_t, uint32_t> convictPhotoMap;
                static std::unordered_map<uint16_t, const char*> convictNames;
                
                auto it = convictPhotoMap.find(photoId);
                if (it != convictPhotoMap.end())
                {
                    psychopathId = it->second;
                }
                else
                {
                    // Convicts: Sam (0x6DC), Miguel (0x6DD), Reginald (0x6DE)
                    // Map based on em file directly
                    if (emHexValue == 0x4C)
                    {
                        psychopathId = 0x6DC;  // Sam
                        convictNames[photoId] = "Sam (Convict)";
                    }
                    else if (emHexValue == 0x4D)
                    {
                        psychopathId = 0x6DD;  // Miguel
                        convictNames[photoId] = "Miguel (Convict)";
                    }
                    else if (emHexValue == 0x4E)
                    {
                        psychopathId = 0x6DE;  // Reginald
                        convictNames[photoId] = "Reginald (Convict)";
                    }
                    
                    convictPhotoMap[photoId] = psychopathId;
                    
                    char debugBuf[256];
                    sprintf_s(debugBuf, "[CONVICTS] Discovered %s: em%02x, photoId=0x%04X → psychopathId=0x%X", 
                        convictNames[photoId], emHexValue, photoId, psychopathId);
                    LogLine(debugBuf);
                }
            }
            else
            {
                // Fallback if photo ID not readable
                if (emHexValue == 0x4C)
                    psychopathId = 0x6DC;  // Sam
                else if (emHexValue == 0x4D)
                    psychopathId = 0x6DD;  // Miguel
                else if (emHexValue == 0x4E)
                    psychopathId = 0x6DE;  // Reginald
                    
                char debugBuf[256];
                sprintf_s(debugBuf, "[CONVICTS] em%02x spawned but photo ID not readable, assigned psychopathId=0x%X", 
                    emHexValue, psychopathId);
                LogLine(debugBuf);
            }
        }
        // Special handling for Hall Family (em57, em58, em59)
        else if (emHexValue == 0x57 || emHexValue == 0x58 || emHexValue == 0x59)
        {
            if (photoIdValid)
            {
                static std::unordered_map<uint16_t, uint32_t> hallFamilyPhotoMap;
                static std::unordered_map<uint16_t, const char*> hallFamilyNames;
                
                auto it = hallFamilyPhotoMap.find(photoId);
                if (it != hallFamilyPhotoMap.end())
                {
                    psychopathId = it->second;
                }
                else
                {
                    // Hall Family: Roger (0x6E7), Jack (0x6E8), Thomas (0x6E9)
                    if (emHexValue == 0x57)
                    {
                        psychopathId = 0x6E7;  // Roger
                        hallFamilyNames[photoId] = "Roger Hall";
                    }
                    else if (emHexValue == 0x58)
                    {
                        psychopathId = 0x6E8;  // Jack
                        hallFamilyNames[photoId] = "Jack Hall";
                    }
                    else if (emHexValue == 0x59)
                    {
                        psychopathId = 0x6E9;  // Thomas
                        hallFamilyNames[photoId] = "Thomas Hall";
                    }
                    
                    hallFamilyPhotoMap[photoId] = psychopathId;
                    
                    char debugBuf[256];
                    sprintf_s(debugBuf, "[HALL FAMILY] Discovered %s: em%02x, photoId=0x%04X → psychopathId=0x%X", 
                        hallFamilyNames[photoId], emHexValue, photoId, psychopathId);
                    LogLine(debugBuf);
                }
            }
            else
            {
                // Fallback if photo ID not readable
                if (emHexValue == 0x57)
                    psychopathId = 0x6E7;  // Roger
                else if (emHexValue == 0x58)
                    psychopathId = 0x6E8;  // Jack
                else if (emHexValue == 0x59)
                    psychopathId = 0x6E9;  // Thomas
                    
                char debugBuf[256];
                sprintf_s(debugBuf, "[HALL FAMILY] em%02x spawned but photo ID not readable, assigned psychopathId=0x%X", 
                    emHexValue, psychopathId);
                LogLine(debugBuf);
            }
        }
        else
        {
            // Standard formula for other psychopaths
            psychopathId = 0x6D0 + (emHexValue - 0x40);
        }
        
        if (psychopathId != 0 && photoIdValid)
        {
            s_photoIdToSurvivorId[photoId] = psychopathId;
            
            char buf[512];
            sprintf_s(buf, "[PSYCHOPATH SCOOP] Registered %s → psychopathId=0x%X, photoId=0x%04X, obj=%p", 
                scoopPath, psychopathId, photoId, npcObject);
            LogLine(buf);
            
            PsychopathPhotoCheck::RegisterPsychopathScoop(npcObject, scoopPath);
        }
        else if (psychopathId != 0)
        {
            char buf[512];
            sprintf_s(buf, "[PSYCHOPATH SCOOP] Registered %s → psychopathId=0x%X, obj=%p (photo ID not readable)", 
                scoopPath, psychopathId, npcObject);
            LogLine(buf);
            
            PsychopathPhotoCheck::RegisterPsychopathScoop(npcObject, scoopPath);
        }
    }
}

// ─────────────────────────────────────────────
//  Hook — Scoop setup (track both survivors and psychopaths)
// ─────────────────────────────────────────────

uint64_t __stdcall Hook_ScoopSetup(int64_t param_1, const char* param_2)
{
    uint64_t result = s_OrigScoopSetup(param_1, param_2);
    
    void* npcObject = (void*)param_1;
    SurvivorPhotoCheck::RegisterScoopFromHook(npcObject, param_2);
    
    return result;
}

// ─────────────────────────────────────────────
//  Public callback
// ─────────────────────────────────────────────

void SurvivorPhotoCheck::OnSurvivorPhotographed(uint32_t survivorId, int totalCollected, int totalSurvivors)
{
    if (s_photographedSurvivors.count(survivorId) > 0)
        return;
    
    s_photographedSurvivors.insert(survivorId);
    
    int total = (int)s_photographedSurvivors.size();
    bool isKnown = s_knownSurvivors.count(survivorId) > 0;
    
    char buf[256];
    sprintf_s(buf, "[SURVIVOR PHOTO] *** Photographed survivor ID=0x%X | Total: %d / 67 | Known: %s ***",
        survivorId, total, isKnown ? "YES" : "NO");
    LogLine(buf);

    CheckSystem::CompleteCheck(CheckType::SurvivorPhoto, survivorId);
}

// ─────────────────────────────────────────────
//  Registration
// ─────────────────────────────────────────────

void SurvivorPhotoCheck::RegisterTypeInfo()
{
    s_OrigScoopSetup = (FnScoopSetup)GetModuleAddress(ADDR_SCOOP_SETUP);
    DetourAttach((void**)&s_OrigScoopSetup, Hook_ScoopSetup);
    
    LogLine("[SURVIVOR PHOTO] Scoop registration hook installed (handles survivors and psychopaths)");
}