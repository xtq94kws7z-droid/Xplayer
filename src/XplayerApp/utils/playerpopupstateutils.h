#ifndef PLAYERPOPUPSTATEUTILS_H
#define PLAYERPOPUPSTATEUTILS_H

namespace PlayerPopupStateUtils
{

enum class PopupRequestAction
{
    Open,
    Replace,
    CloseCurrent
};

PopupRequestAction actionForPopupRequest(bool hasActivePopup,
                                         bool sameOwnerButton);

} // namespace PlayerPopupStateUtils

#endif // PLAYERPOPUPSTATEUTILS_H
