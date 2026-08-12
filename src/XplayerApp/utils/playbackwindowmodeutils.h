#pragma once

#include <QString>

namespace PlaybackWindowModeUtils
{

bool defaultIndependentWindowEnabled();
bool shouldUseIndependentWindow(bool settingValue);
int launchTransitionDurationMs();
QString maximizeIconPath(bool isMaximized);
QString maximizeTooltipText(bool isMaximized);
QString topbarMaximizeIconPath(bool isMaximized);
QString topbarMaximizeTooltipText(bool isMaximized);

} // namespace PlaybackWindowModeUtils
