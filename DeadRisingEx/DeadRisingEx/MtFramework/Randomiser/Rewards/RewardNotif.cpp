#include "RewardNotif.h"
#include "DeadRisingEx/MtFramework/Randomiser/InputSystem.h"
#include "DeadRisingEx/Utilities/DebugLog.h"
#include "DeadRisingEx/MtFramework/Rendering/ImGui/ImGuiRenderer.h"
#include "DeadRisingEx/MtFramework/Rendering/ImGui/imgui.h"

#include <vector>
#include <string>
#include <Windows.h>

// ─────────────────────────────────────────────
//  Notification state
// ─────────────────────────────────────────────

struct RewardNotification
{
    std::string text;
    float       timeRemaining;
    float       totalDuration;
};

static std::vector<RewardNotification> s_notifications;

// ─────────────────────────────────────────────
//  Internal helpers
// ─────────────────────────────────────────────

static void PushNotification(const char* text, float duration = 4.0f)
{
    if (!text || !*text) return;

    s_notifications.push_back({ text, duration, duration });

    char buf[256];
    sprintf_s(buf, sizeof(buf), "[REWARD NOTIF] %s (%.1fs)", text, duration);
    LogLine(buf);
}

static const char* GetRewardText(RewardType type, const char* rewardName, int value, int itemId)
{
    static char s_buf[128];

    switch (type)
    {
        case RewardType::LevelUp:
            return "LEVEL UP!";

        case RewardType::BatteryRefill:
            return "CAMERA REFILLED!";

        case RewardType::TimeChunk:
            sprintf_s(s_buf, sizeof(s_buf), "TIME EXTENDED! +%d CHUNK!", value);
            return s_buf;

        case RewardType::SetItem:
            if (rewardName && *rewardName)
                sprintf_s(s_buf, sizeof(s_buf), "ITEM: %s", rewardName);
            else
                return "ITEM UNLOCKED!";
            return s_buf;

        default:
            return "REWARD!";
    }
}


// ─────────────────────────────────────────────
//  Public API
// ─────────────────────────────────────────────

void InitializeRewardNotifications()
{
    s_notifications.clear();
    LogLine("[REWARD NOTIF] Initialized");
}

void ShutdownRewardNotifications()
{
    s_notifications.clear();
    LogLine("[REWARD NOTIF] Shutdown");
}

void ShowRewardNotification(RewardType type, const char* rewardName, int value, int itemId)
{
    PushNotification(GetRewardText(type, rewardName, value, itemId));
}

void UpdateRewardNotifications()
{
    if (s_notifications.empty()) return;

    static DWORD s_lastTick = GetTickCount();
    DWORD now  = GetTickCount();
    float dt   = (now - s_lastTick) / 1000.0f;
    s_lastTick = now;
    if (dt > 0.1f) dt = 0.1f;

    ImGuiIO& io       = ImGui::GetIO();
    ImVec2 screenSize = io.DisplaySize;

    ImGui::SetNextWindowPos(
        ImVec2(screenSize.x * 0.5f, screenSize.y * 0.15f),
        ImGuiCond_Always,
        ImVec2(0.5f, 0.5f));
    ImGui::SetNextWindowSize(ImVec2(600, 0), ImGuiCond_Always);
    ImGui::SetNextWindowBgAlpha(0.0f);

    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    ImGui::Begin("##rewardnotif", nullptr,
        ImGuiWindowFlags_NoDecoration    |
        ImGuiWindowFlags_NoInputs        |
        ImGuiWindowFlags_NoNav           |
        ImGuiWindowFlags_NoMove          |
        ImGuiWindowFlags_NoSavedSettings |
        ImGuiWindowFlags_NoBringToFrontOnFocus);
    ImGui::PopStyleVar();

    ImGui::SetWindowFontScale(2.0f);

    ImDrawList* drawList = ImGui::GetWindowDrawList();
    ImVec2 windowPos     = ImGui::GetWindowPos();
    float  windowWidth   = ImGui::GetWindowWidth();

    for (auto& notif : s_notifications)
    {
        float alpha   = 1.0f;
        float elapsed = notif.totalDuration - notif.timeRemaining;
        if (elapsed < 0.3f)
            alpha = elapsed / 0.3f;
        else if (notif.timeRemaining < 0.5f)
            alpha = notif.timeRemaining / 0.5f;

        ImU32 white = ImGui::ColorConvertFloat4ToU32(ImVec4(1.0f, 1.0f, 1.0f, alpha));
        ImU32 black = ImGui::ColorConvertFloat4ToU32(ImVec4(0.0f, 0.0f, 0.0f, alpha));

        
        ImFont* font     = ImGui::GetFont();
        float  fontSize  = font->FontSize * 2.0f;

        ImVec2 textSize  = font->CalcTextSizeA(fontSize, FLT_MAX, 0.0f, notif.text.c_str());
        float  cursorY   = ImGui::GetCursorPosY();
        float  textX     = windowPos.x + (windowWidth - textSize.x) * 0.5f;
        float  textY     = windowPos.y + cursorY;

        // Black outline — draw text at 8 offsets around the original position
        float offsets[][2] = {
            {-2, -2}, { 0, -2}, { 2, -2},
            {-2,  0},            { 2,  0},
            {-2,  2}, { 0,  2}, { 2,  2}
        };
        for (auto& off : offsets)
        {
            drawList->AddText(font, fontSize,
                ImVec2(textX + off[0], textY + off[1]),
                black, notif.text.c_str());
        }

        // White fill — draw twice for thickness
        drawList->AddText(font, fontSize, ImVec2(textX + 1, textY), white, notif.text.c_str());
        drawList->AddText(font, fontSize, ImVec2(textX - 1, textY), white, notif.text.c_str());
        drawList->AddText(font, fontSize, ImVec2(textX,     textY), white, notif.text.c_str());

        // Advance cursor so next notification renders below
        ImGui::Dummy(ImVec2(0, textSize.y + 6.0f));

        notif.timeRemaining -= dt;
    }

    ImGui::End();

    s_notifications.erase(
        std::remove_if(s_notifications.begin(), s_notifications.end(),
            [](const RewardNotification& n) { return n.timeRemaining <= 0.0f; }),
        s_notifications.end());
}