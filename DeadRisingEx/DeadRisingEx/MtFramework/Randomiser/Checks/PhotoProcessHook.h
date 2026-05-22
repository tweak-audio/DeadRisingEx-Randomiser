

#pragma once
#include <cstdint>

typedef unsigned long long(__fastcall* ScoopPhotoFunc_t)(void* param_1, int param_2);

void InitScoopPhotoHook();
void StopScoopPhotoHook();