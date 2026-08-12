#ifndef POSTERWALLUTILS_H
#define POSTERWALLUTILS_H

#include <QColor>
#include <QImage>
#include <QList>
#include <functional>

#include <models/media/mediaitem.h>

namespace PosterWallUtils
{

QList<MediaItem> mergeUniqueItems(const QList<MediaItem>& resume,
                                  const QList<MediaItem>& latest,
                                  const QList<MediaItem>& recommended,
                                  const QList<MediaItem>& completed,
                                  int maxItems);

int nextIndex(int currentIndex, int itemCount, int direction);

QList<int> visibleIndices(int currentIndex, int itemCount, int maxVisible);

QColor dominantColor(const QImage& image);

bool areImagesVisuallySimilar(const QImage& first, const QImage& second);

bool sameStageItems(const QList<MediaItem>& first,
                    const QList<MediaItem>& second);

QList<int> posterStageIndices(
    const QList<MediaItem>& items, int currentIndex, int maxVisible,
    const std::function<QImage(const MediaItem&)>& imageProvider);

} // namespace PosterWallUtils

#endif
