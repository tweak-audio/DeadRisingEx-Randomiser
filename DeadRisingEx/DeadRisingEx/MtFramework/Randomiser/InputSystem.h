

#pragma once
#include <cstdint>

void HandleDebugInput();
void LogLine(const char* text);
void InitRandomiserLog();
void ShutdownRandomiserLog();

void QueueGameEvent(uint32_t eventId);