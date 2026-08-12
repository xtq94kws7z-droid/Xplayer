#include "playerhudvisibilityutils.h"

namespace PlayerHudVisibilityUtils
{

AutoHideDecision autoHideDecision(const AutoHideContext &context)
{
    if (!context.isPlaying)
    {
        return AutoHideDecision::KeepVisibleNoRetry;
    }

    if (context.isLoading || context.hasActiveDialog || context.mediaSwitcherVisible)
    {
        return AutoHideDecision::KeepVisibleAndRetry;
    }

    const bool pointerIsOnInteractiveSurface =
        context.isAppActive && context.pointerInsidePlayer &&
        (context.pointerOnChrome || context.pointerOnPopup ||
         (context.rightSidebarVisible && context.pointerOnRightSidebar));

    if (pointerIsOnInteractiveSurface)
    {
        return AutoHideDecision::KeepVisibleAndRetry;
    }

    return AutoHideDecision::HideNow;
}

bool shouldUseAutoHideTimer(const AutoHideContext &context)
{
    return context.isPlaying && !context.hasActiveDialog;
}

} // namespace PlayerHudVisibilityUtils
