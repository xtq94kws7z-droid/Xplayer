#include <QGuiApplication>
#include <QPixmap>
#include <QTextStream>

#include "../src/XplayerApp/utils/pixmapcachebudgetutils.h"

namespace
{
bool check(bool condition, const char *message)
{
    if (condition) {
        return true;
    }

    QTextStream(stderr) << "pixmapcachebudgetutils_test failed: "
                        << message << Qt::endl;
    return false;
}
} // namespace

int main(int argc, char** argv)
{
    QGuiApplication app(argc, argv);

    const QPixmap nullPixmap;
    if (!check(PixmapCacheBudgetUtils::costBytes(nullPixmap) == 0,
               "null pixmaps have zero cache cost")) {
        return 1;
    }

    QPixmap pixmap(64, 32);
    pixmap.fill(Qt::black);
    const qsizetype cost = PixmapCacheBudgetUtils::costBytes(pixmap);
    if (!check(cost >= 64 * 32 * 4,
               "valid pixmaps report a byte cost at least RGBA sized")) {
        return 1;
    }

    if (!check(PixmapCacheBudgetUtils::exceedsBudget(0, 0) == false,
               "empty caches do not exceed an empty budget")) {
        return 1;
    }

    if (!check(PixmapCacheBudgetUtils::exceedsBudget(cost, cost - 1),
               "costs above a positive budget exceed it")) {
        return 1;
    }

    return 0;
}
