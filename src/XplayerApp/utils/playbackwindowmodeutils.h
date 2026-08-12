#pragma once

#include <QString>
#include <Qt>
#include <QtGui/qwindowdefs.h>

namespace PlaybackWindowModeUtils
{

bool defaultIndependentWindowEnabled();
bool shouldUseIndependentWindow(bool settingValue);
int launchTransitionDurationMs();
QString maximizeIconPath(bool isMaximized);
QString maximizeTooltipText(bool isMaximized);
QString topbarMaximizeIconPath(bool isMaximized);
QString topbarMaximizeTooltipText(bool isMaximized);
bool shouldNotifyWindowsShellFullscreen(Qt::WindowStates windowState);
void notifyWindowsShellFullscreen(WId windowId, bool fullscreen);

} // namespace PlaybackWindowModeUtils
