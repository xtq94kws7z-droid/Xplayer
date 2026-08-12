#include "modernswitch.h"
#include "../utils/uianimationdefaults.h"
#include <QPainterPath>
#include <QtGlobal>

ModernSwitch::ModernSwitch(QWidget* parent)
    : QAbstractButton(parent),
    m_offset(0.0),
    m_trackColorActive(QColor("#00A4FF")),
    m_trackColorInactive(QColor("#555555")),
    m_thumbColor(QColor("#FFFFFF")),
    m_isHovered(false)
{
    setCheckable(true);
    setCursor(Qt::PointingHandCursor);

    
    m_animation = new QPropertyAnimation(this, "offset", this);
    m_animation->setDuration(
        XplayerUi::durationMs(XplayerUi::MotionDuration::Quick));
    m_animation->setEasingCurve(
        XplayerUi::easingCurve(XplayerUi::MotionCurve::Resize));

    connect(this, &ModernSwitch::toggled, this, &ModernSwitch::updateAnimation);
}

ModernSwitch::~ModernSwitch() = default;

qreal ModernSwitch::offset() const { return m_offset; }
void ModernSwitch::setOffset(qreal offset) {
    if (qAbs(m_offset - offset) < 0.001) {
        return;
    }
    m_offset = offset;
    update(); 
}

QColor ModernSwitch::trackColorActive() const { return m_trackColorActive; }
void ModernSwitch::setTrackColorActive(const QColor& color) {
    if (m_trackColorActive == color) {
        return;
    }
    m_trackColorActive = color;
    update();
}

QColor ModernSwitch::trackColorInactive() const { return m_trackColorInactive; }
void ModernSwitch::setTrackColorInactive(const QColor& color) {
    if (m_trackColorInactive == color) {
        return;
    }
    m_trackColorInactive = color;
    update();
}

QColor ModernSwitch::thumbColor() const { return m_thumbColor; }
void ModernSwitch::setThumbColor(const QColor& color) {
    if (m_thumbColor == color) {
        return;
    }
    m_thumbColor = color;
    update();
}

QSize ModernSwitch::sizeHint() const {
    return QSize(44, 24); 
}

void ModernSwitch::paintEvent(QPaintEvent* ) {
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setPen(Qt::NoPen);

    
    const QRectF trackRect(0.5, 0.5, width() - 1.0, height() - 1.0);
    const qreal radius = trackRect.height() / 2.0;

    
    QColor trackColor = isChecked() ? m_trackColorActive : m_trackColorInactive;

    
    if (m_isHovered) {
        trackColor = trackColor.lighter(110);
    }

    painter.setBrush(trackColor);
    painter.drawRoundedRect(trackRect, radius, radius);

    
    constexpr qreal thumbMargin = 3.0;
    const qreal thumbDiameter = trackRect.height() - thumbMargin * 2.0;
    const qreal thumbTravel = trackRect.width() - thumbDiameter - thumbMargin * 2.0;
    const qreal thumbX = trackRect.left() + thumbMargin + m_offset * thumbTravel;
    const qreal thumbY = trackRect.top() + thumbMargin;

    painter.setBrush(m_thumbColor);
    painter.drawEllipse(QRectF(thumbX, thumbY, thumbDiameter, thumbDiameter));
}

void ModernSwitch::nextCheckState() {
    QAbstractButton::nextCheckState();
}

void ModernSwitch::checkStateSet() {
    QAbstractButton::checkStateSet();

    
    
    if (signalsBlocked()) {
        m_animation->stop();
        setOffset(isChecked() ? 1.0 : 0.0);
    }
}

void ModernSwitch::enterEvent(QEnterEvent* event) {
    if (m_isHovered) {
        QAbstractButton::enterEvent(event);
        return;
    }
    m_isHovered = true;
    update();
    QAbstractButton::enterEvent(event);
}

void ModernSwitch::leaveEvent(QEvent* event) {
    if (!m_isHovered) {
        QAbstractButton::leaveEvent(event);
        return;
    }
    m_isHovered = false;
    update();
    QAbstractButton::leaveEvent(event);
}

void ModernSwitch::updateAnimation(bool checked) {
    const qreal targetOffset = checked ? 1.0 : 0.0;
    if (m_animation->state() != QAbstractAnimation::Running &&
        qAbs(m_offset - targetOffset) < 0.001) {
        return;
    }
    m_animation->stop();
    m_animation->setStartValue(m_offset);
    m_animation->setEndValue(targetOffset);
    m_animation->start();
}
