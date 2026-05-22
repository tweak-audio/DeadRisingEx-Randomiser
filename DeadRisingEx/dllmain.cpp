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

#include "DeadRisingEx/MtFramework/Randomiser/TimeManager.h"
#include "DeadRisingEx/MtFramework/Randomiser/Checks/CheckSystem.h"
#include "DeadRisingEx/MtFramework/Randomiser/Checks/PPStickerCheck.h"
#include "DeadRisingEx/MtFramework/Randomiser/Checks/SurvivorPhotoCheck.h"
#include "DeadRisingEx/MtFramework/Randomiser/Checks/PsychopathPhotoCheck.h"
#include "DeadRisingEx/MtFramework/Randomiser/Checks/PhotoProcessHook.h"
//#include "DeadRisingEx/MtFramework/Randomiser/Checks/PsychopathPhotoHook.h"
#include "DeadRisingEx/MtFramework/Randomiser/Rewards/SetItemRewardSystem.h"
#include "DeadRisingEx/MtFramework/Randomiser/Rewards/LevelUpRewardSystem.h"
#include "DeadRisingEx/MtFramework/Randomiser/Rewards/CameraRefillReward.h"
#include "DeadRisingEx/MtFramework/Randomiser/Rewards/TimeChunkReward.h"
#include "DeadRisingEx/MtFramework/Randomiser/Rewards/RewardNotif.h"
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
    // Print the message to the imgui console and the debugger.
    ImGuiConsole::Instance()->ConsolePrint(lpOutputString);
    pOutputDebugStringA(lpOutputString);

    // If the debug log is enabled write all messages to debug log.
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

__declspec(dllexport) void DummyExport()
{
    // Required for detours.
}

bool __declspec(dllexport) LaunchDeadRisingEx(const char *psGameDirectory)
{
    CHAR sGameExe[MAX_PATH];
    CHAR sExDll[MAX_PATH];
    STARTUPINFO StartupInfo = { 0 };
    PROCESS_INFORMATION ProcInfo = { 0 };

    // Initialize the startup info structure.
    StartupInfo.cb = sizeof(STARTUPINFO);

    // Format the file paths for the game exe and ex dll.
    snprintf(sGameExe, sizeof(sGameExe), "%s\\DeadRising.exe", psGameDirectory);
    snprintf(sExDll, sizeof(sExDll), "%s\\DeadRisingEx.dll", psGameDirectory);

    // Build our list of dlls to inject.
    LPCSTR DllsToInject[1] =
    {
        sExDll
    };

    // Create the game process in a suspended state with our dll.
    if (DetourCreateProcessWithDllsA(sGameExe, NULL, NULL, NULL, FALSE, CREATE_SUSPENDED, NULL, 
        psGameDirectory, &StartupInfo, &ProcInfo, 1, DllsToInject, NULL) == FALSE)
    {
        // Failed to create the game process.
        return false;
    }

    // Resume the process.
    ResumeThread(ProcInfo.hThread);

    // Close the process and thread handles.
    CloseHandle(ProcInfo.hProcess);
    CloseHandle(ProcInfo.hThread);
    return true;
}

BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
                     )
{
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
    {
        CHAR sModulePath[MAX_PATH] = { 0 };
        CHAR sModuleName[32] = { 0 };

        // Set the module handle.
        SnatcherModuleHandle = GetModuleHandle(NULL);

        // Get the name of the exe we are running under.
        GetModuleFileName(GetModuleHandle(NULL), sModulePath, sizeof(sModulePath));
        _splitpath_s(sModulePath, nullptr, 0, nullptr, 0, sModuleName, sizeof(sModuleName), nullptr, 0);

        // Check if we were run from the launcher or the game process.
        if (_stricmp(sModuleName, "DeadRisingLauncher") == 0)
        {
            // Bail out as we are not in the game process.
            return TRUE;
        }

        // Check the game version string to make sure we are loading with the correct game version.
        if (strncmp(sMain::mBuildVersion, g_SupportedGameVersionString, strlen(g_SupportedGameVersionString)) != 0)
        {
            // Game version not supported, display an error and kill the process.
            MessageBoxW(NULL, L"This version of Dead Rising is not supported by DeadRisingEx! Please update the game to the Oct 6th 2016 version in order to use DeadRisingEx.",
                L"Game version not supported", MB_OK | MB_ICONERROR | MB_APPLMODAL);
            TerminateProcess(GetCurrentProcess(), 0xBAD0C0DE);
        }

        // Load the mod config file.
        if (ModConfig::Instance()->LoadConfigFile("DeadRisingEx.ini") == false)
        {
            // Failed to load the mod config.
            DbgPrint("Failed to load mod config file, using default settings!\n");
        }

        // Register built in types.
        /*RegisterTypeInfo(&Vector3TypeInfo);
        RegisterTypeInfo(&Vector4TypeInfo);
        RegisterTypeInfo(&Matrix4x4TypeInfo);*/

        // Register types and commands.
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

        InitializeRewardNotifications();

        if (ModConfig::Instance()->RecursiveGrenade == true)
            uOm08Impl::RegisterTypeInfo();

        // Begin the detour transaction.
        DetourTransactionBegin();
        DetourUpdateThread(GetCurrentThread());

        // If the debug log is enabled hook debug output to file.
        if (ModConfig::Instance()->DebugLog == true)
            DebugLog::Initialize();

#ifdef _DEBUG
        ForcePatchInfinityMode();
#endif

        // Hook debug output to the imgui console.
        DetourAttach((void**)&pOutputDebugStringA, Hook_OutputDebugStringA);

        //DetourAttach((void**)&LoadSpriteData, Hook_LoadSpriteData);

        // Hook functions.
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

        // Register check ranges and initialize BEFORE any hooks can fire

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
                    
                case RewardType::None:
                default:
                    break;
            }
        });

        CheckSystem::Initialize(); 
        TimeManager::Initialize();
        // Initialize notification hook system (for displaying rewards)
        InitializeRewardNotifications();

        // Dummy function to force non-used symbols to be emitted in the pdb file.
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

