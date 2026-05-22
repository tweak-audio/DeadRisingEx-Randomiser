

#pragma once
#include <cstdint>

namespace PsychopathPhotoCheck
{
    void RegisterTypeInfo();
    void OnPsychopathPhotographed(uint32_t psychopathId, int totalCollected, int totalPsychopaths);
    void RegisterPsychopathScoop(void* npcObject, const char* scoopPath);
    bool IsKnownPsychopath(uint32_t psychopathId);
    uint32_t GetPsychopathIdFromObject(void* npcObject);
    uint32_t GetPsychopathIdFromRuntimeId(uint32_t runtimeId);
}

constexpr int TOTAL_PSYCHOPATHS = 17;