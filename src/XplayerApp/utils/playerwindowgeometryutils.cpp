#include "playerwindowgeometryutils.h"

#include <QtGlobal>

namespace PlayerWindowGeometryUtils
{
QRect boundedVideoGeometry(const QRect &currentGeometry,
                           const QRect &availableGeometry,
                           double videoAspectRatio)
{
    if (!currentGeometry.isValid() || !availableGeometry.isValid())
        return currentGeometry;
    if (availableGeometry.contains(currentGeometry))
        return currentGeometry;

    const double aspect = videoAspectRatio > 0.1
                              ? videoAspectRatio
                              : 16.0 / 9.0;
    int width = qMin(currentGeometry.width(), availableGeometry.width());
    int height = qRound(width / aspect);
    if (height > availableGeometry.height())
    {
        height = availableGeometry.height();
        width = qRound(height * aspect);
    }

    width = qMax(320, qMin(width, availableGeometry.width()));
    height = qMax(180, qMin(height, availableGeometry.height()));
    if (width % 2 != 0)
        --width;
    if (height % 2 != 0)
        --height;

    return QRect(availableGeometry.x() +
                     (availableGeometry.width() - width) / 2,
                 availableGeometry.y() +
                     (availableGeometry.height() - height) / 2,
                 width, height);
}
}
