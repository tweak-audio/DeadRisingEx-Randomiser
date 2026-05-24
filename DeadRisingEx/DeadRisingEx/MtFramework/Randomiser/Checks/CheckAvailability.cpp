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
    // Helper for time conversion
    auto HoursAfterStart = [](int hours) {
        return TimeManager::GAME_START_TICK + (hours * TimeManager::TICKS_PER_HOUR);
    };
    
    // PP Stickers - most are available at game start
    // Starting areas (Paradise Plaza, Al Fresca Plaza, etc.)
    for (int i = 128; i <= 227; i++)
    {
        // Skip the cult hideout stickers - they'll be registered separately
        if (i == 224 || i == 227)
            continue;
            
        s_availabilityDB[{CheckType::PPSticker, (uint32_t)i}] = {
            TimeManager::GAME_START_TICK,
            false,
            "PP Sticker - always accessible"
        };
    }
    
    // Cult Hideout Stickers (need cultists to spawn after 25 hours)
    s_availabilityDB[{CheckType::PPSticker, 224}] = {
        HoursAfterStart(25),  // Day 2, 1:00 PM
        true,
        "Cult Hideout - need cultists to spawn (25 hours)"
    };
    
    s_availabilityDB[{CheckType::PPSticker, 227}] = {
        HoursAfterStart(25),  // Day 2, 1:00 PM
        true,
        "Cult Hideout - need cultists to spawn (25 hours)"
    };
}

// Example format:
    // s_availabilityDB[{CheckType::SurvivorPhoto, ID}] = {
    //     Time(DAY, HOUR, MINUTE),
    //     requiresItem,
    //     "Name - Location - Notes"
    // };

void CheckAvailability::RegisterSurvivorTimes()
{
    auto TimeOfDay = [](int day, int hour, int minute, bool isPM) {
        // Convert to 24-hour format
        int hour24 = hour;
        if (isPM && hour != 12)
            hour24 += 12;
        else if (!isPM && hour == 12)
            hour24 = 0;
        
        // Game starts Day 1 at 12:00 PM (hour 12)
        int gameStartHour = 12;  // Noon
        int hoursFromStart = ((day - 1) * 24) + (hour24 - gameStartHour);
        
        return TimeManager::GAME_START_TICK + 
               (hoursFromStart * TimeManager::TICKS_PER_HOUR) +
               (minute * TimeManager::TICKS_PER_MINUTE);
    };
    
    // === DAY 1 - Sept 19 ===
    
    // 12:00 PM - Game Start

    //Entrance Plaza
    s_availabilityDB[{CheckType::SurvivorPhoto, 0x555}] = {  // npc85 - Lindsey
        TimeOfDay(1, 12, 0, true),
        false,
        "Lindsey - Entrance Plaza"
    };

    s_availabilityDB[{CheckType::SurvivorPhoto, 0x560}] = {  // npc90 - Ryan
        TimeOfDay(1, 12, 0, true),
        false,
        "Ryan - Entrance Plaza"
    };

    s_availabilityDB[{CheckType::SurvivorPhoto, 0x561}] = {  // npc91 - Chris
        TimeOfDay(1, 12, 0, true),
        false,
        "Chris - Entrance Plaza"
    };

    s_availabilityDB[{CheckType::SurvivorPhoto, 0x562}] = {  // npc92 - Todd
        TimeOfDay(1, 12, 0, true),
        false,
        "Todd - Entrance Plaza"
    };

    s_availabilityDB[{CheckType::SurvivorPhoto, 0x563}] = {  // npc93 - Brian
        TimeOfDay(1, 12, 0, true),
        false,
        "Brian - Entrance Plaza"
    };

    s_availabilityDB[{CheckType::SurvivorPhoto, 0x564}] = {  // npc94 - Dana
        TimeOfDay(1, 12, 0, true),
        false,
        "Dana - Entrance Plaza"
    };

    s_availabilityDB[{CheckType::SurvivorPhoto, 0x565}] = {  // npc95 - Verlene
        TimeOfDay(1, 12, 0, true),
        false,
        "Verlene - Entrance Plaza"
    };

    s_availabilityDB[{CheckType::SurvivorPhoto, 0x566}] = {  // npc96 - Mark
        TimeOfDay(1, 12, 0, true),
        false,
        "Mark - Entrance Plaza"
    };

    s_availabilityDB[{CheckType::SurvivorPhoto, 0x567}] = {  // npc97 - Kathy
        TimeOfDay(1, 12, 0, true),
        false,
        "Kathy - Entrance Plaza"
    };

    s_availabilityDB[{CheckType::SurvivorPhoto, 0x568}] = {  // npc98 - Alan
        TimeOfDay(1, 12, 0, true),
        false,
        "Alan - Entrance Plaza"
    };

    s_availabilityDB[{CheckType::SurvivorPhoto, 0x56C}] = {  // npc9C - Freddie
        TimeOfDay(1, 12, 0, true),
        false,
        "Freddie - Entrance Plaza"
    };

    s_availabilityDB[{CheckType::SurvivorPhoto, 0x550}] = {  // npc80 - Brad
        TimeOfDay(1, 12, 0, true),
        false,
        "Brad - Food Court"
    };

    s_availabilityDB[{CheckType::SurvivorPhoto, 0x551}] = {  // npc81 - Jessie
        TimeOfDay(1, 12, 0, true),
        false,
        "Jessie - Security Room"
    };

    s_availabilityDB[{CheckType::SurvivorPhoto, 0x556}] = {  // npc81 - Otis
        TimeOfDay(1, 12, 0, true),
        false,
        "Otis - Security Room"
    };

    s_availabilityDB[{CheckType::SurvivorPhoto, 0x552}] = {  // npc82 - Carlito
        TimeOfDay(1, 12, 0, true),
        false,
        "Carlito - Rooftop"
    };

    s_availabilityDB[{CheckType::SurvivorPhoto, 0x4D5}] = {  // npc80 - Jeff Meyer
        TimeOfDay(1, 12, 0, true),
        false,
        "Jeff Meyer - Rooftop"
    };
    
    s_availabilityDB[{CheckType::SurvivorPhoto, 0x4D2}] = {  // npc81 - Natalie Meyer
        TimeOfDay(1, 12, 0, true),
        false,
        "Natalie Meyer - Rooftop"
    };
    
    s_availabilityDB[{CheckType::SurvivorPhoto, 0x4E4}] = {  // npc82 - Bill Brenton
        TimeOfDay(1, 12, 0, true),
        false,
        "Bill Brenton - In The Closet, Entrance Plaza"
    };

    s_availabilityDB[{CheckType::SurvivorPhoto, 0x4F1}] = {  // npc82 - Kent
        TimeOfDay(1, 2, 0, true),
        false,
        "Kent - Columbian Roastmaster"
    };
    
    // 4:00 PM
    s_availabilityDB[{CheckType::SurvivorPhoto, 0x4D4}] = {  // npc40 - Aaron Swoop
        TimeOfDay(1, 4, 0, true),
        false,
        "Aaron Swoop - Weber's Garments, Al Fresca Plaza"
    };
    
    s_availabilityDB[{CheckType::SurvivorPhoto, 0x4D0}] = {  // npc42 - Burt Thompson
        TimeOfDay(1, 4, 0, true),
        false,
        "Burt Thompson - Weber's Garments, Al Fresca Plaza"
    };
    
    // 5:00 PM
    s_availabilityDB[{CheckType::SurvivorPhoto, 0x4DC}] = {  // npc44 - Leah Stein
        TimeOfDay(1, 5, 0, true),
        false,
        "Leah Stein - Riverfield Jewelry, Al Fresca Plaza"
    };
    
    // 6:00 PM
    s_availabilityDB[{CheckType::SurvivorPhoto, 0x4EF}] = {  // npc5A - Sophie Richard
        TimeOfDay(1, 6, 0, true),
        false,
        "Sophie Richard - Leisure Park"
    };
    
    // 9:00 PM
    s_availabilityDB[{CheckType::SurvivorPhoto, 0x524}] = {  // npc90 - Greg Simpson
        TimeOfDay(1, 9, 0, true),
        false,
        "Greg Simpson - Wonderland Plaza Space Ride"
    };
    
    // 10:00 PM
    s_availabilityDB[{CheckType::SurvivorPhoto, 0x4DF}] = {  // npc91 - Yuu Tanaka
        TimeOfDay(1, 10, 0, true),
        false,
        "Yuu Tanaka - Sir Book-A-Lot, Wonderland Plaza"
    };
    
    s_availabilityDB[{CheckType::SurvivorPhoto, 0x4E0}] = {  // npc92 - Shinji Kitano
        TimeOfDay(1, 10, 0, true),
        false,
        "Shinji Kitano - Sir Book-A-Lot, Wonderland Plaza"
    };
    
    // 11:00 PM
    s_availabilityDB[{CheckType::SurvivorPhoto, 0x4DD}] = {  // npc93 - David Bailey
        TimeOfDay(1, 11, 0, true),
        false,
        "David Bailey - Empty store, North Plaza"
    };
    
    // === DAY 2 - Sept 20 ===

    // 12:00 AM
    s_availabilityDB[{CheckType::SurvivorPhoto, 0x554}] = {  // npc94 - Barnaby
        TimeOfDay(2, 12, 0, false),
        false,
        "Tonya Waters - Run Like The Wind, Wonderland Plaza"
    };
    
    // 7:00 AM
    s_availabilityDB[{CheckType::SurvivorPhoto, 0x4E1}] = {  // npc94 - Tonya Waters
        TimeOfDay(2, 7, 0, false),
        false,
        "Tonya Waters - Run Like The Wind, Wonderland Plaza"
    };
    
    s_availabilityDB[{CheckType::SurvivorPhoto, 0x4E2}] = {  // npc95 - Ross Folk
        TimeOfDay(2, 7, 0, false),
        false,
        "Ross Folk - Run Like The Wind, Wonderland Plaza"
    };
    
    // 8:00 AM
    s_availabilityDB[{CheckType::SurvivorPhoto, 0x51C}] = {  // npc20 - Josh Manning
        TimeOfDay(2, 8, 0, false),
        false,
        "Josh Manning - Empty store, North Plaza"
    };
    
    s_availabilityDB[{CheckType::SurvivorPhoto, 0x51D}] = {  // npc21 - Barbara Patterson
        TimeOfDay(2, 8, 0, false),
        false,
        "Barbara Patterson - Empty store, North Plaza"
    };
    
    s_availabilityDB[{CheckType::SurvivorPhoto, 0x51E}] = {  // npc00 - Rich Atkins
        TimeOfDay(2, 8, 0, false),
        false,
        "Rich Atkins - Empty store, North Plaza"
    };
    
    // 9:00 AM
    s_availabilityDB[{CheckType::SurvivorPhoto, 0x4D1}] = {  // npc96 - Heather Tompkins
        TimeOfDay(2, 9, 0, false),
        false,
        "Heather Tompkins - Child's Play, Paradise Plaza"
    };
    
    s_availabilityDB[{CheckType::SurvivorPhoto, 0x4D6}] = {  // npc97 - Pamela Tompkins
        TimeOfDay(2, 9, 0, false),
        false,
        "Pamela Tompkins - Outside Child's Play, Paradise Plaza"
    };
    
    s_availabilityDB[{CheckType::SurvivorPhoto, 0x4D3}] = {  // npc45 - Gordon Stalworth
        TimeOfDay(2, 9, 0, false),
        false,
        "Gordon Stalworth - McHandy's Hardware, Al Fresca Plaza"
    };
    
    // 11:00 AM
    s_availabilityDB[{CheckType::SurvivorPhoto, 0x4DB}] = {  // npc46 - Ronald Shiner
        TimeOfDay(2, 11, 0, false),
        false,
        "Ronald Shiner - Jill's Sandwiches, Paradise Plaza"
    };
    
    // 12:00 PM (Noon)
    s_availabilityDB[{CheckType::SurvivorPhoto, 0x526}] = {  //  - Kay Nelson
        TimeOfDay(2, 12, 0, true),
        false,
        "Kay Nelson - Lovely Fashion House, Wonderland Plaza"
    };
    
    s_availabilityDB[{CheckType::SurvivorPhoto, 0x527}] = {  //  - Lilly Deacon
        TimeOfDay(2, 12, 0, true),
        false,
        "Lilly Deacon - Lovely Fashion House, Wonderland Plaza"
    };
    
    s_availabilityDB[{CheckType::SurvivorPhoto, 0x529}] = {  //  - Kelly Carpenter
        TimeOfDay(2, 12, 0, true),
        false,
        "Kelly Carpenter - Lovely Fashion House, Wonderland Plaza"
    };
    
    s_availabilityDB[{CheckType::SurvivorPhoto, 0x52A}] = {  // - Janet Star
        TimeOfDay(2, 12, 0, true),
        false,
        "Janet Star - Lovely Fashion House, Wonderland Plaza"
    };
    
    // 1:00 PM
    s_availabilityDB[{CheckType::SurvivorPhoto, 0x4F0}] = {  //  - Jennifer Gorman
        TimeOfDay(2, 1, 0, true),
        false,
        "Jennifer Gorman - Paradise Plaza near Jill's Sandwiches"
    };
    
    s_availabilityDB[{CheckType::SurvivorPhoto, 0x4E5}] = {  // npc50 - Sally Mills
        TimeOfDay(2, 1, 0, true),
        false,
        "Sally Mills - Wonderland Plaza"
    };
    
    s_availabilityDB[{CheckType::SurvivorPhoto, 0x4E6}] = {  // npc52 - Nick Evans
        TimeOfDay(2, 1, 0, true),
        false,
        "Nick Evans - Wonderland Plaza"
    };
    
    // 5:00 PM
    s_availabilityDB[{CheckType::SurvivorPhoto, 0x4E3}] = {  // npc54 - Wayne Blackwell
        TimeOfDay(2, 5, 0, true),
        false,
        "Wayne Blackwell - Estelle's Fine-lady Cosmetics, Entrance Plaza"
    };
    
    s_availabilityDB[{CheckType::SurvivorPhoto, 0x4D8}] = {  // npc01 - Jolie Wu
        TimeOfDay(2, 5, 0, true),
        false,
        "Jolie Wu - Gramma's Kids, Entrance Plaza"
    };
    
    s_availabilityDB[{CheckType::SurvivorPhoto, 0x4D9}] = {  // npc02 - Rachel Decker
        TimeOfDay(2, 5, 0, true),
        false,
        "Rachel Decker - Gramma's Kids, Entrance Plaza"
    };
    
    s_availabilityDB[{CheckType::SurvivorPhoto, 0x4DE}] = {  // npc03 - Floyd Sanders
        TimeOfDay(2, 5, 0, true),
        false,
        "Floyd Sanders - Ned's Knicknackery, Entrance Plaza"
    };
    
    // === DAY 3 - Sept 21 ===
    
    // 12:00 AM (Midnight)

    s_availabilityDB[{CheckType::SurvivorPhoto, 0x553}] = {  // Isabella (survivor encounter)
        TimeOfDay(2, 3, 0, true),  
        false,
        "Isabella - North Plaza (first meeting)"
    };

    s_availabilityDB[{CheckType::SurvivorPhoto, 0x4D7}] = {  // npc98 - Kindell Johnson
        TimeOfDay(3, 12, 0, false),
        false,
        "Kindell Johnson - North Plaza"
    };
    
    s_availabilityDB[{CheckType::SurvivorPhoto, 0x510}] = {  // npc0A - Ray Mathison
        TimeOfDay(3, 12, 0, false),
        false,
        "Ray Mathison - Colby's Movieland, Cinema 4"
    };
    
    s_availabilityDB[{CheckType::SurvivorPhoto, 0x512}] = {  // npc0B - Nathan Crabbe
        TimeOfDay(3, 12, 0, false),
        false,
        "Nathan Crabbe - Colby's Movieland, Cinema 4"
    };
    
    s_availabilityDB[{CheckType::SurvivorPhoto, 0x514}] = {  // npc0C - Michelle Feltz
        TimeOfDay(3, 12, 0, false),
        false,
        "Michelle Feltz - Colby's Movieland, Cinema 4"
    };
    
    s_availabilityDB[{CheckType::SurvivorPhoto, 0x516}] = {  // npc0D - Beth Shrake
        TimeOfDay(3, 12, 0, false),
        false,
        "Beth Shrake - Colby's Movieland, Cinema 4"
    };
    
    s_availabilityDB[{CheckType::SurvivorPhoto, 0x525}] = {  // npc0E - Cheryl Jones
        TimeOfDay(3, 12, 0, false),
        false,
        "Cheryl Jones - Colby's Movieland, Screen 4 storage room"
    };
    
    // 1:00 AM
    s_availabilityDB[{CheckType::SurvivorPhoto, 0x4EA}] = {  // npc04 - Brett Styles
        TimeOfDay(3, 1, 0, false),
        false,
        "Brett Styles - Huntin' Shack, North Plaza"
    };
    
    s_availabilityDB[{CheckType::SurvivorPhoto, 0x4EB}] = {  // npc05 - Jonathan Picardson
        TimeOfDay(3, 1, 0, false),
        false,
        "Jonathan Picardson - Huntin' Shack, North Plaza"
    };
    
    s_availabilityDB[{CheckType::SurvivorPhoto, 0x4ED}] = {  // npc06 - Alyssa Laurent
        TimeOfDay(3, 1, 0, false),
        false,
        "Alyssa Laurent - Huntin' Shack, North Plaza"
    };
    
    // 2:00 AM
    s_availabilityDB[{CheckType::SurvivorPhoto, 0x4E9}] = {  // npc07 - Gil Jiminez
        TimeOfDay(3, 2, 0, false),
        false,
        "Gil Jiminez - Chris' Fine Foods, Food Court"
    };
    
    // 5:00 AM
    s_availabilityDB[{CheckType::SurvivorPhoto, 0x51F}] = {  // npc0F - Mindy Baker
        TimeOfDay(3, 5, 0, false),
        false,
        "Mindy Baker - Casual Gals, Wonderland Plaza"
    };
    
    s_availabilityDB[{CheckType::SurvivorPhoto, 0x520}] = {  // npc10 - Debbie Willet
        TimeOfDay(3, 5, 0, false),
        false,
        "Debbie Willet - Casual Gals, Wonderland Plaza"
    };
    
    s_availabilityDB[{CheckType::SurvivorPhoto, 0x4EE}] = {  // npc56 - Paul Carson
        TimeOfDay(3, 5, 0, false),
        false,
        "Paul Carson - Casual Gals, Wonderland Plaza"
    };
    
    // 8:00 AM
    s_availabilityDB[{CheckType::SurvivorPhoto, 0x4E7}] = {  // npc08 - Leroy McKenna
        TimeOfDay(3, 8, 0, false),
        false,
        "Leroy McKenna - Estelle's Fine-lady Cosmetics, Wonderland Plaza"
    };
    
    s_availabilityDB[{CheckType::SurvivorPhoto, 0x4DA}] = {  // npc09 - Susan Walsh
        TimeOfDay(3, 8, 0, false),
        false,
        "Susan Walsh - Play Area, Wonderland Plaza"
    };
    
    // 12:00 PM (Noon)
    s_availabilityDB[{CheckType::SurvivorPhoto, 0x522}] = {  // npc11 - Tad Hawthorne
        TimeOfDay(3, 12, 0, true),
        false,
        "Tad Hawthorne - Colombian Roastmasters, Paradise Plaza"
    };
    
    s_availabilityDB[{CheckType::SurvivorPhoto, 0x4E8}] = {  // npc12 - Simone Ravendark
        TimeOfDay(3, 12, 0, true),
        false,
        "Simone Ravendark - Players, Paradise Plaza"
    };
}
    
// cultist at 6D3 trigger check?

void CheckAvailability::RegisterPsychopathTimes()
{
    // Better helper that's explicit about AM/PM
    auto TimeOfDay = [](int day, int hour, int minute, bool isPM) {
        // Convert to 24-hour format
        int hour24 = hour;
        if (isPM && hour != 12)
            hour24 += 12;
        else if (!isPM && hour == 12)
            hour24 = 0;
        
        // Game starts Day 1 at 12:00 PM (hour 12)
        // Calculate hours elapsed since start
        int gameStartHour = 12;  // Noon
        int hoursFromStart = ((day - 1) * 24) + (hour24 - gameStartHour);
        
        return TimeManager::GAME_START_TICK + 
               (hoursFromStart * TimeManager::TICKS_PER_HOUR) +
               (minute * TimeManager::TICKS_PER_MINUTE);
    };
    
    // Psychopaths appear at specific story times
    // Range: 0x6D0 - 0x6E9 (1744 - 1769 decimal)
    
    // Carlito (Food Court) - 0x6E3
    s_availabilityDB[{CheckType::PsychopathPhoto, 0x6E3}] = {
        TimeOfDay(1, 12, 0, true),  
        false,
        "Carlito - Food Court"
    };
    
    // Kent - 0x6D5
    s_availabilityDB[{CheckType::PsychopathPhoto, 0x6D5}] = {
        TimeOfDay(1, 2, 0, true),  
        false,
        "Kent - Paradise Plaza"
    };

    // Cletus - 0x6DF
    s_availabilityDB[{CheckType::PsychopathPhoto, 0x6DF}] = {
        TimeOfDay(1, 3, 0, true),  
        false,
        "Cletus - Huntin' Shack"
    };

    // Sam Convict - 0x6DC
    s_availabilityDB[{CheckType::PsychopathPhoto, 0x6DC}] = {
        TimeOfDay(1, 6, 0, true),  
        false,
        "Sam Convict - Leisure Park"
    };

    // Reginald Convict - 0x6DE
    s_availabilityDB[{CheckType::PsychopathPhoto, 0x6DE}] = {
        TimeOfDay(1, 6, 0, true),  
        false,
        "Reginald Convict - Leisure Park"
    };

    // Miguel Convict - 0x6DD
    s_availabilityDB[{CheckType::PsychopathPhoto, 0x6DD}] = {
        TimeOfDay(1, 6, 0, true),  
        false,
        "Miguel Convict - Leisure Park"
    };

    // Adam - 0x6D8
    s_availabilityDB[{CheckType::PsychopathPhoto, 0x6D8}] = {
        TimeOfDay(1, 9, 0, true),  
        false,
        "Adam - Wonderland Plaza"
    };

    // Steven - 0x6DA
    s_availabilityDB[{CheckType::PsychopathPhoto, 0x6DA}] = {
        TimeOfDay(2, 8, 0, false),  
        false,
        "Steven - Seon's Food & Stuff"
    };

    // Cliff - 0x6D6
    s_availabilityDB[{CheckType::PsychopathPhoto, 0x6D6}] = {
        TimeOfDay(2, 8, 0, false),  
        false,
        "Cliff - Crislip's Home Saloon"
    };

    // Isabela - 0x6E6
    s_availabilityDB[{CheckType::PsychopathPhoto, 0x6E6}] = {
        TimeOfDay(2, 3, 0, true),  
        false,
        "Isabela - North Plaza"
    };

    // Jo - 0x6DB
    s_availabilityDB[{CheckType::PsychopathPhoto, 0x6DB}] = {
        TimeOfDay(2, 12, 0, true),  
        false,
        "Jo - Lovely Fashion House"
    };

    // Roger Hall - 0x6E7
    s_availabilityDB[{CheckType::PsychopathPhoto, 0x6E7}] = {
        TimeOfDay(2, 5, 0, true),  
        false,
        "Roger Hall - Entrance Plaza"
    };

    // Jack Hall - 0x6E8
    s_availabilityDB[{CheckType::PsychopathPhoto, 0x6E8}] = {
        TimeOfDay(2, 5, 0, true),  
        false,
        "Jack Hall - Entrance Plaza"
    };

    // Thomas Hall - 0x6E9
    s_availabilityDB[{CheckType::PsychopathPhoto, 0x6E9}] = {
        TimeOfDay(2, 5, 0, true),  
        false,
        "Thomas Hall - Entrance Plaza"
    };

    // Sean - 0x6D4
    s_availabilityDB[{CheckType::PsychopathPhoto, 0x6D4}] = {
        TimeOfDay(3, 12, 0, false),  
        false,
        "Sean - Colby's Movieland"
    };

    // Paul - 0x6D7
    s_availabilityDB[{CheckType::PsychopathPhoto, 0x6D7}] = {
        TimeOfDay(3, 4, 0, false),  
        false,
        "Paul - Casual Gals"
    };

    // Larry - 0x6D9
    s_availabilityDB[{CheckType::PsychopathPhoto, 0x6D9}] = {
        TimeOfDay(3, 5, 0, true),  
        false,
        "Larry - Meat Processing"
    };
}

void CheckAvailability::RegisterClothingTimes()
{
    // All clothing pickups are available from game start —
    // stores are accessible immediately and none are story-gated.

    // Slot 0 - Outfits
    for (int id = 0; id <= 26; id++)
        s_availabilityDB[{CheckType::Clothing, (uint32_t)id}] = {
            TimeManager::GAME_START_TICK, false,
            "Clothing - Slot 0 Outfit"
        };
    for (int id = 28; id <= 33; id++)
        s_availabilityDB[{CheckType::Clothing, (uint32_t)id}] = {
            TimeManager::GAME_START_TICK, false,
            "Clothing - Slot 0 Outfit"
        };
    s_availabilityDB[{CheckType::Clothing, 38}] = {
        TimeManager::GAME_START_TICK, false, "Clothing - Slot 0 Outfit" };
    for (int id = 40; id <= 42; id++)
        s_availabilityDB[{CheckType::Clothing, (uint32_t)id}] = {
            TimeManager::GAME_START_TICK, false,
            "Clothing - Slot 0 Outfit"
        };

    // Slot 1 - Shoes
    for (int id = 100; id <= 109; id++)
        s_availabilityDB[{CheckType::Clothing, (uint32_t)id}] = {
            TimeManager::GAME_START_TICK, false,
            "Clothing - Slot 1 Shoes"
        };

    // Slot 2 - Head
    for (int id : {202, 203, 204, 205, 207, 208, 209, 210, 211, 214, 215, 216, 217, 219})
        s_availabilityDB[{CheckType::Clothing, (uint32_t)id}] = {
            TimeManager::GAME_START_TICK, false,
            "Clothing - Slot 2 Head Wear"
        };

    // Slot 3 - Accessories
    for (int id : {301, 302, 304, 306, 310})
        s_availabilityDB[{CheckType::Clothing, (uint32_t)id}] = {
            TimeManager::GAME_START_TICK, false,
            "Clothing - Slot 3 Accessory"
        };

}

uint32_t CheckAvailability::GetEarliestTime(CheckId check)
{
    auto it = s_availabilityDB.find(check);
    if (it != s_availabilityDB.end())
        return it->second.earliestTime;
    
    // Unknown check - log which one is missing
    char buf[128];
    sprintf_s(buf, "[AVAILABILITY] WARNING: Unknown check [type=%u, id=0x%X (%u)] - assuming late game", 
              (uint32_t)check.type, check.id, check.id);
    LogLine(buf);
    
    return TimeManager::GAME_END_TICK;
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
        // Extract just the name part before the " - " location separator
        std::string fullDesc = it->second.notes;
        size_t dashPos = fullDesc.find(" - ");
        if (dashPos != std::string::npos)
        {
            return fullDesc.substr(0, dashPos);
        }
        return fullDesc;
    }
    
    // Fallback for unknown checks
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