#include "components/moderntoast.h"
#include "components/playerosdlayer.h"
#include "components/collapsiblesection.h"
#include "components/playerlongpresshandler.h"

#include <QLabel>
#include <QParallelAnimationGroup>
#include <QPropertyAnimation>
#include <QGraphicsOpacityEffect>
#include <QVBoxLayout>
#include <QtTest>

class MotionComponentsTest final : public QObject
{
    Q_OBJECT

private slots:
    void toastRetargetsPopFromCurrentGeometry();
    void osdFadeInRetargetsRunningFadeOut();
    void osdFadeOutHidesContainerWhenFinished();
    void collapsibleSectionRetargetsFromCurrentHeight();
    void longPressTeardownClearsInputState();
};

void MotionComponentsTest::toastRetargetsPopFromCurrentGeometry()
{
    QWidget host;
    host.setGeometry(100, 100, 900, 620);
    host.show();

    ModernToast toast(&host);
    toast.showWithAnimation(QStringLiteral("第一条消息"), 5000);
    QVERIFY(toast.m_showGroup != nullptr);
    toast.m_showGroup->setCurrentTime(80);

    const QRect inFlightGeometry = toast.geometry();
    QVERIFY(inFlightGeometry.isValid());
    QVERIFY(inFlightGeometry != toast.m_baseGeometry);

    toast.showWithAnimation(QStringLiteral("第二条消息"), 5000);

    QCOMPARE(toast.m_shellExpand->startValue().toRect(), inFlightGeometry);
    QCOMPARE(toast.geometry(), inFlightGeometry);
}

void MotionComponentsTest::osdFadeInRetargetsRunningFadeOut()
{
    QWidget host;
    host.resize(1280, 720);
    host.show();

    PlayerOsdLayer osd(&host);
    osd.updateGeometry(host.width(), host.height());
    osd.showSeek(30.0, 120.0, QStringLiteral("00:30"));
    osd.m_fadeAnim->setCurrentTime(osd.m_fadeAnim->duration());

    osd.hide();
    QVERIFY(osd.m_fadeAnim->state() == QAbstractAnimation::Running);
    osd.m_fadeAnim->setCurrentTime(osd.m_fadeAnim->duration() / 2);
    const qreal fadingOpacity = osd.m_opacity->opacity();
    QVERIFY(fadingOpacity > 0.0);
    QVERIFY(fadingOpacity < 1.0);

    osd.showVolume(42, QStringLiteral("42%"), false);

    QCOMPARE(osd.m_fadeAnim->endValue().toReal(), 1.0);
    QVERIFY(osd.m_fadeAnim->state() == QAbstractAnimation::Running);
    QVERIFY(osd.m_fadeAnim->startValue().toReal() >= fadingOpacity - 0.001);
}

void MotionComponentsTest::osdFadeOutHidesContainerWhenFinished()
{
    QWidget host;
    host.resize(1280, 720);
    host.show();

    PlayerOsdLayer osd(&host);
    osd.updateGeometry(host.width(), host.height());
    osd.showSeek(30.0, 120.0, QStringLiteral("00:30"));
    osd.m_fadeAnim->setCurrentTime(osd.m_fadeAnim->duration());
    QVERIFY(osd.isVisible());

    osd.hide();
    osd.m_fadeAnim->setCurrentTime(osd.m_fadeAnim->duration());

    QVERIFY(!osd.isVisible());
    QCOMPARE(osd.m_opacity->opacity(), 0.0);
}

void MotionComponentsTest::collapsibleSectionRetargetsFromCurrentHeight()
{
    CollapsibleSection section(QStringLiteral("高级设置"));
    section.contentLayout()->addWidget(new QLabel(QStringLiteral("Line 1")));
    section.contentLayout()->addWidget(new QLabel(QStringLiteral("Line 2")));
    section.resize(420, 240);

    section.setExpanded(true);
    QVERIFY(section.m_heightAnimation != nullptr);
    section.m_heightAnimation->setCurrentTime(
        section.m_heightAnimation->duration() / 2);
    const int currentHeight = section.m_content->maximumHeight();
    QVERIFY(currentHeight > 0);

    section.setExpanded(false);

    QCOMPARE(section.m_heightAnimation->startValue().toInt(), currentHeight);
    QCOMPARE(section.m_heightAnimation->endValue().toInt(), 0);
    QVERIFY(!section.m_content->isHidden());
}

void MotionComponentsTest::longPressTeardownClearsInputState()
{
    QWidget host;
    host.resize(1280, 720);

    PlayerLongPressHandler handler(&host);
    handler.startKeyLongPress(1, true);
    QCOMPARE(handler.keyLongPressDirection(), 1);

    handler.startMouseEdgeLongPress(-1);
    QCOMPARE(handler.mouseEdgeLongPressDirection(), -1);

    handler.setTeardown(true);

    QCOMPARE(handler.keyLongPressDirection(), 0);
    QCOMPARE(handler.mouseEdgeLongPressDirection(), 0);
    QVERIFY(!handler.isMouseEdgeActive());
    QVERIFY(!handler.isKeyLongPressTriggered());
}

QTEST_MAIN(MotionComponentsTest)
#include "motioncomponents_test.moc"
