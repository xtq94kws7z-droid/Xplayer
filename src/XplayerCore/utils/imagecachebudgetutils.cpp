#include "imagecachebudgetutils.h"

#include <QImage>
#include <QSize>
#include <QString>
#include <QtGlobal>
#include <limits>

namespace ImageCacheBudgetUtils
{
int costKb(const QImage& image, int maxSingleItemCostKb)
{
    const qint64 bytes = image.isNull() ? 0 : image.sizeInBytes();
    const qint64 uncappedCostKb = qMax<qint64>(1, (bytes + 1023) / 1024);
    if (maxSingleItemCostKb <= 0) {
        return static_cast<int>(qMin<qint64>(
            uncappedCostKb, std::numeric_limits<int>::max()));
    }

    return static_cast<int>(qMin<qint64>(
        uncappedCostKb, maxSingleItemCostKb));
}

int effectiveMaxWidth(int requestedMaxWidth, const QString& quality,
                      bool enforceRequestedLimit)
{
    if (requestedMaxWidth <= 0) {
        return 0;
    }
    if (enforceRequestedLimit) {
        return requestedMaxWidth;
    }
    if (quality == QLatin1String("low")) {
        return requestedMaxWidth / 2;
    }
    if (quality == QLatin1String("medium")) {
        return requestedMaxWidth * 3 / 4;
    }
    if (quality == QLatin1String("original")) {
        return 0;
    }
    return requestedMaxWidth;
}

QSize decodeSizeForMaxWidth(const QSize& sourceSize, int maxWidth)
{
    if (!sourceSize.isValid() || maxWidth <= 0) {
        return sourceSize;
    }
    const QSize maxDecodeSize(maxWidth, maxWidth * 2);
    if (sourceSize.width() <= maxDecodeSize.width() &&
        sourceSize.height() <= maxDecodeSize.height()) {
        return sourceSize;
    }
    return sourceSize.scaled(maxDecodeSize, Qt::KeepAspectRatio);
}
} // namespace ImageCacheBudgetUtils
