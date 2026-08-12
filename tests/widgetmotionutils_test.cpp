#include "utils/uianimationdefaults.h"
#include "utils/widgetgeometryutils.h"
#include "utils/widgetmotion.h"

#include <QLabel>
#include <QPainter>
#include <QPointer>
#include <QPropertyAnimation>
#include <QPushButton>
#include <QRect>
#include <QWidget>
#include <QtTest>

class WidgetMotionUtilsTest : public QObject
{
    Q_OBJECT

private slots:
    void motionTokensMatchExistingBudgets();
    void geometryWritesOnlyWhenValueChanges();
    void textWritesOnlyWhenValueChanges();
    void moveAnimationSkipsSameTarget();
    void newMoveAnimationStopsPreviousMoveOwner();
    void propertyAnimationSkipsSameTarget();
    void propertyAnimationRetargetsFromCurrentValue();
    void valueAnimationUsesOwnerKeyForSingleFlight();
    void buttonIconSpinSkipsWhileAlreadyRunning();
};

void WidgetMotionUtilsTest::motionTokensMatchExistingBudgets()
{
    QCOMPARE(XplayerUi::durationMs(XplayerUi::MotionDuration::Instant), 0);
    QCOMPARE(XplayerUi::durationMs(XplayerUi::MotionDuration::Micro),
             XplayerUi::kMicroAnimationMs);
    QCOMPARE(XplayerUi::durationMs(XplayerUi::MotionDuration::Quick),
             XplayerUi::kQuickAnimationMs);
    QCOMPARE(XplayerUi::durationMs(XplayerUi::MotionDuration::Standard),
             XplayerUi::kStandardAnimationMs);
    QCOMPARE(XplayerUi::durationMs(XplayerUi::MotionDuration::Panel),
             XplayerUi::kPanelAnimationMs);
    QCOMPARE(XplayerUi::kScrollAnimationMs, 170);
    QCOMPARE(XplayerUi::durationMs(XplayerUi::MotionDuration::Large), 360);
    QCOMPARE(XplayerUi::durationMs(XplayerUi::MotionDuration::Hero),
             XplayerUi::kHeroAnimationMs);
    QCOMPARE(XplayerUi::durationMs(XplayerUi::MotionDuration::SpinnerCycle),
             XplayerUi::kSpinnerCycleAnimationMs);
    QCOMPARE(XplayerUi::easingCurve(XplayerUi::MotionCurve::Spinner).type(),
             QEasingCurve::Linear);
    QCOMPARE(XplayerUi::easingCurve(XplayerUi::MotionCurve::Pop).type(),
             QEasingCurve::InExpo);
}

void WidgetMotionUtilsTest::geometryWritesOnlyWhenValueChanges()
{
    QWidget widget;
    widget.setGeometry(QRect(10, 20, 30, 40));

    QVERIFY(!XplayerUi::setGeometryIfChanged(&widget, QRect(10, 20, 30, 40)));
    QVERIFY(XplayerUi::setGeometryIfChanged(&widget, QRect(11, 20, 30, 40)));
    QCOMPARE(widget.geometry(), QRect(11, 20, 30, 40));

    QVERIFY(!XplayerUi::moveIfChanged(&widget, QPoint(11, 20)));
    QVERIFY(XplayerUi::moveIfChanged(&widget, QPoint(12, 20)));
    QCOMPARE(widget.pos(), QPoint(12, 20));

    QVERIFY(!XplayerUi::resizeIfChanged(&widget, QSize(30, 40)));
    QVERIFY(XplayerUi::resizeIfChanged(&widget, QSize(31, 40)));
    QCOMPARE(widget.size(), QSize(31, 40));
}

void WidgetMotionUtilsTest::textWritesOnlyWhenValueChanges()
{
    QLabel label;
    label.setText(QStringLiteral("Ready"));

    QVERIFY(!XplayerUi::setTextIfChanged(&label, QStringLiteral("Ready")));
    QVERIFY(XplayerUi::setTextIfChanged(&label, QStringLiteral("Go")));
    QCOMPARE(label.text(), QStringLiteral("Go"));
}

void WidgetMotionUtilsTest::moveAnimationSkipsSameTarget()
{
    QWidget widget;
    widget.move(42, 24);

    auto* animation = XplayerUi::animateMove(
        &widget,
        QPoint(42, 24),
        XplayerUi::MotionSpec{XplayerUi::MotionDuration::Quick,
                              XplayerUi::MotionCurve::Move});

    QVERIFY(animation == nullptr);
    QCOMPARE(widget.pos(), QPoint(42, 24));
}

void WidgetMotionUtilsTest::newMoveAnimationStopsPreviousMoveOwner()
{
    QWidget widget;
    QPointer<QPropertyAnimation> first = XplayerUi::animateMove(
        &widget,
        QPoint(10, 0),
        XplayerUi::MotionSpec{XplayerUi::MotionDuration::Panel,
                              XplayerUi::MotionCurve::Move});
    QVERIFY(!first.isNull());
    QCOMPARE(first->state(), QAbstractAnimation::Running);

    auto* second = XplayerUi::animateMove(
        &widget,
        QPoint(20, 0),
        XplayerUi::MotionSpec{XplayerUi::MotionDuration::Panel,
                              XplayerUi::MotionCurve::Move});

    QVERIFY(second != nullptr);
    QVERIFY(first.isNull() || first->state() != QAbstractAnimation::Running);
    QCOMPARE(second->state(), QAbstractAnimation::Running);
}

void WidgetMotionUtilsTest::propertyAnimationSkipsSameTarget()
{
    QWidget widget;
    widget.setGeometry(QRect(10, 20, 30, 40));

    auto* animation = XplayerUi::animateProperty(
        &widget,
        "geometry",
        widget.geometry(),
        XplayerUi::MotionSpec{XplayerUi::MotionDuration::Panel,
                              XplayerUi::MotionCurve::Resize});

    QVERIFY(animation == nullptr);
    QCOMPARE(widget.geometry(), QRect(10, 20, 30, 40));
}

void WidgetMotionUtilsTest::propertyAnimationRetargetsFromCurrentValue()
{
    QWidget widget;
    widget.setGeometry(QRect(0, 0, 100, 100));

    QPointer<QPropertyAnimation> first = XplayerUi::animateProperty(
        &widget,
        "geometry",
        QRect(100, 0, 100, 100),
        XplayerUi::MotionSpec{XplayerUi::MotionDuration::Panel,
                              XplayerUi::MotionCurve::Resize});
    QVERIFY(!first.isNull());
    first->setCurrentTime(first->duration() / 2);
    const QRect currentGeometry = widget.geometry();

    auto* second = XplayerUi::animateProperty(
        &widget,
        "geometry",
        QRect(200, 0, 100, 100),
        XplayerUi::MotionSpec{XplayerUi::MotionDuration::Panel,
                              XplayerUi::MotionCurve::Resize});

    QVERIFY(second != nullptr);
    QVERIFY(first.isNull() || first->state() != QAbstractAnimation::Running);
    QCOMPARE(second->startValue().toRect(), currentGeometry);
    QCOMPARE(second->endValue().toRect(), QRect(200, 0, 100, 100));
}

void WidgetMotionUtilsTest::valueAnimationUsesOwnerKeyForSingleFlight()
{
    QObject owner;
    int lastValue = -1;
    QPointer<QVariantAnimation> first = XplayerUi::animateValue(
        &owner,
        "refreshRotation",
        0.0,
        360.0,
        XplayerUi::MotionSpec{XplayerUi::MotionDuration::Large,
                              XplayerUi::MotionCurve::Spinner},
        &owner,
        [&lastValue](const QVariant& value) {
            lastValue = qRound(value.toReal());
        });

    QVERIFY(!first.isNull());
    QCOMPARE(first->state(), QAbstractAnimation::Running);

    auto* second = XplayerUi::animateValue(
        &owner,
        "refreshRotation",
        0.0,
        180.0,
        XplayerUi::MotionSpec{XplayerUi::MotionDuration::Large,
                              XplayerUi::MotionCurve::Spinner},
        &owner,
        [&lastValue](const QVariant& value) {
            lastValue = qRound(value.toReal());
        });

    QVERIFY(second != nullptr);
    QVERIFY(first.isNull() || first->state() != QAbstractAnimation::Running);
    QCOMPARE(second->state(), QAbstractAnimation::Running);
    QVERIFY(lastValue >= 0);
}

void WidgetMotionUtilsTest::buttonIconSpinSkipsWhileAlreadyRunning()
{
    QPixmap pixmap(16, 16);
    pixmap.fill(Qt::transparent);
    QPainter painter(&pixmap);
    painter.fillRect(QRect(4, 2, 8, 12), Qt::white);
    painter.end();

    QPushButton button;
    button.setIcon(QIcon(pixmap));
    button.setIconSize(QSize(16, 16));

    auto* first = XplayerUi::animateButtonIconSpin(
        &button,
        XplayerUi::MotionSpec{XplayerUi::MotionDuration::SpinnerCycle,
                              XplayerUi::MotionCurve::Spinner});

    QVERIFY(first != nullptr);
    QCOMPARE(first->state(), QAbstractAnimation::Running);
    QVERIFY(button.property("xplayerIconSpinRunning").toBool());

    auto* second = XplayerUi::animateButtonIconSpin(
        &button,
        XplayerUi::MotionSpec{XplayerUi::MotionDuration::SpinnerCycle,
                              XplayerUi::MotionCurve::Spinner});

    QVERIFY(second == nullptr);
    QCOMPARE(first->state(), QAbstractAnimation::Running);
}

QTEST_MAIN(WidgetMotionUtilsTest)
#include "widgetmotionutils_test.moc"
