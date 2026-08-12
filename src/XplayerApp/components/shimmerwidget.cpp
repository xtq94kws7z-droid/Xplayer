#include "shimmerwidget.h"
#include "../managers/thememanager.h"
#include "../utils/uianimationdefaults.h"
#include <QPainter>
#include <QPainterPath>
#include <QPropertyAnimation>

ShimmerWidget::ShimmerWidget(QWidget* parent)
    : QWidget(parent)
{
    setObjectName("shimmer-widget");
    
    setAttribute(Qt::WA_TransparentForMouseEvents, true);

    m_animation = new QPropertyAnimation(this, "shimmerPhase", this);
    m_animation->setStartValue(0.0);
    m_animation->setEndValue(1.0);
    m_animation->setDuration(
        XplayerUi::durationMs(XplayerUi::MotionDuration::SpinnerCycle) * 2);
    m_animation->setLoopCount(-1);
    m_animation->setEasingCurve(
        XplayerUi::easingCurve(XplayerUi::MotionCurve::Spinner));
}

ShimmerWidget::~ShimmerWidget()
{
    if (m_animation) {
        m_animation->stop();
    }
}

void ShimmerWidget::setCardSize(const QSize& size)
{
    if (size == m_cardSize) {
        return;
    }
    m_cardSize = size;
    m_shapePathDirty = true;
    update();
}

void ShimmerWidget::setCardSpacing(int spacing)
{
    if (spacing == m_cardSpacing) {
        return;
    }
    m_cardSpacing = spacing;
    m_shapePathDirty = true;
    update();
}

void ShimmerWidget::setCardRadius(int radius)
{
    if (radius == m_cardRadius) {
        return;
    }
    m_cardRadius = radius;
    m_shapePathDirty = true;
    update();
}

void ShimmerWidget::setShowSubtitle(bool show)
{
    if (show == m_showSubtitle) {
        return;
    }
    m_showSubtitle = show;
    m_shapePathDirty = true;
    update();
}

qreal ShimmerWidget::shimmerPhase() const
{
    return m_shimmerPhase;
}

void ShimmerWidget::setShimmerPhase(qreal phase)
{
    if (qFuzzyCompare(m_shimmerPhase, phase)) {
        return;
    }
    m_shimmerPhase = phase;
    update();
}

void ShimmerWidget::startAnimation()
{
    if (m_animation && m_animation->state() != QAbstractAnimation::Running) {
        m_shimmerPhase = 0.0;
        m_animation->start();
    }
}

void ShimmerWidget::stopAnimation()
{
    if (m_animation) {
        m_animation->stop();
    }
}

bool ShimmerWidget::isAnimating() const
{
    return m_animation &&
           m_animation->state() == QAbstractAnimation::Running;
}

void ShimmerWidget::rebuildShapePath()
{
    m_cachedCanvasSize = size();
    m_cachedShapePath = QPainterPath();

    const int padding = 8;
    const int cardW = m_cardSize.width();
    const int cardH = m_cardSize.height();
    if (cardW <= 0 || cardH <= 0) {
        m_shapePathDirty = false;
        return;
    }

    const int imgW = cardW - padding * 2;
    const qreal ratio =
        static_cast<qreal>(cardH) / qMax(1, cardW);
    const int imgH =
        (ratio < 1.0) ? qRound(imgW * 9.0 / 16.0) : qRound(imgW * 1.5);

    const int maxCards = 60;
    int count = 0;
    int y = 0;

    while (y < height() + cardH && count < maxCards) {
        int x = 0;
        while (x < width() + cardW && count < maxCards) {
            QRectF imgRect(x + padding, y + padding, imgW, imgH);
            m_cachedShapePath.addRoundedRect(
                imgRect, m_cardRadius, m_cardRadius);

            if (m_showSubtitle) {
                const QRectF titleRect(
                    x + padding, y + padding + imgH + 10,
                    qRound(imgW * 0.82), 12);
                const QRectF subtitleRect(
                    x + padding, titleRect.bottom() + 7,
                    qRound(imgW * 0.58), 10);
                m_cachedShapePath.addRoundedRect(titleRect, 5, 5);
                m_cachedShapePath.addRoundedRect(subtitleRect, 5, 5);
            }

            x += cardW + m_cardSpacing;
            ++count;
        }
        y += cardH + m_cardSpacing;
    }

    m_shapePathDirty = false;
}

void ShimmerWidget::paintEvent(QPaintEvent* event)
{
    Q_UNUSED(event);

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);

    const bool isDark = ThemeManager::instance()->isDarkMode();

    
    const QColor baseBg = isDark ? QColor(50, 50, 55) : QColor(229, 231, 235);

    if (m_shapePathDirty || m_cachedCanvasSize != size()) {
        rebuildShapePath();
    }

    
    painter.setPen(Qt::NoPen);
    painter.setBrush(baseBg);
    painter.drawPath(m_cachedShapePath);

    
    
    
    const int totalW = width();
    const int shimmerW = qRound(totalW * 0.4);
    const int shimmerX =
        qRound(m_shimmerPhase * (totalW + shimmerW)) - shimmerW;

    QLinearGradient gradient(shimmerX, 0, shimmerX + shimmerW, 0);
    const QColor shimmerHi =
        isDark ? QColor(255, 255, 255, 14) : QColor(255, 255, 255, 100);
    gradient.setColorAt(0.0, Qt::transparent);
    gradient.setColorAt(0.5, shimmerHi);
    gradient.setColorAt(1.0, Qt::transparent);

    painter.save();
    painter.setClipPath(m_cachedShapePath);
    painter.setBrush(gradient);
    painter.drawRect(rect());
    painter.restore();
}
