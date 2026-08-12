#include "mediacardlayoututils.h"

namespace MediaCardLayoutUtils
{

namespace
{
constexpr int kImageTitleGap = 6;
constexpr int kTitleMetaGap = 3;
constexpr int kCardPaintInset = 2;
}

int minimumTileHeight(int contentPadding, int imageHeight,
                      int titleLineCount, int titleLineSpacing,
                      int metaHeight, int bottomPadding)
{
    return kCardPaintInset * 2 + qMax(0, contentPadding) +
           qMax(0, imageHeight) + kImageTitleGap +
           qMax(1, titleLineCount) * qMax(1, titleLineSpacing) +
           kTitleMetaGap + qMax(0, metaHeight) + qMax(0, bottomPadding);
}

QSize detailEpisodeTileSize(int imageHeight, int horizontalPadding,
                            int contentPadding, int titleLineSpacing,
                            int titleLineCount, int metaHeight,
                            int bottomPadding)
{
    const int safeImageHeight = qMax(1, imageHeight);
    const int width = qRound(safeImageHeight * 16.0 / 9.0) +
                      qMax(0, horizontalPadding);
    const int paintedImageWidth = qMax(1, width - qMax(0, contentPadding) * 2);
    const int paintedImageHeight = qRound(paintedImageWidth * 9.0 / 16.0);
    return QSize(width,
                 minimumTileHeight(contentPadding, paintedImageHeight,
                                   titleLineCount, titleLineSpacing,
                                   metaHeight, bottomPadding));
}

TextLayout textLayout(const QRect& cardRect, const QRect& imageRect,
                      int contentPadding, bool episodeList,
                      int titleLineCount, int titleLineSpacing)
{
    const int padding = qMax(0, contentPadding);
    if (episodeList) {
        const int left = imageRect.right() + padding + 8;
        const QRect titleRect(left, cardRect.top() + padding,
                              cardRect.right() - left - padding,
                              qMax(24, cardRect.height() / 2 - padding));
        return {titleRect,
                QRect(left, titleRect.bottom() + 3, titleRect.width(), 22),
                Qt::AlignLeft | Qt::AlignTop,
                Qt::AlignLeft | Qt::AlignVCenter};
    }

    const int textTop = imageRect.bottom() + kImageTitleGap;
    const int titleHeight =
        qMax(1, titleLineCount) * qMax(1, titleLineSpacing);
    const QRect titleRect(cardRect.left() + padding, textTop,
                          cardRect.width() - padding * 2, titleHeight);
    return {titleRect,
            QRect(titleRect.left(), titleRect.bottom() + kTitleMetaGap,
                  titleRect.width(), 20),
            Qt::AlignHCenter | Qt::AlignTop,
            Qt::AlignCenter};
}

} // namespace MediaCardLayoutUtils
