#include "imagecachebudgetutils.h"

#include <QImage>
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
} // namespace ImageCacheBudgetUtils
