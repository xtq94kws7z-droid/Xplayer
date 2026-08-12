#include "dialogresponsiveutils.h"

#include <QtGlobal>

namespace DialogResponsiveUtils
{
QSize boundedMinimumSize(const QSize& preferredSize,
                         const QSize& availableSize,
                         qreal availableRatio,
                         const QSize& floorSize)
{
    if (availableSize.isEmpty())
        return preferredSize;

    const qreal ratio = qBound<qreal>(0.5, availableRatio, 1.0);
    const int maxWidth = qMax(1, qRound(availableSize.width() * ratio));
    const int maxHeight = qMax(1, qRound(availableSize.height() * ratio));

    const int floorWidth = qMin(floorSize.width(), maxWidth);
    const int floorHeight = qMin(floorSize.height(), maxHeight);

    return QSize(qBound(floorWidth, preferredSize.width(), maxWidth),
                 qBound(floorHeight, preferredSize.height(), maxHeight));
}
} // namespace DialogResponsiveUtils
