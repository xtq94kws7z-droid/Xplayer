#include "posterstagewidget.h"

#include "../utils/posterwallutils.h"
#include "../utils/posterstageresponsiveutils.h"
#include "../utils/uianimationdefaults.h"

#include <QAbstractItemModel>
#include <QEnterEvent>
#include <QGraphicsOpacityEffect>
#include <QHideEvent>
#include <QHBoxLayout>
#include <QImage>
#include <QLayout>
#include <QLabel>
#include <QMouseEvent>
#include <QPainter>
#include <QPainterPath>
#include <QPen>
#include <QPushButton>
#include <QPropertyAnimation>
#include <QShowEvent>
#include <QTimer>
#include <QVariantAnimation>
#include <QVBoxLayout>
#include <QVector>

#include <cmath>
#include <utility>

namespace {

constexpr int kItemDataRole = Qt::UserRole + 1;
constexpr int kPosterPixmapRole = Qt::UserRole + 2;
constexpr int kRotationIntervalMs = 8000;
constexpr int kResizeBackdropRefreshDelayMs = 90;
struct PosterDrawSlot {
    int itemIndex = -1;
    QRectF rect;
    qreal opacity = 1.0;
    qreal position = 0.0;
    bool focused = false;
};

QRect centeredAspectRect(const QRect& bounds, const QSize& sourceSize)
{
    if (sourceSize.isEmpty()) {
        return bounds;
    }

    const QSize fitted = sourceSize.scaled(bounds.size(),
                                           Qt::KeepAspectRatioByExpanding);
    return QRect(bounds.center() - QPoint(fitted.width() / 2,
                                          fitted.height() / 2),
                 fitted);
}

QPixmap buildBackdropPixmap(const QPixmap& pixmap, const QSize& targetSize)
{
    if (pixmap.isNull() || !targetSize.isValid()) {
        return {};
    }

    const QSize coverSize =
        pixmap.size().scaled(targetSize, Qt::KeepAspectRatioByExpanding);
    const QImage cover =
        pixmap.toImage()
            .scaled(coverSize, Qt::IgnoreAspectRatio, Qt::SmoothTransformation)
            .convertToFormat(QImage::Format_ARGB32_Premultiplied);

    const QRect sourceRect((cover.width() - targetSize.width()) / 2,
                           (cover.height() - targetSize.height()) / 2,
                           targetSize.width(), targetSize.height());
    QImage cropped = cover.copy(sourceRect);
    if (cropped.isNull()) {
        return {};
    }

    const QSize blurSize(qMax(48, targetSize.width() / 10),
                         qMax(48, targetSize.height() / 10));
    QImage blurred =
        cropped.scaled(blurSize, Qt::IgnoreAspectRatio, Qt::SmoothTransformation)
            .scaled(targetSize, Qt::IgnoreAspectRatio,
                    Qt::SmoothTransformation);
    return QPixmap::fromImage(blurred);
}

QRectF mixRect(const QRectF& from, const QRectF& to, qreal amount)
{
    return QRectF(from.x() + (to.x() - from.x()) * amount,
                  from.y() + (to.y() - from.y()) * amount,
                  from.width() + (to.width() - from.width()) * amount,
                  from.height() + (to.height() - from.height()) * amount);
}

QRectF slotRectForPosition(const QRect& stageBounds, qreal position)
{
    const qreal stageScale = qBound<qreal>(
        0.78, static_cast<qreal>(stageBounds.width()) / 1872.0, 1.08);
    const int panelWidth =
        PosterStageResponsiveUtils::infoPanelWidth(stageBounds.width());
    const int wallLeft =
        PosterStageResponsiveUtils::posterWallLeft(stageBounds.width(),
                                                   panelWidth);
    const int wallRightMargin = qRound(34 * stageScale);
    const int wallWidth = qMax(260, stageBounds.width() - wallLeft -
                                        wallRightMargin);
    const qreal centerHeight =
        qMax<qreal>(218 * stageScale, stageBounds.height() - 54 * stageScale);
    const qreal centerWidth =
        qBound<qreal>(136 * stageScale, centerHeight * 0.66,
                      wallWidth * 0.36);
    const qreal sideWidth = centerWidth * 0.78;
    const qreal sideHeight = centerHeight * 0.78;
    const qreal centerBias =
        stageBounds.width() >= 1700 ? 0.43 : 0.47;
    const qreal centerX = wallLeft + wallWidth * centerBias;
    const qreal centerY = stageBounds.height() * 0.50;
    const qreal sideOffset =
        qMin<qreal>(wallWidth * 0.28, centerWidth * 0.86);

    const QRectF center(centerX - centerWidth / 2,
                        centerY - centerHeight / 2,
                        centerWidth, centerHeight);
    const QRectF side(position < 0 ? centerX - sideOffset - sideWidth / 2
                                   : centerX + sideOffset - sideWidth / 2,
                      centerY - sideHeight / 2 + 10 * stageScale,
                      sideWidth, sideHeight);

    if (qAbs(position) <= 1.0) {
        return mixRect(center, side, qAbs(position));
    }

    const qreal offWidth = sideWidth * 0.84;
    const qreal offHeight = sideHeight * 0.84;
    const qreal offOffset = sideOffset + sideWidth * 0.92;
    const QRectF off(position < 0 ? centerX - offOffset - offWidth / 2
                                  : centerX + offOffset - offWidth / 2,
                     centerY - offHeight / 2 + 18 * stageScale,
                     offWidth, offHeight);
    return mixRect(side, off, qBound<qreal>(0.0, qAbs(position) - 1.0, 1.0));
}

qreal opacityForPosition(qreal position)
{
    const qreal distance = qAbs(position);
    if (distance <= 1.0) {
        return 1.0 - distance * 0.28;
    }
    if (distance <= 2.0) {
        return 0.72 - (distance - 1.0) * 0.42;
    }
    return 0.0;
}

qreal normalizedVisualCenter(qreal center, int itemCount)
{
    if (itemCount <= 0) {
        return 0.0;
    }

    qreal normalized = std::fmod(center, static_cast<qreal>(itemCount));
    if (normalized < 0.0) {
        normalized += itemCount;
    }
    return normalized;
}

qreal signedCircularDistance(int itemIndex, qreal visualCenter, int itemCount)
{
    if (itemCount <= 0) {
        return 0.0;
    }

    qreal distance =
        static_cast<qreal>(itemIndex) -
        normalizedVisualCenter(visualCenter, itemCount);
    const qreal half = static_cast<qreal>(itemCount) / 2.0;
    while (distance > half) {
        distance -= itemCount;
    }
    while (distance < -half) {
        distance += itemCount;
    }
    return distance;
}

qreal nearestVisualCoordinateForIndex(int itemIndex, qreal visualCenter,
                                      int itemCount)
{
    return visualCenter +
           signedCircularDistance(itemIndex, visualCenter, itemCount);
}

QPixmap blendedBackdropPixmap(const QPixmap& from, const QPixmap& to,
                              qreal blend, const QSize& size)
{
    if (from.isNull()) {
        return to;
    }
    if (to.isNull()) {
        return from;
    }
    if (!size.isValid()) {
        return to;
    }

    QPixmap blended(size);
    blended.fill(Qt::transparent);
    QPainter painter(&blended);
    painter.drawPixmap(QRect(QPoint(0, 0), size), from);
    painter.setOpacity(qBound<qreal>(0.0, blend, 1.0));
    painter.drawPixmap(QRect(QPoint(0, 0), size), to);
    painter.end();
    return blended;
}

void drawPoster(QPainter& painter, const QRectF& bounds, const QPixmap& pixmap,
                int radius, const QColor& borderColor, qreal opacity,
                bool focused)
{
    painter.save();
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.setRenderHint(QPainter::SmoothPixmapTransform, true);
    painter.setOpacity(opacity);

    const QRectF shadowBounds = bounds.adjusted(-12, 8, 12, 18);
    QRadialGradient shadow(shadowBounds.center(),
                           qMax(shadowBounds.width(), shadowBounds.height()) *
                               0.56);
    shadow.setColorAt(0.0,
                      focused ? QColor(0, 0, 0, 138)
                              : QColor(0, 0, 0, 78));
    shadow.setColorAt(1.0, QColor(0, 0, 0, 0));
    painter.setPen(Qt::NoPen);
    painter.setBrush(shadow);
    painter.drawEllipse(shadowBounds);

    QPainterPath path;
    path.addRoundedRect(bounds, radius, radius);
    painter.setClipPath(path);
    painter.fillPath(path, QColor(255, 255, 255, 28));

    if (!pixmap.isNull()) {
        const QRect sourceRect =
            centeredAspectRect(bounds.toRect(), pixmap.size());
        painter.drawPixmap(sourceRect, pixmap);
    }

    QLinearGradient wash(bounds.topLeft(), bounds.bottomLeft());
    wash.setColorAt(0.0, QColor(255, 255, 255, 28));
    wash.setColorAt(0.62, QColor(0, 0, 0, 0));
    wash.setColorAt(1.0, QColor(0, 0, 0, 126));
    painter.fillPath(path, wash);
    painter.setClipping(false);
    painter.setPen(QPen(borderColor, 1.0));
    painter.setBrush(Qt::NoBrush);
    painter.drawRoundedRect(bounds.adjusted(0, 0, -1, -1), radius, radius);
    if (focused) {
        painter.setPen(QPen(QColor(255, 255, 255, 90), 1.2));
        painter.drawRoundedRect(bounds.adjusted(3, 3, -4, -4),
                                radius - 3, radius - 3);
    }
    painter.restore();
}

void drawPosterPlaceholder(QPainter& painter, const QRect& bounds, int radius,
                           const QColor& tint)
{
    painter.save();
    painter.setRenderHint(QPainter::Antialiasing, true);

    QPainterPath path;
    path.addRoundedRect(bounds, radius, radius);
    painter.setClipPath(path);

    QLinearGradient base(bounds.topLeft(), bounds.bottomRight());
    base.setColorAt(0.0, QColor(tint.red(), tint.green(), tint.blue(), 120));
    base.setColorAt(0.42, QColor(18, 20, 30, 168));
    base.setColorAt(1.0, QColor(5, 8, 14, 220));
    painter.fillPath(path, base);

    QLinearGradient shine(bounds.topLeft(), bounds.bottomLeft());
    shine.setColorAt(0.0, QColor(255, 255, 255, 54));
    shine.setColorAt(0.24, QColor(255, 255, 255, 10));
    shine.setColorAt(1.0, QColor(255, 255, 255, 0));
    painter.fillPath(path, shine);

    painter.setClipping(false);
    painter.setPen(QPen(QColor(255, 255, 255, 90), 1.0));
    painter.setBrush(Qt::NoBrush);
    painter.drawRoundedRect(bounds.adjusted(0, 0, -1, -1), radius, radius);
    painter.restore();
}

} // namespace

PosterStageWidget::PosterStageWidget(QWidget* parent)
    : QWidget(parent)
{
    setObjectName(QStringLiteral("poster-stage"));
    setAttribute(Qt::WA_StyledBackground, true);
    setMouseTracking(true);
    setMinimumHeight(300);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

    auto* overlay = new QWidget(this);
    overlay->setObjectName(QStringLiteral("poster-stage-overlay"));

    auto* overlayLayout = new QHBoxLayout(overlay);
    overlayLayout->setContentsMargins(34, 28, 34, 28);
    overlayLayout->setSpacing(0);

    auto* infoPanel = new QWidget(overlay);
    infoPanel->setObjectName(QStringLiteral("poster-stage-info"));
    infoPanel->setFixedHeight(236);
    m_infoPanel = infoPanel;

    auto* infoLayout = new QVBoxLayout(infoPanel);
    infoLayout->setContentsMargins(24, 22, 24, 20);
    infoLayout->setSpacing(7);

    m_eyebrowLabel = new QLabel(tr("精选推荐"), infoPanel);
    m_eyebrowLabel->setObjectName(QStringLiteral("poster-stage-eyebrow"));

    m_titleLabel = new QLabel(infoPanel);
    m_titleLabel->setObjectName(QStringLiteral("poster-stage-title"));
    m_titleLabel->setWordWrap(true);

    m_metaLabel = new QLabel(infoPanel);
    m_metaLabel->setObjectName(QStringLiteral("poster-stage-meta"));

    m_overviewLabel = new QLabel(infoPanel);
    m_overviewLabel->setObjectName(QStringLiteral("poster-stage-overview"));
    m_overviewLabel->setWordWrap(true);
    m_overviewLabel->setMaximumWidth(302);
    m_overviewLabel->setFixedHeight(54);

    auto* bottomRow = new QHBoxLayout();
    bottomRow->setContentsMargins(0, 8, 0, 0);
    bottomRow->setSpacing(8);

    m_playButton = new QPushButton(tr("立即播放"), infoPanel);
    m_playButton->setObjectName(QStringLiteral("poster-stage-play"));
    m_playButton->setCursor(Qt::PointingHandCursor);
    connect(m_playButton, &QPushButton::clicked, this, [this]() {
        if (m_currentIndex >= 0 && m_currentIndex < m_items.size()) {
            Q_EMIT itemActivated(m_items.at(m_currentIndex));
        }
    });

    m_previousButton = new QPushButton(QStringLiteral("‹"), infoPanel);
    m_previousButton->setObjectName(QStringLiteral("poster-stage-previous"));
    m_previousButton->setCursor(Qt::PointingHandCursor);
    m_previousButton->setToolTip(tr("上一部精选"));
    connect(m_previousButton, &QPushButton::clicked, this,
            [this]() { advance(-1); });

    m_nextButton = new QPushButton(QStringLiteral("›"), infoPanel);
    m_nextButton->setObjectName(QStringLiteral("poster-stage-next"));
    m_nextButton->setCursor(Qt::PointingHandCursor);
    m_nextButton->setToolTip(tr("下一部精选"));
    connect(m_nextButton, &QPushButton::clicked, this,
            [this]() { advance(1); });

    m_dotsWidget = new QWidget(infoPanel);
    m_dotsWidget->setObjectName(QStringLiteral("poster-stage-dots"));
    auto* dotsLayout = new QHBoxLayout(m_dotsWidget);
    dotsLayout->setContentsMargins(4, 0, 4, 0);
    dotsLayout->setSpacing(4);

    bottomRow->addWidget(m_playButton);
    bottomRow->addStretch();
    bottomRow->addWidget(m_dotsWidget);
    bottomRow->addWidget(m_previousButton);
    bottomRow->addWidget(m_nextButton);

    infoLayout->addWidget(m_eyebrowLabel);
    infoLayout->addWidget(m_titleLabel);
    infoLayout->addWidget(m_metaLabel);
    infoLayout->addWidget(m_overviewLabel);
    infoLayout->addStretch();
    infoLayout->addLayout(bottomRow);

    overlayLayout->addWidget(infoPanel, 0, Qt::AlignLeft | Qt::AlignTop);
    overlayLayout->addStretch();

    m_overlayOpacity = new QGraphicsOpacityEffect(overlay);
    m_overlayOpacity->setOpacity(1.0);
    overlay->setGraphicsEffect(m_overlayOpacity);

    m_overlayAnimation = new QPropertyAnimation(m_overlayOpacity, "opacity",
                                                this);
    m_overlayAnimation->setDuration(
        XplayerUi::durationMs(XplayerUi::MotionDuration::Hero));
    m_overlayAnimation->setEasingCurve(
        XplayerUi::easingCurve(XplayerUi::MotionCurve::Resize));

    m_backdropAnimation = new QVariantAnimation(this);
    m_backdropAnimation->setDuration(
        XplayerUi::durationMs(XplayerUi::MotionDuration::Hero));
    m_backdropAnimation->setEasingCurve(
        XplayerUi::easingCurve(XplayerUi::MotionCurve::Resize));
    connect(m_backdropAnimation, &QVariantAnimation::valueChanged, this,
            [this](const QVariant& value) {
                m_backdropBlend = value.toReal();
                update();
            });
    connect(m_backdropAnimation, &QVariantAnimation::finished, this, [this]() {
        m_backdropBlend = 1.0;
        m_previousBackdropPixmap = {};
        update();
    });

    m_slideAnimation = new QVariantAnimation(this);
    m_slideAnimation->setDuration(
        XplayerUi::durationMs(XplayerUi::MotionDuration::Hero));
    m_slideAnimation->setEasingCurve(
        XplayerUi::easingCurve(XplayerUi::MotionCurve::Move));
    connect(m_slideAnimation, &QVariantAnimation::valueChanged, this,
            [this](const QVariant& value) {
                const qreal valueCenter = value.toReal();
                m_visualCenter = valueCenter;
                update();
            });
    connect(m_slideAnimation, &QVariantAnimation::finished, this, [this]() {
        m_slideDirection = 0;
        m_visualCenter = normalizedVisualCenter(m_visualCenter,
                                                m_items.size());
        update();
    });

    m_rotationTimer = new QTimer(this);
    m_rotationTimer->setInterval(kRotationIntervalMs);
    m_rotationTimer->setTimerType(Qt::PreciseTimer);
    connect(m_rotationTimer, &QTimer::timeout, this,
            [this]() { advance(1); });

    m_backdropRefreshTimer = new QTimer(this);
    m_backdropRefreshTimer->setSingleShot(true);
    m_backdropRefreshTimer->setTimerType(Qt::CoarseTimer);
    m_backdropRefreshTimer->setInterval(kResizeBackdropRefreshDelayMs);
    connect(m_backdropRefreshTimer, &QTimer::timeout, this, [this]() {
        updateBackdrop(false);
        update();
    });

    updateOverlayGeometry();
    updateControls();
    updateBackdrop();
}

void PosterStageWidget::setModel(QAbstractItemModel* model)
{
    if (m_model == model) {
        return;
    }

    if (m_modelDataChangedConnection) {
        disconnect(m_modelDataChangedConnection);
    }
    if (m_modelResetConnection) {
        disconnect(m_modelResetConnection);
    }

    m_model = model;
    m_modelIndexCache.clear();
    m_posterPixmapCache.clear();
    updateModelConnection();
    refreshCurrentItem();
}

void PosterStageWidget::setItems(const QList<MediaItem>& items)
{
    if (PosterWallUtils::sameStageItems(m_items, items)) {
        return;
    }

    const QString currentId =
        m_currentIndex >= 0 && m_currentIndex < m_items.size()
            ? m_items.at(m_currentIndex).id
            : QString();

    m_items = items;
    m_posterPixmapCache.clear();
    m_modelIndexCache.clear();
    if (m_items.isEmpty()) {
        m_currentIndex = -1;
        m_rotationTimer->stop();
    } else if (!currentId.isEmpty()) {
        m_currentIndex = 0;
        for (int i = 0; i < m_items.size(); ++i) {
            if (m_items.at(i).id == currentId) {
                m_currentIndex = i;
                break;
            }
        }
    } else {
        m_currentIndex = 0;
    }
    m_visualCenter = m_currentIndex >= 0 ? m_currentIndex : 0.0;

    refreshCurrentItem();
    updateControls();
}

QList<MediaItem> PosterStageWidget::items() const
{
    return m_items;
}

bool PosterStageWidget::hasContent() const
{
    return m_currentIndex >= 0 && m_currentIndex < m_items.size();
}

void PosterStageWidget::setReducedMotion(bool enabled)
{
    m_reducedMotion = enabled;
}

void PosterStageWidget::enterEvent(QEnterEvent* event)
{
    m_hovering = true;
    m_rotationTimer->stop();
    QWidget::enterEvent(event);
}

void PosterStageWidget::leaveEvent(QEvent* event)
{
    m_hovering = false;
    updateRotationTimer();
    QWidget::leaveEvent(event);
}

void PosterStageWidget::mousePressEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton && hasContent()) {
        const QPoint pos = event->position().toPoint();
        for (auto it = m_posterHitRects.constBegin();
             it != m_posterHitRects.constEnd(); ++it) {
            if (!it.value().contains(pos)) {
                continue;
            }
            if (it.key() == m_currentIndex) {
                Q_EMIT itemActivated(m_items.at(m_currentIndex));
            } else {
                setCurrentIndex(it.key(), !m_reducedMotion);
            }
            event->accept();
            return;
        }

        Q_EMIT itemActivated(m_items.at(m_currentIndex));
        event->accept();
        return;
    }
    QWidget::mousePressEvent(event);
}

void PosterStageWidget::paintEvent(QPaintEvent* event)
{
    Q_UNUSED(event);

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.setRenderHint(QPainter::SmoothPixmapTransform, true);

    const qreal stageScale = qBound<qreal>(
        0.78, static_cast<qreal>(width()) / 1872.0, 1.08);
    const qreal stageRadius = qBound<qreal>(20.0, 28.0 * stageScale, 42.0);
    QPainterPath stagePath;
    stagePath.addRoundedRect(rect().adjusted(0, 0, -1, -1), stageRadius,
                             stageRadius);
    painter.setClipPath(stagePath);

    if (!hasContent()) {
        painter.fillPath(stagePath, QColor(10, 14, 22, 120));
        return;
    }

    if (!m_previousBackdropPixmap.isNull() && m_backdropBlend < 1.0) {
        painter.drawPixmap(rect(), m_previousBackdropPixmap);
        if (!m_backdropPixmap.isNull()) {
            painter.save();
            painter.setOpacity(m_backdropBlend);
            painter.drawPixmap(rect(), m_backdropPixmap);
            painter.restore();
        }
    } else if (m_backdropPixmap.isNull()) {
        const QColor tone = m_atmosphere;
        painter.fillPath(stagePath,
                         QColor(tone.red(), tone.green(), tone.blue(), 36));
    } else {
        painter.drawPixmap(rect(), m_backdropPixmap);
    }

    const QColor tone = m_atmosphere;
    QLinearGradient toneWash(rect().topLeft(), rect().topRight());
    toneWash.setColorAt(0.0, QColor(tone.red(), tone.green(), tone.blue(), 128));
    toneWash.setColorAt(0.44, QColor(tone.red(), tone.green(), tone.blue(), 44));
    toneWash.setColorAt(1.0, QColor(8, 10, 16, 184));
    painter.fillPath(stagePath, toneWash);

    QLinearGradient leftShade(rect().topLeft(), QPointF(width() * 0.42, 0));
    leftShade.setColorAt(0.0, QColor(6, 8, 14, 218));
    leftShade.setColorAt(0.72, QColor(7, 10, 18, 78));
    leftShade.setColorAt(1.0, QColor(7, 10, 18, 0));
    painter.fillPath(stagePath, leftShade);

    QLinearGradient bottomFade(QPointF(0, height() * 0.54), QPointF(0, height()));
    bottomFade.setColorAt(0.0, QColor(8, 10, 16, 0));
    bottomFade.setColorAt(0.76, QColor(8, 10, 16, 126));
    bottomFade.setColorAt(1.0, QColor(8, 10, 16, 210));
    painter.fillPath(stagePath, bottomFade);

    m_posterHitRects.clear();
    QVector<PosterDrawSlot> drawSlots;
    if (!m_visiblePosterIndices.isEmpty()) {
        QHash<int, PosterDrawSlot> bestSlotForItem;
        for (const int itemIndex : std::as_const(m_visiblePosterIndices)) {
            if (itemIndex < 0) {
                continue;
            }
            const qreal position =
                signedCircularDistance(itemIndex, m_visualCenter,
                                       m_items.size());
            if (qAbs(position) > 2.36) {
                continue;
            }
            PosterDrawSlot slot;
            slot.itemIndex = itemIndex;
            slot.position = position;
            slot.rect = slotRectForPosition(rect(), position);
            slot.opacity = opacityForPosition(position);
            slot.focused = qAbs(position) < 0.42;

            const auto existing = bestSlotForItem.constFind(itemIndex);
            if (existing == bestSlotForItem.cend() ||
                qAbs(slot.position) < qAbs(existing->position)) {
                bestSlotForItem.insert(itemIndex, slot);
            }
        }

        drawSlots = bestSlotForItem.values().toVector();
        std::sort(drawSlots.begin(), drawSlots.end(),
                  [](const PosterDrawSlot& first,
                     const PosterDrawSlot& second) {
                      return qAbs(first.position) > qAbs(second.position);
                  });
    }

    for (const PosterDrawSlot& slot : std::as_const(drawSlots)) {
        if (slot.itemIndex < 0 || slot.itemIndex >= m_items.size()) {
            continue;
        }
        const QColor border =
            slot.focused ? QColor(255, 255, 255, 220)
                         : QColor(255, 255, 255, 78);
        drawPoster(painter, slot.rect,
                   pixmapForItem(m_items.at(slot.itemIndex)),
                   qRound((slot.focused ? 20 : 17) * stageScale),
                   border,
                   slot.opacity,
                   slot.focused);
        const int hitPadding = qRound(4 * stageScale);
        m_posterHitRects.insert(
            slot.itemIndex,
            slot.rect.adjusted(-hitPadding, -hitPadding, hitPadding,
                               hitPadding)
                .toRect());
    }

    painter.setClipping(false);
    painter.setPen(QPen(QColor(255, 255, 255, 66), 1.0));
    painter.setBrush(Qt::NoBrush);
    painter.drawRoundedRect(rect().adjusted(0, 0, -1, -1), stageRadius,
                            stageRadius);
}

void PosterStageWidget::resizeEvent(QResizeEvent* event)
{
    QWidget::resizeEvent(event);
    updateOverlayGeometry();
    update();
    if (m_backdropRefreshTimer) {
        m_backdropRefreshTimer->start();
    }
}

void PosterStageWidget::showEvent(QShowEvent* event)
{
    QWidget::showEvent(event);
    updateOverlayGeometry();
    updateRotationTimer();
}

void PosterStageWidget::hideEvent(QHideEvent* event)
{
    if (m_rotationTimer) {
        m_rotationTimer->stop();
    }
    if (m_backdropRefreshTimer) {
        m_backdropRefreshTimer->stop();
    }
    if (m_slideAnimation) {
        m_slideAnimation->stop();
    }
    m_slideDirection = 0;
    m_visualCenter = hasContent() ? static_cast<qreal>(m_currentIndex) : 0.0;
    if (m_backdropAnimation) {
        m_backdropAnimation->stop();
    }
    m_backdropBlend = 1.0;
    m_previousBackdropPixmap = {};
    if (m_overlayAnimation) {
        m_overlayAnimation->stop();
    }
    if (m_overlayOpacity && !qFuzzyCompare(m_overlayOpacity->opacity(), 1.0)) {
        m_overlayOpacity->setOpacity(1.0);
    }
    QWidget::hideEvent(event);
}

void PosterStageWidget::advance(int direction)
{
    if (m_items.size() < 2) {
        return;
    }

    const int next =
        PosterWallUtils::nextIndex(m_currentIndex, m_items.size(), direction);
    setCurrentIndex(next, !m_reducedMotion);
}

void PosterStageWidget::refreshCurrentItem()
{
    updateOverlayGeometry();
    updateAtmosphere();
    updateBackdrop(false);
    updateVisiblePosterIndices();
    update();

    updateRotationTimer();
}

void PosterStageWidget::updateOverlayGeometry()
{
    QWidget* overlay = findChild<QWidget*>(
        QStringLiteral("poster-stage-overlay"));
    if (!overlay) {
        return;
    }
    overlay->setGeometry(rect());
    if (m_infoPanel) {
        const int panelWidth =
            PosterStageResponsiveUtils::infoPanelWidth(width());
        const qreal scale = qBound<qreal>(0.78,
                                          static_cast<qreal>(panelWidth) / 350.0,
                                          1.08);
        if (m_infoPanel->minimumWidth() != panelWidth ||
            m_infoPanel->maximumWidth() != panelWidth) {
            m_infoPanel->setFixedWidth(panelWidth);
        }

        const int panelVerticalMargin = qRound(28 * scale) * 2;
        const int availablePanelHeight =
            qMax(qRound(220 * scale), height() - panelVerticalMargin);
        const int panelHeight =
            qMin(availablePanelHeight, qRound(276 * scale));
        if (m_infoPanel->minimumHeight() != panelHeight ||
            m_infoPanel->maximumHeight() != panelHeight) {
            m_infoPanel->setFixedHeight(panelHeight);
        }

        const int overviewWidth =
            qMax(220, panelWidth - qRound(48 * scale));
        if (m_overviewLabel->maximumWidth() != overviewWidth) {
            m_overviewLabel->setMaximumWidth(overviewWidth);
        }
        const int overviewHeight = qRound(54 * scale);
        if (m_overviewLabel->minimumHeight() != overviewHeight ||
            m_overviewLabel->maximumHeight() != overviewHeight) {
            m_overviewLabel->setFixedHeight(overviewHeight);
        }

        if (auto* overlayLayout =
                qobject_cast<QHBoxLayout*>(overlay->layout())) {
            const QMargins margins(qRound(34 * scale),
                                   qRound(28 * scale),
                                   qRound(34 * scale),
                                   qRound(28 * scale));
            if (overlayLayout->contentsMargins() != margins) {
                overlayLayout->setContentsMargins(margins);
            }
        }
        if (auto* infoLayout =
                qobject_cast<QVBoxLayout*>(m_infoPanel->layout())) {
            const QMargins margins(qRound(24 * scale),
                                   qRound(22 * scale),
                                   qRound(24 * scale),
                                   qRound(20 * scale));
            if (infoLayout->contentsMargins() != margins) {
                infoLayout->setContentsMargins(margins);
            }
            const int spacing = qRound(7 * scale);
            if (infoLayout->spacing() != spacing) {
                infoLayout->setSpacing(spacing);
            }
        }

        if (qAbs(m_lastResponsiveScale - scale) > 0.01) {
            m_lastResponsiveScale = scale;
            m_eyebrowLabel->setStyleSheet(
                QStringLiteral("font-size: %1px;").arg(qRound(11 * scale)));
            m_titleLabel->setStyleSheet(
                QStringLiteral("font-size: %1px;").arg(qRound(30 * scale)));
            m_metaLabel->setStyleSheet(
                QStringLiteral("font-size: %1px;").arg(qRound(13 * scale)));
            m_overviewLabel->setStyleSheet(
                QStringLiteral("font-size: %1px;").arg(qRound(13 * scale)));
            m_playButton->setStyleSheet(
                QStringLiteral("font-size: %1px;").arg(qRound(13 * scale)));
            m_previousButton->setStyleSheet(
                QStringLiteral("font-size: %1px;").arg(qRound(22 * scale)));
            m_nextButton->setStyleSheet(
                QStringLiteral("font-size: %1px;").arg(qRound(22 * scale)));
        }

        const QSize navSize(qRound(28 * scale), qRound(28 * scale));
        if (m_previousButton->size() != navSize ||
            m_previousButton->minimumSize() != navSize ||
            m_previousButton->maximumSize() != navSize) {
            m_previousButton->setFixedSize(navSize);
        }
        if (m_nextButton->size() != navSize ||
            m_nextButton->minimumSize() != navSize ||
            m_nextButton->maximumSize() != navSize) {
            m_nextButton->setFixedSize(navSize);
        }
        m_playButton->ensurePolished();
        const int playButtonWidth =
            qMax(qRound(84 * scale), m_playButton->sizeHint().width());
        const int playButtonHeight =
            qMax(qRound(34 * scale), m_playButton->sizeHint().height());
        const QSize playButtonSize(playButtonWidth, playButtonHeight);
        if (m_playButton->minimumSize() != playButtonSize ||
            m_playButton->maximumSize() != playButtonSize) {
            m_playButton->setFixedSize(playButtonSize);
        }
    }

    if (!hasContent()) {
        m_titleLabel->clear();
        m_metaLabel->clear();
        m_overviewLabel->clear();
        return;
    }

    const MediaItem& item = m_items.at(m_currentIndex);
    m_titleLabel->setText(item.name.trimmed().isEmpty()
                              ? tr("未命名作品")
                              : item.name.trimmed());

    QStringList meta;
    if (item.productionYear > 0) {
        meta << QString::number(item.productionYear);
    }
    if (!item.seriesName.trimmed().isEmpty()) {
        meta << item.seriesName.trimmed();
    }
    m_metaLabel->setText(meta.join(QStringLiteral("  ·  ")));
    m_overviewLabel->setText(item.overview.simplified());
}

void PosterStageWidget::updateControls()
{
    const bool content = hasContent();
    m_playButton->setVisible(content);
    m_previousButton->setVisible(content && m_items.size() > 1);
    m_nextButton->setVisible(content && m_items.size() > 1);
    m_dotsWidget->setVisible(content && m_items.size() > 1);

    auto* layout = qobject_cast<QHBoxLayout*>(m_dotsWidget->layout());
    if (!layout) {
        return;
    }

    while (QLayoutItem* child = layout->takeAt(0)) {
        if (QWidget* widget = child->widget()) {
            widget->deleteLater();
        }
        delete child;
    }

    constexpr int kMaximumVisibleDots = 7;
    const int visibleDotCount = qMin(m_items.size(), kMaximumVisibleDots);
    const int firstVisibleIndex = qBound(
        0, m_currentIndex - visibleDotCount / 2,
        qMax(0, m_items.size() - visibleDotCount));
    const int lastVisibleIndex = firstVisibleIndex + visibleDotCount;
    for (int i = firstVisibleIndex; i < lastVisibleIndex; ++i) {
        auto* dot = new QLabel(m_dotsWidget);
        dot->setObjectName(i == m_currentIndex
                               ? QStringLiteral("poster-stage-dot-active")
                               : QStringLiteral("poster-stage-dot"));
        dot->setFixedSize(i == m_currentIndex ? QSize(16, 4)
                                              : QSize(4, 4));
        layout->addWidget(dot);
    }
}

void PosterStageWidget::updateAtmosphere()
{
    if (!hasContent()) {
        return;
    }

    const QPixmap pixmap = pixmapForItem(m_items.at(m_currentIndex));
    if (pixmap.isNull()) {
        return;
    }

    m_atmosphere = PosterWallUtils::dominantColor(pixmap.toImage());
    Q_EMIT atmosphereChanged(m_atmosphere);
}

void PosterStageWidget::updateBackdrop(bool animate)
{
    if (!hasContent()) {
        m_backdropPixmap = {};
        m_previousBackdropPixmap = {};
        m_backdropSourceCacheKey = 0;
        m_backdropBlend = 1.0;
        m_visualCenter = 0.0;
        return;
    }

    const QPixmap pixmap = pixmapForItem(m_items.at(m_currentIndex));
    const qint64 sourceCacheKey =
        static_cast<qint64>(pixmap.cacheKey());
    if (sourceCacheKey == m_backdropSourceCacheKey &&
        m_backdropPixmap.size() == size()) {
        return;
    }

    const QPixmap currentBackdrop =
        !m_previousBackdropPixmap.isNull() && m_backdropBlend < 1.0
            ? blendedBackdropPixmap(m_previousBackdropPixmap, m_backdropPixmap,
                                    m_backdropBlend, size())
            : m_backdropPixmap;
    const QPixmap nextBackdrop = buildBackdropPixmap(pixmap, size());
    if (m_backdropAnimation) {
        m_backdropAnimation->stop();
    }

    const bool canAnimate =
        animate && !m_reducedMotion && !m_backdropPixmap.isNull() &&
        !nextBackdrop.isNull();
    m_previousBackdropPixmap = canAnimate ? currentBackdrop : QPixmap();
    m_backdropPixmap = nextBackdrop;
    m_backdropSourceCacheKey = sourceCacheKey;
    m_backdropBlend = canAnimate ? 0.0 : 1.0;

    if (canAnimate && m_backdropAnimation) {
        m_backdropAnimation->setStartValue(0.0);
        m_backdropAnimation->setEndValue(1.0);
        m_backdropAnimation->start();
    }
}

void PosterStageWidget::updateVisiblePosterIndices()
{
    m_visiblePosterIndices = PosterWallUtils::posterStageIndices(
        m_items, m_currentIndex, PosterStageResponsiveUtils::posterSlotCount(),
        [this](const MediaItem& item) { return pixmapForItem(item).toImage(); });
}

bool PosterStageWidget::shouldRunRotation() const
{
    return isVisible() && hasContent() && !m_hovering && m_items.size() > 1;
}

void PosterStageWidget::updateRotationTimer()
{
    if (!m_rotationTimer) {
        return;
    }

    if (shouldRunRotation()) {
        if (!m_rotationTimer->isActive()) {
            m_rotationTimer->start();
        }
        return;
    }

    if (m_rotationTimer->isActive()) {
        m_rotationTimer->stop();
    }
}

void PosterStageWidget::invalidatePosterCache(const QModelIndex& topLeft,
                                              const QModelIndex& bottomRight)
{
    if (!m_model || !topLeft.isValid() || !bottomRight.isValid()) {
        return;
    }

    const int firstRow = qMax(0, qMin(topLeft.row(), bottomRight.row()));
    const int lastRow = qMin(m_model->rowCount() - 1,
                             qMax(topLeft.row(), bottomRight.row()));
    for (int row = firstRow; row <= lastRow; ++row) {
        const QModelIndex index = m_model->index(row, 0);
        const MediaItem item =
            m_model->data(index, kItemDataRole).value<MediaItem>();
        if (!item.id.isEmpty()) {
            m_posterPixmapCache.remove(item.id);
            m_modelIndexCache.insert(item.id, index);
        }
    }
}

bool PosterStageWidget::modelRangeContainsCurrentItem(
    const QModelIndex& topLeft, const QModelIndex& bottomRight) const
{
    if (!hasContent() || !topLeft.isValid() || !bottomRight.isValid()) {
        return false;
    }

    const QModelIndex currentIndex =
        modelIndexForItem(m_items.at(m_currentIndex));
    if (!currentIndex.isValid()) {
        return false;
    }

    const int firstRow = qMin(topLeft.row(), bottomRight.row());
    const int lastRow = qMax(topLeft.row(), bottomRight.row());
    return currentIndex.row() >= firstRow && currentIndex.row() <= lastRow;
}

void PosterStageWidget::updateModelConnection()
{
    if (!m_model) {
        return;
    }

    m_modelDataChangedConnection =
        connect(m_model, &QAbstractItemModel::dataChanged, this,
                [this](const QModelIndex& topLeft,
                       const QModelIndex& bottomRight,
                       const QVector<int>& roles) {
                    const bool posterChanged =
                        roles.isEmpty() || roles.contains(kPosterPixmapRole);
                    if (!posterChanged) {
                        return;
                    }

                    const bool currentPosterChanged =
                        modelRangeContainsCurrentItem(topLeft, bottomRight);
                    invalidatePosterCache(topLeft, bottomRight);
                    updateVisiblePosterIndices();
                    if (currentPosterChanged) {
                        updateAtmosphere();
                        updateBackdrop(false);
                    }
                    update();
                });
    m_modelResetConnection =
        connect(m_model, &QAbstractItemModel::modelReset, this, [this]() {
            m_modelIndexCache.clear();
            m_posterPixmapCache.clear();
            refreshCurrentItem();
        });
}

void PosterStageWidget::setCurrentIndex(int index, bool animated)
{
    if (index < 0 || index >= m_items.size()) {
        return;
    }
    if (index == m_currentIndex) {
        return;
    }

    const int previousIndex = m_currentIndex;
    int direction = 1;
    if (previousIndex >= 0 && m_items.size() > 1) {
        const int nextIndex =
            PosterWallUtils::nextIndex(previousIndex, m_items.size(), 1);
        const int previous =
            PosterWallUtils::nextIndex(previousIndex, m_items.size(), -1);
        if (index == previous) {
            direction = -1;
        } else if (index == nextIndex) {
            direction = 1;
        } else {
            const int forwardDistance =
                (index - previousIndex + m_items.size()) % m_items.size();
            const int backwardDistance =
                (previousIndex - index + m_items.size()) % m_items.size();
            direction = backwardDistance < forwardDistance ? -1 : 1;
        }
    }

    const qreal startVisualCenter = m_visualCenter;
    const qreal targetVisualCenter =
        animated && !m_reducedMotion
            ? nearestVisualCoordinateForIndex(index, startVisualCenter,
                                              m_items.size())
            : static_cast<qreal>(index);

    if (m_slideAnimation) {
        m_slideAnimation->stop();
    }
    m_slideDirection = animated && !m_reducedMotion ? direction : 0;
    if (m_slideDirection == 0) {
        m_visualCenter = normalizedVisualCenter(targetVisualCenter,
                                                m_items.size());
    }
    m_currentIndex = index;
    updateOverlayGeometry();
    updateControls();
    updateVisiblePosterIndices();
    updateAtmosphere();
    updateBackdrop(animated);
    update();

    if (animated && m_overlayOpacity && m_overlayAnimation) {
        m_overlayAnimation->stop();
        m_overlayAnimation->setStartValue(m_overlayOpacity->opacity());
        m_overlayAnimation->setEndValue(1.0);
        m_overlayAnimation->start();
    }
    if (m_slideDirection != 0 && m_slideAnimation &&
        !qFuzzyCompare(startVisualCenter, targetVisualCenter)) {
        m_slideAnimation->setStartValue(startVisualCenter);
        m_slideAnimation->setEndValue(targetVisualCenter);
        m_slideAnimation->start();
    }
}

QModelIndex PosterStageWidget::modelIndexForItem(const MediaItem& item) const
{
    if (!m_model) {
        return {};
    }

    const auto cached = m_modelIndexCache.constFind(item.id);
    if (cached != m_modelIndexCache.cend() && cached->isValid()) {
        const MediaItem cachedItem =
            m_model->data(*cached, kItemDataRole).value<MediaItem>();
        if (cachedItem.id == item.id) {
            return *cached;
        }
    }

    for (int row = 0; row < m_model->rowCount(); ++row) {
        const QModelIndex index = m_model->index(row, 0);
        const MediaItem modelItem =
            m_model->data(index, kItemDataRole).value<MediaItem>();
        if (modelItem.id == item.id) {
            m_modelIndexCache.insert(item.id, index);
            return index;
        }
    }
    return {};
}

QPixmap PosterStageWidget::pixmapForItem(const MediaItem& item) const
{
    const auto cached = m_posterPixmapCache.constFind(item.id);
    if (cached != m_posterPixmapCache.cend()) {
        return *cached;
    }

    const QModelIndex index = modelIndexForItem(item);
    if (!index.isValid() || !m_model) {
        return {};
    }
    const QPixmap pixmap =
        m_model->data(index, kPosterPixmapRole).value<QPixmap>();
    m_posterPixmapCache.insert(item.id, pixmap);
    return pixmap;
}
