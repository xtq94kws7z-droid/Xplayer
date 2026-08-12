#include "playerinpututils.h"

namespace PlayerInputUtils
{
int consumeWheelVolumeDelta(int angleDeltaY, int pixelDeltaY,
                            int &angleRemainder, int &pixelRemainder)
{
    if (angleDeltaY != 0)
    {
        angleRemainder += angleDeltaY;
        const int steps = angleRemainder / 120;
        angleRemainder %= 120;
        return steps * 5;
    }

    if (pixelDeltaY == 0)
        return 0;

    pixelRemainder += pixelDeltaY;
    const int steps = pixelRemainder / 24;
    pixelRemainder %= 24;
    return steps * 2;
}
}
