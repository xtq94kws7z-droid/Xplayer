#pragma once

#include <QSize>
#include <QtGlobal>
#include <limits>

namespace XplayerResponsiveUtils
{

qreal minimumScale();
qreal maximumScale();
qreal scaleForViewport(const QSize& viewportSize);
int scaled(int baseValue, qreal scale, int minimum = 0,
           int maximum = std::numeric_limits<int>::max());

} // namespace XplayerResponsiveUtils
