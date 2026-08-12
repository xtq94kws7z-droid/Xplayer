#include "playbackwindowmodeutils.h"

#include <QString>

#ifdef Q_OS_WIN
#include <objbase.h>
#include <shobjidl.h>
#include <windows.h>
#endif

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

bool shouldNotifyWindowsShellFullscreen(Qt::WindowStates windowState)
{
    return windowState.testFlag(Qt::WindowFullScreen);
}

void notifyWindowsShellFullscreen(WId windowId, bool fullscreen)
{
#ifdef Q_OS_WIN
    if (!windowId)
    {
        return;
    }

    const HRESULT initializationResult =
        CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    ITaskbarList2 *taskbarList = nullptr;
    const HRESULT createResult = CoCreateInstance(
        CLSID_TaskbarList, nullptr, CLSCTX_INPROC_SERVER,
        IID_PPV_ARGS(&taskbarList));
    if (SUCCEEDED(createResult))
    {
        if (SUCCEEDED(taskbarList->HrInit()))
        {
            taskbarList->MarkFullscreenWindow(
                reinterpret_cast<HWND>(windowId), fullscreen ? TRUE : FALSE);
        }
        taskbarList->Release();
    }
    if (SUCCEEDED(initializationResult))
    {
        CoUninitialize();
    }
#else
    Q_UNUSED(windowId);
    Q_UNUSED(fullscreen);
#endif
}

} // namespace PlaybackWindowModeUtils
