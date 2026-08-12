#pragma once

#include <QRect>
#include <QSize>
#include <Qt>

namespace MediaCardLayoutUtils
{

struct TextLayout
{
    QRect titleRect;
    QRect metaRect;
    Qt::Alignment titleAlignment;
    Qt::Alignment metaAlignment;
};

TextLayout textLayout(const QRect& cardRect, const QRect& imageRect,
                      int contentPadding, bool episodeList,
                      int titleLineCount, int titleLineSpacing);
int minimumTileHeight(int contentPadding, int imageHeight,
                      int titleLineCount, int titleLineSpacing,
                      int metaHeight = 20, int bottomPadding = 8);
QSize detailEpisodeTileSize(int imageHeight, int horizontalPadding,
                            int contentPadding, int titleLineSpacing,
                            int titleLineCount = 2, int metaHeight = 20,
                            int bottomPadding = 8);

} // namespace MediaCardLayoutUtils
