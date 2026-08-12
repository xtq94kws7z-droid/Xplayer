#pragma once

#include <QString>

namespace PlayerTrackOptionTextUtils
{

enum class TrackKind
{
    Audio,
    Subtitle
};

QString trackLabel(TrackKind kind, int id, QString title, QString language);
QString disableLabel(TrackKind kind);
QString disabledToast(TrackKind kind);
QString selectedToast(TrackKind kind, const QString &label);

} // namespace PlayerTrackOptionTextUtils
