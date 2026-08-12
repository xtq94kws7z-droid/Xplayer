#include "smoothscrollcontroller.h"
#include "uianimationdefaults.h"

#include <QGuiApplication>
#include <QScreen>
#include <QScrollBar>
#include <QWindow>
#include <QWheelEvent>
#include <cmath>

namespace {
constexpr int kPixelScrollIdleWindowMs = 120;
}

SmoothScrollController::SmoothScrollController(QScrollBar *scrollBar,
                                               QObject *parent)
    : QObject(parent), m_scrollBar(scrollBar)
{
    if (m_scrollBar) {
        m_targetValue = m_scrollBar->value();
        m_position = m_targetValue;

        connect(m_scrollBar, &QObject::destroyed, this, [this]() {
            m_ticker.stop();
            setAnimating(false);
            m_scrollBar = nullptr;
        });
        connect(m_scrollBar, &QScrollBar::valueChanged, this, [this](int value) {
            if (!m_animating) {
                m_targetValue = value;
                m_position = value;
            }
        });
        connect(m_scrollBar, &QScrollBar::rangeChanged, this,
                [this](int, int) {
                    m_targetValue = boundedValue(m_targetValue);
                    if (!m_animating) {
                        m_position = m_targetValue;
                    }
                });
    }

    m_ticker.setTimerType(Qt::PreciseTimer);
    m_ticker.setInterval(XplayerUi::kFrameIntervalMs);
    m_pixelScrollIdleTimer.setSingleShot(true);
    m_pixelScrollIdleTimer.setInterval(kPixelScrollIdleWindowMs);
    connect(&m_pixelScrollIdleTimer, &QTimer::timeout, this,
            [this]() { setAnimating(false); });
    connect(&m_ticker, &QTimer::timeout, this, [this]() {
        if (!m_scrollBar) {
            m_ticker.stop();
            setAnimating(false);
            return;
        }

        const int current = m_scrollBar->value();
        const int distance = m_targetValue - current;
        if (qAbs(distance) <= 1) {
            m_position = m_targetValue;
            m_scrollBar->setValue(m_targetValue);
            m_ticker.stop();
            setAnimating(false);
            return;
        }

        qint64 elapsedMs =
            m_frameClock.isValid() ? m_frameClock.restart()
                                   : m_ticker.interval();
        if (elapsedMs <= 0) {
            elapsedMs = m_ticker.interval();
        }
        const qreal frameMs = qMin<qint64>(50, elapsedMs);
        const qreal factor = 1.0 - std::pow(0.08, frameMs / qMax(1, m_durationMs));
        m_position += (m_targetValue - m_position) * factor;
        m_position = qBound<qreal>(m_scrollBar->minimum(), m_position,
                                   m_scrollBar->maximum());

        const int nextValue = qBound(
            m_scrollBar->minimum(), qRound(m_position),
            m_scrollBar->maximum());
        if (nextValue != current) {
            m_scrollBar->setValue(nextValue);
        }
    });
}

int SmoothScrollController::frameIntervalForRefreshRate(qreal refreshRate)
{
    const qreal boundedRate =
        qBound<qreal>(30.0, refreshRate > 0.0 ? refreshRate : 60.0, 240.0);
    return qMax(1, qRound(1000.0 / boundedRate));
}

void SmoothScrollController::setDuration(int durationMs)
{
    m_durationMs = qMax(1, durationMs);
}

void SmoothScrollController::setWheelMultiplier(qreal multiplier)
{
    m_wheelMultiplier = qMax(0.1, multiplier);
}

void SmoothScrollController::stop()
{
    m_ticker.stop();
    m_pixelScrollIdleTimer.stop();
    m_pendingWheelRemainder = 0.0;
    if (m_scrollBar) {
        m_targetValue = m_scrollBar->value();
        m_position = m_targetValue;
    }
    setAnimating(false);
}

void SmoothScrollController::scrollTo(int value, bool animated)
{
    if (!m_scrollBar) {
        return;
    }

    m_targetValue = boundedValue(value);
    if (!animated) {
        m_ticker.stop();
        setAnimating(false);
        m_position = m_targetValue;
        m_scrollBar->setValue(m_targetValue);
        return;
    }

    startTicker();
}

bool SmoothScrollController::scrollByWheelEvent(const QWheelEvent *event,
                                                Qt::Orientation orientation)
{
    if (!m_scrollBar || !event) {
        return false;
    }

    const QPoint pixelDelta = event->pixelDelta();
    const int delta = wheelDelta(event, orientation);
    if (delta == 0) {
        return false;
    }

    if (!pixelDelta.isNull()) {
        m_ticker.stop();
        m_pendingWheelRemainder = 0.0;
        const int immediateValue = boundedValue(
            m_scrollBar->value() - qRound(delta * m_wheelMultiplier));
        m_targetValue = immediateValue;
        m_position = immediateValue;
        m_scrollBar->setValue(immediateValue);
        m_pixelScrollIdleTimer.start();
        setAnimating(true);
        return true;
    }

    const qreal requestedDistance =
        -delta * m_wheelMultiplier + m_pendingWheelRemainder;
    const int requestedStep = qRound(requestedDistance);
    m_pendingWheelRemainder = requestedDistance - requestedStep;
    if (requestedStep == 0) {
        return true;
    }

    const int currentValue = m_scrollBar->value();
    const int pendingDistance = m_targetValue - currentValue;
    const bool isReversing =
        m_ticker.isActive() && pendingDistance != 0 &&
        ((pendingDistance > 0) != (requestedStep > 0));
    const int baseValue =
        isReversing ? currentValue
                    : (m_ticker.isActive() ? m_targetValue : currentValue);
    const int nextTarget = boundedValue(baseValue + requestedStep);
    if (nextTarget == currentValue && !m_ticker.isActive()) {
        m_pendingWheelRemainder = 0.0;
        return false;
    }

    if (nextTarget != m_targetValue || !m_ticker.isActive()) {
        m_targetValue = nextTarget;
        startTicker();
    }
    return true;
}

int SmoothScrollController::targetValue() const
{
    return m_targetValue;
}

bool SmoothScrollController::isAnimating() const
{
    return m_animating;
}

int SmoothScrollController::boundedValue(int value) const
{
    if (!m_scrollBar) {
        return value;
    }
    return qBound(m_scrollBar->minimum(), value, m_scrollBar->maximum());
}

int SmoothScrollController::adaptiveFrameInterval() const
{
    QScreen* screen = nullptr;
    if (m_scrollBar) {
        if (QWidget* widget = m_scrollBar->window()) {
            if (QWindow* windowHandle = widget->windowHandle()) {
                screen = windowHandle->screen();
            }
        }
    }
    if (!screen) {
        screen = QGuiApplication::primaryScreen();
    }
    return frameIntervalForRefreshRate(screen ? screen->refreshRate() : 60.0);
}

int SmoothScrollController::wheelDelta(const QWheelEvent *event,
                                       Qt::Orientation orientation) const
{
    const QPoint pixelDelta = event->pixelDelta();
    if (!pixelDelta.isNull()) {
        return orientation == Qt::Horizontal ? pixelDelta.x() : pixelDelta.y();
    }

    const QPoint angleDelta = event->angleDelta();
    return orientation == Qt::Horizontal ? angleDelta.x() : angleDelta.y();
}

void SmoothScrollController::startTicker()
{
    if (!m_scrollBar || m_scrollBar->value() == m_targetValue) {
        m_ticker.stop();
        if (m_scrollBar) {
            m_position = m_targetValue;
        }
        setAnimating(false);
        return;
    }

    if (!m_ticker.isActive()) {
        m_ticker.setInterval(adaptiveFrameInterval());
        m_position = m_scrollBar->value();
        m_frameClock.restart();
        m_ticker.start();
        setAnimating(true);
    }
}

void SmoothScrollController::setAnimating(bool active)
{
    if (m_animating == active) {
        return;
    }

    m_animating = active;
    Q_EMIT scrollingChanged(active);
}
