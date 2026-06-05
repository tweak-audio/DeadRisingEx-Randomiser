/*
This is so the randomizer can have logic
*/


#include "CheckAvailability.h"
#include "DeadRisingEx/Utilities/DebugLog.h"
#include "DeadRisingEx/MtFramework/Randomiser/InputSystem.h"

std::unordered_map<CheckId, CheckAvailabilityInfo, CheckIdHash> CheckAvailability::s_availabilityDB;

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

void CheckAvailability::RegisterPPStickerTimes()
{
    auto HoursAfterStart = [](int hours) {
        return TimeManager::GAME_START_TICK + (hours * TimeManager::TICKS_PER_HOUR);
    };

    // PP Stickers — zone data not yet mapped per sticker, so COUNT (no key required).
    // Update individual entries below once per-sticker zone data is known.
    for (int i = 128; i <= 227; i++)
    {
        if (i == 224 || i == 227)
            continue;

        s_availabilityDB[{CheckType::PPSticker, (uint32_t)i}] = {
            TimeManager::GAME_START_TICK,
            false,
            ZoneID::COUNT,
            "PP Sticker"
        };
    }

    // Cult Hideout Stickers (cultists spawn after 25 hours in-game)
    s_availabilityDB[{CheckType::PPSticker, 224}] = {
        HoursAfterStart(25),
        true,
        ZoneID::COUNT,
        "Cult Hideout - need cultists to spawn (25 hours)"
    };
    s_availabilityDB[{CheckType::PPSticker, 227}] = {
        HoursAfterStart(25),
        true,
        ZoneID::COUNT,
        "Cult Hideout - need cultists to spawn (25 hours)"
    };
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

void CheckAvailability::RegisterClothingTimes()
{
    // Clothing zone data not yet mapped per costume ID — using COUNT (no key required).
    // Update individual entries once per-costume store locations are confirmed.

    for (int id = 0; id <= 26; id++)
        s_availabilityDB[{CheckType::Clothing, (uint32_t)id}] = {
            TimeManager::GAME_START_TICK, false, ZoneID::COUNT, "Clothing - Slot 0 Outfit" };
    for (int id = 28; id <= 33; id++)
        s_availabilityDB[{CheckType::Clothing, (uint32_t)id}] = {
            TimeManager::GAME_START_TICK, false, ZoneID::COUNT, "Clothing - Slot 0 Outfit" };
    s_availabilityDB[{CheckType::Clothing, 38}] = {
        TimeManager::GAME_START_TICK, false, ZoneID::COUNT, "Clothing - Slot 0 Outfit" };
    for (int id = 40; id <= 42; id++)
        s_availabilityDB[{CheckType::Clothing, (uint32_t)id}] = {
            TimeManager::GAME_START_TICK, false, ZoneID::COUNT, "Clothing - Slot 0 Outfit" };

    for (int id = 100; id <= 109; id++)
        s_availabilityDB[{CheckType::Clothing, (uint32_t)id}] = {
            TimeManager::GAME_START_TICK, false, ZoneID::COUNT, "Clothing - Slot 1 Shoes" };

    for (int id : {202, 203, 204, 205, 207, 208, 209, 210, 211, 214, 215, 216, 217, 219})
        s_availabilityDB[{CheckType::Clothing, (uint32_t)id}] = {
            TimeManager::GAME_START_TICK, false, ZoneID::COUNT, "Clothing - Slot 2 Head Wear" };

    for (int id : {301, 302, 304, 306, 310})
        s_availabilityDB[{CheckType::Clothing, (uint32_t)id}] = {
            TimeManager::GAME_START_TICK, false, ZoneID::COUNT, "Clothing - Slot 3 Accessory" };
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
