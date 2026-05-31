

#include "AreaKeySystem.h"
#include "Misc/AsmHelpers.h"

AreaKeySystem& AreaKeySystem::Get()
{
    static AreaKeySystem instance;
    return instance;
}

void AreaKeySystem::SetIntroComplete(bool complete)
{
    m_introComplete = complete;
}

bool AreaKeySystem::IsIntroComplete() const
{
    return m_introComplete;
}

ZoneID AreaKeySystem::ZoneFromAreaId(uint32_t id)
{
    // Always unlocked — intro sequence only
    if (id == 0x000 || id == 0x003) return ZoneID::COUNT;
    if (id == 0x11f || id == 0x120) return ZoneID::COUNT; // Helipad/Security Room
    if (id == 0x216 || id == 0x217) return ZoneID::COUNT; // Rooftop/Warehouse

    switch (id)
    {
    case 0x100: return ZoneID::EntrancePlaza;
    case 0x200: return ZoneID::ParadisePlaza;
    case 0x503: return ZoneID::ColbysMovieland;
    case 0x700: return ZoneID::LeisurePark;
    case 0x600: return ZoneID::MaintenanceTunnel;
    case 0x601: return ZoneID::MeatProcessing;
    case 0x400: return ZoneID::NorthPlaza;
    case 0x501: return ZoneID::CrislipsHomeSaloon;
    case 0x500: return ZoneID::SeonsFood;
    case 0x300: return ZoneID::WonderlandPlaza;
    case 0xa00: return ZoneID::FoodCourt;
    case 0x900: return ZoneID::AlFrescaPlaza;
    default:    return ZoneID::COUNT;
    }
}

void AreaKeySystem::Init(uint32_t seed)
{
    memset(m_hasKey, 0, sizeof(m_hasKey));
    m_introComplete = false;
    (void)seed;
}

bool AreaKeySystem::HasKey(uint32_t rawAreaId) const
{
    if (!m_introComplete) return true;

    ZoneID zone = ZoneFromAreaId(rawAreaId);
    if (zone == ZoneID::COUNT) return true;
    return m_hasKey[static_cast<int>(zone)];
}

void AreaKeySystem::GiveKey(ZoneID zone)
{
    if (zone < ZoneID::COUNT)
        m_hasKey[static_cast<int>(zone)] = true;
}

void AreaKeySystem::OnCheckCompleted(DWORD checkId)
{
    for (int i = 0; i < KEY_COUNT; ++i)
    {
        if (m_keyCheckIds[i] == checkId && !m_hasKey[i])
        {
            m_hasKey[i] = true;
            // TODO: HUD message
            break;
        }
    }
}

void AreaKeySystem::SetAreaTriggersEnabled(bool enabled)
{
    void** gameManagerPtr = reinterpret_cast<void**>(GetModuleAddress(0x141cf2aa0));
    if (!gameManagerPtr || !*gameManagerPtr) return;

    __try
    {
        uint8_t* flag = reinterpret_cast<uint8_t*>(
            reinterpret_cast<uintptr_t>(*gameManagerPtr) + 0x20e7c);

        if (!enabled)
            *flag |= 0x10;
        else
            *flag &= ~0x10;
    }
    __except(EXCEPTION_EXECUTE_HANDLER) {}
}