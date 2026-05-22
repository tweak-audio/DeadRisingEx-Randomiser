#include "PhotoProcessHook.h"
#include "PhotoIDLogger.h"  // ← ADD THIS
#include "PPStickerCheck.h"
#include "SurvivorPhotoCheck.h"
#include "PsychopathPhotoCheck.h"
#include "CheckSystem.h"
#include "../InputSystem.h"
#include "../../uPlayerImpl.h"

#include "MtFramework/Utils/Utilities.h"
#include <detours.h>
#include <cstdint>

typedef void(__fastcall* PhotoAwardFunc_t)(void* param_1, uint16_t subjectId, int basePP, int multiplier);

static PhotoAwardFunc_t s_OrigPhotoAward = nullptr;

void __fastcall Hook_PhotoAward(void* param_1, uint16_t subjectId, int basePP, int multiplier)
{
    __try
    {
        PhotoIDLogger::LogPhotoSubject(subjectId, basePP, multiplier);
        
        // Check if this is a PP sticker
        if (subjectId >= 0x80 && subjectId <= 0xE3)
        {
            CheckSystem::CompleteCheck(CheckType::PPSticker, subjectId);
        }
        // Check if this photo ID maps to a survivor
        // Check if this photo ID maps to a survivor OR psychopath
        else
        {
            uint32_t entityId = SurvivorPhotoCheck::GetSurvivorIdFromPhotoId(subjectId);
            if (entityId != 0)
            {
                char buf[256];
                
                // Check if it's a psychopath (0x6D0-0x6E9) or survivor (0x4D0-0x56C)
                if (entityId >= 0x6D0 && entityId <= 0x6E9)
                {
                    sprintf_s(buf, "[PHOTO AWARD] *** Psychopath PhotoID 0x%04X → PsychopathID 0x%X ***", 
                        subjectId, entityId);
                    LogLine(buf);
                    CheckSystem::CompleteCheck(CheckType::PsychopathPhoto, entityId);
                }
                else if (entityId >= 0x4D0 && entityId <= 0x56C)
                {
                    sprintf_s(buf, "[PHOTO AWARD] *** Survivor PhotoID 0x%04X → SurvivorID 0x%X ***", 
                        subjectId, entityId);
                    LogLine(buf);
                    CheckSystem::CompleteCheck(CheckType::SurvivorPhoto, entityId);
                }
            }
        }
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        LogLine("[PHOTO AWARD] Exception in detection logic");
    }
    
    s_OrigPhotoAward(param_1, subjectId, basePP, multiplier);
}

void InitScoopPhotoHook()
{
    // Initialize the logger
    PhotoIDLogger::Initialize();
    
    void* addr = GetModuleAddress(0x14001eaf0);
    if (!addr)
    {
        LogLine("[PHOTO AWARD ERROR] Failed to get address 0x14001eaf0");
        return;
    }
    
    s_OrigPhotoAward = (PhotoAwardFunc_t)addr;
    
    DetourTransactionBegin();
    DetourUpdateThread(GetCurrentThread());
    
    if (DetourAttach(&(PVOID&)s_OrigPhotoAward, Hook_PhotoAward) != NO_ERROR)
    {
        LogLine("[PHOTO AWARD ERROR] DetourAttach failed");
        DetourTransactionAbort();
        return;
    }
    
    if (DetourTransactionCommit() != NO_ERROR)
    {
        LogLine("[PHOTO AWARD ERROR] DetourTransactionCommit failed");
        return;
    }
    
    LogLine("[PHOTO AWARD] Hooked FUN_14001eaf0 - logging all photo subject IDs to file");
}