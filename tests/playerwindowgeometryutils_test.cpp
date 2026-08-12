#include <QTextStream>

#include "../src/XplayerApp/utils/playerwindowgeometryutils.h"

namespace
{
bool check(bool condition, const char *message)
{
    if (condition)
        return true;
    QTextStream(stderr) << "playerwindowgeometryutils_test failed: "
                        << message << Qt::endl;
    return false;
}
}

int main()
{
    const QRect available(1920, 0, 1536, 864);
    const QRect oversized(1600, -100, 2200, 1400);
    const QRect bounded = PlayerWindowGeometryUtils::boundedVideoGeometry(
        oversized, available, 16.0 / 9.0);

    if (!check(available.contains(bounded),
               "player geometry stays inside the active screen"))
        return 1;
    if (!check(bounded.width() <= available.width() &&
                   bounded.height() <= available.height(),
               "player geometry is capped by available pixels"))
        return 1;

    const double aspect = static_cast<double>(bounded.width()) /
                          static_cast<double>(bounded.height());
    if (!check(qAbs(aspect - (16.0 / 9.0)) < 0.03,
               "bounded geometry preserves the video aspect ratio"))
        return 1;

    const QRect alreadyValid(2100, 100, 960, 540);
    if (!check(PlayerWindowGeometryUtils::boundedVideoGeometry(
                   alreadyValid, available, 16.0 / 9.0) == alreadyValid,
               "valid player geometry remains stable"))
        return 1;

    return 0;
}
