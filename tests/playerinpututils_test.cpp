#include <QTextStream>

#include "../src/XplayerApp/utils/playerinpututils.h"

namespace
{
bool check(bool condition, const char *message)
{
    if (condition)
        return true;

    QTextStream(stderr) << "playerinpututils_test failed: " << message
                        << Qt::endl;
    return false;
}
}

int main()
{
    int angleRemainder = 0;
    int pixelRemainder = 0;

    if (!check(PlayerInputUtils::consumeWheelVolumeDelta(
                   120, 0, angleRemainder, pixelRemainder) == 5,
               "one mouse-wheel notch raises volume by five"))
        return 1;

    if (!check(PlayerInputUtils::consumeWheelVolumeDelta(
                   0, 0, angleRemainder, pixelRemainder) == 0,
               "empty high-resolution wheel events do not lower volume"))
        return 1;

    if (!check(PlayerInputUtils::consumeWheelVolumeDelta(
                   0, 8, angleRemainder, pixelRemainder) == 0,
               "small touchpad deltas accumulate without abrupt jumps"))
        return 1;
    if (!check(PlayerInputUtils::consumeWheelVolumeDelta(
                   0, 8, angleRemainder, pixelRemainder) == 0,
               "touchpad accumulation remains smooth"))
        return 1;
    if (!check(PlayerInputUtils::consumeWheelVolumeDelta(
                   0, 8, angleRemainder, pixelRemainder) == 2,
               "touchpad movement produces a small volume step"))
        return 1;

    if (!check(PlayerInputUtils::consumeWheelVolumeDelta(
                   -240, 0, angleRemainder, pixelRemainder) == -10,
               "multiple wheel notches preserve their magnitude"))
        return 1;

    return 0;
}
