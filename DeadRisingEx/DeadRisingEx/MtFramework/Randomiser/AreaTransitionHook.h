#pragma once
#include <Windows.h>
#include <cstdint>

class AreaTransitionHook
{
public:
    static void Install();

private:
    // FUN_14008fb70 (AreaHitProcess) not hooked — see Install() comment.

    using FnTransitionStart = void(__fastcall*)(int64_t);
    static FnTransitionStart s_originalTransitionStart;
    static void __fastcall Hook_TransitionStart(int64_t param_1);

    using FnAreaChange = void(__fastcall*)(int64_t, uint32_t);
    static FnAreaChange s_originalAreaChange;
    static void __fastcall Hook_AreaChange(int64_t param_1, uint32_t areaId);
};