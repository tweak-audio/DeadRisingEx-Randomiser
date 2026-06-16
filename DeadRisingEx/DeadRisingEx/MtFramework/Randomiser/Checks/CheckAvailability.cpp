/*
This is so the randomizer can have logic
*/


#include "CheckAvailability.h"
#include "DeadRisingEx/Utilities/DebugLog.h"
#include "DeadRisingEx/MtFramework/Randomiser/InputSystem.h"

std::unordered_map<CheckId, CheckAvailabilityInfo, CheckIdHash> CheckAvailability::s_availabilityDB;

// ─────────────────────────────────────────────────────────────────────────────
//  Shared table entry — used for PP stickers, survivors, psychopaths, costumes.
//  PP stickers/survivors/psychopaths notes: "Name - Store/Location, Zone"
//  Costume notes: "Costume Name" only (no shop suffix).
//  GetCheckName() returns everything before the first " - ", or the full string.
// ─────────────────────────────────────────────────────────────────────────────
struct LocationEntry
{
    uint32_t    checkId;
    ZoneID      zone;
    const char* notes;   // "Item Name - Store, Zone name"
};

void CheckAvailability::Initialize()
{
    LogLine("[AVAILABILITY] Building check availability database...");

    RegisterPPStickerTimes();
    RegisterSurvivorTimes();
    RegisterPsychopathTimes();
    RegisterCostumeTimes();
    BuildCostumeLookup();

    // Key items — zones need verifying once all 4 event IDs are confirmed in-game
    // id=0: uOm0028 (MT key, event 0x822) — found inside MaintenanceTunnel
    // id=1: uOm0081 (event TBD) — zone TBD
    // id=2: uOm0084 (event TBD) — zone TBD
    // id=3: uOm00de (event TBD) — zone TBD
    s_availabilityDB[{ CheckType::KeyItem, 0 }] = { TimeManager::GAME_START_TICK, false, ZoneID::MaintenanceTunnel, "MT Key" };
    s_availabilityDB[{ CheckType::KeyItem, 1 }] = { TimeManager::GAME_START_TICK, false, ZoneID::COUNT,             "Key Item uOm0081" };
    s_availabilityDB[{ CheckType::KeyItem, 2 }] = { TimeManager::GAME_START_TICK, false, ZoneID::COUNT,             "Key Item uOm0084" };
    s_availabilityDB[{ CheckType::KeyItem, 3 }] = { TimeManager::GAME_START_TICK, false, ZoneID::COUNT,             "Key Item uOm00de" };

    char buf[128];
    sprintf_s(buf, "[AVAILABILITY] Registered %d checks", (int)s_availabilityDB.size());
    LogLine(buf);
}

// ─────────────────────────────────────────────────────────────────────────────
//  PP Sticker table
//  ID range 128-227. IDs 224 and 227 are cult-hideout (handled separately).
//  Fill in "Name - Store/location, Zone" and the correct ZoneID per sticker.
// ─────────────────────────────────────────────────────────────────────────────
static const LocationEntry kPPStickerDB[] =
{
    // id   zone                          notes
    { 128,  ZoneID::ParadisePlaza,      "PP Sticker 128 - TODO" },
    { 129,  ZoneID::FoodCourt,          "PP Sticker 129 - TODO" },
    { 130,  ZoneID::ColbysMovieland,    "PP Sticker 130 - TODO" },
    { 131,  ZoneID::ParadisePlaza,      "PP Sticker 131 - TODO" },
    { 132,  ZoneID::SeonsFood,          "PP Sticker 132 - TODO" },
    { 133,  ZoneID::NorthPlaza,         "PP Sticker 133 - TODO" },
    { 134,  ZoneID::WonderlandPlaza,    "PP Sticker 134 - TODO" },
    { 135,  ZoneID::WonderlandPlaza,    "PP Sticker 135 - TODO" },
    { 136,  ZoneID::WonderlandPlaza,    "PP Sticker 136 - TODO" },
    { 137,  ZoneID::AlFrescaPlaza,      "PP Sticker 137 - TODO" },
    { 138,  ZoneID::AlFrescaPlaza,      "PP Sticker 138 - TODO" },
    { 139,  ZoneID::AlFrescaPlaza,      "PP Sticker 139 - TODO" },
    { 140,  ZoneID::FoodCourt,          "PP Sticker 140 - TODO" },
    { 141,  ZoneID::FoodCourt,          "PP Sticker 141 - TODO" },
    { 142,  ZoneID::FoodCourt,          "PP Sticker 142 - TODO" },
    { 143,  ZoneID::ParadisePlaza,      "PP Sticker 143 - TODO" },
    { 144,  ZoneID::ParadisePlaza,      "PP Sticker 144 - TODO" },
    { 145,  ZoneID::ParadisePlaza,      "PP Sticker 145 - TODO" },
    { 146,  ZoneID::FoodCourt,          "PP Sticker 146 - TODO" },
    { 147,  ZoneID::FoodCourt,          "PP Sticker 147 - TODO" },
    { 148,  ZoneID::FoodCourt,          "PP Sticker 148 - TODO" },
    { 149,  ZoneID::FoodCourt,          "PP Sticker 149 - TODO" },
    { 150,  ZoneID::WonderlandPlaza,    "PP Sticker 150 - TODO" },
    { 151,  ZoneID::NorthPlaza,         "PP Sticker 151 - TODO" },
    { 152,  ZoneID::NorthPlaza,         "PP Sticker 152 - TODO" },
    { 153,  ZoneID::ParadisePlaza,      "PP Sticker 153 - TODO" },
    { 154,  ZoneID::ParadisePlaza,      "PP Sticker 154 - TODO" },
    { 155,  ZoneID::NorthPlaza,         "PP Sticker 155 - TODO" },
    { 156,  ZoneID::NorthPlaza,         "PP Sticker 156 - TODO" },
    { 157,  ZoneID::NorthPlaza,         "PP Sticker 157 - TODO" },
    { 158,  ZoneID::NorthPlaza,         "PP Sticker 158 - TODO" },
    { 159,  ZoneID::EntrancePlaza,      "PP Sticker 159 - TODO" },
    { 160,  ZoneID::WonderlandPlaza,    "PP Sticker 160 - TODO" },
    { 161,  ZoneID::LeisurePark,        "PP Sticker 161 - TODO" },
    { 162,  ZoneID::ParadisePlaza,      "PP Sticker 162 - TODO" },
    { 163,  ZoneID::ParadisePlaza,      "PP Sticker 163 - TODO" },
    { 164,  ZoneID::ParadisePlaza,      "PP Sticker 164 - TODO" },
    { 165,  ZoneID::COUNT,              "PP Sticker 165 - TODO" },
    { 166,  ZoneID::EntrancePlaza,      "PP Sticker 166 - TODO" },
    { 167,  ZoneID::ParadisePlaza,      "PP Sticker 167 - TODO" },
    { 168,  ZoneID::ParadisePlaza,      "PP Sticker 168 - TODO" },
    { 169,  ZoneID::EntrancePlaza,      "PP Sticker 169 - TODO" },
    { 170,  ZoneID::EntrancePlaza,      "PP Sticker 170 - TODO" },
    { 171,  ZoneID::EntrancePlaza,      "PP Sticker 171 - TODO" },
    { 172,  ZoneID::EntrancePlaza,      "PP Sticker 172 - TODO" },
    { 173,  ZoneID::WonderlandPlaza,    "PP Sticker 173 - TODO" },
    { 174,  ZoneID::COUNT,              "PP Sticker 174 - TODO" },
    { 175,  ZoneID::WonderlandPlaza,    "PP Sticker 175 - TODO" },
    { 176,  ZoneID::WonderlandPlaza,    "PP Sticker 176 - TODO" },
    { 177,  ZoneID::WonderlandPlaza,    "PP Sticker 177 - TODO" },
    { 178,  ZoneID::WonderlandPlaza,    "PP Sticker 178 - TODO" },
    { 179,  ZoneID::NorthPlaza,         "PP Sticker 179 - TODO" },
    { 180,  ZoneID::NorthPlaza,         "PP Sticker 180 - TODO" },
    { 181,  ZoneID::SeonsFood,          "PP Sticker 181 - TODO" },
    { 182,  ZoneID::SeonsFood,          "PP Sticker 182 - TODO" },
    { 183,  ZoneID::ParadisePlaza,      "PP Sticker 183 - TODO" },
    { 184,  ZoneID::ParadisePlaza,      "PP Sticker 184 - TODO" },
    { 185,  ZoneID::WonderlandPlaza,    "PP Sticker 185 - TODO" },
    { 186,  ZoneID::WonderlandPlaza,    "PP Sticker 186 - TODO" },
    { 187,  ZoneID::WonderlandPlaza,    "PP Sticker 187 - TODO" },
    { 188,  ZoneID::WonderlandPlaza,    "PP Sticker 188 - TODO" },
    { 189,  ZoneID::WonderlandPlaza,    "PP Sticker 189 - TODO" },
    { 190,  ZoneID::EntrancePlaza,      "PP Sticker 190 - TODO" },
    { 191,  ZoneID::EntrancePlaza,      "PP Sticker 191 - TODO" },
    { 192,  ZoneID::CrislipsHomeSaloon, "PP Sticker 192 - TODO" },
    { 193,  ZoneID::CrislipsHomeSaloon, "PP Sticker 193 - TODO" },
    { 194,  ZoneID::ColbysMovieland,    "PP Sticker 194 - TODO" },
    { 195,  ZoneID::ParadisePlaza,      "PP Sticker 195 - TODO" },
    { 196,  ZoneID::ColbysMovieland,    "PP Sticker 196 - TODO" },
    { 197,  ZoneID::ColbysMovieland,    "PP Sticker 197 - TODO" },
    { 198,  ZoneID::ColbysMovieland,    "PP Sticker 198 - TODO" },
    { 199,  ZoneID::ColbysMovieland,    "PP Sticker 199 - TODO" },
    { 200,  ZoneID::ColbysMovieland,    "PP Sticker 200 - TODO" },
    { 201,  ZoneID::ColbysMovieland,    "PP Sticker 201 - TODO" },
    { 202,  ZoneID::MaintenanceTunnel,  "PP Sticker 202 - TODO" },
    { 203,  ZoneID::MaintenanceTunnel,  "PP Sticker 203 - TODO" },
    { 204,  ZoneID::MaintenanceTunnel,  "PP Sticker 204 - TODO" },
    { 205,  ZoneID::MaintenanceTunnel,  "PP Sticker 205 - TODO" },
    { 206,  ZoneID::MaintenanceTunnel,  "PP Sticker 206 - TODO" },
    { 207,  ZoneID::MeatProcessing,     "PP Sticker 207 - TODO" },
    { 208,  ZoneID::MeatProcessing,     "PP Sticker 208 - TODO" },
    { 209,  ZoneID::LeisurePark,        "PP Sticker 209 - TODO" },
    { 210,  ZoneID::LeisurePark,        "PP Sticker 210 - TODO" },
    { 211,  ZoneID::LeisurePark,        "PP Sticker 211 - TODO" },
    { 212,  ZoneID::AlFrescaPlaza,      "PP Sticker 212 - TODO" },
    { 213,  ZoneID::AlFrescaPlaza,      "PP Sticker 213 - TODO" },
    { 214,  ZoneID::AlFrescaPlaza,      "PP Sticker 214 - TODO" },
    { 215,  ZoneID::AlFrescaPlaza,      "PP Sticker 215 - TODO" },
    { 216,  ZoneID::AlFrescaPlaza,      "PP Sticker 216 - TODO" },
    { 217,  ZoneID::AlFrescaPlaza,      "PP Sticker 217 - TODO" },
    { 218,  ZoneID::AlFrescaPlaza,      "PP Sticker 218 - TODO" },
    { 219,  ZoneID::AlFrescaPlaza,      "PP Sticker 219 - TODO" },
    { 220,  ZoneID::FoodCourt,          "PP Sticker 220 - TODO" },
    { 221,  ZoneID::FoodCourt,          "PP Sticker 221 - TODO" },
    { 222,  ZoneID::FoodCourt,          "PP Sticker 222 - TODO" },
    { 223,  ZoneID::EntrancePlaza,      "PP Sticker 223 - TODO" },
    { 225,  ZoneID::EntrancePlaza,      "PP Sticker 225 - TODO" },
    { 226,  ZoneID::COUNT,              "PP Sticker 226 - TODO" },
};

void CheckAvailability::RegisterPPStickerTimes()
{
    for (const auto& e : kPPStickerDB)
        s_availabilityDB[{CheckType::PPSticker, e.checkId}] = {
            TimeManager::GAME_START_TICK, false, e.zone, e.notes };

    // Cult Hideout stickers — time-gated (cultists spawn after 25 in-game hours)
    uint32_t cultTime = TimeManager::GAME_START_TICK + (25 * TimeManager::TICKS_PER_HOUR);
    s_availabilityDB[{CheckType::PPSticker, 224}] = {
        cultTime, true, ZoneID::COUNT, "Cult Hideout Sticker A - Cult Hideout" };
    s_availabilityDB[{CheckType::PPSticker, 227}] = {
        cultTime, true, ZoneID::COUNT, "Cult Hideout Sticker B - Cult Hideout" };
}

void CheckAvailability::RegisterSurvivorTimes()
{
    auto TimeOfDay = [](int day, int hour, int minute, bool isPM) {
        int hour24 = hour;
        if (isPM && hour != 12)
            hour24 += 12;
        else if (!isPM && hour == 12)
            hour24 = 0;

        int gameStartHour = 12;
        int hoursFromStart = ((day - 1) * 24) + (hour24 - gameStartHour);

        return TimeManager::GAME_START_TICK +
               (hoursFromStart * TimeManager::TICKS_PER_HOUR) +
               (minute * TimeManager::TICKS_PER_MINUTE);
    };

    // === DAY 1 - Sept 19 ===

    // 12:00 PM - Game Start

    // Entrance Plaza
    s_availabilityDB[{CheckType::SurvivorPhoto, 0x555}] = {
        TimeOfDay(1, 12, 0, true), false, ZoneID::EntrancePlaza, "Lindsey - Entrance Plaza" };
    s_availabilityDB[{CheckType::SurvivorPhoto, 0x560}] = {
        TimeOfDay(1, 12, 0, true), false, ZoneID::EntrancePlaza, "Ryan - Entrance Plaza" };
    s_availabilityDB[{CheckType::SurvivorPhoto, 0x561}] = {
        TimeOfDay(1, 12, 0, true), false, ZoneID::EntrancePlaza, "Chris - Entrance Plaza" };
    s_availabilityDB[{CheckType::SurvivorPhoto, 0x562}] = {
        TimeOfDay(1, 12, 0, true), false, ZoneID::EntrancePlaza, "Todd - Entrance Plaza" };
    s_availabilityDB[{CheckType::SurvivorPhoto, 0x563}] = {
        TimeOfDay(1, 12, 0, true), false, ZoneID::EntrancePlaza, "Brian - Entrance Plaza" };
    s_availabilityDB[{CheckType::SurvivorPhoto, 0x564}] = {
        TimeOfDay(1, 12, 0, true), false, ZoneID::EntrancePlaza, "Dana - Entrance Plaza" };
    s_availabilityDB[{CheckType::SurvivorPhoto, 0x565}] = {
        TimeOfDay(1, 12, 0, true), false, ZoneID::EntrancePlaza, "Verlene - Entrance Plaza" };
    s_availabilityDB[{CheckType::SurvivorPhoto, 0x566}] = {
        TimeOfDay(1, 12, 0, true), false, ZoneID::EntrancePlaza, "Mark - Entrance Plaza" };
    s_availabilityDB[{CheckType::SurvivorPhoto, 0x567}] = {
        TimeOfDay(1, 12, 0, true), false, ZoneID::EntrancePlaza, "Kathy - Entrance Plaza" };
    s_availabilityDB[{CheckType::SurvivorPhoto, 0x568}] = {
        TimeOfDay(1, 12, 0, true), false, ZoneID::EntrancePlaza, "Alan - Entrance Plaza" };
    s_availabilityDB[{CheckType::SurvivorPhoto, 0x56C}] = {
        TimeOfDay(1, 12, 0, true), false, ZoneID::EntrancePlaza, "Freddie - Entrance Plaza" };

    // Special areas — always reachable (Security Room, Rooftop)
    s_availabilityDB[{CheckType::SurvivorPhoto, 0x550}] = {
        TimeOfDay(1, 12, 0, true), false, ZoneID::FoodCourt, "Brad - Food Court" };
    s_availabilityDB[{CheckType::SurvivorPhoto, 0x551}] = {
        TimeOfDay(1, 12, 0, true), false, ZoneID::COUNT, "Jessie - Security Room" };
    s_availabilityDB[{CheckType::SurvivorPhoto, 0x556}] = {
        TimeOfDay(1, 12, 0, true), false, ZoneID::COUNT, "Otis - Security Room" };
    s_availabilityDB[{CheckType::SurvivorPhoto, 0x552}] = {
        TimeOfDay(1, 12, 0, true), false, ZoneID::COUNT, "Carlito - Rooftop" };
    s_availabilityDB[{CheckType::SurvivorPhoto, 0x4D5}] = {
        TimeOfDay(1, 12, 0, true), false, ZoneID::COUNT, "Jeff Meyer - Rooftop" };
    s_availabilityDB[{CheckType::SurvivorPhoto, 0x4D2}] = {
        TimeOfDay(1, 12, 0, true), false, ZoneID::COUNT, "Natalie Meyer - Rooftop" };
    s_availabilityDB[{CheckType::SurvivorPhoto, 0x4E4}] = {
        TimeOfDay(1, 12, 0, true), false, ZoneID::EntrancePlaza, "Bill Brenton - In The Closet, Entrance Plaza" };

    // Paradise Plaza
    s_availabilityDB[{CheckType::SurvivorPhoto, 0x4F1}] = {
        TimeOfDay(1, 2, 0, true), false, ZoneID::ParadisePlaza, "Kent - Columbian Roastmaster, Paradise Plaza" };

    // 4:00 PM
    s_availabilityDB[{CheckType::SurvivorPhoto, 0x4D4}] = {
        TimeOfDay(1, 4, 0, true), false, ZoneID::AlFrescaPlaza, "Aaron Swoop - Weber's Garments, Al Fresca Plaza" };
    s_availabilityDB[{CheckType::SurvivorPhoto, 0x4D0}] = {
        TimeOfDay(1, 4, 0, true), false, ZoneID::AlFrescaPlaza, "Burt Thompson - Weber's Garments, Al Fresca Plaza" };

    // 5:00 PM
    s_availabilityDB[{CheckType::SurvivorPhoto, 0x4DC}] = {
        TimeOfDay(1, 5, 0, true), false, ZoneID::AlFrescaPlaza, "Leah Stein - Riverfield Jewelry, Al Fresca Plaza" };

    // 6:00 PM
    s_availabilityDB[{CheckType::SurvivorPhoto, 0x4EF}] = {
        TimeOfDay(1, 6, 0, true), false, ZoneID::LeisurePark, "Sophie Richard - Leisure Park" };

    // 9:00 PM
    s_availabilityDB[{CheckType::SurvivorPhoto, 0x524}] = {
        TimeOfDay(1, 9, 0, true), false, ZoneID::WonderlandPlaza, "Greg Simpson - Wonderland Plaza Space Ride" };

    // 10:00 PM
    s_availabilityDB[{CheckType::SurvivorPhoto, 0x4DF}] = {
        TimeOfDay(1, 10, 0, true), false, ZoneID::WonderlandPlaza, "Yuu Tanaka - Sir Book-A-Lot, Wonderland Plaza" };
    s_availabilityDB[{CheckType::SurvivorPhoto, 0x4E0}] = {
        TimeOfDay(1, 10, 0, true), false, ZoneID::WonderlandPlaza, "Shinji Kitano - Sir Book-A-Lot, Wonderland Plaza" };

    // 11:00 PM
    s_availabilityDB[{CheckType::SurvivorPhoto, 0x4DD}] = {
        TimeOfDay(1, 11, 0, true), false, ZoneID::NorthPlaza, "David Bailey - Empty store, North Plaza" };

    // === DAY 2 - Sept 20 ===

    // 12:00 AM
    s_availabilityDB[{CheckType::SurvivorPhoto, 0x554}] = {
        TimeOfDay(2, 12, 0, false), false, ZoneID::WonderlandPlaza, "Barnaby - Wonderland Plaza" };

    // 7:00 AM
    s_availabilityDB[{CheckType::SurvivorPhoto, 0x4E1}] = {
        TimeOfDay(2, 7, 0, false), false, ZoneID::WonderlandPlaza, "Tonya Waters - Run Like The Wind, Wonderland Plaza" };
    s_availabilityDB[{CheckType::SurvivorPhoto, 0x4E2}] = {
        TimeOfDay(2, 7, 0, false), false, ZoneID::WonderlandPlaza, "Ross Folk - Run Like The Wind, Wonderland Plaza" };

    // 8:00 AM
    s_availabilityDB[{CheckType::SurvivorPhoto, 0x51C}] = {
        TimeOfDay(2, 8, 0, false), false, ZoneID::NorthPlaza, "Josh Manning - Empty store, North Plaza" };
    s_availabilityDB[{CheckType::SurvivorPhoto, 0x51D}] = {
        TimeOfDay(2, 8, 0, false), false, ZoneID::NorthPlaza, "Barbara Patterson - Empty store, North Plaza" };
    s_availabilityDB[{CheckType::SurvivorPhoto, 0x51E}] = {
        TimeOfDay(2, 8, 0, false), false, ZoneID::NorthPlaza, "Rich Atkins - Empty store, North Plaza" };

    // 9:00 AM
    s_availabilityDB[{CheckType::SurvivorPhoto, 0x4D1}] = {
        TimeOfDay(2, 9, 0, false), false, ZoneID::ParadisePlaza, "Heather Tompkins - Child's Play, Paradise Plaza" };
    s_availabilityDB[{CheckType::SurvivorPhoto, 0x4D6}] = {
        TimeOfDay(2, 9, 0, false), false, ZoneID::ParadisePlaza, "Pamela Tompkins - Outside Child's Play, Paradise Plaza" };
    s_availabilityDB[{CheckType::SurvivorPhoto, 0x4D3}] = {
        TimeOfDay(2, 9, 0, false), false, ZoneID::AlFrescaPlaza, "Gordon Stalworth - McHandy's Hardware, Al Fresca Plaza" };

    // 11:00 AM
    s_availabilityDB[{CheckType::SurvivorPhoto, 0x4DB}] = {
        TimeOfDay(2, 11, 0, false), false, ZoneID::ParadisePlaza, "Ronald Shiner - Jill's Sandwiches, Paradise Plaza" };

    // 12:00 PM (Noon)
    s_availabilityDB[{CheckType::SurvivorPhoto, 0x526}] = {
        TimeOfDay(2, 12, 0, true), false, ZoneID::WonderlandPlaza, "Kay Nelson - Lovely Fashion House, Wonderland Plaza" };
    s_availabilityDB[{CheckType::SurvivorPhoto, 0x527}] = {
        TimeOfDay(2, 12, 0, true), false, ZoneID::WonderlandPlaza, "Lilly Deacon - Lovely Fashion House, Wonderland Plaza" };
    s_availabilityDB[{CheckType::SurvivorPhoto, 0x529}] = {
        TimeOfDay(2, 12, 0, true), false, ZoneID::WonderlandPlaza, "Kelly Carpenter - Lovely Fashion House, Wonderland Plaza" };
    s_availabilityDB[{CheckType::SurvivorPhoto, 0x52A}] = {
        TimeOfDay(2, 12, 0, true), false, ZoneID::WonderlandPlaza, "Janet Star - Lovely Fashion House, Wonderland Plaza" };

    // 1:00 PM
    s_availabilityDB[{CheckType::SurvivorPhoto, 0x4F0}] = {
        TimeOfDay(2, 1, 0, true), false, ZoneID::ParadisePlaza, "Jennifer Gorman - Paradise Plaza near Jill's Sandwiches" };
    s_availabilityDB[{CheckType::SurvivorPhoto, 0x4E5}] = {
        TimeOfDay(2, 1, 0, true), false, ZoneID::WonderlandPlaza, "Sally Mills - Wonderland Plaza" };
    s_availabilityDB[{CheckType::SurvivorPhoto, 0x4E6}] = {
        TimeOfDay(2, 1, 0, true), false, ZoneID::WonderlandPlaza, "Nick Evans - Wonderland Plaza" };

    // 5:00 PM
    s_availabilityDB[{CheckType::SurvivorPhoto, 0x4E3}] = {
        TimeOfDay(2, 5, 0, true), false, ZoneID::EntrancePlaza, "Wayne Blackwell - Estelle's Fine-lady Cosmetics, Entrance Plaza" };
    s_availabilityDB[{CheckType::SurvivorPhoto, 0x4D8}] = {
        TimeOfDay(2, 5, 0, true), false, ZoneID::EntrancePlaza, "Jolie Wu - Gramma's Kids, Entrance Plaza" };
    s_availabilityDB[{CheckType::SurvivorPhoto, 0x4D9}] = {
        TimeOfDay(2, 5, 0, true), false, ZoneID::EntrancePlaza, "Rachel Decker - Gramma's Kids, Entrance Plaza" };
    s_availabilityDB[{CheckType::SurvivorPhoto, 0x4DE}] = {
        TimeOfDay(2, 5, 0, true), false, ZoneID::EntrancePlaza, "Floyd Sanders - Ned's Knicknackery, Entrance Plaza" };

    // === DAY 3 - Sept 21 ===

    s_availabilityDB[{CheckType::SurvivorPhoto, 0x553}] = {
        TimeOfDay(2, 3, 0, true), false, ZoneID::NorthPlaza, "Isabella - North Plaza (first meeting)" };

    // 12:00 AM
    s_availabilityDB[{CheckType::SurvivorPhoto, 0x4D7}] = {
        TimeOfDay(3, 12, 0, false), false, ZoneID::NorthPlaza, "Kindell Johnson - North Plaza" };
    s_availabilityDB[{CheckType::SurvivorPhoto, 0x510}] = {
        TimeOfDay(3, 12, 0, false), false, ZoneID::ColbysMovieland, "Ray Mathison - Colby's Movieland, Cinema 4" };
    s_availabilityDB[{CheckType::SurvivorPhoto, 0x512}] = {
        TimeOfDay(3, 12, 0, false), false, ZoneID::ColbysMovieland, "Nathan Crabbe - Colby's Movieland, Cinema 4" };
    s_availabilityDB[{CheckType::SurvivorPhoto, 0x514}] = {
        TimeOfDay(3, 12, 0, false), false, ZoneID::ColbysMovieland, "Michelle Feltz - Colby's Movieland, Cinema 4" };
    s_availabilityDB[{CheckType::SurvivorPhoto, 0x516}] = {
        TimeOfDay(3, 12, 0, false), false, ZoneID::ColbysMovieland, "Beth Shrake - Colby's Movieland, Cinema 4" };
    s_availabilityDB[{CheckType::SurvivorPhoto, 0x525}] = {
        TimeOfDay(3, 12, 0, false), false, ZoneID::ColbysMovieland, "Cheryl Jones - Colby's Movieland, Screen 4 storage room" };

    // 1:00 AM
    s_availabilityDB[{CheckType::SurvivorPhoto, 0x4EA}] = {
        TimeOfDay(3, 1, 0, false), false, ZoneID::NorthPlaza, "Brett Styles - Huntin' Shack, North Plaza" };
    s_availabilityDB[{CheckType::SurvivorPhoto, 0x4EB}] = {
        TimeOfDay(3, 1, 0, false), false, ZoneID::NorthPlaza, "Jonathan Picardson - Huntin' Shack, North Plaza" };
    s_availabilityDB[{CheckType::SurvivorPhoto, 0x4ED}] = {
        TimeOfDay(3, 1, 0, false), false, ZoneID::NorthPlaza, "Alyssa Laurent - Huntin' Shack, North Plaza" };

    // 2:00 AM
    s_availabilityDB[{CheckType::SurvivorPhoto, 0x4E9}] = {
        TimeOfDay(3, 2, 0, false), false, ZoneID::FoodCourt, "Gil Jiminez - Chris' Fine Foods, Food Court" };

    // 5:00 AM
    s_availabilityDB[{CheckType::SurvivorPhoto, 0x51F}] = {
        TimeOfDay(3, 5, 0, false), false, ZoneID::WonderlandPlaza, "Mindy Baker - Casual Gals, Wonderland Plaza" };
    s_availabilityDB[{CheckType::SurvivorPhoto, 0x520}] = {
        TimeOfDay(3, 5, 0, false), false, ZoneID::WonderlandPlaza, "Debbie Willet - Casual Gals, Wonderland Plaza" };
    s_availabilityDB[{CheckType::SurvivorPhoto, 0x4EE}] = {
        TimeOfDay(3, 5, 0, false), false, ZoneID::WonderlandPlaza, "Paul Carson - Casual Gals, Wonderland Plaza" };

    // 8:00 AM
    s_availabilityDB[{CheckType::SurvivorPhoto, 0x4E7}] = {
        TimeOfDay(3, 8, 0, false), false, ZoneID::WonderlandPlaza, "Leroy McKenna - Estelle's Fine-lady Cosmetics, Wonderland Plaza" };
    s_availabilityDB[{CheckType::SurvivorPhoto, 0x4DA}] = {
        TimeOfDay(3, 8, 0, false), false, ZoneID::WonderlandPlaza, "Susan Walsh - Play Area, Wonderland Plaza" };

    // 12:00 PM (Noon)
    s_availabilityDB[{CheckType::SurvivorPhoto, 0x522}] = {
        TimeOfDay(3, 12, 0, true), false, ZoneID::ParadisePlaza, "Tad Hawthorne - Colombian Roastmasters, Paradise Plaza" };
    s_availabilityDB[{CheckType::SurvivorPhoto, 0x4E8}] = {
        TimeOfDay(3, 12, 0, true), false, ZoneID::ParadisePlaza, "Simone Ravendark - Players, Paradise Plaza" };
}

void CheckAvailability::RegisterPsychopathTimes()
{
    auto TimeOfDay = [](int day, int hour, int minute, bool isPM) {
        int hour24 = hour;
        if (isPM && hour != 12)
            hour24 += 12;
        else if (!isPM && hour == 12)
            hour24 = 0;

        int gameStartHour = 12;
        int hoursFromStart = ((day - 1) * 24) + (hour24 - gameStartHour);

        return TimeManager::GAME_START_TICK +
               (hoursFromStart * TimeManager::TICKS_PER_HOUR) +
               (minute * TimeManager::TICKS_PER_MINUTE);
    };

    s_availabilityDB[{CheckType::PsychopathPhoto, 0x6E3}] = {
        TimeOfDay(1, 12, 0, true), false, ZoneID::FoodCourt, "Carlito - Food Court" };

    s_availabilityDB[{CheckType::PsychopathPhoto, 0x6D5}] = {
        TimeOfDay(1, 2, 0, true), false, ZoneID::ParadisePlaza, "Kent - Paradise Plaza" };

    s_availabilityDB[{CheckType::PsychopathPhoto, 0x6DF}] = {
        TimeOfDay(1, 3, 0, true), false, ZoneID::NorthPlaza, "Cletus - Huntin' Shack, North Plaza" };

    s_availabilityDB[{CheckType::PsychopathPhoto, 0x6DC}] = {
        TimeOfDay(1, 6, 0, true), false, ZoneID::LeisurePark, "Sam Convict - Leisure Park" };
    s_availabilityDB[{CheckType::PsychopathPhoto, 0x6DE}] = {
        TimeOfDay(1, 6, 0, true), false, ZoneID::LeisurePark, "Reginald Convict - Leisure Park" };
    s_availabilityDB[{CheckType::PsychopathPhoto, 0x6DD}] = {
        TimeOfDay(1, 6, 0, true), false, ZoneID::LeisurePark, "Miguel Convict - Leisure Park" };

    s_availabilityDB[{CheckType::PsychopathPhoto, 0x6D8}] = {
        TimeOfDay(1, 9, 0, true), false, ZoneID::WonderlandPlaza, "Adam - Wonderland Plaza" };

    s_availabilityDB[{CheckType::PsychopathPhoto, 0x6DA}] = {
        TimeOfDay(2, 8, 0, false), false, ZoneID::SeonsFood, "Steven - Seon's Food & Stuff" };

    s_availabilityDB[{CheckType::PsychopathPhoto, 0x6D6}] = {
        TimeOfDay(2, 8, 0, false), false, ZoneID::CrislipsHomeSaloon, "Cliff - Crislip's Home Saloon" };

    s_availabilityDB[{CheckType::PsychopathPhoto, 0x6E6}] = {
        TimeOfDay(2, 3, 0, true), false, ZoneID::NorthPlaza, "Isabela - North Plaza" };

    s_availabilityDB[{CheckType::PsychopathPhoto, 0x6DB}] = {
        TimeOfDay(2, 12, 0, true), false, ZoneID::WonderlandPlaza, "Jo - Lovely Fashion House, Wonderland Plaza" };

    s_availabilityDB[{CheckType::PsychopathPhoto, 0x6E7}] = {
        TimeOfDay(2, 5, 0, true), false, ZoneID::EntrancePlaza, "Roger Hall - Entrance Plaza" };
    s_availabilityDB[{CheckType::PsychopathPhoto, 0x6E8}] = {
        TimeOfDay(2, 5, 0, true), false, ZoneID::EntrancePlaza, "Jack Hall - Entrance Plaza" };
    s_availabilityDB[{CheckType::PsychopathPhoto, 0x6E9}] = {
        TimeOfDay(2, 5, 0, true), false, ZoneID::EntrancePlaza, "Thomas Hall - Entrance Plaza" };

    s_availabilityDB[{CheckType::PsychopathPhoto, 0x6D4}] = {
        TimeOfDay(3, 12, 0, false), false, ZoneID::ColbysMovieland, "Sean - Colby's Movieland" };

    s_availabilityDB[{CheckType::PsychopathPhoto, 0x6D7}] = {
        TimeOfDay(3, 4, 0, false), false, ZoneID::WonderlandPlaza, "Paul - Casual Gals, Wonderland Plaza" };

    s_availabilityDB[{CheckType::PsychopathPhoto, 0x6D9}] = {
        TimeOfDay(3, 5, 0, true), false, ZoneID::MeatProcessing, "Larry - Meat Processing" };
}

// ─────────────────────────────────────────────────────────────────────────────
//  Costume table
//  checkId  = unique sequential location ID (1-N), one per physical store slot
//  costumeId = slot * 100 + id  (game-native value from the pickup hook)
//  areaId    = game's internal area/store ID (0 = unknown, populate from costume_log.txt)
//  Slot 0 = outfit, Slot 1 = shoes, Slot 2 = head, Slot 3 = accessory
// ─────────────────────────────────────────────────────────────────────────────
struct CostumeEntry
{
    uint32_t    checkId;    // unique location ID
    uint32_t    costumeId;  // slot * 100 + id
    ZoneID      zone;
    const char* notes;
};

static const CostumeEntry kCostumeDB[] =
{
    // checkId  costumeId  zone                   notes
    // ── Slot 0 — Outfits ────────────────────────────────────────────────────
    {  1,   2,  ZoneID::EntrancePlaza,    "Brown Jacket with Fur Trim Tan Shirt and Black Pants"   },
    {  2,   3,  ZoneID::EntrancePlaza,    "Black and White Sleeveless Sports Top with Black and Grey Shorts"   },
    {  3,   3,  ZoneID::ParadisePlaza,    "Black and White Sleeveless Sports Top with Black and Grey Shorts"   },
    {  4,   4,  ZoneID::EntrancePlaza,    "Red and White Sleeveless Shirt with Red and Black Checkered Shorts"   },
    {  5,   5,  ZoneID::EntrancePlaza,    "White Dress Shirt, Black Tie, and Grey Dress Pants"   },
    {  6,   5,  ZoneID::ParadisePlaza,    "White Dress Shirt, Black Tie, and Grey Dress Pants"   },
    {  7,   6,  ZoneID::EntrancePlaza,    "Black and Brown Checkered Dress Shirt, Black Tie, and Striped Pants"   },
    {  8,   7,  ZoneID::EntrancePlaza,    "Grey Business Suit with Striped Tie"   },
    {  9,   8,  ZoneID::EntrancePlaza,    "White Business Suit and Striped Tie"   },
    { 10,   9,  ZoneID::WonderlandPlaza,  "Yellow Suit with Yellow Striped Tie"   },
    { 11,  10,  ZoneID::EntrancePlaza,    "White Skirt"   },
    { 12,  11,  ZoneID::EntrancePlaza,    "Black Skirt"   },
    { 13,  11,  ZoneID::WonderlandPlaza,  "Black Skirt"   },
    { 14,  12,  ZoneID::AlFrescaPlaza,    "Pink Skirt"   },
    { 15,  13,  ZoneID::EntrancePlaza,    "Purple Dress"   },
    { 16,  14,  ZoneID::EntrancePlaza,    "White Teddy"   },
    { 17,  15,  ZoneID::EntrancePlaza,    "Black Sundress with Red Roses"   },
    { 18,  15,  ZoneID::EntrancePlaza,    "Black Sundress with Red Roses"   },
    { 19,  15,  ZoneID::WonderlandPlaza,  "Black Sundress with Red Roses"   },
    { 20,  16,  ZoneID::EntrancePlaza,    "Blue and White Flowery Dress"   },
    { 21,  16,  ZoneID::WonderlandPlaza,  "Blue and White Flowery Dress"   },
    { 22,  17,  ZoneID::EntrancePlaza,    "Red and Grey Ratman T-shirt with Brown Shorts"   },
    { 23,  17,  ZoneID::ParadisePlaza,    "Red and Grey Ratman T-shirt with Brown Shorts"   },
    { 24,  18,  ZoneID::EntrancePlaza,    "Green Ratman T-shirt and Blue Jean Shorts"   },
    { 25,  18,  ZoneID::ParadisePlaza,    "Green Ratman T-shirt and Blue Jean Shorts"   },
    { 26,  19,  ZoneID::WonderlandPlaza,  "Pink and Black Striped T-shirt with Pink Jean Shorts"   },
    { 27,  20,  ZoneID::WonderlandPlaza,  "Blue T-shirt with White Stars and Red Shorts"   },
    { 28,  21,  ZoneID::COUNT,            "Miami Nights Outfit"   },
    { 29,  22,  ZoneID::COUNT,            "Casual Outfit"   },
    { 30,  23,  ZoneID::EntrancePlaza,    "USA Track Outfit"   },
    { 31,  24,  ZoneID::EntrancePlaza,    "Black and White Spandex Track Suit"   },
    { 32,  24,  ZoneID::WonderlandPlaza,  "Black and White Spandex Track Suit"   },
    { 33,  25,  ZoneID::EntrancePlaza,    "Tan Camouflage Vest with Dark Brown Shorts"   },
    { 34,  26,  ZoneID::WonderlandPlaza,  "Blue Vest with Tan Shorts"   },
    { 35,  28,  ZoneID::COUNT,            "Weekender Outfit"   },
    { 36,  29,  ZoneID::COUNT,            "Man in Black Outfit"   },
    { 37,  30,  ZoneID::COUNT,            "Strike Outfit"   },
    { 38,  31,  ZoneID::COUNT,            "Accountant Outfit"   },
    { 39,  33,  ZoneID::COUNT,            "Pink Paparazzi Outfit"   },
    { 40,  34,  ZoneID::COUNT,            "Grandpa Outfit"   },
    { 41,  38,  ZoneID::COUNT,            "Burgundy Wine Outfit"   },
    { 42,  39,  ZoneID::COUNT,            "Cold Hearted Snake Outfit"   },
    { 43,  40,  ZoneID::COUNT,            "Pure White Outfit"   },

    // ── Slot 1 — Shoes ──────────────────────────────────────────────────────
    { 44, 100,  ZoneID::EntrancePlaza,    "Bare Feet"   },
    { 45, 102,  ZoneID::EntrancePlaza,    "White Dress Shoes"   },
    { 46, 102,  ZoneID::EntrancePlaza,    "White Dress Shoes"   },
    { 47, 102,  ZoneID::EntrancePlaza,    "White Dress Shoes"   },
    { 48, 102,  ZoneID::ParadisePlaza,    "White Dress Shoes"   },
    { 49, 103,  ZoneID::EntrancePlaza,    "Black Dress Shoes with Purple Socks"   },
    { 50, 103,  ZoneID::EntrancePlaza,    "Black Dress Shoes with Purple Socks"   },
    { 51, 103,  ZoneID::ParadisePlaza,    "Black Dress Shoes with Purple Socks"   },
    { 52, 103,  ZoneID::ParadisePlaza,    "Black Dress Shoes with Purple Socks"   },
    { 53, 104,  ZoneID::EntrancePlaza,    "Red and Black Running Shoes"   },
    { 54, 104,  ZoneID::WonderlandPlaza,  "Red and Black Running Shoes"   },
    { 55, 105,  ZoneID::EntrancePlaza,    "White and Red Lowtops with Soccer Socks"   },
    { 56, 105,  ZoneID::ParadisePlaza,    "White and Red Lowtops with Soccer Socks"   },
    { 57, 105,  ZoneID::WonderlandPlaza,  "White and Red Lowtops with Soccer Socks"   },
    { 58, 106,  ZoneID::EntrancePlaza,    "Orange Lowtops with White Pearl Anklet"   },
    { 59, 106,  ZoneID::WonderlandPlaza,  "Orange Lowtops with White Pearl Anklet"   },

    // ── Slot 2 — Head ───────────────────────────────────────────────────────
    { 60, 201,  ZoneID::EntrancePlaza,    "Brown Hair Dye"   },
    { 61, 202,  ZoneID::ColbysMovieland,  "Mega Man Helmet"   },
    { 62, 203,  ZoneID::EntrancePlaza,    "Grey Hair Dye"   },
    { 63, 204,  ZoneID::WonderlandPlaza,  "Light Brown Hair Dye"   },
    { 64, 205,  ZoneID::WonderlandPlaza,  "Red Hair Dye"   },
    { 65, 207,  ZoneID::ParadisePlaza,    "Black Baseball Cap"   },
    { 66, 207,  ZoneID::WonderlandPlaza,  "Black Baseball Cap"   },
    { 67, 207,  ZoneID::NorthPlaza,       "Black Baseball Cap"   },
    { 68, 208,  ZoneID::EntrancePlaza,    "Blue and White Baseball Cap"   },
    { 69, 208,  ZoneID::WonderlandPlaza,  "Blue and White Baseball Cap"   },
    { 70, 208,  ZoneID::NorthPlaza,       "Blue and White Baseball Cap"   },
    { 71, 209,  ZoneID::EntrancePlaza,    "Tan Fedora"   },
    { 72, 209,  ZoneID::NorthPlaza,       "Tan Fedora"   },
    { 73, 210,  ZoneID::NorthPlaza,       "Black Fedora"   },
    { 74, 214,  ZoneID::EntrancePlaza,    "Ghoul Mask"   },
    { 75, 215,  ZoneID::ParadisePlaza,    "Horse Mask"   },
    { 76, 216,  ZoneID::ParadisePlaza,    "Teddy Bear Mask"   },
    { 77, 217,  ZoneID::ParadisePlaza,    "Servbot Mask"   },

    // ── Slot 3 — Accessories ────────────────────────────────────────────────
    { 78, 301,  ZoneID::AlFrescaPlaza,    "Grey Sunglasses"   },
    { 79, 301,  ZoneID::EntrancePlaza,    "Grey Sunglasses"   },
    { 80, 301,  ZoneID::ParadisePlaza,    "Grey Sunglasses"   },
    { 81, 301,  ZoneID::WonderlandPlaza,  "Grey Sunglasses"   },
    { 82, 302,  ZoneID::AlFrescaPlaza,    "Silver Wire-Frame Dark-tinted Glasses"   },
    { 83, 302,  ZoneID::EntrancePlaza,    "Silver Wire-Frame Dark-tinted Glasses"   },
    { 84, 302,  ZoneID::ParadisePlaza,    "Silver Wire-Frame Dark-tinted Glasses"   },
    { 85, 303,  ZoneID::AlFrescaPlaza,    "Grey Rimless Wire-Frame Glasses"   },
    { 86, 303,  ZoneID::EntrancePlaza,    "Grey Rimless Wire-Frame Glasses"   },
    { 87, 304,  ZoneID::AlFrescaPlaza,    "Red Armless Sunglasses"   },
    { 88, 304,  ZoneID::ParadisePlaza,    "Red Armless Sunglasses"   },
    { 89, 304,  ZoneID::WonderlandPlaza,  "Red Armless Sunglasses"   },
    { 90, 305,  ZoneID::AlFrescaPlaza,    "Silver Rimless Wire-Frame Glasses"   },
    { 91, 305,  ZoneID::EntrancePlaza,    "Silver Rimless Wire-Frame Glasses"   },
    { 92, 305,  ZoneID::ParadisePlaza,    "Silver Rimless Wire-Frame Glasses"   },
    { 93, 305,  ZoneID::WonderlandPlaza,  "Silver Rimless Wire-Frame Glasses"   },
    { 94, 306,  ZoneID::ParadisePlaza,    "Black and Orange Sunglasses"   },
    { 95, 306,  ZoneID::WonderlandPlaza,  "Black and Orange Sunglasses"   },
    { 96, 310,  ZoneID::COUNT,            "Round Shades Outfit"   },
};

void CheckAvailability::RegisterCostumeTimes()
{
    for (const auto& e : kCostumeDB)
        s_availabilityDB[{ CheckType::Costume, e.checkId }] = {
            TimeManager::GAME_START_TICK, false, e.zone, e.notes };
}

static std::unordered_map<uint64_t, std::vector<uint32_t>> s_costumeZoneMap;

void CheckAvailability::BuildCostumeLookup()
{
    for (const auto& e : kCostumeDB)
    {
        uint64_t key = ((uint64_t)e.costumeId << 32) | (uint32_t)(int)e.zone;
        s_costumeZoneMap[key].push_back(e.checkId);
    }
}

const std::vector<uint32_t>* CheckAvailability::GetCostumeCheckIds(uint32_t costumeId, ZoneID zone)
{
    uint64_t key = ((uint64_t)costumeId << 32) | (uint32_t)(int)zone;
    auto it = s_costumeZoneMap.find(key);
    if (it != s_costumeZoneMap.end())
        return &it->second;
    return nullptr;
}

uint32_t CheckAvailability::GetCostumeCheckCount()
{
    return (uint32_t)(sizeof(kCostumeDB) / sizeof(kCostumeDB[0]));
}

uint32_t CheckAvailability::GetEarliestTime(CheckId check)
{
    auto it = s_availabilityDB.find(check);
    if (it != s_availabilityDB.end())
        return it->second.earliestTime;

    char buf[128];
    sprintf_s(buf, "[AVAILABILITY] WARNING: Unknown check [type=%u, id=0x%X (%u)] - assuming late game",
              (uint32_t)check.type, check.id, check.id);
    LogLine(buf);

    return TimeManager::GAME_END_TICK;
}

ZoneID CheckAvailability::GetRequiredZone(CheckId check)
{
    auto it = s_availabilityDB.find(check);
    if (it != s_availabilityDB.end())
        return it->second.requiredZone;
    return ZoneID::COUNT;  // Unknown check — assume no zone requirement
}

bool CheckAvailability::IsAvailableAt(CheckId check, uint32_t time)
{
    return GetEarliestTime(check) <= time;
}

std::string CheckAvailability::GetCheckName(CheckId check)
{
    auto it = s_availabilityDB.find(check);
    if (it != s_availabilityDB.end())
    {
        std::string fullDesc = it->second.notes;
        size_t dashPos = fullDesc.find(" - ");
        if (dashPos != std::string::npos)
            return fullDesc.substr(0, dashPos);
        return fullDesc;
    }

    char buf[64];
    switch (check.type)
    {
        case CheckType::PPSticker:
            sprintf_s(buf, "PP Sticker #%u", check.id);
            break;
        case CheckType::SurvivorPhoto:
            sprintf_s(buf, "Survivor #%u", check.id);
            break;
        case CheckType::PsychopathPhoto:
            sprintf_s(buf, "Psychopath #%u", check.id);
            break;
        case CheckType::Costume:
            sprintf_s(buf, "Costume #%u", check.id);
            break;
        case CheckType::KeyItem:
            sprintf_s(buf, "Key Item #%u", check.id);
            break;
        default:
            sprintf_s(buf, "Unknown Check #%u", check.id);
            break;
    }
    return std::string(buf);
}
