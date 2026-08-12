#include "playbackwindowmodeutils.h"

#include <QString>

namespace PlaybackWindowModeUtils
{

bool defaultIndependentWindowEnabled()
{
    return true;
}

bool shouldUseIndependentWindow(bool settingValue)
{
    return settingValue;
}

int launchTransitionDurationMs()
{
    return 180;
}

QString maximizeIconPath(bool isMaximized)
{
    return isMaximized ? QStringLiteral(":/svg/player/restore.svg")
                       : QStringLiteral(":/svg/player/max.svg");
}

QString maximizeTooltipText(bool isMaximized)
{
    return isMaximized ? QStringLiteral("退出全屏")
                       : QStringLiteral("全屏播放");
}

QString topbarMaximizeIconPath(bool isMaximized)
{
    return maximizeIconPath(isMaximized);
}

QString topbarMaximizeTooltipText(bool isMaximized)
{
    return maximizeTooltipText(isMaximized);
}

} // namespace PlaybackWindowModeUtils
