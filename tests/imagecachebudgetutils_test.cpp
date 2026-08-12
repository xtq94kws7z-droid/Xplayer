#include <QImage>
#include <QTextStream>

#include "../src/XplayerCore/utils/imagecachebudgetutils.h"

namespace
{
bool check(bool condition, const char *message)
{
    if (condition) {
        return true;
    }

    QTextStream(stderr) << "imagecachebudgetutils_test failed: "
                        << message << Qt::endl;
    return false;
}
} // namespace

int main()
{
    const QImage nullImage;
    if (!check(ImageCacheBudgetUtils::costKb(nullImage, 0) == 1,
               "null images have a minimum cost")) {
        return 1;
    }

    const QImage smallImage(32, 32, QImage::Format_RGBA8888);
    const int smallCost = ImageCacheBudgetUtils::costKb(smallImage, 0);
    if (!check(smallCost >= 4, "valid images use their decoded byte size")) {
        return 1;
    }

    const QImage largeImage(4096, 4096, QImage::Format_RGBA8888);
    if (!check(ImageCacheBudgetUtils::costKb(largeImage, 1024) == 1024,
               "oversized images are clamped to the single-item budget")) {
        return 1;
    }

    if (!check(ImageCacheBudgetUtils::costKb(smallImage, 1) == 1,
               "positive caps can clamp normal images")) {
        return 1;
    }

    return 0;
}
