#pragma once

#include <QRect>

namespace PlayerWindowGeometryUtils
{
QRect boundedVideoGeometry(const QRect &currentGeometry,
                           const QRect &availableGeometry,
                           double videoAspectRatio);
}
