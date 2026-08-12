#include "dashboardresponsiveutils.h"

#include <QtMath>
#include <QtGlobal>

namespace DashboardResponsiveUtils
{
namespace
{

constexpr int kReferenceViewportWidth = 1920;
constexpr int kReferenceViewportHeight = 1080;
constexpr int kReferenceContentWidth = 1872;
constexpr int kReferencePosterWidth = 160;
constexpr int kReferenceHeroHeight = 388;
constexpr qreal kMinimumScale = 0.78;
constexpr qreal kMaximumScale = 1.08;
constexpr int kMaximumContentWidth = 1960;

int posterWidthForContentWidth(int contentWidth)
{
    const qreal scale =
        qBound(kMinimumScale,
               static_cast<qreal>(qMax(0, contentWidth)) /
                   kReferenceContentWidth,
               kMaximumScale);
    const int idealWidth =
        qBound(142, qRound(kReferencePosterWidth * scale), 184);
    const int visibleColumns =
        qBound(6, qRound(static_cast<qreal>(contentWidth) /
                         qMax(1, idealWidth)),
               11);
    return qBound(144, contentWidth / qMax(1, visibleColumns), 184);
}

} // namespace

int maxContentWidth()
{
    return kMaximumContentWidth;
}

int contentWidthForViewport(int viewportWidth)
{
    return qMin(qMax(0, viewportWidth - 48), kMaximumContentWidth - 48);
}

Metrics metricsForViewport(const QSize& viewportSize)
{
    const int viewportWidth =
        qMin(qMax(1, viewportSize.width()), kMaximumContentWidth);
    const int viewportHeight = qMax(1, viewportSize.height());
    const qreal widthScale =
        static_cast<qreal>(viewportWidth) / kReferenceViewportWidth;
    const qreal heightScale =
        static_cast<qreal>(viewportHeight) / kReferenceViewportHeight;
    const qreal scale =
        qBound(kMinimumScale, qMin(widthScale, heightScale), kMaximumScale);

    Metrics metrics;
    metrics.scale = scale;
    metrics.horizontalMargin = qMax(18, qRound(26 * scale));
    metrics.topMargin = qMax(14, qRound(20 * scale));
    metrics.bottomMargin = qMax(18, qRound(26 * scale));
    metrics.sectionSpacing = qMax(12, qRound(16 * scale));
    metrics.contentWidth =
        qMax(0, viewportWidth - metrics.horizontalMargin * 2);
    metrics.heroHeight = qMax(260, qRound(kReferenceHeroHeight * scale));

    const int posterWidth = posterWidthForContentWidth(metrics.contentWidth);
    metrics.posterTile =
        QSize(posterWidth, qRound(posterWidth * 1.85));
    metrics.galleryHeight = metrics.posterTile.height() + qMax(4, qRound(4 * scale));

    const int libraryImageHeight = qMax(132, qRound(160 * scale));
    const int libraryImageWidth = qRound(libraryImageHeight * 16.0 / 9.0);
    metrics.libraryGalleryHeight =
        qRound(8 * scale) + libraryImageHeight +
        qRound(libraryImageHeight * 0.035) + qRound(6 * scale) +
        qRound(32 * scale) + qRound(2 * scale) +
        qRound(18 * scale) + qRound(8 * scale);
    metrics.libraryTile =
        QSize(libraryImageWidth + qRound(16 * scale),
              metrics.libraryGalleryHeight);

    metrics.posterTitlePixels =
        qBound(11, qRound(14 * scale), 18);
    metrics.posterSubtitlePixels =
        qBound(10, qRound(12 * scale), 15);
    metrics.sectionTitlePixels =
        qBound(18, qRound(22 * scale), 28);
    metrics.sectionMorePixels =
        qBound(12, qRound(14 * scale), 18);

    return metrics;
}

int heroHeightForContentWidth(int contentWidth)
{
    const qreal scale =
        qBound(kMinimumScale,
               static_cast<qreal>(qMax(0, contentWidth)) /
                   kReferenceContentWidth,
               kMaximumScale);
    return qMax(260, qRound(kReferenceHeroHeight * scale));
}

QSize posterTileSizeForContentWidth(int contentWidth)
{
    const int posterWidth = posterWidthForContentWidth(contentWidth);
    return QSize(posterWidth, qRound(posterWidth * 1.85));
}

int posterTitlePixelSize(int posterWidth)
{
    const qreal scale =
        qBound(kMinimumScale,
               static_cast<qreal>(qMax(0, posterWidth)) /
                   kReferencePosterWidth,
               kMaximumScale);
    return qBound(11, qRound(14 * scale), 18);
}

int posterSubtitlePixelSize(int posterWidth)
{
    const qreal scale =
        qBound(kMinimumScale,
               static_cast<qreal>(qMax(0, posterWidth)) /
                   kReferencePosterWidth,
               kMaximumScale);
    return qBound(10, qRound(12 * scale), 15);
}

} // namespace DashboardResponsiveUtils
