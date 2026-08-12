#include "playercontroltextutils.h"

namespace PlayerControlTextUtils
{

QString networkSpeedLabel()
{
    return QStringLiteral("网络速度");
}

QString statisticsLabel()
{
    return QStringLiteral("播放信息");
}

QString subtitleSettingsLabel()
{
    return QStringLiteral("字幕设置");
}

QString danmakuSettingsLabel()
{
    return QStringLiteral("弹幕设置");
}

QString strmDirectPlayLabel()
{
    return QStringLiteral("STRM 直连播放");
}

QString networkSpeedToast(bool enabled)
{
    return enabled ? QStringLiteral("已显示网络速度")
                   : QStringLiteral("已隐藏网络速度");
}

QString speedToast(const QString &speedText)
{
    return QStringLiteral("倍速：%1x").arg(speedText);
}

QString strmDirectToast(bool enabled)
{
    return enabled ? QStringLiteral("STRM 直连播放已开启，正在重新加载...")
                   : QStringLiteral("STRM 直连播放已关闭，正在重新加载...");
}

} // namespace PlayerControlTextUtils
