#pragma once

#include <QSize>

namespace SeasonResponsiveUtils
{
struct Metrics
{
    QSize posterSize;
    int actionButtonSize = 0;
    int actionIconSize = 0;
    int headerHorizontalMargin = 0;
    int headerTopMargin = 0;
    int headerSpacing = 0;
    int actionSpacing = 0;
    int textSpacing = 0;
    int overviewBottomMargin = 0;
    int filterHorizontalMargin = 0;
    int filterSpacing = 0;
    int mediaGridBasePadding = 0;
};

Metrics metricsForViewport(const QSize& viewportSize);
} // namespace SeasonResponsiveUtils
