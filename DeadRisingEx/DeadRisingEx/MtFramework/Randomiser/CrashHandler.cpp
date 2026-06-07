
#include "CrashHandler.h"
#include <stdio.h>
#include <dbghelp.h>
#pragma comment(lib, "dbghelp")

static HMODULE g_ourDllBase = nullptr;

// ── helpers ────────────────────────────────────────────────────────────────

static void GetLogPath(char* out, DWORD size)
{
    GetModuleFileNameA(NULL, out, size);
    char* sl = strrchr(out, '\\'); if (sl) *(sl+1) = 0;
    strcat_s(out, size, "crash_log.txt");
}

static void PrintAddr(FILE* f, HANDLE hProc, uintptr_t addr,
                      uintptr_t gameBase, uintptr_t dllBase, uintptr_t dllSize,
                      bool symOk)
{
    // Module label
    if (dllSize && addr >= dllBase && addr < dllBase + dllSize)
        fprintf(f, "DLL+0x%05llX", (unsigned long long)(addr - dllBase));
    else if (gameBase && addr >= gameBase && addr < gameBase + 0x10000000ULL)
        fprintf(f, "GAME+0x%llX", (unsigned long long)(addr - gameBase));
    else
        fprintf(f, "0x%llX", (unsigned long long)addr);

    if (!symOk) return;

    // Function name + offset
    char symBuf[sizeof(SYMBOL_INFO) + 512] = {};
    SYMBOL_INFO* sym = (SYMBOL_INFO*)symBuf;
    sym->SizeOfStruct = sizeof(SYMBOL_INFO);
    sym->MaxNameLen   = 512;
    DWORD64 fnDisp = 0;
    if (SymFromAddr(hProc, (DWORD64)addr, &fnDisp, sym))
    {
        fprintf(f, "  ->  %s", sym->Name);
        if (fnDisp) fprintf(f, "+0x%llX", (unsigned long long)fnDisp);
    }

    // Source file + line
    IMAGEHLP_LINE64 li = {};
    li.SizeOfStruct = sizeof(IMAGEHLP_LINE64);
    DWORD lineDisp = 0;
    if (SymGetLineFromAddr64(hProc, (DWORD64)addr, &lineDisp, &li))
        fprintf(f, "  [%s:%lu]", li.FileName, li.LineNumber);
}

// ── exception handler ──────────────────────────────────────────────────────

static LONG WINAPI CrashHandler(EXCEPTION_POINTERS* ep)
{
    char logPath[MAX_PATH] = {};
    GetLogPath(logPath, MAX_PATH);
    FILE* f = fopen(logPath, "a");
    if (!f) return EXCEPTION_CONTINUE_SEARCH;

    SYSTEMTIME t;
    GetLocalTime(&t);
    fprintf(f, "\n[CRASH] ===== %04d-%02d-%02d %02d:%02d:%02d =====\n",
        t.wYear, t.wMonth, t.wDay, t.wHour, t.wMinute, t.wSecond);

    // ── DbgHelp init ───────────────────────────────────────────────────────
    HANDLE hProc = GetCurrentProcess();

    // Build symbol search path: exe dir + DLL dir
    char exeDir[MAX_PATH] = {};
    GetModuleFileNameA(NULL, exeDir, MAX_PATH);
    { char* s = strrchr(exeDir, '\\'); if (s) *(s+1) = 0; }

    char dllDir[MAX_PATH] = {};
    GetModuleFileNameA(g_ourDllBase, dllDir, MAX_PATH);
    { char* s = strrchr(dllDir, '\\'); if (s) *(s+1) = 0; }

    char symPath[MAX_PATH * 2] = {};
    sprintf_s(symPath, "%s;%s", exeDir, dllDir);

    SymSetOptions(SYMOPT_UNDNAME | SYMOPT_LOAD_LINES | SYMOPT_DEFERRED_LOADS);
    bool symOk = SymInitialize(hProc, symPath, TRUE) != 0;

    // ── module layout ──────────────────────────────────────────────────────
    uintptr_t gameBase = (uintptr_t)GetModuleHandleA(nullptr);
    uintptr_t dllBase  = (uintptr_t)g_ourDllBase;

    // DLL size from PE headers (no extra libs needed)
    uintptr_t dllSize = 0;
    if (dllBase)
    {
        auto* dos = (IMAGE_DOS_HEADER*)dllBase;
        auto* nt  = (IMAGE_NT_HEADERS*)((BYTE*)dllBase + dos->e_lfanew);
        dllSize = nt->OptionalHeader.SizeOfImage;
    }

    uintptr_t fault = (uintptr_t)ep->ExceptionRecord->ExceptionAddress;

    // ── fault location ─────────────────────────────────────────────────────
    fprintf(f, "[CRASH] Code=0x%08X\n", ep->ExceptionRecord->ExceptionCode);
    fprintf(f, "[CRASH] Fault: ");
    PrintAddr(f, hProc, fault, gameBase, dllBase, dllSize, symOk);
    fprintf(f, "\n");

    if (ep->ExceptionRecord->ExceptionCode == 0xC0000005 &&
        ep->ExceptionRecord->NumberParameters >= 2)
    {
        fprintf(f, "[CRASH] AccessType=%s  BadAddr=0x%llX\n",
            ep->ExceptionRecord->ExceptionInformation[0] == 0 ? "READ" :
            ep->ExceptionRecord->ExceptionInformation[0] == 1 ? "WRITE" : "EXEC",
            (unsigned long long)ep->ExceptionRecord->ExceptionInformation[1]);
    }

    // ── registers ─────────────────────────────────────────────────────────
    CONTEXT* ctx = ep->ContextRecord;
    fprintf(f, "[CRASH] RAX=%016llX  RCX=%016llX  RDX=%016llX  RBX=%016llX\n",
        ctx->Rax, ctx->Rcx, ctx->Rdx, ctx->Rbx);
    fprintf(f, "[CRASH] RSP=%016llX  RBP=%016llX  RSI=%016llX  RDI=%016llX\n",
        ctx->Rsp, ctx->Rbp, ctx->Rsi, ctx->Rdi);
    fprintf(f, "[CRASH]  R8=%016llX   R9=%016llX  R10=%016llX  R11=%016llX\n",
        ctx->R8, ctx->R9, ctx->R10, ctx->R11);
    fprintf(f, "[CRASH] R12=%016llX  R13=%016llX  R14=%016llX  R15=%016llX\n",
        ctx->R12, ctx->R13, ctx->R14, ctx->R15);

    // ── stack walk from fault context ──────────────────────────────────────
    fprintf(f, "[CRASH] Stack:\n");

    STACKFRAME64 sf = {};
    sf.AddrPC.Offset    = ctx->Rip;  sf.AddrPC.Mode    = AddrModeFlat;
    sf.AddrFrame.Offset = ctx->Rbp;  sf.AddrFrame.Mode = AddrModeFlat;
    sf.AddrStack.Offset = ctx->Rsp;  sf.AddrStack.Mode = AddrModeFlat;

    CONTEXT ctxCopy = *ctx;
    for (int i = 0; i < 48; i++)
    {
        if (!StackWalk64(IMAGE_FILE_MACHINE_AMD64, hProc, GetCurrentThread(),
            &sf, &ctxCopy, nullptr,
            SymFunctionTableAccess64, SymGetModuleBase64, nullptr))
            break;
        if (sf.AddrPC.Offset == 0) break;

        fprintf(f, "[CRASH]   [%2d] ", i);
        PrintAddr(f, hProc, (uintptr_t)sf.AddrPC.Offset,
                  gameBase, dllBase, dllSize, symOk);
        fprintf(f, "\n");
    }

    if (symOk) SymCleanup(hProc);
    fclose(f);
    return EXCEPTION_CONTINUE_SEARCH;
}

void InstallCrashHandler(HMODULE hModule)
{
    g_ourDllBase = hModule;
    SetUnhandledExceptionFilter(CrashHandler);
}
