#pragma once
#include <Windows.h>
#include <cstdint>

class AreaTransitionHook
{
public:
    static void Install();

private:
    using FnAreaHitProcess = void(__fastcall*)(int64_t, uint16_t, int16_t, uint32_t);
    static FnAreaHitProcess s_originalAreaHitProcess;
    static void __fastcall Hook_AreaHitProcess(int64_t param_1, uint16_t param_2, int16_t param_3, uint32_t param_4);

    using FnTransitionStart = void(__fastcall*)(int64_t);
    static FnTransitionStart s_originalTransitionStart;
    static void __fastcall Hook_TransitionStart(int64_t param_1);

    using FnAreaChange = void(__fastcall*)(int64_t, uint32_t);
    static FnAreaChange s_originalAreaChange;
    static void __fastcall Hook_AreaChange(int64_t param_1, uint32_t areaId);
};