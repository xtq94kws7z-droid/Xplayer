#include "horizontalwidgetgallery.h"
#include "../utils/uianimationdefaults.h"
#include <QScrollArea>
#include <QHBoxLayout>
#include <QPushButton>
#include <QScrollBar>
#include <QPropertyAnimation>
#include <QEvent>
#include <QWheelEvent>
#include <QTimer>
#include <QCursor>
#include <QScroller>           
#include <QScrollerProperties> 
#include "../utils/uianimationdefaults.h"

HorizontalWidgetGallery::HorizontalWidgetGallery(QWidget* parent)
    : QWidget(parent), m_hScrollAnim(nullptr), m_hScrollTarget(0)
{
    setObjectName("horizontal-widget-gallery");
    setAttribute(Qt::WA_StyledBackground, true);

    setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Preferred);
    this->setMinimumWidth(1);
    this->setMouseTracking(true);

    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);

    m_scrollArea = new QScrollArea(this);
    m_scrollArea->setMinimumWidth(1);
    m_scrollArea->setWidgetResizable(true);
    m_scrollArea->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Preferred);
    m_scrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_scrollArea->setFrameShape(QFrame::NoFrame);
    m_scrollArea->setStyleSheet("QScrollArea { background: transparent; border: none; }");
    m_scrollArea->viewport()->setObjectName("gallery-viewport");
    m_scrollArea->viewport()->setStyleSheet("#gallery-viewport { background: transparent; }");
    m_scrollArea->viewport()->setMouseTracking(true);

    
    
    
    QScroller::grabGesture(m_scrollArea->viewport(), QScroller::LeftMouseButtonGesture);
    QScroller* scroller = QScroller::scroller(m_scrollArea->viewport());
    QScrollerProperties props = scroller->scrollerProperties();
    
    props.setScrollMetric(QScrollerProperties::VerticalOvershootPolicy, QScrollerProperties::OvershootAlwaysOff);
    props.setScrollMetric(QScrollerProperties::HorizontalOvershootPolicy, QScrollerProperties::OvershootAlwaysOff);
    
    props.setScrollMetric(QScrollerProperties::DragStartDistance, 0.001);
    scroller->setScrollerProperties(props);

    
    m_hScrollAnim = new QPropertyAnimation(m_scrollArea->horizontalScrollBar(), "value", this);
    m_hScrollAnim->setEasingCurve(
        XplayerUi::easingCurve(XplayerUi::MotionCurve::Move));
    m_hScrollAnim->setDuration(
        XplayerUi::durationMs(XplayerUi::MotionDuration::Compact));

    m_heightRefreshTimer = new QTimer(this);
    m_heightRefreshTimer->setSingleShot(true);
    m_heightRefreshTimer->setTimerType(Qt::PreciseTimer);
    m_heightRefreshTimer->setInterval(0);
    connect(m_heightRefreshTimer, &QTimer::timeout, this,
            &HorizontalWidgetGallery::applyContentHeight);

    m_contentWidget = new QWidget(m_scrollArea);
    m_contentWidget->setObjectName("gallery-content-widget");
    m_contentWidget->setStyleSheet("#gallery-content-widget { background: transparent; }");
    m_contentWidget->setMouseTracking(true);

    m_contentLayout = new QHBoxLayout(m_contentWidget);
    m_contentLayout->setSizeConstraint(QLayout::SetMinimumSize);
    m_contentLayout->setContentsMargins(0, 0, 100, 0);
    m_contentLayout->setSpacing(16);
    m_contentLayout->setAlignment(Qt::AlignLeft | Qt::AlignTop);

    m_scrollArea->setWidget(m_contentWidget);
    mainLayout->addWidget(m_scrollArea);

    
    
    
    m_btnLeft = new QPushButton("❮", this);
    m_btnRight = new QPushButton("❯", this);
    m_btnLeft->setObjectName("gallery-nav-btn");
    m_btnRight->setObjectName("gallery-nav-btn");

    m_btnLeft->setFixedSize(44, 62);
    m_btnRight->setFixedSize(44, 62);
    m_btnLeft->setCursor(Qt::PointingHandCursor);
    m_btnRight->setCursor(Qt::PointingHandCursor);
    m_btnLeft->setFocusPolicy(Qt::NoFocus);
    m_btnRight->setFocusPolicy(Qt::NoFocus);

    m_btnLeft->hide();
    m_btnRight->hide();

    auto scrollAction = [this](int directionMultiplier) {
        QScrollBar* bar = m_scrollArea->horizontalScrollBar();
        int step = this->width() / 2;
        int targetValue = bar->value() + directionMultiplier * step;
        targetValue = qBound(0, targetValue, bar->maximum());

        if (!m_hScrollAnim || !m_hScrollAnim->targetObject()) {
            return;
        }
        m_hScrollTarget = targetValue;
        m_hScrollAnim->stop();
        m_hScrollAnim->setStartValue(bar->value());
        m_hScrollAnim->setEndValue(m_hScrollTarget);
        m_hScrollAnim->start();
    };

    connect(m_btnLeft, &QPushButton::clicked, [scrollAction]() { scrollAction(-1); });
    connect(m_btnRight, &QPushButton::clicked, [scrollAction]() { scrollAction(1); });

    connect(m_scrollArea->horizontalScrollBar(), &QScrollBar::valueChanged, this, &HorizontalWidgetGallery::updateButtonsVisibility);
    connect(m_scrollArea->horizontalScrollBar(), &QScrollBar::rangeChanged, this, &HorizontalWidgetGallery::updateButtonsVisibility);

    m_scrollArea->viewport()->installEventFilter(this);
    this->installEventFilter(this);
}

void HorizontalWidgetGallery::addWidget(QWidget* widget)
{
    if (!widget) return;
    m_contentLayout->addWidget(widget);
    scheduleContentHeightRefresh();
}

void HorizontalWidgetGallery::clear()
{
    QLayoutItem *child;
    while ((child = m_contentLayout->takeAt(0)) != nullptr) {
        if (child->widget()) {
            child->widget()->hide();
            child->widget()->deleteLater();
        }
        delete child;
    }
    m_hScrollTarget = 0;
    if (m_scrollArea && m_scrollArea->horizontalScrollBar()) {
        m_scrollArea->horizontalScrollBar()->setValue(0);
    }
    scheduleContentHeightRefresh();
}

void HorizontalWidgetGallery::setSpacing(int spacing)
{
    if (m_contentLayout->spacing() == spacing) {
        return;
    }
    m_contentLayout->setSpacing(spacing);
    scheduleContentHeightRefresh();
}

void HorizontalWidgetGallery::updateButtonPositions()
{
    int currentWidth = this->width();
    int btnY = (this->height() - m_btnLeft->height()) / 2;

    m_btnLeft->move(10, btnY);
    m_btnRight->move(currentWidth - m_btnRight->width() - 10, btnY);

    m_btnLeft->raise();
    m_btnRight->raise();
}

void HorizontalWidgetGallery::applyContentHeight()
{
    m_contentWidget->adjustSize();
    const int contentHeight = m_contentWidget->sizeHint().height();
    const int targetHeight = qMax(0, contentHeight + 16);
    if (height() != targetHeight ||
        minimumHeight() != targetHeight ||
        maximumHeight() != targetHeight) {
        setFixedHeight(targetHeight);
    }
    updateButtonPositions();
    updateGeometry();
}

void HorizontalWidgetGallery::adjustHeightToContent()
{
    scheduleContentHeightRefresh();
}

void HorizontalWidgetGallery::scheduleContentHeightRefresh()
{
    if (!m_heightRefreshTimer || m_heightRefreshTimer->isActive()) {
        return;
    }
    m_heightRefreshTimer->start();
}

void HorizontalWidgetGallery::resizeEvent(QResizeEvent* event)
{
    QScrollBar* bar = m_scrollArea->horizontalScrollBar();
    bool wasAtEnd = (bar->maximum() > 0 && bar->value() >= bar->maximum() - 5);

    QWidget::resizeEvent(event);
    updateButtonPositions();
    updateButtonsVisibility();

    if (wasAtEnd) {
        QTimer::singleShot(0, this, [bar]() {
            bar->setValue(bar->maximum());
        });
    }
}

bool HorizontalWidgetGallery::eventFilter(QObject* obj, QEvent* event)
{
    if (event->type() == QEvent::Enter || event->type() == QEvent::MouseMove) {
        updateButtonsVisibility();
    } else if (event->type() == QEvent::Leave) {
        QPoint globalPos = QCursor::pos();
        if (!this->rect().contains(this->mapFromGlobal(globalPos))) {
            if (m_btnLeft->isVisible()) {
                m_btnLeft->hide();
            }
            if (m_btnRight->isVisible()) {
                m_btnRight->hide();
            }
        }
    } else if (event->type() == QEvent::Wheel) {
        
        QWheelEvent* wheelEvent = static_cast<QWheelEvent*>(event);
        const QPoint scrollDelta =
            !wheelEvent->pixelDelta().isNull()
                ? wheelEvent->pixelDelta()
                : wheelEvent->angleDelta();
        if (qAbs(scrollDelta.x()) > qAbs(scrollDelta.y())) {
            
            QScrollBar* hBar = m_scrollArea->horizontalScrollBar();
            if (hBar) {
                int currentVal = hBar->value();
                if (m_hScrollAnim->state() == QAbstractAnimation::Running) {
                    currentVal = m_hScrollTarget;
                }
                int step = scrollDelta.x();
                int newTarget = currentVal - step;
                newTarget = qBound(hBar->minimum(), newTarget, hBar->maximum());

                if (newTarget != hBar->value()) {
                    m_hScrollTarget = newTarget;
                    m_hScrollAnim->stop();
                    m_hScrollAnim->setStartValue(hBar->value());
                    m_hScrollAnim->setEndValue(m_hScrollTarget);
                    m_hScrollAnim->start();
                }
            }
            return true; 
        } else {
            
            wheelEvent->ignore();
            return false;
        }
    }
    return QWidget::eventFilter(obj, event);
}

void HorizontalWidgetGallery::wheelEvent(QWheelEvent *event) {
    const QPoint scrollDelta =
        !event->pixelDelta().isNull() ? event->pixelDelta()
                                      : event->angleDelta();
    if (qAbs(scrollDelta.x()) <= qAbs(scrollDelta.y())) {
        event->ignore();
    }
}

void HorizontalWidgetGallery::updateButtonsVisibility()
{
    QPoint globalPos = QCursor::pos();
    QPoint localPos = this->mapFromGlobal(globalPos);

    if (!this->rect().contains(localPos)) {
        if (m_btnLeft->isVisible()) {
            m_btnLeft->hide();
        }
        if (m_btnRight->isVisible()) {
            m_btnRight->hide();
        }
        return;
    }

    int currentWidth = this->width();
    QScrollBar* bar = m_scrollArea->horizontalScrollBar();
    if (!bar) {
        if (m_btnLeft->isVisible()) {
            m_btnLeft->hide();
        }
        if (m_btnRight->isVisible()) {
            m_btnRight->hide();
        }
        return;
    }

    bool isLeftHalf = localPos.x() < (currentWidth / 2);

    const bool showLeft = isLeftHalf && bar->value() > 0;
    const bool showRight = !isLeftHalf && bar->value() < bar->maximum();
    if (m_btnLeft->isVisible() != showLeft) {
        m_btnLeft->setVisible(showLeft);
    }
    if (m_btnRight->isVisible() != showRight) {
        m_btnRight->setVisible(showRight);
    }
}
