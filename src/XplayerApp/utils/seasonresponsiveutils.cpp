#include "seasonresponsiveutils.h"

#include "xplayerresponsiveutils.h"

#include <QtGlobal>

namespace SeasonResponsiveUtils
{
Metrics metricsForViewport(const QSize& viewportSize)
{
    const qreal scale = XplayerResponsiveUtils::scaleForViewport(viewportSize);
    const int posterWidth =
        XplayerResponsiveUtils::scaled(250, scale, 190, 360);

    Metrics metrics;
    metrics.posterSize = QSize(posterWidth, qRound(posterWidth * 1.5));
    metrics.actionButtonSize =
        XplayerResponsiveUtils::scaled(36, scale, 30, 52);
    metrics.actionIconSize =
        XplayerResponsiveUtils::scaled(20, scale, 16, 28);
    metrics.headerHorizontalMargin =
        XplayerResponsiveUtils::scaled(40, scale, 28, 60);
    metrics.headerTopMargin =
        XplayerResponsiveUtils::scaled(40, scale, 28, 60);
    metrics.headerSpacing =
        XplayerResponsiveUtils::scaled(30, scale, 20, 44);
    metrics.actionSpacing =
        XplayerResponsiveUtils::scaled(12, scale, 8, 18);
    metrics.textSpacing =
        XplayerResponsiveUtils::scaled(10, scale, 7, 16);
    metrics.overviewBottomMargin =
        XplayerResponsiveUtils::scaled(15, scale, 10, 22);
    metrics.filterHorizontalMargin =
        XplayerResponsiveUtils::scaled(40, scale, 28, 60);
    metrics.filterSpacing =
        XplayerResponsiveUtils::scaled(15, scale, 10, 22);
    metrics.mediaGridBasePadding =
        XplayerResponsiveUtils::scaled(40, scale, 28, 60);
    return metrics;
}
} // namespace SeasonResponsiveUtils
