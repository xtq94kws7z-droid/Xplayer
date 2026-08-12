#include "xplayerresponsiveutils.h"

#include <QtGlobal>

namespace XplayerResponsiveUtils
{
namespace
{

constexpr int kReferenceViewportWidth = 1920;
constexpr int kReferenceViewportHeight = 1080;
constexpr qreal kMinimumScale = 0.78;
constexpr qreal kMaximumScale = 1.45;

} // namespace

qreal minimumScale()
{
    return kMinimumScale;
}

qreal maximumScale()
{
    return kMaximumScale;
}

qreal scaleForViewport(const QSize& viewportSize)
{
    const int width = qMax(1, viewportSize.width());
    const int height = qMax(1, viewportSize.height());
    const qreal widthScale =
        static_cast<qreal>(width) / kReferenceViewportWidth;
    const qreal heightScale =
        static_cast<qreal>(height) / kReferenceViewportHeight;
    return qBound(kMinimumScale, qMin(widthScale, heightScale),
                  kMaximumScale);
}

int scaled(int baseValue, qreal scale, int minimum, int maximum)
{
    return qBound(minimum, qRound(baseValue * scale), maximum);
}

} // namespace XplayerResponsiveUtils
