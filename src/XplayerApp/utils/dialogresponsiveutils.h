#pragma once

#include <QSize>

namespace DialogResponsiveUtils
{
QSize boundedMinimumSize(const QSize& preferredSize,
                         const QSize& availableSize,
                         qreal availableRatio = 0.86,
                         const QSize& floorSize = QSize(760, 520));
} // namespace DialogResponsiveUtils
