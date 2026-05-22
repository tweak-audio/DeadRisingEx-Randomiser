
#pragma once
#include <Windows.h>
#include "DeadRisingEx/MtFramework/Player/Archipelago/Checks/CheckSystem.h"

void InitializeRewardNotifications();
void ShutdownRewardNotifications();
void UpdateRewardNotifications();
void ShowRewardNotification(RewardType type, const char* rewardName = nullptr, int value = 0, int itemId = -1);