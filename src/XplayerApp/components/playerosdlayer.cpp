#include "playerosdlayer.h"
#include "../utils/uianimationdefaults.h"
#include <QAbstractAnimation>
#include <QGraphicsOpacityEffect>
#include <QIcon>
#include <QLabel>
#include <QProgressBar>
#include <QPropertyAnimation>
#include <QTimer>
#include <QWidget>
#include <cmath>

namespace
{
constexpr int kSeekLineHeight = 4;
constexpr int kSeekLabelMinWidth = 80;
constexpr int kSeekLabelMaxWidth = 148;
constexpr int kSeekLabelHeight = 28;
constexpr int kSeekLabelBottomOffset = 46;
constexpr int kSeekHorizontalMargin = 10;
constexpr int kSeekMarkerSize = 7;
constexpr int kVolumePanelWidth = 292;
constexpr int kVolumePanelHeight = 46;
constexpr int kVolumePanelMargin = 16;
constexpr int kVolumePanelTopMin = 72;
constexpr int kVolumeIconSize = 20;
constexpr int kVolumeTextWidth = 46;
constexpr int kVolumeContentPadding = 18;
constexpr int kVolumeContentGap = 14;
constexpr int kVolumeBarHeight = 5;

void stopFadeAnimationSafely(QPropertyAnimation *animation)
{
    if (!animation || !animation->targetObject() ||
        animation->state() == QAbstractAnimation::Stopped)
    {
        return;
    }

    animation->stop();
}

void setVisibleIfChanged(QWidget *widget, bool visible)
{
    if (widget && widget->isVisible() != visible)
    {
        widget->setVisible(visible);
    }
}

void setGeometryIfChanged(QWidget *widget, const QRect &geometry)
{
    if (widget && widget->geometry() != geometry)
    {
        widget->setGeometry(geometry);
    }
}
} 

PlayerOsdLayer::PlayerOsdLayer(QWidget *parent)
    : QObject(parent)
{
    
    m_container = new QWidget(parent);
    m_container->setObjectName("osdContainer");
    m_container->setAttribute(Qt::WA_TransparentForMouseEvents); 
    m_container->hide();

    
    m_opacity = new QGraphicsOpacityEffect(m_container);
    m_opacity->setOpacity(0.0);
    m_container->setGraphicsEffect(m_opacity);

    
    m_fadeAnim = new QPropertyAnimation(m_opacity, "opacity", this);
    m_fadeAnim->setDuration(XplayerUi::kQuickAnimationMs);
    m_fadeAnim->setEasingCurve(
        XplayerUi::easingCurve(XplayerUi::MotionCurve::Enter));
    connect(m_fadeAnim, &QPropertyAnimation::finished, this, [this]() {
        if (!m_container || !m_opacity) {
            return;
        }
        if (m_opacity->opacity() <= 0.001) {
            m_opacity->setOpacity(0.0);
            m_container->hide();
        }
    });

    
    m_hideTimer = new QTimer(this);
    m_hideTimer->setSingleShot(true);
    connect(m_hideTimer, &QTimer::timeout, this, &PlayerOsdLayer::hide);

    
    m_seekLine = new QProgressBar(m_container);
    m_seekLine->setObjectName("osdSeekLine");
    m_seekLine->setTextVisible(false);
    m_seekLine->setFixedHeight(kSeekLineHeight);

    
    m_seekTimeLabel = new QLabel(m_container);
    m_seekTimeLabel->setObjectName("osdSeekTimeLabel");
    m_seekTimeLabel->setAlignment(Qt::AlignCenter);
    m_seekTimeLabel->setMinimumWidth(kSeekLabelMinWidth);

    
    m_seekStem = new QWidget(m_container);
    m_seekStem->setObjectName("osdSeekStem");

    m_seekMarker = new QWidget(m_container);
    m_seekMarker->setObjectName("osdSeekMarker");

    
    m_volumePanel = new QWidget(m_container);
    m_volumePanel->setObjectName("osdVolumePanel");
    m_volumePanel->setAttribute(Qt::WA_TransparentForMouseEvents);
    m_volumePanel->hide();

    m_volumeIconLabel = new QLabel(m_volumePanel);
    m_volumeIconLabel->setObjectName("osdVolumeIconLabel");
    m_volumeIconLabel->setAlignment(Qt::AlignCenter);

    m_volumeBar = new QProgressBar(m_volumePanel);
    m_volumeBar->setObjectName("osdVolumeBar");
    m_volumeBar->setTextVisible(false);
    m_volumeBar->setRange(0, 100);
    m_volumeBar->setFixedHeight(kVolumeBarHeight);

    m_volumeLabel = new QLabel(m_volumePanel);
    m_volumeLabel->setObjectName("osdVolumeLabel");
    m_volumeLabel->setAlignment(Qt::AlignCenter);
}

void PlayerOsdLayer::showSeek(double position, double duration, const QString &timeText)
{
    fadeIn();

    setVisibleIfChanged(m_seekLine, true);
    setVisibleIfChanged(m_seekTimeLabel, true);
    setVisibleIfChanged(m_seekStem, true);
    setVisibleIfChanged(m_seekMarker, true);
    setVisibleIfChanged(m_volumePanel, false);

    m_seekDuration = std::isfinite(duration) && duration > 0.0 ? duration : 0.0;
    m_seekPosition = std::isfinite(position) ? position : 0.0;
    if (m_seekDuration > 0.0)
    {
        m_seekPosition = qBound(0.0, m_seekPosition, m_seekDuration);
    }
    else
    {
        m_seekPosition = 0.0;
    }

    const int maximum = qMax(1, static_cast<int>(std::ceil(m_seekDuration)));
    if (m_seekLine->minimum() != 0 || m_seekLine->maximum() != maximum)
    {
        m_seekLine->setRange(0, maximum);
    }
    const int seekValue =
        qBound(0, static_cast<int>(std::round(m_seekPosition)), maximum);
    if (m_seekLine->value() != seekValue)
    {
        m_seekLine->setValue(seekValue);
    }
    if (m_seekTimeLabel->text() != timeText)
    {
        m_seekTimeLabel->setText(timeText);
    }
    updateSeekLayout();

    m_hideTimer->start(XplayerUi::kOsdHoldMs);
}

void PlayerOsdLayer::showVolume(int volumePercent, const QString &text, bool muted)
{
    fadeIn();

    setVisibleIfChanged(m_seekLine, false);
    setVisibleIfChanged(m_seekTimeLabel, false);
    setVisibleIfChanged(m_seekStem, false);
    setVisibleIfChanged(m_seekMarker, false);
    setVisibleIfChanged(m_volumePanel, true);

    m_volumePercent = qBound(0, volumePercent, 100);
    const bool effectiveMuted = muted || m_volumePercent <= 0;
    const QString iconPath =
        effectiveMuted ? QStringLiteral(":/svg/player/volume-mute.svg") : QStringLiteral(":/svg/player/volume.svg");
    const QPixmap volumeIconPixmap =
        QIcon(iconPath).pixmap(kVolumeIconSize, kVolumeIconSize);
    if (m_volumeIconLabel->property("xplayerIconPath").toString() != iconPath)
    {
        m_volumeIconLabel->setPixmap(volumeIconPixmap);
        m_volumeIconLabel->setProperty("xplayerIconPath", iconPath);
    }
    if (m_volumeBar->value() != m_volumePercent)
    {
        m_volumeBar->setValue(m_volumePercent);
    }
    if (m_volumeLabel->text() != text)
    {
        m_volumeLabel->setText(text);
    }
    updateVolumeLayout();

    m_hideTimer->start(XplayerUi::kOsdHoldMs);
}

void PlayerOsdLayer::hide()
{
    if (m_opacity->opacity() <= 0.0)
    {
        if (m_container)
        {
            m_container->hide();
        }
        return;
    }
    stopFadeAnimationSafely(m_fadeAnim);
    m_fadeAnim->setStartValue(m_opacity->opacity());
    m_fadeAnim->setEndValue(0.0);
    m_fadeAnim->setEasingCurve(
        XplayerUi::easingCurve(XplayerUi::MotionCurve::Exit));
    if (canStartAnimation())
    {
        m_fadeAnim->start();
    }
    else
    {
        m_opacity->setOpacity(0.0);
        if (m_container)
        {
            m_container->hide();
        }
    }
}

void PlayerOsdLayer::forceHide()
{
    if (m_hideTimer)
    {
        m_hideTimer->stop();
    }
    stopFadeAnimationSafely(m_fadeAnim);
    if (m_opacity)
    {
        m_opacity->setOpacity(0.0);
    }
    if (m_container)
    {
        m_container->hide();
    }
}

void PlayerOsdLayer::updateGeometry(int parentWidth, int parentHeight)
{
    if (m_parentWidth == parentWidth && m_parentHeight == parentHeight)
    {
        return;
    }

    m_parentWidth = parentWidth;
    m_parentHeight = parentHeight;

    setGeometryIfChanged(m_container, QRect(0, 0, parentWidth, parentHeight));
    setGeometryIfChanged(m_seekLine,
                         QRect(0, parentHeight - kSeekLineHeight,
                               parentWidth, kSeekLineHeight));
    updateSeekLayout();
    updateVolumeLayout();
}

void PlayerOsdLayer::stopAnimations()
{
    if (m_hideTimer)
    {
        m_hideTimer->stop();
    }
    if (m_fadeAnim)
    {
        stopFadeAnimationSafely(m_fadeAnim);
    }
    if (m_opacity)
    {
        m_opacity->setOpacity(0.0);
    }
    if (m_container)
    {
        m_container->hide();
    }
}

bool PlayerOsdLayer::isVisible() const
{
    return m_container && m_container->isVisible();
}

bool PlayerOsdLayer::isSeekLineVisible() const
{
    return isVisible() && m_seekLine && !m_seekLine->isHidden();
}

QWidget *PlayerOsdLayer::container() const
{
    return m_container;
}

void PlayerOsdLayer::updateSeekPosition(int position, const QString &timeText)
{
    m_seekPosition = position;
    if (m_seekDuration > 0.0)
    {
        m_seekPosition = qBound(0.0, m_seekPosition, m_seekDuration);
    }

    const int maximum = qMax(1, m_seekLine->maximum());
    const int seekValue =
        qBound(0, static_cast<int>(std::round(m_seekPosition)), maximum);
    if (m_seekLine->value() != seekValue)
    {
        m_seekLine->setValue(seekValue);
    }
    if (m_seekTimeLabel->text() != timeText)
    {
        m_seekTimeLabel->setText(timeText);
    }
    updateSeekLayout();
}

void PlayerOsdLayer::fadeIn()
{
    if (!m_container || !m_opacity)
    {
        return;
    }

    const bool fadeOutRunning =
        m_fadeAnim && m_fadeAnim->state() == QAbstractAnimation::Running &&
        m_fadeAnim->endValue().toReal() <= 0.0;
    if (!m_container->isVisible() || m_opacity->opacity() < 1.0 ||
        fadeOutRunning)
    {
        m_container->show();
        stopFadeAnimationSafely(m_fadeAnim);
        m_fadeAnim->setStartValue(m_opacity->opacity());
        m_fadeAnim->setEndValue(1.0);
        m_fadeAnim->setEasingCurve(
            XplayerUi::easingCurve(XplayerUi::MotionCurve::Enter));
        if (canStartAnimation())
        {
            m_fadeAnim->start();
        }
        else
        {
            m_opacity->setOpacity(1.0);
        }
    }
}

bool PlayerOsdLayer::canStartAnimation() const
{
    return m_fadeAnim && m_fadeAnim->targetObject();
}

void PlayerOsdLayer::updateSeekLayout()
{
    if (!m_seekTimeLabel || !m_seekStem || !m_seekMarker || m_parentWidth <= 0 || m_parentHeight <= 0)
    {
        return;
    }

    const double progressRatio = m_seekDuration > 0.0 ? qBound(0.0, m_seekPosition / m_seekDuration, 1.0) : 0.0;
    const int progressX = qBound(0, static_cast<int>(std::round(progressRatio * (m_parentWidth - 1))),
                                 qMax(0, m_parentWidth - 1));

    const int preferredWidth = m_seekTimeLabel->sizeHint().width();
    const int labelWidth = qBound(kSeekLabelMinWidth, preferredWidth, kSeekLabelMaxWidth);
    const int labelY = qMax(12, m_parentHeight - kSeekLabelBottomOffset);

    int labelX = 0;
    if (m_parentWidth <= labelWidth + kSeekHorizontalMargin * 2)
    {
        labelX = qMax(0, (m_parentWidth - labelWidth) / 2);
    }
    else
    {
        labelX = qBound(kSeekHorizontalMargin, progressX - labelWidth / 2,
                        m_parentWidth - labelWidth - kSeekHorizontalMargin);
    }

    setGeometryIfChanged(m_seekTimeLabel,
                         QRect(labelX, labelY, labelWidth, kSeekLabelHeight));

    const int markerX = qBound(0, progressX - kSeekMarkerSize / 2, qMax(0, m_parentWidth - kSeekMarkerSize));
    const int markerY = qMax(labelY + kSeekLabelHeight + 3, m_parentHeight - kSeekLineHeight - kSeekMarkerSize - 1);
    setGeometryIfChanged(m_seekMarker,
                         QRect(markerX, markerY,
                               kSeekMarkerSize, kSeekMarkerSize));

    const int stemTop = labelY + kSeekLabelHeight + 2;
    const int stemBottom = qMax(stemTop, markerY - 1);
    const int stemHeight = qMax(2, stemBottom - stemTop);
    const int stemX = qBound(0, progressX, qMax(0, m_parentWidth - 1));
    setGeometryIfChanged(m_seekStem, QRect(stemX, stemTop, 1, stemHeight));
}

void PlayerOsdLayer::updateVolumeLayout()
{
    if (!m_volumePanel || !m_volumeIconLabel || !m_volumeBar || !m_volumeLabel || m_parentWidth <= 0 ||
        m_parentHeight <= 0)
    {
        return;
    }

    const int panelWidth = qMin(kVolumePanelWidth, qMax(180, m_parentWidth - kVolumePanelMargin * 2));
    const int panelX = qMax(0, (m_parentWidth - panelWidth) / 2);
    const int preferredY = static_cast<int>(std::round(m_parentHeight * 0.22));
    const int maxPanelY = qMax(0, m_parentHeight - kVolumePanelHeight - kVolumePanelMargin);
    const int panelY = qMin(qMax(kVolumePanelTopMin, preferredY), maxPanelY);
    setGeometryIfChanged(m_volumePanel,
                         QRect(panelX, panelY,
                               panelWidth, kVolumePanelHeight));

    const int availableWidth = panelWidth - kVolumeContentPadding * 2;
    const int barWidth =
        qMax(86, availableWidth - kVolumeIconSize - kVolumeTextWidth - kVolumeContentGap * 2);
    const int contentWidth = kVolumeIconSize + kVolumeContentGap + barWidth + kVolumeContentGap + kVolumeTextWidth;
    const int contentX = qMax(kVolumeContentPadding, (panelWidth - contentWidth) / 2);

    const int iconY = (kVolumePanelHeight - kVolumeIconSize) / 2;
    setGeometryIfChanged(m_volumeIconLabel,
                         QRect(contentX, iconY,
                               kVolumeIconSize, kVolumeIconSize));

    const int barX = contentX + kVolumeIconSize + kVolumeContentGap;
    const int barY = (kVolumePanelHeight - kVolumeBarHeight) / 2;
    setGeometryIfChanged(m_volumeBar,
                         QRect(barX, barY, barWidth, kVolumeBarHeight));

    const int textX = barX + barWidth + kVolumeContentGap;
    setGeometryIfChanged(m_volumeLabel,
                         QRect(textX, 0, kVolumeTextWidth, kVolumePanelHeight));
}
