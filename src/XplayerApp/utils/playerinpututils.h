#pragma once

namespace PlayerInputUtils
{
int consumeWheelVolumeDelta(int angleDeltaY, int pixelDeltaY,
                            int &angleRemainder, int &pixelRemainder);
}
