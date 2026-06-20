/*
Save Checks to file to be recalled later
*/


#include "ChecksManager.h"
#include "DeadRisingEx/MtFramework/Randomiser/InputSystem.h"
#include "DeadRisingEx/Utilities/DebugLog.h"
#include <fstream>
#include <algorithm>
#include <Windows.h>

std::vector<uint16_t> SaveStateManager::s_completedChecks;
uint8_t SaveStateManager::s_rewardCostumeIds[6] = {};
bool    SaveStateManager::s_hasRewardCostume[6] = {};
bool    SaveStateManager::s_areaKeysGranted[12] = {};

void SaveStateManager::Initialize()
{
    LogLine("[SAVESTATE] Initializing SaveStateManager");
    LoadCheckState();
}

std::string SaveStateManager::GetSaveFilePath()
{
    // Save in the same directory as the DLL
    char dllPath[MAX_PATH];
    HMODULE hModule = NULL;
    
    if (GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | 
                           GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                           (LPCSTR)&GetSaveFilePath, &hModule))
    {
        GetModuleFileNameA(hModule, dllPath, MAX_PATH);
        std::string path(dllPath);
        size_t lastSlash = path.find_last_of("\\/");
        if (lastSlash != std::string::npos)
        {
            path = path.substr(0, lastSlash + 1);
        }
        return path + "archipelago_checks.dat";
    }
    
    return "archipelago_checks.dat";
}

void SaveStateManager::SaveCheckState()
{
    std::string filePath = GetSaveFilePath();
    
    std::ofstream file(filePath, std::ios::binary);
    if (!file.is_open())
    {
        char buf[256];
        sprintf_s(buf, "[SAVESTATE] Failed to open file for writing: %s", filePath.c_str());
        LogLine(buf);
        return;
    }
    
    uint32_t version = 3;
    file.write(reinterpret_cast<const char*>(&version), sizeof(version));

    uint32_t count = static_cast<uint32_t>(s_completedChecks.size());
    file.write(reinterpret_cast<const char*>(&count), sizeof(count));

    for (uint16_t checkId : s_completedChecks)
        file.write(reinterpret_cast<const char*>(&checkId), sizeof(checkId));

    // Reward costume state (6 id bytes + 6 flag bytes)
    file.write(reinterpret_cast<const char*>(s_rewardCostumeIds), sizeof(s_rewardCostumeIds));
    for (int i = 0; i < 6; i++)
    {
        uint8_t flag = s_hasRewardCostume[i] ? 1 : 0;
        file.write(reinterpret_cast<const char*>(&flag), sizeof(flag));
    }

    // Area key state (12 bytes)
    for (int i = 0; i < 12; i++)
    {
        uint8_t flag = s_areaKeysGranted[i] ? 1 : 0;
        file.write(reinterpret_cast<const char*>(&flag), sizeof(flag));
    }
    
    file.close();
    
    char buf[256];
    sprintf_s(buf, "[SAVESTATE] Saved %d completed checks to: %s", count, filePath.c_str());
    LogLine(buf);
}

void SaveStateManager::LoadCheckState()
{
    std::string filePath = GetSaveFilePath();
    
    std::ifstream file(filePath, std::ios::binary);
    if (!file.is_open())
    {
        char buf[256];
        sprintf_s(buf, "[SAVESTATE] No save file found: %s (starting fresh)", filePath.c_str());
        LogLine(buf);
        return;
    }
    
    // Read version
    uint32_t version = 0;
    file.read(reinterpret_cast<char*>(&version), sizeof(version));
    
    if (version < 1 || version > 3)
    {
        LogLine("[SAVESTATE] Unknown save file version");
        file.close();
        return;
    }

    uint32_t count = 0;
    file.read(reinterpret_cast<char*>(&count), sizeof(count));

    s_completedChecks.clear();
    s_completedChecks.reserve(count);

    for (uint32_t i = 0; i < count; i++)
    {
        uint16_t checkId = 0;
        file.read(reinterpret_cast<char*>(&checkId), sizeof(checkId));
        s_completedChecks.push_back(checkId);
    }

    if (version >= 2)
    {
        file.read(reinterpret_cast<char*>(s_rewardCostumeIds), sizeof(s_rewardCostumeIds));
        for (int i = 0; i < 6; i++)
        {
            uint8_t flag = 0;
            file.read(reinterpret_cast<char*>(&flag), sizeof(flag));
            s_hasRewardCostume[i] = flag != 0;
        }
    }

    if (version >= 3)
    {
        for (int i = 0; i < 12; i++)
        {
            uint8_t flag = 0;
            file.read(reinterpret_cast<char*>(&flag), sizeof(flag));
            s_areaKeysGranted[i] = flag != 0;
        }
    }
    
    file.close();
    
    char buf[256];
    sprintf_s(buf, "[SAVESTATE] Loaded %d completed checks from: %s", count, filePath.c_str());
    LogLine(buf);
}

void SaveStateManager::MarkCheckCompleted(uint16_t checkId)
{
    // Don't add duplicates
    if (std::find(s_completedChecks.begin(), s_completedChecks.end(), checkId) != s_completedChecks.end())
    {
        return;
    }
    
    s_completedChecks.push_back(checkId);
    
    char buf[128];
    sprintf_s(buf, "[SAVESTATE] Marked check %d as completed (%d total)", 
        checkId, (int)s_completedChecks.size());
    LogLine(buf);
    
    // Auto-save after each check
    SaveCheckState();
}

bool SaveStateManager::IsCheckCompleted(uint16_t checkId)
{
    return std::find(s_completedChecks.begin(), s_completedChecks.end(), checkId) != s_completedChecks.end();
}

std::vector<uint16_t> SaveStateManager::GetCompletedChecks()
{
    return s_completedChecks;
}

void SaveStateManager::ResetCompletedChecks()
{
    s_completedChecks.clear();
    SaveCheckState();
    LogLine("[SAVESTATE] Completed checks reset (re-seed)");
}

void SaveStateManager::SetRewardCostume(uint8_t slot, uint8_t id)
{
    if (slot >= 6) return;
    s_rewardCostumeIds[slot]  = id;
    s_hasRewardCostume[slot]  = true;
    SaveCheckState();
}

bool SaveStateManager::HasRewardCostume(uint8_t slot)
{
    return slot < 6 && s_hasRewardCostume[slot];
}

uint8_t SaveStateManager::GetRewardCostumeId(uint8_t slot)
{
    return slot < 6 ? s_rewardCostumeIds[slot] : 0;
}

void SaveStateManager::SetAreaKeyGranted(int zoneIdx)
{
    if (zoneIdx < 0 || zoneIdx >= 12) return;
    if (s_areaKeysGranted[zoneIdx]) return;
    s_areaKeysGranted[zoneIdx] = true;
    SaveCheckState();
}

bool SaveStateManager::IsAreaKeyGranted(int zoneIdx)
{
    return zoneIdx >= 0 && zoneIdx < 12 && s_areaKeysGranted[zoneIdx];
}