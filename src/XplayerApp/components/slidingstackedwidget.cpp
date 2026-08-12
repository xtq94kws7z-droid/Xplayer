#include "slidingstackedwidget.h"
#include <QWidget>
#include <QLabel>
#include <QPainter>
#include <QTimer>
#include "../../XplayerCore/config/configstore.h"
#include "config/config_keys.h"
#include "../utils/uianimationdefaults.h"

SlidingStackedWidget::SlidingStackedWidget(QWidget *parent)
    : QStackedWidget(parent),
    m_speed(XplayerUi::durationMs(XplayerUi::MotionDuration::Panel)),
    m_animationType(
        XplayerUi::easingCurve(XplayerUi::MotionCurve::Move).type()),
    m_isAnimating(false),
    m_nextIndex(0)
{
    m_animGroup = new QParallelAnimationGroup(this);
    connect(m_animGroup, &QParallelAnimationGroup::finished, this, &SlidingStackedWidget::animationDoneSlot);
}

void SlidingStackedWidget::setSpeed(int speed)
{
    m_speed = qMax(0, speed);
}

void SlidingStackedWidget::setEasingCurve(QEasingCurve::Type curveType)
{
    if (m_animationType == curveType) {
        return;
    }
    m_animationType = curveType;
}

void SlidingStackedWidget::slideInWgt(QWidget *widget, SlideDirection direction)
{
    int idx = indexOf(widget);
    if (idx != -1) {
        slideInIdx(idx, direction);
    }
}

void SlidingStackedWidget::slideInIdx(int idx, SlideDirection direction)
{
    if (idx < 0 || idx >= count())
        return;

    
    if (currentIndex() == idx && !m_isAnimating)
        return;

    if (m_isAnimating && m_nextIndex == idx)
        return;

    
    
    if (m_isAnimating) {
        
        
        
        bool allTargetsAlive = true;
        for (int i = 0; i < m_animGroup->animationCount(); ++i) {
            auto *anim = qobject_cast<QPropertyAnimation *>(m_animGroup->animationAt(i));
            if (anim && !anim->targetObject()) {
                allTargetsAlive = false;
                break;
            }
        }

        if (allTargetsAlive) {
            m_animGroup->stop();       
        }
        
        completeAnimation(false);
    }

    
    bool reduceAnimations = ConfigStore::instance()->get<bool>(ConfigKeys::UiAnimations, false);
    if (reduceAnimations) {
        m_nextIndex = idx;
        setCurrentIndex(idx);
        QWidget *w = widget(idx);
        if (w) w->raise();
        flushPendingWidgetDisposals();
        Q_EMIT animationFinished();
        return;
    }

    m_isAnimating = true;
    m_nextIndex = idx;

    int now = currentIndex();
    QWidget *widgetNow = widget(now);
    QWidget *widgetNext = widget(idx);

    
    if (!widgetNow || !widgetNext) {
        m_isAnimating = false;
        if (widgetNext) {
            setCurrentIndex(idx);
        }
        return;
    }

    const bool pinSnapshotTransition =
        widgetNow->property("pinSnapshotTransition").toBool() ||
        widgetNext->property("pinSnapshotTransition").toBool();
    if (pinSnapshotTransition) {
        m_nextIndex = idx;
        setCurrentIndex(idx);
        widgetNext->raise();
        m_isAnimating = false;
        flushPendingWidgetDisposals();
        Q_EMIT animationFinished();
        return;
    }

    int width = this->width();
    int height = this->height();

    int offsetX = 0;
    int offsetY = 0;

    SlideDirection actualDirection = direction;
    if (direction == Automatic) {
        if (now < idx)
            actualDirection = RightToLeft;
        else
            actualDirection = LeftToRight;
    }

    if (actualDirection == RightToLeft) {
        offsetX = width;
        offsetY = 0;
    } else if (actualDirection == LeftToRight) {
        offsetX = -width;
        offsetY = 0;
    } else if (actualDirection == BottomToTop) {
        offsetX = 0;
        offsetY = height;
    } else if (actualDirection == TopToBottom) {
        offsetX = 0;
        offsetY = -height;
    }

    
    const QRect nextGeometry(0, 0, width, height);
    if (widgetNext->geometry() != nextGeometry) {
        widgetNext->setGeometry(nextGeometry);
    }

    QPoint pNext = widgetNext->pos();
    QPoint pNow = widgetNow->pos();

    
    
    const bool useSnapshot =
        ConfigStore::instance()->get<bool>(ConfigKeys::SnapshotNavigation, false) ||
        widgetNow->property("isImmersive").toBool() ||
        widgetNow->property("preferSnapshotTransition").toBool() ||
        widgetNext->property("preferSnapshotTransition").toBool();
    if (useSnapshot) {
        clearSnapshotLabels();
        QLabel *currentSnapshot = createSnapshotLabel(widgetNow, widgetNow->geometry());
        QLabel *nextSnapshot = createSnapshotLabel(widgetNext, QRect(pNext, QSize(width, height)));
        currentSnapshot->move(pNow);
        nextSnapshot->move(QPoint(pNext.x() + offsetX,
                                  pNext.y() + offsetY));
        currentSnapshot->show();
        nextSnapshot->show();
        currentSnapshot->raise();
        nextSnapshot->raise();

        widgetNow->hide();
        widgetNext->hide();
        m_snapshotHiddenWidgets = {widgetNow, widgetNext};
        m_snapshotLabels = {currentSnapshot, nextSnapshot};
    }

    
    if (!useSnapshot) {
        const QPoint nextStartPos(pNext.x() + offsetX, pNext.y() + offsetY);
        if (widgetNext->pos() != nextStartPos) {
            widgetNext->move(nextStartPos);
        }
        widgetNext->show();
        widgetNext->raise();
    }

    
    QObject *animTarget = useSnapshot ? static_cast<QObject*>(m_snapshotLabels.value(0).data())
                                      : static_cast<QObject*>(widgetNow);
    QPropertyAnimation *animNow = new QPropertyAnimation(animTarget, "pos");
    animNow->setDuration(m_speed);
    animNow->setEasingCurve(m_animationType);
    animNow->setStartValue(pNow);
    animNow->setEndValue(QPoint(pNow.x() - offsetX,
                                pNow.y() - offsetY));

    
    QObject *nextAnimTarget = useSnapshot ? static_cast<QObject*>(m_snapshotLabels.value(1).data())
                                          : static_cast<QObject*>(widgetNext);
    QPropertyAnimation *animNext = new QPropertyAnimation(nextAnimTarget, "pos");
    animNext->setDuration(m_speed);
    animNext->setEasingCurve(m_animationType);
    animNext->setStartValue(useSnapshot ? m_snapshotLabels.value(1)->pos() : widgetNext->pos());
    animNext->setEndValue(pNext);

    m_animGroup->clear();
    m_animGroup->addAnimation(animNow);
    m_animGroup->addAnimation(animNext);
    m_animGroup->start();
}

void SlidingStackedWidget::disposeWidgetWhenSafe(QWidget *widget)
{
    if (!widget)
    {
        return;
    }

    const QPointer<QWidget> safeWidget(widget);
    if (!m_pendingWidgetDisposals.contains(safeWidget))
    {
        m_pendingWidgetDisposals.append(safeWidget);
    }

    if (m_isAnimating)
    {
        m_deferPendingDisposalsUntilAfterAnimation = true;
    }
    else
    {
        flushPendingWidgetDisposals();
    }
}

void SlidingStackedWidget::animationDoneSlot()
{
    completeAnimation(true);
}

void SlidingStackedWidget::completeAnimation(bool emitFinishedSignal)
{
    m_snapshotHiddenWidgets.clear();
    clearSnapshotLabels();

    if (m_nextIndex >= 0 && m_nextIndex < count()) {
        setCurrentIndex(m_nextIndex);
        QWidget *w = widget(m_nextIndex);
        if (w) {
            w->raise();
        }
    }
    m_isAnimating = false;
    if (m_deferPendingDisposalsUntilAfterAnimation)
    {
        m_deferPendingDisposalsUntilAfterAnimation = false;
        QTimer::singleShot(XplayerUi::kPostNavigationCleanupDelayMs, this,
                           [this]() { flushPendingWidgetDisposals(); });
    }
    else
    {
        flushPendingWidgetDisposals();
    }
    if (emitFinishedSignal) {
        Q_EMIT animationFinished();
    }
}

void SlidingStackedWidget::clearSnapshotLabels()
{
    for (const QPointer<QLabel> &label : std::as_const(m_snapshotLabels)) {
        if (label) {
            label->hide();
            label->deleteLater();
        }
    }
    m_snapshotLabels.clear();
}

QLabel *SlidingStackedWidget::createSnapshotLabel(QWidget *source, const QRect &geometry)
{
    auto *label = new QLabel(this);
    label->setAttribute(Qt::WA_TransparentForMouseEvents, true);
    label->setScaledContents(false);
    const QSize logicalSize = geometry.size();
    const qreal snapshotDpr = qMin<qreal>(source->devicePixelRatioF(), 1.25);
    const QSize pixelSize(qMax(1, qRound(logicalSize.width() * snapshotDpr)),
                          qMax(1, qRound(logicalSize.height() * snapshotDpr)));
    QPixmap snapshot(pixelSize);
    snapshot.setDevicePixelRatio(snapshotDpr);
    snapshot.fill(Qt::transparent);

    QPainter painter(&snapshot);
    source->render(&painter, QPoint(), QRect(QPoint(0, 0), logicalSize));
    painter.end();

    label->setPixmap(snapshot);
    label->setGeometry(geometry);
    return label;
}

void SlidingStackedWidget::flushPendingWidgetDisposals()
{
    if (m_isAnimating || m_pendingWidgetDisposals.isEmpty())
    {
        return;
    }

    const auto widgetsToDispose = m_pendingWidgetDisposals;
    m_pendingWidgetDisposals.clear();

    for (const QPointer<QWidget> &widget : widgetsToDispose)
    {
        if (!widget)
        {
            continue;
        }

        if (indexOf(widget) != -1)
        {
            removeWidget(widget);
        }
        widget->deleteLater();
    }
}
