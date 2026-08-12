#pragma once

namespace PlayerHudVisibilityUtils
{

enum class AutoHideDecision
{
    HideNow,
    KeepVisibleAndRetry,
    KeepVisibleNoRetry
};

struct AutoHideContext
{
    bool isPlaying = false;
    bool isLoading = false;
    bool hasActiveDialog = false;
    bool isAppActive = false;
    bool pointerInsidePlayer = false;
    bool pointerOnChrome = false;
    bool pointerOnPopup = false;
    bool rightSidebarVisible = false;
    bool pointerOnRightSidebar = false;
    bool mediaSwitcherVisible = false;
};

AutoHideDecision autoHideDecision(const AutoHideContext &context);
bool shouldUseAutoHideTimer(const AutoHideContext &context);

} // namespace PlayerHudVisibilityUtils
