#pragma once

#include <QString>

namespace PlayerControlTextUtils
{

QString networkSpeedLabel();
QString statisticsLabel();
QString subtitleSettingsLabel();
QString danmakuSettingsLabel();
QString strmDirectPlayLabel();
QString networkSpeedToast(bool enabled);
QString speedToast(const QString &speedText);
QString strmDirectToast(bool enabled);

} // namespace PlayerControlTextUtils
