
#pragma once
#include <cstdint>

namespace SurvivorPhotoCheck
{
    void RegisterTypeInfo();
    void OnSurvivorPhotographed(uint32_t survivorId, int totalCollected, int totalSurvivors);
    bool IsKnownSurvivor(uint32_t survivorId);
    void OnAreaTransition();
    uint32_t GetSurvivorIdFromObject(void* npcObject);
    uint32_t GetSurvivorIdFromRuntimeId(uint32_t runtimeId);
    uint32_t GetSurvivorIdFromPhotoId(uint16_t photoId);
    
    // Add this function to allow psychopath registration through the same hook
    void RegisterScoopFromHook(void* npcObject, const char* scoopPath);
}

constexpr int TOTAL_SURVIVORS = 68;