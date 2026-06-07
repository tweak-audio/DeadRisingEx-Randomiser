/*
This is so the randomizer can have logic
*/


#include "CheckAvailability.h"
#include "DeadRisingEx/Utilities/DebugLog.h"
#include "DeadRisingEx/MtFramework/Randomiser/InputSystem.h"

std::unordered_map<CheckId, CheckAvailabilityInfo, CheckIdHash> CheckAvailability::s_availabilityDB;

// ─────────────────────────────────────────────────────────────────────────────
//  Shared table entry — used for PP stickers and clothing.
//  notes format: "Name - Store/Location, Zone"
//  GetCheckName() returns everything before the first " - ".
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
    RegisterClothingTimes();

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
    { 174,  ZoneID::COUNT,            "PP Sticker 174 - TODO" },
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
    { 226,  ZoneID::COUNT,       "PP Sticker 226 - TODO" },
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
//  Clothing table
//  checkId = slot * 100 + id  (matches ClothingCheck.cpp formula)
//  Fill in "Costume Name - Store, Zone" and the correct ZoneID per item.
//  Slot 0 = outfit, Slot 1 = shoes, Slot 2 = head, Slot 3 = accessory
// ─────────────────────────────────────────────────────────────────────────────
static const LocationEntry kClothingDB[] =
{
    // ── Slot 0 — Outfits (checkId = id) ─────────────────────────────────────
    {   0,  ZoneID::COUNT,   "Outfit 0 - TODO"   },
    {   1,  ZoneID::COUNT,   "Outfit 1 - TODO"   },
    {   2,  ZoneID::COUNT,   "Outfit 2 - TODO"   },
    {   3,  ZoneID::COUNT,   "Outfit 3 - TODO"   },
    {   4,  ZoneID::COUNT,   "Outfit 4 - TODO"   },
    {   5,  ZoneID::COUNT,   "Outfit 5 - TODO"   },
    {   6,  ZoneID::COUNT,   "Outfit 6 - TODO"   },
    {   7,  ZoneID::COUNT,   "Outfit 7 - TODO"   },
    {   8,  ZoneID::COUNT,   "Outfit 8 - TODO"   },
    {   9,  ZoneID::COUNT,   "Outfit 9 - TODO"   },
    {  10,  ZoneID::COUNT,   "Outfit 10 - TODO"  },
    {  11,  ZoneID::COUNT,   "Outfit 11 - TODO"  },
    {  12,  ZoneID::COUNT,   "Outfit 12 - TODO"  },
    {  13,  ZoneID::COUNT,   "Outfit 13 - TODO"  },
    {  14,  ZoneID::COUNT,   "Outfit 14 - TODO"  },
    {  15,  ZoneID::COUNT,   "Outfit 15 - TODO"  },
    {  16,  ZoneID::COUNT,   "Outfit 16 - TODO"  },
    {  17,  ZoneID::COUNT,   "Outfit 17 - TODO"  },
    {  18,  ZoneID::COUNT,   "Outfit 18 - TODO"  },
    {  19,  ZoneID::COUNT,   "Outfit 19 - TODO"  },
    {  20,  ZoneID::COUNT,   "Outfit 20 - TODO"  },
    {  21,  ZoneID::COUNT,   "Outfit 21 - TODO"  },
    {  22,  ZoneID::COUNT,   "Outfit 22 - TODO"  },
    {  23,  ZoneID::COUNT,   "Outfit 23 - TODO"  },
    {  24,  ZoneID::COUNT,   "Outfit 24 - TODO"  },
    {  25,  ZoneID::COUNT,   "Outfit 25 - TODO"  },
    {  26,  ZoneID::COUNT,   "Outfit 26 - TODO"  },
    {  28,  ZoneID::COUNT,   "Outfit 28 - TODO"  },
    {  29,  ZoneID::COUNT,   "Outfit 29 - TODO"  },
    {  30,  ZoneID::COUNT,   "Outfit 30 - TODO"  },
    {  31,  ZoneID::COUNT,   "Outfit 31 - TODO"  },
    {  32,  ZoneID::COUNT,   "Outfit 32 - TODO"  },
    {  33,  ZoneID::COUNT,   "Outfit 33 - TODO"  },
    {  38,  ZoneID::COUNT,   "Outfit 38 - TODO"  },
    {  40,  ZoneID::COUNT,   "Outfit 40 - TODO"  },
    {  41,  ZoneID::COUNT,   "Outfit 41 - TODO"  },
    {  42,  ZoneID::COUNT,   "Outfit 42 - TODO"  },

    // ── Slot 1 — Shoes (checkId = 100 + id) ─────────────────────────────────
    { 100,  ZoneID::COUNT,   "Shoes 100 - TODO"  },
    { 101,  ZoneID::COUNT,   "Shoes 101 - TODO"  },
    { 102,  ZoneID::COUNT,   "Shoes 102 - TODO"  },
    { 103,  ZoneID::COUNT,   "Shoes 103 - TODO"  },
    { 104,  ZoneID::COUNT,   "Shoes 104 - TODO"  },
    { 105,  ZoneID::COUNT,   "Shoes 105 - TODO"  },
    { 106,  ZoneID::COUNT,   "Shoes 106 - TODO"  },
    { 107,  ZoneID::COUNT,   "Shoes 107 - TODO"  },
    { 108,  ZoneID::COUNT,   "Shoes 108 - TODO"  },
    { 109,  ZoneID::COUNT,   "Shoes 109 - TODO"  },

    // ── Slot 2 — Head (checkId = 200 + id) ──────────────────────────────────
    { 202,  ZoneID::COUNT,   "Hat 202 - TODO"    },
    { 203,  ZoneID::COUNT,   "Hat 203 - TODO"    },
    { 204,  ZoneID::COUNT,   "Hat 204 - TODO"    },
    { 205,  ZoneID::COUNT,   "Hat 205 - TODO"    },
    { 207,  ZoneID::COUNT,   "Hat 207 - TODO"    },
    { 208,  ZoneID::COUNT,   "Hat 208 - TODO"    },
    { 209,  ZoneID::COUNT,   "Hat 209 - TODO"    },
    { 210,  ZoneID::COUNT,   "Hat 210 - TODO"    },
    { 211,  ZoneID::COUNT,   "Hat 211 - TODO"    },
    { 214,  ZoneID::COUNT,   "Hat 214 - TODO"    },
    { 215,  ZoneID::COUNT,   "Hat 215 - TODO"    },
    { 216,  ZoneID::COUNT,   "Hat 216 - TODO"    },
    { 217,  ZoneID::COUNT,   "Hat 217 - TODO"    },
    { 219,  ZoneID::COUNT,   "Hat 219 - TODO"    },

    // ── Slot 3 — Accessories (checkId = 300 + id) ───────────────────────────
    { 301,  ZoneID::COUNT,   "Accessory 301 - TODO" },
    { 302,  ZoneID::COUNT,   "Accessory 302 - TODO" },
    { 304,  ZoneID::COUNT,   "Accessory 304 - TODO" },
    { 306,  ZoneID::COUNT,   "Accessory 306 - TODO" },
    { 310,  ZoneID::COUNT,   "Accessory 310 - TODO" },
};

void CheckAvailability::RegisterClothingTimes()
{
    for (const auto& e : kClothingDB)
        s_availabilityDB[{CheckType::Clothing, e.checkId}] = {
            TimeManager::GAME_START_TICK, false, e.zone, e.notes };
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
        case CheckType::Clothing:
            sprintf_s(buf, "Costume #%u", check.id);
            break;
        default:
            sprintf_s(buf, "Unknown Check #%u", check.id);
            break;
    }
    return std::string(buf);
}
