


#pragma once
#include <Windows.h>
#include <cstdint>
#include <vector>

enum class ZoneID : int
{
    EntrancePlaza    = 0,  // 0x100
    ParadisePlaza    = 1,  // 0x200
    ColbysMovieland  = 2,  // 0x503
    LeisurePark      = 3,  // 0x700
    MaintenanceTunnel= 4,  // 0x600
    MeatProcessing   = 5,  // 0x601
    NorthPlaza       = 6,  // 0x400
    CrislipsHomeSaloon=7,  // 0x501
    SeonsFood        = 8,  // 0x500
    WonderlandPlaza  = 9,  // 0x300
    FoodCourt        = 10, // 0xa00
    AlFrescaPlaza    = 11, // 0x900
    COUNT            = 12
};

class AreaKeySystem
{
public:
    static AreaKeySystem& Get();
    static constexpr int KEY_COUNT = static_cast<int>(ZoneID::COUNT);
    static void SetAreaTriggersEnabled(bool enabled);

    void Init(uint32_t seed);

    bool HasKey(uint32_t rawAreaId) const;
    void GiveKey(ZoneID zone);
    void OnCheckCompleted(DWORD checkId);

    static ZoneID ZoneFromAreaId(uint32_t rawAreaId);
    static uint32_t GetBlockFallback(uint32_t sourceAreaId, uint32_t lockedAreaId);
    static const char* GetZoneName(ZoneID zone);
    static std::vector<ZoneID> GetAdjacentZones(ZoneID zone);

    void SetIntroComplete(bool complete);
    bool IsIntroComplete() const;

    void ReapplyFromSave();

private:
    bool m_introComplete = false;
    AreaKeySystem() = default;

    DWORD m_keyCheckIds[KEY_COUNT] = {};
    bool  m_hasKey[KEY_COUNT] = {};
};