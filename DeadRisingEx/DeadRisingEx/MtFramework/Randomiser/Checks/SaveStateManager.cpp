/*
Rename to Checks manager 
*/


#include "SaveStateManager.h"
#include "../InputSystem.h"
#include "DeadRisingEx/Utilities/DebugLog.h"
#include <fstream>
#include <algorithm>
#include <Windows.h>

std::vector<uint16_t> SaveStateManager::s_completedChecks;

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
    
    // Write version number
    uint32_t version = 1;
    file.write(reinterpret_cast<const char*>(&version), sizeof(version));
    
    // Write number of completed checks
    uint32_t count = static_cast<uint32_t>(s_completedChecks.size());
    file.write(reinterpret_cast<const char*>(&count), sizeof(count));
    
    // Write each check ID
    for (uint16_t checkId : s_completedChecks)
    {
        file.write(reinterpret_cast<const char*>(&checkId), sizeof(checkId));
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
    
    if (version != 1)
    {
        LogLine("[SAVESTATE] Unknown save file version");
        file.close();
        return;
    }
    
    // Read count
    uint32_t count = 0;
    file.read(reinterpret_cast<char*>(&count), sizeof(count));
    
    // Read all check IDs
    s_completedChecks.clear();
    s_completedChecks.reserve(count);
    
    for (uint32_t i = 0; i < count; i++)
    {
        uint16_t checkId = 0;
        file.read(reinterpret_cast<char*>(&checkId), sizeof(checkId));
        s_completedChecks.push_back(checkId);
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