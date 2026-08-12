#include <QtTest>

#include "utils/imageconcurrencyutils.h"

class ImageConcurrencyUtilsTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void keepsStartupBudgetDuringBurst();
    void restoresNormalBudgetAfterBurst();
};

void ImageConcurrencyUtilsTest::keepsStartupBudgetDuringBurst()
{
    QCOMPARE(ImageConcurrencyUtils::maxConcurrentRequests(0), 4);
    QCOMPARE(ImageConcurrencyUtils::maxConcurrentRequests(4999), 4);
}

void ImageConcurrencyUtilsTest::restoresNormalBudgetAfterBurst()
{
    QCOMPARE(ImageConcurrencyUtils::maxConcurrentRequests(5000), 8);
    QCOMPARE(ImageConcurrencyUtils::maxConcurrentRequests(6000), 8);
}

QTEST_MAIN(ImageConcurrencyUtilsTest)
#include "imageconcurrencyutils_test.moc"
