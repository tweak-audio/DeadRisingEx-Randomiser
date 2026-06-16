#pragma once
#include <cstdint>

namespace KeyItemReward
{
    // Grants a key item directly to the player (vanilla flags + sounds).
    // keyId must match an entry in KeyItemCheck::kKeyEvents.
    // Returns the name string for notification, or nullptr if keyId is unknown.
    const char* GrantKeyItem(uint32_t keyId);
}
