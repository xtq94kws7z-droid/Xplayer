#include "utils/smoothscrollcontroller.h"

#include <QScrollBar>
#include <QSignalSpy>
#include <QtTest>

class SmoothScrollControllerTest : public QObject
{
    Q_OBJECT

private slots:
    void frameIntervalAdaptsToDisplayRefreshRate_data();
    void frameIntervalAdaptsToDisplayRefreshRate();
    void pixelPreciseWheelInputUpdatesImmediately();
    void pixelPreciseWheelInputExposesShortScrollingWindow();
    void consecutivePixelWheelEventsKeepOneScrollingSession();
    void scrollingStateIsSingleFlightAndSettles();
};

void SmoothScrollControllerTest::frameIntervalAdaptsToDisplayRefreshRate_data()
{
    QTest::addColumn<qreal>("refreshRate");
    QTest::addColumn<int>("expectedIntervalMs");

    QTest::newRow("invalid-low-falls-back-to-60hz") << 0.0 << 17;
    QTest::newRow("60hz") << 60.0 << 17;
    QTest::newRow("90hz") << 90.0 << 11;
    QTest::newRow("144hz") << 144.0 << 7;
    QTest::newRow("240hz") << 240.0 << 4;
    QTest::newRow("refresh-rate-is-capped") << 360.0 << 4;
}

void SmoothScrollControllerTest::frameIntervalAdaptsToDisplayRefreshRate()
{
    QFETCH(qreal, refreshRate);
    QFETCH(int, expectedIntervalMs);

    QCOMPARE(SmoothScrollController::frameIntervalForRefreshRate(refreshRate),
             expectedIntervalMs);
}

void SmoothScrollControllerTest::pixelPreciseWheelInputUpdatesImmediately()
{
    QScrollBar scrollBar;
    scrollBar.setRange(0, 500);
    scrollBar.setValue(200);
    SmoothScrollController controller(&scrollBar);
    const QWheelEvent event(QPointF(), QPointF(), QPoint(0, -24), QPoint(),
                            Qt::NoButton, Qt::NoModifier,
                            Qt::ScrollUpdate, false);

    QVERIFY(controller.scrollByWheelEvent(&event, Qt::Vertical));
    QCOMPARE(scrollBar.value(), 224);
}

void SmoothScrollControllerTest::pixelPreciseWheelInputExposesShortScrollingWindow()
{
    QScrollBar scrollBar;
    scrollBar.setRange(0, 500);
    scrollBar.setValue(200);
    SmoothScrollController controller(&scrollBar);
    QSignalSpy scrollingSpy(&controller,
                            &SmoothScrollController::scrollingChanged);
    const QWheelEvent event(QPointF(), QPointF(), QPoint(0, -24), QPoint(),
                            Qt::NoButton, Qt::NoModifier,
                            Qt::ScrollUpdate, false);

    QVERIFY(controller.scrollByWheelEvent(&event, Qt::Vertical));
    QVERIFY(controller.isAnimating());
    QCOMPARE(scrollingSpy.count(), 1);
    QTRY_VERIFY_WITH_TIMEOUT(!controller.isAnimating(), 500);
    QCOMPARE(scrollingSpy.count(), 2);
}

void SmoothScrollControllerTest::consecutivePixelWheelEventsKeepOneScrollingSession()
{
    QScrollBar scrollBar;
    scrollBar.setRange(0, 500);
    scrollBar.setValue(200);
    SmoothScrollController controller(&scrollBar);
    QSignalSpy scrollingSpy(&controller,
                            &SmoothScrollController::scrollingChanged);
    const QWheelEvent event(QPointF(), QPointF(), QPoint(0, -12), QPoint(),
                            Qt::NoButton, Qt::NoModifier,
                            Qt::ScrollUpdate, false);

    QVERIFY(controller.scrollByWheelEvent(&event, Qt::Vertical));
    QVERIFY(controller.scrollByWheelEvent(&event, Qt::Vertical));
    QCOMPARE(scrollBar.value(), 224);
    QCOMPARE(scrollingSpy.count(), 1);
    QCOMPARE(scrollingSpy.at(0).at(0).toBool(), true);
}

void SmoothScrollControllerTest::scrollingStateIsSingleFlightAndSettles()
{
    QScrollBar scrollBar;
    scrollBar.setRange(0, 500);

    SmoothScrollController controller(&scrollBar);
    QSignalSpy scrollingSpy(&controller,
                            &SmoothScrollController::scrollingChanged);

    controller.scrollTo(240);
    controller.scrollTo(420);

    QCOMPARE(scrollingSpy.count(), 1);
    QCOMPARE(scrollingSpy.at(0).at(0).toBool(), true);
    QVERIFY(controller.isAnimating());
    QCOMPARE(controller.targetValue(), 420);

    QTRY_VERIFY_WITH_TIMEOUT(!controller.isAnimating(), 1000);
    QCOMPARE(scrollBar.value(), 420);
    QCOMPARE(scrollingSpy.count(), 2);
    QCOMPARE(scrollingSpy.at(1).at(0).toBool(), false);
}

QTEST_MAIN(SmoothScrollControllerTest)
#include "smoothscrollcontroller_test.moc"
