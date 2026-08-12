#pragma once

#include <QSize>
#include <QtGlobal>

namespace DashboardResponsiveUtils
{

struct Metrics
{
    qreal scale = 1.0;
    int contentWidth = 0;
    int horizontalMargin = 0;
    int topMargin = 0;
    int bottomMargin = 0;
    int sectionSpacing = 0;
    int heroHeight = 0;
    QSize posterTile;
    int galleryHeight = 0;
    QSize libraryTile;
    int libraryGalleryHeight = 0;
    int posterTitlePixels = 0;
    int posterSubtitlePixels = 0;
    int sectionTitlePixels = 0;
    int sectionMorePixels = 0;
};

int maxContentWidth();
int contentWidthForViewport(int viewportWidth);
Metrics metricsForViewport(const QSize& viewportSize);
int heroHeightForContentWidth(int contentWidth);
QSize posterTileSizeForContentWidth(int contentWidth);
int posterTitlePixelSize(int posterWidth);
int posterSubtitlePixelSize(int posterWidth);

} // namespace DashboardResponsiveUtils
