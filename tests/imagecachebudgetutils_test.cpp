#include <QImage>
#include <QSize>
#include <QTextStream>
#include <QString>

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

    if (!check(ImageCacheBudgetUtils::effectiveMaxWidth(
                   720, QStringLiteral("original"), true) == 720,
               "forced image requests ignore original-quality bypass")) {
        return 1;
    }

    if (!check(ImageCacheBudgetUtils::effectiveMaxWidth(
                   720, QStringLiteral("original"), false) == 0,
               "normal requests preserve original-quality behavior")) {
        return 1;
    }

    if (!check(ImageCacheBudgetUtils::decodeSizeForMaxWidth(
                   QSize(8000, 12000), 720) == QSize(720, 1080),
               "oversized portrait images decode within the width budget")) {
        return 1;
    }
    if (!check(ImageCacheBudgetUtils::decodeSizeForMaxWidth(
                   QSize(100, 10000), 720) == QSize(14, 1440),
               "extreme aspect ratios stay within the decoded pixel budget")) {
        return 1;
    }
    if (!check(ImageCacheBudgetUtils::decodeSizeForMaxWidth(
                   QSize(640, 960), 720) == QSize(640, 960),
               "small images keep their source size")) {
        return 1;
    }
    if (!check(ImageCacheBudgetUtils::decodeSizeForMaxWidth(
                   QSize(8000, 12000), 0) == QSize(8000, 12000),
               "unlimited requests keep their source size")) {
        return 1;
    }

    return 0;
}
