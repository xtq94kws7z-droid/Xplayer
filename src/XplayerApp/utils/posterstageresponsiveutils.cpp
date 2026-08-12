#include "posterstageresponsiveutils.h"

#include <QtGlobal>

namespace PosterStageResponsiveUtils
{
namespace
{

constexpr int kReferenceStageWidth = 1872;
constexpr qreal kMinimumScale = 0.78;
constexpr qreal kMaximumScale = 1.08;

qreal scaleForStageWidth(int stageWidth)
{
    return qBound(kMinimumScale,
                  static_cast<qreal>(qMax(0, stageWidth)) /
                      kReferenceStageWidth,
                  kMaximumScale);
}

} // namespace

int posterSlotCount()
{
    return 5;
}

int infoPanelWidth(int stageWidth)
{
    return qBound(292, qRound(356 * scaleForStageWidth(stageWidth)), 424);
}

int posterWallLeft(int stageWidth, int panelWidth)
{
    return panelWidth + qRound(44 * scaleForStageWidth(stageWidth));
}

} // namespace PosterStageResponsiveUtils
