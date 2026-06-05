
#include "CrashHandler.h"
#include <stdio.h>
#include <imagehlp.h>
#pragma comment(lib, "imagehlp")

static HMODULE g_ourDllBase = nullptr;

static LONG WINAPI CrashHandler(EXCEPTION_POINTERS* ep)
{
    CHAR path[MAX_PATH] = { 0 };
    GetModuleFileNameA(NULL, path, MAX_PATH);
    char* sl = strrchr(path, '\\'); if (sl) *(sl+1) = 0;
    strcat_s(path, MAX_PATH, "transition_log.txt");

    FILE* f = fopen(path, "a");
    if (f)
    {
        uintptr_t gameBase = (uintptr_t)GetModuleHandleA(nullptr);
        uintptr_t dllBase  = (uintptr_t)g_ourDllBase;
        uintptr_t fault    = (uintptr_t)ep->ExceptionRecord->ExceptionAddress;

        uintptr_t gameRva = (fault >= gameBase && gameBase) ? (fault - gameBase + 0x140000000ULL) : 0;
        uintptr_t dllRva  = (fault >= dllBase  && dllBase)  ? (fault - dllBase) : 0;

        fprintf(f, "[CRASH] Code=0x%08X\n", ep->ExceptionRecord->ExceptionCode);
        fprintf(f, "[CRASH] GameBase=0x%llX  DLLBase=0x%llX  FaultAddr=0x%llX\n",
            (unsigned long long)gameBase, (unsigned long long)dllBase,
            (unsigned long long)fault);
        fprintf(f, "[CRASH] GameRVA=0x%llX  DLLRVA=0x%llX  (0=not in that module)\n",
            (unsigned long long)gameRva, (unsigned long long)dllRva);

        if (ep->ExceptionRecord->ExceptionCode == 0xC0000005 &&
            ep->ExceptionRecord->NumberParameters >= 2)
        {
            fprintf(f, "[CRASH] AccessType=%s  BadAddr=0x%llX\n",
                ep->ExceptionRecord->ExceptionInformation[0] == 0 ? "READ" :
                ep->ExceptionRecord->ExceptionInformation[0] == 1 ? "WRITE" : "EXEC",
                (unsigned long long)ep->ExceptionRecord->ExceptionInformation[1]);
        }

        CONTEXT* ctx = ep->ContextRecord;
        fprintf(f, "[CRASH] RCX=0x%llX  RDX=0x%llX  R8=0x%llX  R9=0x%llX\n",
            (unsigned long long)ctx->Rcx, (unsigned long long)ctx->Rdx,
            (unsigned long long)ctx->R8,  (unsigned long long)ctx->R9);
        fprintf(f, "[CRASH] RBX=0x%llX  RDI=0x%llX  RSI=0x%llX\n",
            (unsigned long long)ctx->Rbx, (unsigned long long)ctx->Rdi,
            (unsigned long long)ctx->Rsi);

        // Raw stack words
        uintptr_t* rsp = (uintptr_t*)ctx->Rsp;
        fprintf(f, "[CRASH] RSP[0..7]:");
        for (int i = 0; i < 8; i++)
        {
            __try { fprintf(f, " %llX", (unsigned long long)rsp[i]); }
            __except(EXCEPTION_EXECUTE_HANDLER) { break; }
        }
        fprintf(f, "\n");

        // Stack walk — print return addresses with DLL/game RVA labels
        fprintf(f, "[CRASH] Stack walk:\n");
        void* frames[48] = {};
        USHORT count = RtlCaptureStackBackTrace(0, 48, frames, NULL);
        for (USHORT i = 0; i < count; i++)
        {
            uintptr_t addr    = (uintptr_t)frames[i];
            uintptr_t gRva    = (addr >= gameBase && gameBase) ? (addr - gameBase + 0x140000000ULL) : 0;
            uintptr_t dRva    = (addr >= dllBase  && dllBase)  ? (addr - dllBase) : 0;
            if (dRva)
                fprintf(f, "[CRASH]   [%2u] DLL+0x%llX\n", i, (unsigned long long)dRva);
            else if (gRva)
                fprintf(f, "[CRASH]   [%2u] GAME+0x%llX\n", i, (unsigned long long)(addr - gameBase));
            else
                fprintf(f, "[CRASH]   [%2u] 0x%llX\n", i, (unsigned long long)addr);
        }
        fclose(f);
    }
    return EXCEPTION_CONTINUE_SEARCH;
}

void InstallCrashHandler(HMODULE hModule)
{
    g_ourDllBase = hModule;
    SetUnhandledExceptionFilter(CrashHandler);
}
