// dllmain.cpp : Defines the entry point for the DLL application.
#include <stdio.h>
#include <string>
#include <iostream>
#include <locale>
#include <codecvt>
#include <Windows.h>
#include <shellapi.h>
#include "LibMtFramework.h"
#include <detours.h>
#include "Misc/AsmHelpers.h"
#include "MtFramework/Utils/Utilities.h"
#include "DeadRisingEx/ModConfig.h"

#include <MtFramework/Game/sMain.h>
#include <MtFramework/IO/MtDataReader.h>
#include <MtFramework/Graphics/rTexture.h>

#include "DeadRisingEx/MtFramework/MtObjectImpl.h"
#include "DeadRisingEx/MtFramework/Archive/ArchiveOverlay.h"
#include "DeadRisingEx/MtFramework/Archive/sResourceImpl.h"
#include "DeadRisingEx/MtFramework/Debug/sSnatcherToolImpl.h"
#include "DeadRisingEx/MtFramework/Game/sMainImpl.h"
#include "DeadRisingEx/MtFramework/Game/sSnatcherPadImpl.h"
#include "DeadRisingEx/MtFramework/Game/sSnatcherMainImpl.h"
#include "DeadRisingEx/MtFramework/Game/Task/cGametaskTitleImpl.h"
#include "DeadRisingEx/MtFramework/Graphics/rModelImpl.h"
#include "DeadRisingEx/MtFramework/Memory/MtHeapAllocatorImpl.h"
#include "DeadRisingEx/MtFramework/Rendering/ImGui/ImGuiRenderer.h"
#include "DeadRisingEx/MtFramework/Rendering/ImGui/ImGuiConsole.h"
#include "DeadRisingEx/MtFramework/Rendering/sRenderImpl.h"
#include "DeadRisingEx/MtFramework/Rendering/sShaderImpl.h"
#include "DeadRisingEx/MtFramework/Item/uItemImpl.h"
#include "DeadRisingEx/MtFramework/Item/Items/uOm08Impl.h"
#include "DeadRisingEx/MtFramework/Object/sUnitImpl.h"
#include "DeadRisingEx/MtFramework/Object/uPhotoImpl.h"
#include "DeadRisingEx/MtFramework/Object/Model/sSMManagerImpl.h"
#include "DeadRisingEx/MtFramework/Object/Npc/uNpcMarkerImpl.h"
#include "DeadRisingEx/MtFramework/Player/uPlayerImpl.h"

#include "DeadRisingEx/MtFramework/Randomiser/CrashHandler.h"
#include "DeadRisingEx/MtFramework/Randomiser/TimeManager.h"
#include "DeadRisingEx/MtFramework/Randomiser/InputSystem.h"
#include "DeadRisingEx/MtFramework/Randomiser/AreaKeySystem.h"
#include "DeadRisingEx/MtFramework/Randomiser/AreaTransitionHook.h"
#include "DeadRisingEx/MtFramework/Randomiser/Checks/CheckSystem.h"
#include "DeadRisingEx/MtFramework/Randomiser/Checks/PPStickerCheck.h"
#include "DeadRisingEx/MtFramework/Randomiser/Checks/SurvivorPhotoCheck.h"
#include "DeadRisingEx/MtFramework/Randomiser/Checks/PsychopathPhotoCheck.h"
#include "DeadRisingEx/MtFramework/Randomiser/Checks/PhotoProcessHook.h"
#include "DeadRisingEx/MtFramework/Randomiser/Checks/ClothingCheck.h"
#include "DeadRisingEx/MtFramework/Randomiser/Rewards/SetItemRewardSystem.h"
#include "DeadRisingEx/MtFramework/Randomiser/Rewards/LevelUpRewardSystem.h"
#include "DeadRisingEx/MtFramework/Randomiser/Rewards/CameraRefillReward.h"
#include "DeadRisingEx/MtFramework/Randomiser/Rewards/TimeChunkReward.h"
#include "DeadRisingEx/MtFramework/Randomiser/Rewards/RewardNotif.h"
#include "DeadRisingEx/MtFramework/Randomiser/Rewards/ClothingRewardSystem.h"
#include "DeadRisingEx/Utilities/DebugLog.h"


// Version string for update 1 of the game exe.
const char *g_SupportedGameVersionString = "Master Oct  6 2016 23:23:44";

void ForceSymbolsHelper()
{
    MtDataReader *pDataReader = nullptr;
    MtFile *pFile = nullptr;
    MtFileStream *pFileStream = nullptr;
}

void(__stdcall *pOutputDebugStringA)(LPCSTR lpOutputString) = OutputDebugStringA;

void __stdcall Hook_OutputDebugStringA(LPCSTR lpOutputString)
{
    ImGuiConsole::Instance()->ConsolePrint(lpOutputString);
    pOutputDebugStringA(lpOutputString);

    if (ModConfig::Instance()->DebugLog == true)
        DebugLog::WriteMessage(lpOutputString);
}

void ForcePatchInfinityMode()
{
    void *pPatchAddr1 = (void*)GetModuleAddress(0x14021155F);
    void *pPatchAddr2 = (void*)GetModuleAddress(0x1402115EF);

    BYTE NopBytes[2] = { 0x90, 0x90 };

    PatchBytes(pPatchAddr1, NopBytes, sizeof(NopBytes));
    PatchBytes(pPatchAddr2, NopBytes, sizeof(NopBytes));
}

__declspec(dllexport) __declspec(noinline) void DummyExport()
{
    volatile int x = 0;
    (void)x;
}

bool __declspec(dllexport) LaunchDeadRisingEx(const char *psGameDirectory)
{
    CHAR sGameExe[MAX_PATH];
    CHAR sExDll[MAX_PATH];
    STARTUPINFO StartupInfo = { 0 };
    PROCESS_INFORMATION ProcInfo = { 0 };

    StartupInfo.cb = sizeof(STARTUPINFO);

    snprintf(sGameExe, sizeof(sGameExe), "%s\\DeadRising.exe", psGameDirectory);
    snprintf(sExDll, sizeof(sExDll), "%s\\DeadRisingEx.dll", psGameDirectory);

    LPCSTR DllsToInject[1] = { sExDll };

    if (DetourCreateProcessWithDllsA(sGameExe, NULL, NULL, NULL, FALSE, CREATE_SUSPENDED, NULL,
        psGameDirectory, &StartupInfo, &ProcInfo, 1, DllsToInject, NULL) == FALSE)
    {
        return false;
    }

    ResumeThread(ProcInfo.hThread);
    CloseHandle(ProcInfo.hProcess);
    CloseHandle(ProcInfo.hThread);
    return true;
}


BOOL APIENTRY DllMain(HMODULE hModule,
                      DWORD  ul_reason_for_call,
                      LPVOID lpReserved)
{
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
    {
        InstallCrashHandler(hModule);
       
        CHAR sModulePath[MAX_PATH] = { 0 };
        CHAR sModuleName[32] = { 0 };

        SnatcherModuleHandle = GetModuleHandle(NULL);

        GetModuleFileName(GetModuleHandle(NULL), sModulePath, sizeof(sModulePath));
        _splitpath_s(sModulePath, nullptr, 0, nullptr, 0, sModuleName, sizeof(sModuleName), nullptr, 0);

        if (_stricmp(sModuleName, "DeadRisingLauncher") == 0)
            return TRUE;

        if (strncmp(sMain::mBuildVersion, g_SupportedGameVersionString, strlen(g_SupportedGameVersionString)) != 0)
        {
            MessageBoxW(NULL, L"This version of Dead Rising is not supported by DeadRisingEx! Please update the game to the Oct 6th 2016 version in order to use DeadRisingEx.",
                L"Game version not supported", MB_OK | MB_ICONERROR | MB_APPLMODAL);
            TerminateProcess(GetCurrentProcess(), 0xBAD0C0DE);
        }

        if (ModConfig::Instance()->LoadConfigFile("DeadRisingEx.ini") == false)
            DbgPrint("Failed to load mod config file, using default settings!\n");

        if (ModConfig::Instance()->DebugLog == true)
            DebugLog::Initialize();

        InitRandomiserLog();

        MtObjectImpl::RegisterTypeInfo();
        sSnatcherToolImpl::RegisterTypeInfo();
        sResourceImpl::InitializeTypeInfo();
        rModelImpl::InitializeTypeInfo();
        sRenderImpl::RegisterTypeInfo();
        sShaderImpl::RegisterTypeInfo();
        uItemImpl::RegisterTypeInfo();
        sUnitImpl::RegisterTypeInfo();
        sMainImpl::RegisterTypeInfo();
        sSnatcherMainImpl::RegisterTypeInfo();
        MtHeapAllocatorImpl::RegisterTypeInfo();
        ImGuiRenderer::RegisterTypeInfo();

        if (ModConfig::Instance()->RecursiveGrenade == true)
            uOm08Impl::RegisterTypeInfo();

        DetourTransactionBegin();
        DetourUpdateThread(GetCurrentThread());

#ifdef _DEBUG
        ForcePatchInfinityMode();
#endif

        DetourAttach((void**)&pOutputDebugStringA, Hook_OutputDebugStringA);

        Utilities::InstallHooks();
        sRenderImpl::InstallHooks();
        sSnatcherPadImpl::InstallHooks();
        sMainImpl::InstallHooks();
        sSnatcherMainImpl::InstallHooks();
        sSMManagerImpl::InstallHooks();
        cGametaskTitleImpl::InstallHooks();
        uNpcMarkerImpl::InstallHooks();
        uPhotoImpl::InstallHooks();
        uItemImpl::InstallHooks();

        uPlayerImpl::RegisterTypeInfo();
        uPPStickerImpl::RegisterTypeInfo();
        SurvivorPhotoCheck::RegisterTypeInfo();
        PsychopathPhotoCheck::RegisterTypeInfo();

        if (ArchiveOverlay::Instance()->Initialize() == false)
        {
            DbgPrint("Failed to initialize the overlay system!\n");
            TerminateProcess(GetCurrentProcess(), 0xBAD0C0DE);
        }

        if (DetourTransactionCommit() != NO_ERROR)
            TerminateProcess(GetCurrentProcess(), 0xBAD0C0DE);

        InitScoopPhotoHook();
        //InitPsychopathPhotoHook();

        CheckSystem::OnCheckCompleted([](CheckId check, Reward reward)
        {
            
            switch (reward.type)
            {
                case RewardType::TimeChunk:
                    TimeChunkReward::GrantTimeChunk(reward.value);
                    ShowRewardNotification(RewardType::TimeChunk, nullptr, reward.value);
                    break;
                    
                case RewardType::SetItem:
                    {
                        int itemId = SpawnNextRewardSlotNearPlayer();
                        const char* itemName = itemId >= 0 ? GetItemNameFromId((DWORD)itemId) : "Unknown Item";
                        ShowRewardNotification(RewardType::SetItem, itemName, 0);
                        break;
                    }
                    
                case RewardType::LevelUp:
                    GrantLevels(1);
                    ShowRewardNotification(RewardType::LevelUp);
                    break;
                    
                case RewardType::BatteryRefill:
                    CameraRefillReward::SetFilmCount(30);
                    ShowRewardNotification(RewardType::BatteryRefill);
                    break;

                case RewardType::Clothing:
                    {
                        ClothingRewardResult result = GiveNextClothingReward();
                        ShowRewardNotification(RewardType::Clothing, result.name, 0);
                        break;
                    }

                case RewardType::AreaKey:
                    AreaKeySystem::Get().GiveKey(static_cast<ZoneID>(reward.value));
                    ShowRewardNotification(RewardType::AreaKey, nullptr, reward.value);
                    break;

                case RewardType::None:
                default:
                    break;
            }
        });

        CheckSystem::Initialize(); 
        TimeManager::Initialize();

        AreaKeySystem::Get().Init(0);
        AreaTransitionHook::Install();    
        // Initialize notification hook system (for displaying rewards)
        InitializeRewardNotifications();

        ForceSymbolsHelper();
        break;
    }
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
        break;
    case DLL_PROCESS_DETACH:
        ShutdownRewardNotifications(); 
        break;
    }
    return TRUE;
}