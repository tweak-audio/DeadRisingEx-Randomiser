

#include "AreaKeySystem.h"
#include "Misc/AsmHelpers.h"
#include "Checks/ChecksManager.h"
#include "InputSystem.h"

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
    case 0x601: return ZoneID::MeatProcessing;  // 0x601
    case 0x400: return ZoneID::NorthPlaza;
    case 0x501: return ZoneID::CrislipsHomeSaloon;
    case 0x500: return ZoneID::SeonsFood;
    case 0x300: return ZoneID::WonderlandPlaza;
    case 0xa00: return ZoneID::FoodCourt;
    case 0x900: return ZoneID::AlFrescaPlaza;
    default:    return ZoneID::COUNT;
    }
}

uint32_t AreaKeySystem::GetBlockFallback(uint32_t from, uint32_t to)
{
    // Explicit transition map: when zone `to` is locked and Frank came from `from`,
    // reload `fallback`. All entries currently map fallback = from (Frank returns
    // where he came from). Tune individual entries here without touching hook code.
    struct Entry { uint32_t from, to, fallback; };
    static const Entry kTable[] =
    {
        // ── Paradise Plaza (0x200) ───────────────────────────
        { 0x200, 0x700, 0x200 },  // PP → LP
        { 0x200, 0x100, 0x200 },  // PP → EP
        { 0x200, 0x503, 0x200 },  // PP → Movieland
        { 0x200, 0x600, 0x200 },  // PP → MT

        // ── Leisure Park (0x700) ─────────────────────────────
        { 0x700, 0x200, 0x700 },  // LP → PP
        { 0x700, 0x600, 0x700 },  // LP → MT
        { 0x700, 0x400, 0x700 },  // LP → NP
        { 0xa00, 0x700, 0xa00 },  // FC → LP  (FC side)
        // LP → FC: FC is 0xa00, entered from LP
        { 0x700, 0xa00, 0x700 },  // LP → FC

        // ── Entrance Plaza (0x100) ───────────────────────────
        { 0x100, 0x200, 0x100 },  // EP → PP
        { 0x100, 0x900, 0x100 },  // EP → AFP
        { 0x100, 0x600, 0x100 },  // EP → MT

        // ── Al Fresca Plaza (0x900) ──────────────────────────
        { 0x900, 0x100, 0x900 },  // AFP → EP
        { 0x900, 0xa00, 0x900 },  // AFP → FC
        { 0x900, 0x600, 0x900 },  // AFP → MT

        // ── Food Court (0xa00) ───────────────────────────────
        { 0xa00, 0x900, 0xa00 },  // FC → AFP
        { 0xa00, 0x300, 0xa00 },  // FC → WP
        { 0xa00, 0x600, 0xa00 },  // FC → MT

        // ── Wonderland Plaza (0x300) ─────────────────────────
        { 0x300, 0xa00, 0x300 },  // WP → FC
        { 0x300, 0x400, 0x300 },  // WP → NP
        { 0x300, 0x600, 0x300 },  // WP → MT

        // ── North Plaza (0x400) ──────────────────────────────
        { 0x400, 0x300, 0x400 },  // NP → WP
        { 0x400, 0x700, 0x400 },  // NP → LP
        { 0x400, 0x501, 0x400 },  // NP → Crislip's
        { 0x400, 0x500, 0x400 },  // NP → Seon's
        { 0x400, 0x600, 0x400 },  // NP → MT

        // ── Maintenance Tunnel (0x600) — many entry points ───
        { 0x600, 0x700, 0x600 },  // MT → LP
        { 0x600, 0x100, 0x600 },  // MT → EP
        { 0x600, 0x200, 0x600 },  // MT → PP
        { 0x600, 0x300, 0x600 },  // MT → WP
        { 0x600, 0xa00, 0x600 },  // MT → FC
        { 0x600, 0x900, 0x600 },  // MT → AFP
        { 0x600, 0x500, 0x600 },  // MT → Seon's
        { 0x600, 0x400, 0x600 },  // MT → NP
        { 0x600, 0x601, 0x600 },  // MT → Meat Processing

        // ── Meat Processing (0x601) ──────────────────────────
        { 0x601, 0x600, 0x601 },  // Meat Processing → MT (fallback: stay in Meat Processing)

        // ── Crislip's (0x501) / Seon's Food (0x500) ─────────
        { 0x501, 0x400, 0x501 },  // Crislip's → NP
        { 0x500, 0x400, 0x500 },  // Seon's → NP
        { 0x500, 0x600, 0x500 },  // Seon's → MT

        // ── Colby's Movieland (0x503) ────────────────────────
        { 0x503, 0x200, 0x503 },  // Movieland → PP
    };

    for (const auto& e : kTable)
        if (e.from == from && e.to == to)
            return e.fallback;

    return from;  // unlisted transition: default to source area
}

std::vector<ZoneID> AreaKeySystem::GetAdjacentZones(ZoneID zone)
{
    // Derived from the GetBlockFallback transition table — every (from,to) pair
    // in that table implies a physical connection between those two zones.
    switch (zone)
    {
    case ZoneID::ParadisePlaza:
        return { ZoneID::LeisurePark, ZoneID::EntrancePlaza, ZoneID::ColbysMovieland, ZoneID::MaintenanceTunnel };
    case ZoneID::LeisurePark:
        return { ZoneID::ParadisePlaza, ZoneID::MaintenanceTunnel, ZoneID::NorthPlaza, ZoneID::FoodCourt };
    case ZoneID::EntrancePlaza:
        return { ZoneID::ParadisePlaza, ZoneID::AlFrescaPlaza, ZoneID::MaintenanceTunnel };
    case ZoneID::AlFrescaPlaza:
        return { ZoneID::EntrancePlaza, ZoneID::FoodCourt, ZoneID::MaintenanceTunnel };
    case ZoneID::FoodCourt:
        return { ZoneID::AlFrescaPlaza, ZoneID::WonderlandPlaza, ZoneID::LeisurePark, ZoneID::MaintenanceTunnel };
    case ZoneID::WonderlandPlaza:
        return { ZoneID::FoodCourt, ZoneID::NorthPlaza, ZoneID::MaintenanceTunnel };
    case ZoneID::NorthPlaza:
        return { ZoneID::WonderlandPlaza, ZoneID::LeisurePark, ZoneID::CrislipsHomeSaloon, ZoneID::SeonsFood, ZoneID::MaintenanceTunnel };
    case ZoneID::MaintenanceTunnel:
        return { ZoneID::LeisurePark, ZoneID::EntrancePlaza, ZoneID::ParadisePlaza,
                 ZoneID::WonderlandPlaza, ZoneID::FoodCourt, ZoneID::AlFrescaPlaza,
                 ZoneID::SeonsFood, ZoneID::NorthPlaza, ZoneID::MeatProcessing };
    case ZoneID::MeatProcessing:
        return { ZoneID::MaintenanceTunnel };
    case ZoneID::CrislipsHomeSaloon:
        return { ZoneID::NorthPlaza };
    case ZoneID::SeonsFood:
        return { ZoneID::NorthPlaza, ZoneID::MaintenanceTunnel };
    case ZoneID::ColbysMovieland:
        return { ZoneID::ParadisePlaza };
    default:
        return {};
    }
}

const char* AreaKeySystem::GetZoneName(ZoneID zone)
{
    switch (zone)
    {
    case ZoneID::EntrancePlaza:     return "Entrance Plaza";
    case ZoneID::ParadisePlaza:     return "Paradise Plaza";
    case ZoneID::ColbysMovieland:   return "Colby's Movieland";
    case ZoneID::LeisurePark:       return "Leisure Park";
    case ZoneID::MaintenanceTunnel: return "Maintenance Tunnel";
    case ZoneID::MeatProcessing:    return "Meat Processing";
    case ZoneID::NorthPlaza:        return "North Plaza";
    case ZoneID::CrislipsHomeSaloon:return "Crislip's Home Saloon";
    case ZoneID::SeonsFood:         return "Seon's Food & Stuff";
    case ZoneID::WonderlandPlaza:   return "Wonderland Plaza";
    case ZoneID::FoodCourt:         return "Food Court";
    case ZoneID::AlFrescaPlaza:     return "Al Fresca Plaza";
    default:                        return "Unknown Zone";
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
    if (zone >= ZoneID::COUNT) return;
    int idx = static_cast<int>(zone);
    m_hasKey[idx] = true;
    SaveStateManager::SetAreaKeyGranted(idx);
    char buf[64];
    sprintf_s(buf, "[AreaKey] Granted: %s (zone %d)", GetZoneName(zone), idx);
    LogLine(buf);
}

void AreaKeySystem::ReapplyFromSave()
{
    int reapplied = 0;
    for (int i = 0; i < KEY_COUNT; i++)
    {
        if (SaveStateManager::IsAreaKeyGranted(i))
        {
            m_hasKey[i] = true;
            reapplied++;
        }
    }
    char buf[64];
    sprintf_s(buf, "[AreaKeySystem] Reapplied %d area keys from save", reapplied);
    LogLine(buf);
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