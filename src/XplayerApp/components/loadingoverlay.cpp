#include "loadingoverlay.h"
#include "../utils/uianimationdefaults.h"
#include <QPainter>
#include <QPainterPath>
#include <QPropertyAnimation>
#include <QEasingCurve>
#include <QConicalGradient>
#include <QFontMetrics>
#include <QLinearGradient>
#include <QMouseEvent>

namespace
{
bool animationIsHeadingTo(const QPropertyAnimation *animation, qreal target)
{
    return animation && animation->state() == QAbstractAnimation::Running &&
           qFuzzyCompare(animation->endValue().toReal() + 1.0, target + 1.0);
}
} // namespace

LoadingOverlay::LoadingOverlay(QWidget* parent) : QWidget(parent) {
    setAttribute(Qt::WA_TranslucentBackground);
    hide();

    m_rotateAnim = new QPropertyAnimation(this, "angle", this);
    m_rotateAnim->setStartValue(0.0);
    m_rotateAnim->setEndValue(360.0);
    m_rotateAnim->setDuration(
        XplayerUi::durationMs(XplayerUi::MotionDuration::SpinnerCycle));
    m_rotateAnim->setLoopCount(-1);
    m_rotateAnim->setEasingCurve(
        XplayerUi::easingCurve(XplayerUi::MotionCurve::Spinner));

    m_fadeAnim = new QPropertyAnimation(this, "opacity", this);
    m_fadeAnim->setDuration(
        XplayerUi::durationMs(XplayerUi::MotionDuration::Quick));
    m_fadeAnim->setEasingCurve(
        XplayerUi::easingCurve(XplayerUi::MotionCurve::Enter));
}

void LoadingOverlay::start() {
    if (isVisible() && m_rotateAnim->state() == QAbstractAnimation::Running &&
        (m_opacity >= 0.98 || animationIsHeadingTo(m_fadeAnim, 1.0))) {
        return;
    }

    show();
    raise();
    if (m_rotateAnim->state() != QAbstractAnimation::Running) {
        m_rotateAnim->start();
    }

    m_fadeAnim->disconnect();
    m_fadeAnim->setStartValue(m_opacity);
    m_fadeAnim->setEndValue(1.0);
    m_fadeAnim->start();
}

void LoadingOverlay::stop() {
    if (!isVisible() && m_opacity <= 0.0) {
        m_rotateAnim->stop();
        return;
    }

    if (animationIsHeadingTo(m_fadeAnim, 0.0)) {
        return;
    }

    m_fadeAnim->disconnect();
    connect(m_fadeAnim, &QAbstractAnimation::finished, this, [this]() {
        hide();
        m_rotateAnim->stop();
    });
    m_fadeAnim->setStartValue(m_opacity);
    m_fadeAnim->setEndValue(0.0);
    m_fadeAnim->start();
}

void LoadingOverlay::forceStop() {
    m_fadeAnim->stop();
    m_rotateAnim->stop();
    m_opacity = 0.0;
    hide();
    update();
}

void LoadingOverlay::setMessage(const QString &message) {
    if (m_message == message) {
        return;
    }

    m_message = message;
    update();
}

qreal LoadingOverlay::opacity() const { return m_opacity; }
void LoadingOverlay::setOpacity(qreal op) {
    if (qFuzzyCompare(m_opacity + 1.0, op + 1.0)) {
        return;
    }
    m_opacity = op;
    update();
}

qreal LoadingOverlay::angle() const { return m_angle; }
void LoadingOverlay::setAngle(qreal a) {
    if (qFuzzyCompare(m_angle + 1.0, a + 1.0)) {
        return;
    }
    const QRect previousRect = spinnerUpdateRect();
    m_angle = a;
    update(previousRect.united(spinnerUpdateRect()));
}

bool LoadingOverlay::isImmersive() const { return m_isImmersive; }
void LoadingOverlay::setImmersive(bool immersive) {
    if (m_isImmersive == immersive) {
        return;
    }
    m_isImmersive = immersive;
    update();
}

bool LoadingOverlay::isHudPanelVisible() const { return m_hudPanelVisible; }
void LoadingOverlay::setHudPanelVisible(bool visible) {
    if (m_hudPanelVisible == visible) {
        return;
    }

    m_hudPanelVisible = visible;
    update();
}

bool LoadingOverlay::isSubtleOverlay() const { return m_subtleOverlay; }
void LoadingOverlay::setSubtleOverlay(bool subtle) {
    if (m_subtleOverlay == subtle) {
        return;
    }

    m_subtleOverlay = subtle;
    update();
}

QRect LoadingOverlay::spinnerUpdateRect() const
{
    const int halfSize = m_isImmersive ? 46 : (m_hudPanelVisible ? 58 : 42);
    return QRect(width() / 2 - halfSize, height() / 2 - halfSize,
                 halfSize * 2, halfSize * 2)
        .adjusted(-4, -4, 4, 4);
}

void LoadingOverlay::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.setOpacity(m_opacity);

    
    
    if (m_isImmersive) {
        p.fillRect(rect(), QColor(0, 0, 0, 60));
    } else {
        QColor overlayColor = m_overlayBgColor;
        if (m_subtleOverlay) {
            overlayColor.setAlpha(qMax(8, qRound(overlayColor.alpha() * 0.35)));
        }
        p.fillRect(rect(), overlayColor);
    }

    p.translate(width() / 2.0, height() / 2.0);

    qreal radius = 24.0;
    qreal thickness = 3.5;

    
    if (!m_isImmersive && m_hudPanelVisible) {
        int hudSize = 90;
        QRectF hudRect(-hudSize / 2.0, -hudSize / 2.0, hudSize, hudSize);

        QPainterPath shadowPath;
        shadowPath.addRoundedRect(hudRect.translated(0, 8), 16, 16);
        p.fillPath(shadowPath, m_shadowColor);

        QPainterPath hudPath;
        hudPath.addRoundedRect(hudRect, 16, 16);

        QLinearGradient hudGrad(hudRect.topLeft(), hudRect.bottomRight());
        hudGrad.setColorAt(0.0, m_hudColorTop);
        hudGrad.setColorAt(0.5, m_hudColorMid);
        hudGrad.setColorAt(1.0, m_hudColorBottom);
        p.fillPath(hudPath, hudGrad);

        QLinearGradient borderGrad(hudRect.topLeft(), hudRect.bottomLeft());
        borderGrad.setColorAt(0.0, m_borderColor);
        QColor transparentBorder = m_borderColor;
        transparentBorder.setAlpha(qMax(0, m_borderColor.alpha() - 50));
        borderGrad.setColorAt(1.0, transparentBorder);

        QPen borderPen(QBrush(borderGrad), 1.0);
        p.setPen(borderPen);
        p.drawPath(hudPath);
    } else if (m_isImmersive) {
        
        radius = 36.0;
        thickness = 4.0;
    } else {
        
        radius = 30.0;
        thickness = 3.8;
    }

    QRectF ringRect(-radius, -radius, radius * 2, radius * 2);

    
    QColor currentTrackColor = m_isImmersive ? QColor(255, 255, 255, 30) : m_trackColor;
    QColor currentSweepColor = m_isImmersive ? QColor(255, 255, 255, 255) : m_sweepColor;

    QPen trackPen(currentTrackColor);
    trackPen.setWidthF(thickness);
    p.setPen(trackPen);
    p.drawEllipse(ringRect);

    
    QConicalGradient grad(0, 0, -m_angle);
    QColor sweepFull = currentSweepColor;
    QColor sweepMed = currentSweepColor;
    sweepMed.setAlpha(static_cast<int>(currentSweepColor.alpha() * 0.7));
    QColor sweepZero = currentSweepColor;
    sweepZero.setAlpha(0);

    grad.setColorAt(0.0, sweepFull);
    grad.setColorAt(0.1, sweepMed);
    grad.setColorAt(0.75, sweepZero);
    grad.setColorAt(1.0, sweepZero);

    QPen sweepPen(QBrush(grad), thickness);
    sweepPen.setCapStyle(Qt::RoundCap);
    p.setPen(sweepPen);
    p.drawArc(ringRect, qRound(-m_angle * 16.0), 280 * 16);

    if (!m_message.trimmed().isEmpty()) {
        QFont messageFont = font();
        messageFont.setPixelSize(m_isImmersive ? 15 : 13);
        messageFont.setWeight(QFont::Medium);
        p.setFont(messageFont);

        const QFontMetrics metrics(messageFont);
        const QString text = metrics.elidedText(m_message, Qt::ElideRight, 320);
        QRectF textRect(-180.0, radius + 18.0, 360.0, 30.0);

        QColor textColor(255, 255, 255, m_isImmersive ? 230 : 215);
        p.setPen(textColor);
        p.drawText(textRect, Qt::AlignCenter, text);
    }
}


void LoadingOverlay::mousePressEvent(QMouseEvent* event) {
    if (!m_isImmersive) event->accept();
}
void LoadingOverlay::mouseReleaseEvent(QMouseEvent* event) {
    if (!m_isImmersive) event->accept();
}
void LoadingOverlay::mouseDoubleClickEvent(QMouseEvent* event) {
    if (!m_isImmersive) event->accept();
}
