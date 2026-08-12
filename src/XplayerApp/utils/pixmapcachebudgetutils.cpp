#include "pixmapcachebudgetutils.h"

#include <QPixmap>

namespace PixmapCacheBudgetUtils
{
qsizetype costBytes(const QPixmap& pixmap)
{
    if (pixmap.isNull()) {
        return 0;
    }

    const qreal devicePixelRatio = qMax<qreal>(1.0, pixmap.devicePixelRatio());
    const qsizetype logicalPixels =
        static_cast<qsizetype>(pixmap.width()) *
        static_cast<qsizetype>(pixmap.height());
    const qsizetype physicalPixels = static_cast<qsizetype>(
        logicalPixels * devicePixelRatio * devicePixelRatio);
    const qsizetype bytesPerPixel =
        qMax<qsizetype>(4, (pixmap.depth() + 7) / 8);
    return qMax<qsizetype>(1, physicalPixels * bytesPerPixel);
}

bool exceedsBudget(qsizetype currentCostBytes, qsizetype budgetBytes)
{
    return budgetBytes > 0 && currentCostBytes > budgetBytes;
}
} // namespace PixmapCacheBudgetUtils
