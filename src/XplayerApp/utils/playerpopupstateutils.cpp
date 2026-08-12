#include "playerpopupstateutils.h"

namespace PlayerPopupStateUtils
{

PopupRequestAction actionForPopupRequest(bool hasActivePopup,
                                         bool sameOwnerButton)
{
    if (!hasActivePopup)
    {
        return PopupRequestAction::Open;
    }
    if (sameOwnerButton)
    {
        return PopupRequestAction::CloseCurrent;
    }
    return PopupRequestAction::Replace;
}

} // namespace PlayerPopupStateUtils
