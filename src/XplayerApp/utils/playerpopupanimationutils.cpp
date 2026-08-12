#include "playerpopupanimationutils.h"

#include "uianimationdefaults.h"

#include <QtGlobal>

namespace PlayerPopupAnimationUtils
{

int openDurationMs()
{
    return qMin(120, XplayerUi::kMicroAnimationMs);
}

int openOffsetY()
{
    return 8;
}

} // namespace PlayerPopupAnimationUtils
