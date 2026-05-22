#pragma once
#include <Windows.h>

void GenerateRewardSlots();
void ResetRewardSlots();
int SpawnNextRewardSlotNearPlayer();
const char* GetItemNameFromId(DWORD itemId);
