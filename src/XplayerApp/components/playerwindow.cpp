#include "playerwindow.h"
#include "../views/media/playerview.h"
#include "../utils/playbackwindowmodeutils.h"
#include "../utils/uianimationdefaults.h"
#include <QPoint>
#include <QRect>
#include <QSize>
#include <QCloseEvent>
#include <QEasingCurve>
#include <QParallelAnimationGroup>
#include <QPropertyAnimation>
#include <QScreen>
#include <QTimer>
#include <QVBoxLayout>
#include <QWindow>
#include <QWKWidgets/widgetwindowagent.h>

PlayerWindow::PlayerWindow(XplayerCore* core, QWidget *parent)
    : QWidget(parent)
{
#if (defined(Q_OS_MACOS) || defined(Q_OS_MAC)) && \
    (QT_VERSION >= QT_VERSION_CHECK(6, 9, 0))
    setWindowFlag(Qt::ExpandedClientAreaHint, true);
    setWindowFlag(Qt::NoTitleBarBackgroundHint, true);
    setAttribute(Qt::WA_ContentsMarginsRespectsSafeArea, false);
#endif

    
    m_windowAgent = new QWK::WidgetWindowAgent(this);
    m_windowAgent->setup(this);
#if defined(Q_OS_MACOS) || defined(Q_OS_MAC)
    m_windowAgent->setWindowAttribute("no-system-buttons", false);
#endif
#if defined(Q_OS_MAC)
    m_windowAgent->setSystemButtonAreaCallback([](const QSize &size) {
        return QRect(QPoint(0, 0), QSize(80, size.height()));
    });
#endif

    
    m_playerView = new PlayerView(core, this);

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);
    layout->addWidget(m_playerView);

    
    QWidget* topHUD = m_playerView->findChild<QWidget*>("playerTopHUD");
    if (topHUD) {
        m_windowAgent->setTitleBar(topHUD);
    }

    
#if !defined(Q_OS_MACOS) && !defined(Q_OS_MAC)
    if (auto* minBtn = m_playerView->findChild<QWidget*>("hud-min-btn"))
        m_windowAgent->setSystemButton(QWK::WindowAgentBase::Minimize, minBtn);
    if (auto* maxBtn = m_playerView->findChild<QWidget*>("hud-max-btn"))
        m_windowAgent->setSystemButton(QWK::WindowAgentBase::Maximize, maxBtn);
    if (auto* closeBtn = m_playerView->findChild<QWidget*>("hud-close-btn"))
        m_windowAgent->setSystemButton(QWK::WindowAgentBase::Close, closeBtn);
#endif

    
    if (auto* backBtn = m_playerView->findChild<QWidget*>("hud-back-btn"))
        m_windowAgent->setHitTestVisible(backBtn, true);

    connect(m_playerView, &PlayerView::playerChromeVisibilityChanged, this,
            &PlayerWindow::setMacSystemButtonsVisible);
    connect(m_playerView, &PlayerView::playbackTitleChanged, this,
            [this](const QString &title) { setWindowTitle(title); });

    
    connect(m_playerView, &BaseView::navigateBack, this, [this]() {
        close();
    });

    QTimer::singleShot(0, this, &PlayerWindow::bindScreenSignals);
}

void PlayerWindow::bindScreenSignals()
{
    QWindow *handle = windowHandle();
    if (!handle)
        return;

    QObject::disconnect(m_screenChangedConnection);
    QObject::disconnect(m_dpiChangedConnection);
    m_screenChangedConnection = connect(
        handle, &QWindow::screenChanged, this,
        [this](QScreen *)
        {
            bindScreenSignals();
            if (m_playerView)
                QTimer::singleShot(
                    0, m_playerView, &PlayerView::refreshForScreenChange);
        });
    if (QScreen *screen = handle->screen())
    {
        m_dpiChangedConnection = connect(
            screen, &QScreen::logicalDotsPerInchChanged, this,
            [this](qreal)
            {
                if (m_playerView)
                    QTimer::singleShot(
                        0, m_playerView, &PlayerView::refreshForScreenChange);
            });
    }
}

void PlayerWindow::playMedia(const QString &mediaId, const QString &title,
                              const QString &streamUrl, long long startPositionTicks,
                              const QVariant& sourceInfoVar)
{
    setWindowTitle(title);
    m_playerView->playMedia(mediaId, title, streamUrl, startPositionTicks, sourceInfoVar);
}

void PlayerWindow::playLaunchTransition()
{
    if (!isVisible()) {
        return;
    }

    if (m_launchAnimation && m_launchAnimation->state() == QAbstractAnimation::Running) {
        m_launchAnimation->stop();
    }

    delete m_launchAnimation;
    m_launchAnimation = new QParallelAnimationGroup(this);

    const QRect finalGeometry = geometry();
    QRect startGeometry = finalGeometry;
    startGeometry.translate(0, 18);

    setWindowOpacity(0.0);
    setGeometry(startGeometry);

    auto *fade = new QPropertyAnimation(this, "windowOpacity", m_launchAnimation);
    fade->setDuration(PlaybackWindowModeUtils::launchTransitionDurationMs());
    fade->setStartValue(0.0);
    fade->setEndValue(1.0);
    fade->setEasingCurve(
        XplayerUi::easingCurve(XplayerUi::MotionCurve::Enter));

    auto *move = new QPropertyAnimation(this, "geometry", m_launchAnimation);
    move->setDuration(PlaybackWindowModeUtils::launchTransitionDurationMs());
    move->setStartValue(startGeometry);
    move->setEndValue(finalGeometry);
    move->setEasingCurve(
        XplayerUi::easingCurve(XplayerUi::MotionCurve::Enter));

    m_launchAnimation->addAnimation(fade);
    m_launchAnimation->addAnimation(move);
    connect(m_launchAnimation, &QParallelAnimationGroup::finished, this,
            [this, finalGeometry]() {
                setWindowOpacity(1.0);
                setGeometry(finalGeometry);
            });
    QTimer::singleShot(0, this, [this]() {
        if (m_launchAnimation) {
            m_launchAnimation->start();
        }
    });
}

void PlayerWindow::setMacSystemButtonsVisible(bool visible)
{
#if defined(Q_OS_MACOS) || defined(Q_OS_MAC)
    if (m_windowAgent) {
        m_windowAgent->setWindowAttribute("no-system-buttons", !visible);
    }
#else
    Q_UNUSED(visible);
#endif
}

void PlayerWindow::closeEvent(QCloseEvent *event)
{
    if (m_playerView) {
        m_playerView->prepareForStackLeave();
    }
    QWidget::closeEvent(event);
}
