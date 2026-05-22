#pragma once
#include <string>
#include <vector>
#include <cstdint>

class SaveStateManager
{
public:
    static void Initialize();
    static void SaveCheckState();
    static void LoadCheckState();
    
    // Mark a check as completed
    static void MarkCheckCompleted(uint16_t checkId);
    
    // Check if a check is completed
    static bool IsCheckCompleted(uint16_t checkId);
    
    // Get all completed checks
    static std::vector<uint16_t> GetCompletedChecks();
    
private:
    static std::string GetSaveFilePath();
    static std::vector<uint16_t> s_completedChecks;
};