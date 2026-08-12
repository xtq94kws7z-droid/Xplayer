#include "dashboardview.h"

#include "../../components/horizontallistviewgallery.h"
#include "../../components/libraryactionmenu.h"
#include "../../components/mediaimageeditdialog.h"
#include "../../components/moderntoast.h"
#include "../../components/mediasectionwidget.h"
#include "../../components/posterstagewidget.h"
#include "../../utils/dashboardrequestlimitutils.h"
#include "../../utils/dashboardresponsiveutils.h"
#include "../../utils/buttonlayoututils.h"
#include "../../utils/dashboardsectionorderutils.h"
#include "../../utils/dashboardloadstageutils.h"
#include "../../utils/mediaitemutils.h"
#include "../../utils/posterwallutils.h"
#include "../../utils/resumeitemresolver.h"
#include "../../utils/smoothscrollcontroller.h"
#include "../../utils/textwraputils.h"
#include "../../utils/uianimationdefaults.h"
#include "../media/mediacarddelegate.h"
#include "../media/medialistmodel.h"
#include <QApplication>
#include <QDebug>
#include <QFont>
#include <QHBoxLayout>
#include <QLabel>
#include <QLinearGradient>
#include <QListView>
#include <QPainter>
#include <QPainterPath>
#include <QPointer>
#include <QPushButton>
#include <QRadialGradient>
#include <QResizeEvent>
#include <QScrollArea>
#include <QScrollBar>
#include <QSet>
#include <QShowEvent>
#include <QStyle>
#include <QStyledItemDelegate>
#include <QTimer>
#include <QVBoxLayout>
#include <QWheelEvent>
#include <utility>
#include <config/config_keys.h>
#include <config/configstore.h>
#include <xplayercore.h>
#include <services/admin/adminservice.h>
#include <services/manager/servermanager.h>
#include <services/media/mediaservice.h>

namespace {

constexpr int kPosterStageItemLimit = 18;
constexpr int kHeroPosterWallCoalesceMs = 50;
constexpr int kDashboardReloadCoalesceMs = 80;
constexpr int kDashboardUiCommitRetryMs = 64;

bool isLibraryNavigationItem(const MediaItem& item)
{
    return item.type == "BoxSet" || item.type == "Playlist" ||
           item.type == "Folder";
}

MediaCardDelegate::CardStyle dashboardGalleryStyle()
{
    const bool isTile =
        ConfigStore::instance()->get<QString>(ConfigKeys::DefaultLibraryView,
                                              "poster") == "tile";
    return isTile ? MediaCardDelegate::LibraryTile
                  : MediaCardDelegate::Poster;
}

int dashboardGalleryHeight()
{
    return dashboardGalleryStyle() == MediaCardDelegate::LibraryTile ? 230 : 300;
}

void applyResponsiveFont(QWidget* root, const QString& objectName, int pixels)
{
    if (!root) {
        return;
    }

    const auto labels = root->findChildren<QLabel*>(objectName);
    for (QLabel* label : labels) {
        QFont font = label->font();
        font.setPixelSize(pixels);
        label->setFont(font);
    }

    const auto buttons = root->findChildren<QPushButton*>(objectName);
    for (QPushButton* button : buttons) {
        QFont font = button->font();
        font.setPixelSize(pixels);
        button->setFont(font);
        button->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
        button->setMinimumWidth(
            ButtonLayoutUtils::minimumSectionMoreButtonWidth(button->text(),
                                                              font));
        button->updateGeometry();
    }
}

} 

DashboardView::DashboardView(XplayerCore* core, QWidget* parent)
    : BaseView(core, parent)
{
    setObjectName("dashboard-view");

    m_libraryDelegate =
        new MediaCardDelegate(MediaCardDelegate::LibraryTile, this);
    m_libraryDelegate->setShowMoreButtonForNonPlayableTiles(true);

    m_heroPosterWallUpdateTimer = new QTimer(this);
    m_heroPosterWallUpdateTimer->setSingleShot(true);
    m_heroPosterWallUpdateTimer->setTimerType(Qt::CoarseTimer);
    m_heroPosterWallUpdateTimer->setInterval(kHeroPosterWallCoalesceMs);
    connect(m_heroPosterWallUpdateTimer, &QTimer::timeout, this,
            &DashboardView::updateHeroPosterWall);

    m_dashboardReloadTimer = new QTimer(this);
    m_dashboardReloadTimer->setSingleShot(true);
    m_dashboardReloadTimer->setTimerType(Qt::CoarseTimer);
    m_dashboardReloadTimer->setInterval(kDashboardReloadCoalesceMs);
    connect(m_dashboardReloadTimer, &QTimer::timeout, this, [this]() {
        if (isVisible()) {
            launchDashboardTask(loadDashboardData());
        }
    });

    m_dashboardDeferredLoadTimer = new QTimer(this);
    m_dashboardDeferredLoadTimer->setSingleShot(true);
    m_dashboardDeferredLoadTimer->setTimerType(Qt::CoarseTimer);
    connect(m_dashboardDeferredLoadTimer, &QTimer::timeout, this, [this]() {
        if (!isVisible() ||
            m_deferredDashboardGeneration != m_loadGeneration ||
            currentDashboardContextKey() != m_dashboardContextKey) {
            return;
        }

        if (m_deferredShowLatest) {
            launchDashboardTask(loadLatestSection(
                true, m_deferredDashboardGeneration));
        }
        if (m_deferredShowCompleted) {
            launchDashboardTask(loadCompletedSection(
                true, m_deferredDashboardGeneration));
        }
        if (m_deferredShowLibraries || m_deferredShowEachLibrary) {
            launchDashboardTask(loadLibrarySections(
                m_deferredShowLibraries, m_deferredShowEachLibrary,
                m_deferredDashboardGeneration));
        }
    });

    m_dashboardUiCommitTimer = new QTimer(this);
    m_dashboardUiCommitTimer->setSingleShot(true);
    m_dashboardUiCommitTimer->setTimerType(Qt::CoarseTimer);
    connect(m_dashboardUiCommitTimer, &QTimer::timeout, this,
            &DashboardView::flushDeferredDashboardUiCommits);

    connect(m_libraryDelegate, &MediaCardDelegate::playRequested, this,
            &BaseView::handlePlayRequested);
    connect(m_libraryDelegate, &MediaCardDelegate::favoriteRequested, this,
            &BaseView::handleFavoriteRequested);
    connect(m_libraryDelegate, &MediaCardDelegate::moreMenuRequested, this,
            &BaseView::handleMoreMenuRequested);

    setupUi();

    connect(m_libraryListView, &QListView::clicked, this,
            [this](const QModelIndex& index) {
                const MediaItem item = m_libraryModel->getItem(index);
                Q_EMIT navigateToLibrary(item.id, item.name);
            });

    connect(ConfigStore::instance(), &ConfigStore::valueChanged, this,
            [this](const QString& key, const QVariant&) {
                const QString sid = currentServerId();
                const QString customOrderEnabledKey = ConfigKeys::forServer(
                    sid, ConfigKeys::CustomHomeSectionOrderEnabled);
                const QString homeSectionOrderKey =
                    ConfigKeys::forServer(sid, ConfigKeys::HomeSectionOrder);

                if (key == customOrderEnabledKey || key == homeSectionOrderKey) {
                    applyDashboardSectionOrder();
                    if (isVisible()) {
                        scheduleDashboardReload();
                    }
                    return;
                }

                if (key == ConfigKeys::DefaultLibraryView ||
                    key == ConfigKeys::forServer(
                               sid, ConfigKeys::ShowContinueWatching) ||
                    key ==
                        ConfigKeys::forServer(sid, ConfigKeys::ShowLatestAdded) ||
                    key ==
                        ConfigKeys::forServer(sid, ConfigKeys::ShowRecommended) ||
                    key == ConfigKeys::forServer(
                               sid, ConfigKeys::ShowCompletedWatching) ||
                    key == ConfigKeys::forServer(
                               sid, ConfigKeys::ContinueWatchingRequestLimit) ||
                    key == ConfigKeys::forServer(
                               sid, ConfigKeys::LatestMediaRequestLimit) ||
                    key == ConfigKeys::forServer(
                               sid, ConfigKeys::RecommendedRequestLimit) ||
                    key == ConfigKeys::forServer(
                               sid, ConfigKeys::CompletedWatchingRequestLimit) ||
                    key == ConfigKeys::forServer(
                               sid, ConfigKeys::ShowMediaLibraries) ||
                    key ==
                        ConfigKeys::forServer(sid, ConfigKeys::ShowEachLibrary)) {
                    if (isVisible()) {
                        scheduleDashboardReload();
                    }
                }
            });

    applyDashboardSectionOrder();
    m_dashboardContextKey = currentDashboardContextKey();

    if (m_core && m_core->serverManager()) {
        connect(m_core->serverManager(), &ServerManager::activeServerChanged,
                this, [this](const ServerProfile&) {
                    const QString nextContextKey = currentDashboardContextKey();
                    if (nextContextKey == m_dashboardContextKey) {
                        return;
                    }

                    qDebug() << "[DashboardView] Dashboard context changed, "
                                "clearing stale home content"
                             << "| hasActiveSession=" << !nextContextKey.isEmpty()
                             << "| previousHadActiveSession="
                             << !m_dashboardContextKey.isEmpty();

                    m_dashboardContextKey = nextContextKey;
                    ++m_loadGeneration;
                    clearDashboardState(true);
                    applyDashboardSectionOrder();

                    if (!nextContextKey.isEmpty() && isVisible()) {
                        scheduleDashboardReload(0);
                    }
                });
    }
}

void DashboardView::launchDashboardTask(QCoro::Task<void>&& task)
{
    QCoro::connect(std::move(task), this, []() {});
}

void DashboardView::scheduleDashboardReload(int delayMs)
{
    if (!isVisible()) {
        return;
    }

    if (!m_dashboardReloadTimer) {
        launchDashboardTask(loadDashboardData());
        return;
    }

    m_dashboardReloadTimer->start(
        delayMs >= 0 ? delayMs : kDashboardReloadCoalesceMs);
}

void DashboardView::setupUi()
{
    auto* dashLayout = new QVBoxLayout(this);
    dashLayout->setContentsMargins(0, 0, 0, 0);

    m_mainScrollArea = new QScrollArea(this);
    m_mainScrollArea->setObjectName("dashboard-scroll");
    m_mainScrollArea->setWidgetResizable(true);
    m_mainScrollArea->setFrameShape(QFrame::NoFrame);
    m_mainScrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    auto* container = new QWidget(m_mainScrollArea);
    container->setObjectName("dashboard-container");
    m_dashboardContainer = container;
    container->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
    container->setMaximumWidth(DashboardResponsiveUtils::maxContentWidth());
    m_mainScrollArea->setAlignment(Qt::AlignHCenter);

    m_containerLayout = new QVBoxLayout(container);
    m_containerLayout->setContentsMargins(24, 18, 24, 22);
    m_containerLayout->setSpacing(10);

    m_posterStage = new PosterStageWidget(container);
    m_heroPosterWallModel = new MediaListModel(360, m_core, this);
    m_heroPosterWallModel->setImageMaxWidth(720);
    m_posterStage->setModel(m_heroPosterWallModel);
    connect(m_posterStage, &PosterStageWidget::itemActivated, this,
            [this](const MediaItem& item) {
                if (isLibraryNavigationItem(item)) {
                    Q_EMIT navigateToLibrary(item.id, item.name);
                } else {
                    Q_EMIT navigateToDetail(item.id, item.name, item);
                }
            });
    connect(m_posterStage, &PosterStageWidget::atmosphereChanged, this,
            [this](const QColor& color) {
                m_dashboardAtmosphere = color;
                update();
            });

    const MediaCardDelegate::CardStyle galleryStyle = dashboardGalleryStyle();
    const auto initialMetrics =
        DashboardResponsiveUtils::metricsForViewport(QSize(1920, 1080));
    const int galleryHeight = initialMetrics.galleryHeight;

    auto connectGallerySignals = [this](HorizontalListViewGallery* gallery) {
        gallery->listView()->setProperty("isHorizontalListView", true);
        gallery->listView()->viewport()->installEventFilter(this);

        connect(gallery, &HorizontalListViewGallery::itemClicked, this,
                [this](const MediaItem& item) {
                    if (isLibraryNavigationItem(item)) {
                        Q_EMIT navigateToLibrary(item.id, item.name);
                    } else {
                        Q_EMIT navigateToDetail(item.id, item.name, item);
                    }
                });
        connect(gallery, &HorizontalListViewGallery::playRequested, this,
                &BaseView::handlePlayRequested);
        connect(gallery, &HorizontalListViewGallery::favoriteRequested, this,
                &BaseView::handleFavoriteRequested);
        connect(gallery, &HorizontalListViewGallery::moreMenuRequested, this,
                &BaseView::handleMoreMenuRequested);
    };

    auto createGallerySection =
        [this, container, galleryHeight, &connectGallerySignals](
            QWidget* header, HorizontalListViewGallery** outGallery,
            MediaCardDelegate::CardStyle style) {
            auto* section = new QWidget(container);
            auto* layout = new QVBoxLayout(section);
            layout->setContentsMargins(0, 0, 0, 0);
            layout->setSpacing(0);
            layout->addWidget(header);

            *outGallery = new HorizontalListViewGallery(m_core, section);
            (*outGallery)->setFixedHeight(galleryHeight);
            (*outGallery)->setCardStyle(style);
            connectGallerySignals(*outGallery);

            layout->addWidget(*outGallery);
            return section;
        };

    m_resumeHeader = createSectionHeader(tr("继续观看"), "resume");
    m_resumeSection =
        createGallerySection(m_resumeHeader, &m_resumeGallery, galleryStyle);

    m_latestHeader = createSectionHeader(tr("最近新增"), "latest");
    m_latestSection =
        createGallerySection(m_latestHeader, &m_latestGallery, galleryStyle);

    m_recommendHeader = createSectionHeader(tr("为你推荐"), "recommended");
    m_recommendSection = createGallerySection(m_recommendHeader,
                                               &m_recommendGallery, galleryStyle);

    m_completedHeader =
        createSectionHeader(tr("已看完"), "played");
    m_completedSection = createGallerySection(m_completedHeader,
                                              &m_completedGallery, galleryStyle);

    m_librarySection = new QWidget(container);
    auto* libraryLayout = new QVBoxLayout(m_librarySection);
    libraryLayout->setContentsMargins(0, 0, 0, 0);
    libraryLayout->setSpacing(0);

    m_libraryTitle = new QLabel(tr("媒体库"), m_librarySection);
    m_libraryTitle->setObjectName("dashboard-section-title");
    m_libraryGallery = new HorizontalListViewGallery(m_core, m_librarySection);
    m_libraryGallery->setFixedHeight(initialMetrics.libraryGalleryHeight);
    m_libraryGallery->setCardStyle(MediaCardDelegate::LibraryTile);
    m_libraryGallery->setTileSize(initialMetrics.libraryTile);
    m_libraryGallery->setTextPixelSizes(initialMetrics.posterTitlePixels,
                                        initialMetrics.posterSubtitlePixels);
    m_libraryGallery->setContentPadding(8);
    m_libraryGallery->listView()->setProperty("isHorizontalListView", true);
    m_libraryGallery->listView()->viewport()->installEventFilter(this);
    connect(m_libraryGallery, &HorizontalListViewGallery::itemClicked, this,
            [this](const MediaItem& item) {
                Q_EMIT navigateToLibrary(item.id, item.name);
            });
    connect(m_libraryGallery, &HorizontalListViewGallery::moreMenuRequested,
            this, &BaseView::handleMoreMenuRequested);

    libraryLayout->addWidget(m_libraryTitle);
    libraryLayout->addWidget(m_libraryGallery);

    m_libraryModel = new MediaListModel(500, m_core, this);
    m_libraryModel->setImageMaxWidth(500);

    m_librarySectionsContainer = new QWidget(container);
    m_librarySectionsLayout = new QVBoxLayout(m_librarySectionsContainer);
    m_librarySectionsLayout->setContentsMargins(0, 0, 0, 0);
    m_librarySectionsLayout->setSpacing(0);

    m_containerLayout->addWidget(m_posterStage);
    m_containerLayout->addWidget(m_resumeSection);
    m_containerLayout->addWidget(m_latestSection);
    m_containerLayout->addWidget(m_recommendSection);
    m_containerLayout->addWidget(m_completedSection);
    m_containerLayout->addWidget(m_librarySection);
    m_containerLayout->addWidget(m_librarySectionsContainer);
    m_containerLayout->addStretch();

    m_posterStage->hide();
    m_resumeSection->hide();
    m_latestSection->hide();
    m_recommendSection->hide();
    m_completedSection->hide();
    m_librarySection->hide();
    m_librarySectionsContainer->hide();

    m_mainScrollArea->setWidget(container);
    dashLayout->addWidget(m_mainScrollArea);

    m_vScrollController =
        new SmoothScrollController(m_mainScrollArea->verticalScrollBar(), this);
    m_vScrollController->setDuration(XplayerUi::kScrollAnimationMs);
    connect(m_vScrollController, &SmoothScrollController::scrollingChanged,
            this, [this](bool active) {
                const QList<HorizontalListViewGallery*> galleries = {
                    m_resumeGallery, m_latestGallery, m_recommendGallery,
                    m_completedGallery, m_libraryGallery};
                for (HorizontalListViewGallery* gallery : galleries) {
                    if (gallery) {
                        gallery->setVerticalScrollActive(active);
                    }
                }
                for (MediaSectionWidget* section :
                     std::as_const(m_libraryGalleries)) {
                    if (section && section->gallery()) {
                        section->gallery()->setVerticalScrollActive(active);
                    }
                }
                if (!active) {
                    flushDeferredDashboardUiCommits();
                }
            });

    m_mainScrollArea->viewport()->installEventFilter(this);
    applyResponsiveLayout();
}

void DashboardView::applyDashboardSectionOrder()
{
    if (!m_containerLayout) {
        return;
    }

    const QString sid = currentServerId();
    if (!sid.isEmpty()) {
        auto* store = ConfigStore::instance();
        const QString customOrderEnabledKey = ConfigKeys::forServer(
            sid, ConfigKeys::CustomHomeSectionOrderEnabled);
        const QString homeSectionOrderKey =
            ConfigKeys::forServer(sid, ConfigKeys::HomeSectionOrder);

        if (store->get<bool>(customOrderEnabledKey, false)) {
            const QStringList rawOrder =
                store->get<QStringList>(homeSectionOrderKey, {});
            if (!rawOrder.isEmpty()) {
                const QStringList normalizedOrder =
                    DashboardSectionOrderUtils::normalizeSectionOrder(rawOrder);
                if (normalizedOrder != rawOrder) {
                    store->set(homeSectionOrderKey, normalizedOrder);
                }
            }
        }
    }

    int insertIndex = m_posterStage ? 1 : 0;
    for (const QString& sectionId : dashboardSectionOrder()) {
        QWidget* widget = sectionWidgetForId(sectionId);
        if (widget) {
            m_containerLayout->insertWidget(insertIndex++, widget);
        }
    }
}

QStringList DashboardView::dashboardSectionOrder() const
{
    const QString sid = currentServerId();
    if (sid.isEmpty()) {
        return DashboardSectionOrderUtils::defaultSectionOrder();
    }

    auto* store = ConfigStore::instance();
    const QString customOrderEnabledKey = ConfigKeys::forServer(
        sid, ConfigKeys::CustomHomeSectionOrderEnabled);
    if (!store->get<bool>(customOrderEnabledKey, false)) {
        return DashboardSectionOrderUtils::defaultSectionOrder();
    }

    const QString homeSectionOrderKey =
        ConfigKeys::forServer(sid, ConfigKeys::HomeSectionOrder);
    return DashboardSectionOrderUtils::normalizeSectionOrder(
        store->get<QStringList>(homeSectionOrderKey,
                                DashboardSectionOrderUtils::defaultSectionOrder()));
}

QString DashboardView::currentServerId() const
{
    if (!m_core || !m_core->serverManager()) {
        return {};
    }

    const ServerProfile profile = m_core->serverManager()->activeProfile();
    return profile.isValid() ? profile.id : QString();
}

QString DashboardView::currentDashboardContextKey() const
{
    if (!m_core || !m_core->serverManager()) {
        return {};
    }

    const ServerProfile profile = m_core->serverManager()->activeProfile();
    if (!profile.isValid() || profile.id.trimmed().isEmpty()) {
        return {};
    }

    return profile.id + QLatin1Char('|') + profile.userId;
}

QWidget* DashboardView::sectionWidgetForId(const QString& sectionId) const
{
    if (sectionId == QLatin1String(
                         DashboardSectionOrderUtils::ContinueWatchingSectionId)) {
        return m_resumeSection;
    }
    if (sectionId ==
        QLatin1String(DashboardSectionOrderUtils::LatestMediaSectionId)) {
        return m_latestSection;
    }
    if (sectionId ==
        QLatin1String(DashboardSectionOrderUtils::RecommendedSectionId)) {
        return m_recommendSection;
    }
    if (sectionId == QLatin1String(
                         DashboardSectionOrderUtils::CompletedWatchingSectionId)) {
        return m_completedSection;
    }
    if (sectionId ==
        QLatin1String(DashboardSectionOrderUtils::AllLibrariesSectionId)) {
        return m_librarySection;
    }
    if (sectionId == QLatin1String(
                         DashboardSectionOrderUtils::
                             EachLibrarySectionsSectionId)) {
        return m_librarySectionsContainer;
    }

    return nullptr;
}

void DashboardView::clearLibraryGallerySections()
{
    if (!m_librarySectionsLayout) {
        return;
    }

    for (MediaSectionWidget* section : std::as_const(m_libraryGalleries)) {
        if (!section) {
            continue;
        }

        m_librarySectionsLayout->removeWidget(section);
        section->deleteLater();
    }

    m_libraryGalleries.clear();
}

void DashboardView::clearDashboardState(bool resetScrollPositions)
{
    if (m_dashboardUiCommitTimer) {
        m_dashboardUiCommitTimer->stop();
    }
    m_deferredDashboardUiCommits.clear();
    m_deferredUiCommitGeneration = -1;

    clearDashboardGallery(m_resumeGallery);
    clearDashboardGallery(m_latestGallery);
    clearDashboardGallery(m_recommendGallery);
    clearDashboardGallery(m_completedGallery);

    if (m_heroPosterWallModel) {
        m_heroPosterWallModel->setItems(QList<MediaItem> {});
        m_heroPosterWallModel->clearImageCache();
    }
    if (m_posterStage) {
        m_posterStage->setItems({});
        m_posterStage->setVisible(false);
    }

    if (m_resumeSection) {
        m_resumeSection->setVisible(false);
    }
    if (m_latestSection) {
        m_latestSection->setVisible(false);
    }
    if (m_recommendSection) {
        m_recommendSection->setVisible(false);
    }
    if (m_completedSection) {
        m_completedSection->setVisible(false);
    }

    if (m_libraryModel) {
        m_libraryModel->setItems(QList<MediaItem> {});
        m_libraryModel->clearImageCache();
    }
    clearDashboardGallery(m_libraryGallery);
    resetListViewScrollPosition(m_libraryListView);
    if (m_librarySection) {
        m_librarySection->setVisible(false);
    }
    if (m_libraryGridSection) {
        m_libraryGridSection->setVisible(false);
    }

    clearLibraryGallerySections();
    if (m_librarySectionsContainer) {
        m_librarySectionsContainer->setVisible(false);
    }

    adjustLibraryGridHeight();

    if (resetScrollPositions) {
        resetDashboardScrollPositions();
    }
}

void DashboardView::resetDashboardScrollPositions()
{
    if (m_vScrollController) {
        m_vScrollController->scrollTo(0, false);
    } else if (m_mainScrollArea && m_mainScrollArea->verticalScrollBar()) {
        m_mainScrollArea->verticalScrollBar()->setValue(
            m_mainScrollArea->verticalScrollBar()->minimum());
    }

    resetDashboardGalleryScrollPosition(m_resumeGallery);
    resetDashboardGalleryScrollPosition(m_latestGallery);
    resetDashboardGalleryScrollPosition(m_recommendGallery);
    resetDashboardGalleryScrollPosition(m_completedGallery);
    resetDashboardGalleryScrollPosition(m_libraryGallery);
    resetListViewScrollPosition(m_libraryListView);

    for (MediaSectionWidget* section : std::as_const(m_libraryGalleries)) {
        if (section && section->gallery()) {
            resetDashboardGalleryScrollPosition(section->gallery());
        }
    }
}

void DashboardView::clearDashboardGallery(HorizontalListViewGallery* gallery)
{
    if (!gallery) {
        return;
    }

    gallery->setLoading(false);
    gallery->setItems(QList<MediaItem> {});
    gallery->clearImageCache();
    resetListViewScrollPosition(gallery->listView());
}

void DashboardView::updateHeroPosterWall()
{
    if (!m_heroPosterWallModel || !m_posterStage) {
        return;
    }

    const QList<MediaItem> wallItems = PosterWallUtils::mergeUniqueItems(
        m_resumeGallery ? m_resumeGallery->items() : QList<MediaItem> {},
        m_latestGallery ? m_latestGallery->items() : QList<MediaItem> {},
        m_recommendGallery ? m_recommendGallery->items() : QList<MediaItem> {},
        m_completedGallery ? m_completedGallery->items() : QList<MediaItem> {},
        kPosterStageItemLimit);

    m_heroPosterWallModel->setItems(wallItems);
    m_posterStage->setItems(wallItems);
    m_posterStage->setVisible(!wallItems.isEmpty());
}

void DashboardView::scheduleHeroPosterWallUpdate()
{
    if (!m_heroPosterWallUpdateTimer ||
        m_heroPosterWallUpdateTimer->isActive()) {
        return;
    }
    m_heroPosterWallUpdateTimer->start();
}

bool DashboardView::isDashboardScrollActive() const
{
    return m_vScrollController && m_vScrollController->isAnimating();
}

void DashboardView::commitDashboardUiChange(int generation,
                                            std::function<void()> commit)
{
    if (!commit || !isVisible() || generation != m_loadGeneration) {
        return;
    }

    if (!isDashboardScrollActive()) {
        commit();
        return;
    }

    if (m_deferredUiCommitGeneration != generation) {
        m_deferredDashboardUiCommits.clear();
        m_deferredUiCommitGeneration = generation;
    }

    m_deferredDashboardUiCommits.append(std::move(commit));
    if (m_dashboardUiCommitTimer) {
        m_dashboardUiCommitTimer->start(kDashboardUiCommitRetryMs);
    }
}

void DashboardView::flushDeferredDashboardUiCommits()
{
    if (m_deferredDashboardUiCommits.isEmpty()) {
        return;
    }

    if (isDashboardScrollActive()) {
        if (m_dashboardUiCommitTimer) {
            m_dashboardUiCommitTimer->start(kDashboardUiCommitRetryMs);
        }
        return;
    }

    if (m_deferredUiCommitGeneration != m_loadGeneration) {
        m_deferredDashboardUiCommits.clear();
        m_deferredUiCommitGeneration = -1;
        return;
    }

    QList<std::function<void()>> commits =
        std::move(m_deferredDashboardUiCommits);
    m_deferredDashboardUiCommits.clear();
    const int commitGeneration = m_loadGeneration;
    m_deferredUiCommitGeneration = -1;

    QWidget* updateRoot =
        m_dashboardContainer ? m_dashboardContainer : static_cast<QWidget*>(this);
    const bool updatesWereEnabled = updateRoot->updatesEnabled();
    updateRoot->setUpdatesEnabled(false);
    for (const auto& commit : std::as_const(commits)) {
        if (commit && commitGeneration == m_loadGeneration) {
            commit();
        }
    }
    updateRoot->setUpdatesEnabled(updatesWereEnabled);
    if (updatesWereEnabled) {
        updateRoot->update();
    }
}

void DashboardView::resetDashboardGalleryScrollPosition(
    HorizontalListViewGallery* gallery) const
{
    if (!gallery) {
        return;
    }

    resetListViewScrollPosition(gallery->listView());
}

void DashboardView::resetListViewScrollPosition(QListView* listView) const
{
    if (!listView) {
        return;
    }

    if (QScrollBar* horizontalBar = listView->horizontalScrollBar()) {
        horizontalBar->setValue(horizontalBar->minimum());
    }
    if (QScrollBar* verticalBar = listView->verticalScrollBar()) {
        verticalBar->setValue(verticalBar->minimum());
    }
}

void DashboardView::scrollToTop()
{
    resetDashboardScrollPositions();
}

void DashboardView::paintEvent(QPaintEvent* event)
{
    QWidget::paintEvent(event);

    QPainter painter(this);
    const QColor base = palette().color(QPalette::Window);
    painter.fillRect(rect(), base);

    const qreal radius = qMax(width(), height()) * 0.92;
    QRadialGradient atmosphere(QPointF(width() * 0.72, height() * 0.04),
                              radius);
    atmosphere.setColorAt(
        0.0, QColor(m_dashboardAtmosphere.red(), m_dashboardAtmosphere.green(),
                    m_dashboardAtmosphere.blue(), 86));
    atmosphere.setColorAt(
        0.54, QColor(m_dashboardAtmosphere.red(), m_dashboardAtmosphere.green(),
                     m_dashboardAtmosphere.blue(), 22));
    atmosphere.setColorAt(1.0, QColor(base.red(), base.green(), base.blue(), 0));
    painter.fillRect(rect(), atmosphere);
}

QWidget* DashboardView::createSectionHeader(const QString& title,
                                            const QString& type)
{
    auto* w = new QWidget(this);
    auto* l = new QHBoxLayout(w);
    l->setContentsMargins(0, 0, 0, 0);

    auto* label = new QLabel(title, w);
    label->setObjectName("dashboard-section-title");

    auto* btn = new QPushButton(ButtonLayoutUtils::sectionMoreButtonDisplayText(
                                    tr("See All >")),
                                w);
    btn->setObjectName("section-more-btn");
    btn->setCursor(Qt::PointingHandCursor);
    btn->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    btn->setMinimumWidth(ButtonLayoutUtils::minimumSectionMoreButtonWidth(
        btn->text(), btn->font()));

    connect(btn, &QPushButton::clicked, this,
            [this, type, title]() { Q_EMIT navigateToCategory(type, title); });

    l->addWidget(label);
    l->addStretch();
    l->addWidget(btn);

    return w;
}

QListView* DashboardView::createGridListView(MediaListModel** outModel)
{
    auto* listView = new QListView(this);
    listView->setSelectionMode(QAbstractItemView::NoSelection);
    listView->setViewMode(QListView::IconMode);
    listView->setFlow(QListView::LeftToRight);
    listView->setWrapping(true);
    listView->setSpacing(0);
    listView->setMovement(QListView::Static);
    listView->setResizeMode(QListView::Adjust);
    listView->setUniformItemSizes(true);
    listView->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    listView->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    listView->setFrameShape(QFrame::NoFrame);
    listView->setMouseTracking(true);
    listView->viewport()->setAttribute(Qt::WA_Hover);
    listView->viewport()->installEventFilter(this);
    listView->setStyleSheet(
        "QListView { background: transparent; outline: none; }");

    *outModel = new MediaListModel(500, m_core, this);
    listView->setModel(*outModel);
    listView->setItemDelegate(m_libraryDelegate);
    return listView;
}

bool DashboardView::eventFilter(QObject* obj, QEvent* event)
{
    if (m_libraryListView && obj == m_libraryListView->viewport() &&
        event->type() == QEvent::ToolTip) {
        return TextWrapUtils::showWrappedMediaItemToolTip(m_libraryListView,
                                                          event);
    }

    if (event->type() == QEvent::Wheel) {
        const bool isHorizontalViewport =
            obj->parent() &&
            obj->parent()->property("isHorizontalListView").toBool();
        const bool isMainViewport = (obj == m_mainScrollArea->viewport());

        if (isHorizontalViewport || isMainViewport) {
            auto* we = static_cast<QWheelEvent*>(event);
            if (m_vScrollController) {
                m_vScrollController->scrollByWheelEvent(we, Qt::Vertical);
            }
            return true;
        }
    }

    return QWidget::eventFilter(obj, event);
}

void DashboardView::applyResponsiveLayout()
{
    if (!m_mainScrollArea || !m_dashboardContainer) {
        return;
    }

    const int viewportWidth =
        qMax(m_mainScrollArea->viewport()->width(), width());
    const int viewportHeight =
        qMax(m_mainScrollArea->viewport()->height(), height());
    if (viewportWidth <= 0) {
        return;
    }

    const int containerWidth =
        qMin(viewportWidth, DashboardResponsiveUtils::maxContentWidth());
    if (m_dashboardContainer->minimumWidth() != containerWidth) {
        m_dashboardContainer->setMinimumWidth(containerWidth);
    }

    const auto metrics = DashboardResponsiveUtils::metricsForViewport(
        QSize(containerWidth, viewportHeight));
    const MediaCardDelegate::CardStyle style = dashboardGalleryStyle();
    const bool isTileStyle = style == MediaCardDelegate::LibraryTile;
    if (metrics.contentWidth == m_lastResponsiveContentWidth &&
        metrics.heroHeight == m_lastResponsiveHeroHeight &&
        isTileStyle == m_lastResponsiveTileStyle) {
        return;
    }

    m_lastResponsiveContentWidth = metrics.contentWidth;
    m_lastResponsiveHeroHeight = metrics.heroHeight;
    m_lastResponsiveTileStyle = isTileStyle;

    m_containerLayout->setContentsMargins(
        metrics.horizontalMargin, metrics.topMargin, metrics.horizontalMargin,
        metrics.bottomMargin);
    m_containerLayout->setSpacing(metrics.sectionSpacing);

    if (m_posterStage) {
        m_posterStage->setFixedHeight(metrics.heroHeight);
    }

    const auto applyGalleryMetrics =
        [&](HorizontalListViewGallery* gallery, const QSize& tileSize,
            int galleryHeight) {
            if (!gallery) {
                return;
            }
            gallery->setCardStyle(style);
            gallery->setTileSize(tileSize);
            gallery->setTextPixelSizes(metrics.posterTitlePixels,
                                       metrics.posterSubtitlePixels);
            gallery->setContentPadding(qMax(6, qRound(8 * metrics.scale)));
            gallery->setFixedHeight(galleryHeight);
        };

    const QSize tileSize =
        isTileStyle ? metrics.libraryTile : metrics.posterTile;
    const int galleryHeight =
        isTileStyle ? metrics.libraryGalleryHeight : metrics.galleryHeight;

    applyGalleryMetrics(m_resumeGallery, tileSize, galleryHeight);
    applyGalleryMetrics(m_latestGallery, tileSize, galleryHeight);
    applyGalleryMetrics(m_recommendGallery, tileSize, galleryHeight);
    applyGalleryMetrics(m_completedGallery, tileSize, galleryHeight);

    if (m_libraryGallery) {
        m_libraryGallery->setCardStyle(MediaCardDelegate::LibraryTile);
        m_libraryGallery->setTileSize(metrics.libraryTile);
        m_libraryGallery->setTextPixelSizes(metrics.posterTitlePixels,
                                            metrics.posterSubtitlePixels);
        m_libraryGallery->setContentPadding(
            qMax(6, qRound(8 * metrics.scale)));
        m_libraryGallery->setFixedHeight(metrics.libraryGalleryHeight);
    }

    for (MediaSectionWidget* section : std::as_const(m_libraryGalleries)) {
        if (!section || !section->gallery()) {
            continue;
        }
        if (auto* sectionLayout =
                qobject_cast<QVBoxLayout*>(section->layout())) {
            sectionLayout->setContentsMargins(
                qRound(36 * metrics.scale), qRound(24 * metrics.scale),
                qRound(36 * metrics.scale), 0);
        }
        section->setCardStyle(style);
        section->setTileSize(tileSize);
        section->gallery()->setTextPixelSizes(metrics.posterTitlePixels,
                                              metrics.posterSubtitlePixels);
        section->setGalleryHeight(galleryHeight);
    }

    const QList<QWidget*> headers = {
        m_resumeHeader, m_latestHeader, m_recommendHeader, m_completedHeader};
    for (QWidget* header : headers) {
        applyResponsiveFont(header, QStringLiteral("dashboard-section-title"),
                            metrics.sectionTitlePixels);
        applyResponsiveFont(header, QStringLiteral("section-more-btn"),
                            metrics.sectionMorePixels);
    }
    applyResponsiveFont(m_librarySection,
                        QStringLiteral("dashboard-section-title"),
                        metrics.sectionTitlePixels);
    for (MediaSectionWidget* section : std::as_const(m_libraryGalleries)) {
        applyResponsiveFont(section, QStringLiteral("detail-section-title"),
                            metrics.sectionTitlePixels);
        applyResponsiveFont(section, QStringLiteral("section-more-btn"),
                            metrics.sectionMorePixels);
    }
}

void DashboardView::adjustLibraryGridHeight()
{
    if (!m_libraryListView || !m_libraryModel) {
        return;
    }

    const int count = m_libraryModel->rowCount();
    if (count == 0) {
        m_libraryListView->setFixedHeight(0);
        return;
    }

    m_libraryListView->setSpacing(0);

    const int padding = 40;
    const int scrollBarWidth =
        qApp->style()->pixelMetric(QStyle::PM_ScrollBarExtent);
    const int layoutWidth =
        m_dashboardContainer ? m_dashboardContainer->width() : width();
    int availableWidth = layoutWidth - padding - scrollBarWidth;
    if (availableWidth < 100) {
        availableWidth = 100;
    }

    const int defaultCellWidth = 196;
    const int tolerance = 10;

    int cols = (availableWidth + tolerance) / defaultCellWidth;
    if (cols < 1) {
        cols = 1;
    }

    const int cellWidth = availableWidth / cols;
    const int remainder = availableWidth - (cols * cellWidth);
    const int leftPad = remainder / 2;
    const int rightPad = remainder - leftPad;

    m_libraryListView->setStyleSheet(
        QString("QListView { background: transparent; outline: none; "
                "padding-left: %1px; padding-right: %2px; }")
            .arg(leftPad)
            .arg(rightPad));

    const int imgWidth = cellWidth - 16;
    const int imgHeight = qRound(imgWidth * 9.0 / 16.0);
    const int cellHeight = imgHeight + 16 + 26;

    m_libraryDelegate->setTileSize(QSize(cellWidth, cellHeight));
    m_libraryListView->doItemsLayout();

    const int rows = (count + cols - 1) / cols;
    m_libraryListView->setFixedHeight(rows * cellHeight);
}

void DashboardView::resizeEvent(QResizeEvent* event)
{
    QWidget::resizeEvent(event);
    applyResponsiveLayout();
    adjustLibraryGridHeight();
}

void DashboardView::showEvent(QShowEvent* event)
{
    QWidget::showEvent(event);
    scheduleDashboardReload(0);
}

bool DashboardView::isManageableDashboardLibraryCard(const MediaItem& item) const
{
    if (!m_libraryModel || item.id.trimmed().isEmpty()) {
        return false;
    }

    const QString normalizedCollectionType = item.collectionType.trimmed().toLower();
    if (normalizedCollectionType == "playlists" ||
        normalizedCollectionType == "boxsets") {
        return false;
    }

    const QString normalizedType = item.type.trimmed().toLower();
    if (normalizedType == "playlist" || normalizedType == "boxset") {
        return false;
    }

    for (int row = 0; row < m_libraryModel->rowCount(); ++row) {
        const QModelIndex index = m_libraryModel->index(row, 0);
        if (!index.isValid()) {
            continue;
        }

        if (m_libraryModel->getItem(index).id == item.id) {
            return true;
        }
    }

    return false;
}

CardContextMenuRequest DashboardView::showCardContextMenu(
    const MediaItem& item, const QPoint& globalPos)
{
    const bool canShowLibraryMenu =
        isManageableDashboardLibraryCard(item) && m_core &&
        m_core->serverManager() &&
        m_core->serverManager()->activeProfile().isValid() &&
        m_core->serverManager()->activeProfile().isAdmin;
    if (!canShowLibraryMenu) {
        return BaseView::showCardContextMenu(item, globalPos);
    }

    VirtualFolder folder;
    folder.itemId = item.id;
    folder.name = item.name;

    LibraryActionMenu menu(folder, false, this);
    const LibraryContextMenuRequest libraryRequest = menu.execRequest(globalPos);

    CardContextMenuRequest request;
    switch (libraryRequest.action) {
    case LibraryContextMenuAction::None:
        return request;
    case LibraryContextMenuAction::EditImages:
        request.action = CardContextMenuAction::EditImages;
        return request;
    case LibraryContextMenuAction::RefreshMetadata:
        request.action = CardContextMenuAction::RefreshMetadata;
        return request;
    case LibraryContextMenuAction::ScanLibraryFiles:
        request.action = CardContextMenuAction::ScanLibraryFiles;
        return request;
    }

    return request;
}

void DashboardView::dispatchCardContextMenuRequest(
    const MediaItem& item, const CardContextMenuRequest& request)
{
    if (!request.isValid()) {
        return;
    }

    if (isManageableDashboardLibraryCard(item)) {
        switch (request.action) {
        case CardContextMenuAction::EditImages:
            openDashboardLibraryImageEditor(item);
            return;
        case CardContextMenuAction::RefreshMetadata:
            launchDashboardTask(
                executeDashboardLibraryRefresh(item, true, true, true));
            return;
        case CardContextMenuAction::ScanLibraryFiles:
            launchDashboardTask(
                executeDashboardLibraryRefresh(item, false, false, false));
            return;
        default:
            break;
        }
    }

    BaseView::dispatchCardContextMenuRequest(item, request);
}

void DashboardView::openDashboardLibraryImageEditor(const MediaItem& item)
{
    if (!m_core || !m_core->serverManager() ||
        !m_core->serverManager()->activeProfile().isValid() ||
        !m_core->serverManager()->activeProfile().isAdmin) {
        ModernToast::showMessage(
            BaseView::tr("This action requires administrator privileges"));
        return;
    }

    MediaImageEditTarget target;
    target.itemId = item.id;
    target.imageItemId =
        item.images.primaryImageItemId.isEmpty() ? item.id
                                                 : item.images.primaryImageItemId;
    target.displayName = item.name;
    target.itemType = item.type;
    target.mediaType = item.mediaType;
    target.collectionType = item.collectionType;
    target.isLibrary = true;

    qDebug() << "[DashboardView] Opening dashboard library image editor"
             << "| itemId=" << item.id
             << "| imageItemId=" << target.imageItemId
             << "| itemType=" << item.type
             << "| collectionType=" << item.collectionType;

    MediaImageEditDialog dialog(m_core, target, this);
    dialog.exec();
    if (!dialog.hasChanges()) {
        return;
    }

    if (m_core->mediaService()) {
        m_core->mediaService()->clearUserViewsCache();
    }
    if (m_libraryModel) {
        m_libraryModel->clearImageCache();
    }
    for (MediaSectionWidget* section : std::as_const(m_libraryGalleries)) {
        if (section && section->gallery()) {
            section->gallery()->clearImageCache();
        }
    }

    qDebug() << "[DashboardView] Library image editor applied changes"
             << "| itemId=" << item.id
             << "| imageItemId=" << target.imageItemId;
    scheduleDashboardReload(0);
}

QCoro::Task<void> DashboardView::loadDashboardData()
{
    const int generation = ++m_loadGeneration;
    if (m_dashboardUiCommitTimer) {
        m_dashboardUiCommitTimer->stop();
    }
    m_deferredDashboardUiCommits.clear();
    m_deferredUiCommitGeneration = -1;

    const QString contextKey = currentDashboardContextKey();

    if (contextKey != m_dashboardContextKey) {
        qDebug() << "[DashboardView] Dashboard context changed before reload, "
                    "resetting stale state"
                 << "| hasActiveSession=" << !contextKey.isEmpty()
                 << "| previousHadActiveSession="
                 << !m_dashboardContextKey.isEmpty();
        m_dashboardContextKey = contextKey;
        clearDashboardState(true);
    }

    if (contextKey.isEmpty() || !m_core || !m_core->mediaService()) {
        clearDashboardState(true);
        co_return;
    }

    auto* store = ConfigStore::instance();
    const QString sid = currentServerId();
    const bool showResume = store->get<bool>(
        ConfigKeys::forServer(sid, ConfigKeys::ShowContinueWatching), true);
    const bool showLatest = store->get<bool>(
        ConfigKeys::forServer(sid, ConfigKeys::ShowLatestAdded), true);
    const bool showRecommended = store->get<bool>(
        ConfigKeys::forServer(sid, ConfigKeys::ShowRecommended), true);
    const bool showCompleted = store->get<bool>(
        ConfigKeys::forServer(sid, ConfigKeys::ShowCompletedWatching), false);
    const bool showLibraries = store->get<bool>(
        ConfigKeys::forServer(sid, ConfigKeys::ShowMediaLibraries), true);
    const bool showEachLibrary = store->get<bool>(
        ConfigKeys::forServer(sid, ConfigKeys::ShowEachLibrary), true);

    qDebug() << "[DashboardView] loadDashboardData"
             << "| generation=" << generation << "| serverId=" << sid
             << "| showResume=" << showResume
              << "| showLatest=" << showLatest
              << "| showRecommended=" << showRecommended
              << "| showCompleted=" << showCompleted
              << "| showLibraries=" << showLibraries
              << "| showEachLibrary=" << showEachLibrary;

    applyDashboardSectionOrder();

    m_lastResponsiveContentWidth = -1;
    m_lastResponsiveHeroHeight = -1;
    applyResponsiveLayout();

    
    
    
    
    
    const bool shimmerEnabled =
        store->get<bool>(ConfigKeys::ShimmerAnimation, false);

    if (m_resumeSection) {
        m_resumeSection->setVisible(showResume);
        if (showResume && shimmerEnabled && m_resumeGallery &&
            m_resumeGallery->itemCount() == 0) {
            m_resumeGallery->setLoading(true);
        }
    }
    if (m_latestSection) {
        m_latestSection->setVisible(showLatest);
        if (showLatest && shimmerEnabled && m_latestGallery) {
            m_latestGallery->setLoading(true);
        }
    }
    if (m_recommendSection) {
        m_recommendSection->setVisible(showRecommended);
        if (showRecommended && shimmerEnabled && m_recommendGallery) {
            m_recommendGallery->setLoading(true);
        }
    }
    if (m_completedSection) {
        m_completedSection->setVisible(showCompleted);
        if (showCompleted && shimmerEnabled && m_completedGallery) {
            m_completedGallery->setLoading(true);
        }
    }
    if (m_libraryGridSection) {
        m_libraryGridSection->setVisible(false);
    }
    if (m_librarySection) {
        m_librarySection->setVisible(showLibraries);
    }
    if (m_librarySectionsContainer) {
        m_librarySectionsContainer->setVisible(showEachLibrary);
    }

    if (m_dashboardDeferredLoadTimer) {
        m_dashboardDeferredLoadTimer->stop();
    }

    launchDashboardTask(loadResumeSection(showResume, generation));
    launchDashboardTask(loadRecommendedSection(showRecommended, generation));

    const bool deferLatest = showLatest;
    const bool deferCompleted = showCompleted;
    const bool deferLibraries = showLibraries || showEachLibrary;

    if (!deferLatest && m_latestGallery) {
        m_latestGallery->setLoading(false);
    }
    if (!deferCompleted && m_completedGallery) {
        m_completedGallery->setLoading(false);
    }
    if (!deferLibraries) {
        launchDashboardTask(loadLibrarySections(false, false, generation));
    }

    scheduleDeferredDashboardStage(
        generation, deferLatest, deferCompleted, showLibraries,
        showEachLibrary);
    co_return;
}

void DashboardView::scheduleDeferredDashboardStage(int generation,
                                                   bool showLatest,
                                                   bool showCompleted,
                                                   bool showLibraries,
                                                   bool showEachLibrary)
{
    m_deferredDashboardGeneration = generation;
    m_deferredShowLatest = showLatest;
    m_deferredShowCompleted = showCompleted;
    m_deferredShowLibraries = showLibraries;
    m_deferredShowEachLibrary = showEachLibrary;

    if (!m_dashboardDeferredLoadTimer ||
        (!showLatest && !showCompleted && !showLibraries &&
         !showEachLibrary)) {
        return;
    }

    m_dashboardDeferredLoadTimer->start(
        DashboardLoadStageUtils::deferredDelayMs());
}

QCoro::Task<void> DashboardView::executeDashboardLibraryRefresh(
    MediaItem item, bool replaceAllMetadata, bool replaceAllImages,
    bool isMetadataRefresh)
{
    const QString itemId = item.id.trimmed();
    const QString itemName =
        item.name.trimmed().isEmpty() ? BaseView::tr("this item")
                                      : item.name.trimmed();
    if (itemId.isEmpty() || !m_core || !m_core->adminService() ||
        !m_core->mediaService()) {
        co_return;
    }

    if (!m_core->serverManager() ||
        !m_core->serverManager()->activeProfile().isValid() ||
        !m_core->serverManager()->activeProfile().isAdmin) {
        ModernToast::showMessage(
            BaseView::tr("This action requires administrator privileges"));
        co_return;
    }

    QPointer<DashboardView> safeThis(this);
    QPointer<AdminService> adminService(m_core->adminService());
    QPointer<MediaService> mediaService(m_core->mediaService());
    const char* actionName =
        isMetadataRefresh ? "RefreshMetadata" : "ScanLibraryFiles";

    try {
        qDebug() << "[DashboardView] Starting dashboard library action"
                 << "| action=" << actionName
                 << "| itemId=" << itemId
                 << "| itemName=" << item.name
                 << "| replaceAllMetadata=" << replaceAllMetadata
                 << "| replaceAllImages=" << replaceAllImages;
        co_await adminService->refreshItemMetadata(itemId, replaceAllMetadata,
                                                   replaceAllImages);
        if (!safeThis) {
            co_return;
        }

        ModernToast::showMessage(
            isMetadataRefresh
                ? BaseView::tr("Refreshing metadata for \"%1\"...")
                      .arg(itemName)
                : tr("Scanning \"%1\"...").arg(itemName),
            2000);
    } catch (const std::exception& e) {
        if (!safeThis) {
            co_return;
        }

        qWarning() << "[DashboardView] Dashboard library action failed"
                   << "| action=" << actionName
                   << "| itemId=" << itemId
                   << "| itemName=" << item.name
                   << "| error=" << e.what();
        ModernToast::showMessage(
            isMetadataRefresh
                ? BaseView::tr("Metadata refresh failed: %1")
                      .arg(QString::fromUtf8(e.what()))
                : tr("Scan failed: %1").arg(QString::fromUtf8(e.what())),
            3000);
        co_return;
    }

    try {
        if (!mediaService) {
            co_return;
        }

        const MediaItem latestItem = co_await mediaService->getItemDetail(itemId);
        if (safeThis) {
            safeThis->onMediaItemUpdated(latestItem);
        }
    } catch (const std::exception& e) {
        qWarning() << "[DashboardView] Failed to refresh dashboard library card"
                   << "| action=" << actionName
                   << "| itemId=" << itemId
                   << "| error=" << e.what();
    }

    QTimer::singleShot(2000, this, [this, itemId]() {
        if (!m_core || !m_core->mediaService()) {
            return;
        }

        qDebug() << "[DashboardView] Running delayed dashboard library reload"
                 << "| itemId=" << itemId;
        m_core->mediaService()->clearUserViewsCache();
        scheduleDashboardReload(0);
    });
}

QCoro::Task<void> DashboardView::loadResumeSection(bool show, int generation)
{
    if (!show) {
        if (m_resumeSection) {
            m_resumeSection->setVisible(false);
        }
        co_return;
    }

    QPointer<DashboardView> guard(this);
    QPointer<MediaService> mediaService(m_core->mediaService());
    if (!mediaService) {
        co_return;
    }
    const int requestLimit =
        DashboardRequestLimitUtils::homeSectionRequestLimit(
            currentServerId(), ConfigKeys::ContinueWatchingRequestLimit, 0);

    try {
        QList<MediaItem> rawResumeItems =
            co_await mediaService->getResumeItems(requestLimit);
        if (!guard || !mediaService || m_loadGeneration != generation) {
            co_return;
        }

        QList<MediaItem> resumeItems =
            ResumeItemResolver::buildFallbackItems(
                std::move(rawResumeItems), QStringLiteral("dashboard"));
        if (!guard || m_loadGeneration != generation) {
            co_return;
        }

        const QList<MediaItem> existingResumeItems =
            m_resumeGallery ? m_resumeGallery->items() : QList<MediaItem> {};

        if (resumeItems.isEmpty()) {
            commitDashboardUiChange(generation, [this]() {
                m_resumeGallery->setItems({});
                scheduleHeroPosterWallUpdate();
                m_resumeGallery->setLoading(false);
                if (m_resumeSection) {
                    m_resumeSection->setVisible(false);
                }
            });
            qDebug() << "[DashboardView] resume response is empty"
                     << "| generation=" << generation
                     << "| clearedExisting=" << !existingResumeItems.isEmpty();
            co_return;
        }

        
        
        
        
        if (existingResumeItems.isEmpty()) {
            commitDashboardUiChange(generation, [this, resumeItems]() {
                m_resumeGallery->setItems(resumeItems);
                scheduleHeroPosterWallUpdate();
                if (m_resumeSection) {
                    m_resumeSection->setVisible(!resumeItems.isEmpty());
                }
            });
        }

        qDebug() << "[DashboardView] prepared resume fallbacks"
                 << "| generation=" << generation
                 << "| retainedExisting=" << !existingResumeItems.isEmpty()
                 << "| display=" << resumeItems.size();

        resumeItems = co_await ResumeItemResolver::enrichSeriesCards(
            mediaService.data(), std::move(resumeItems),
            QStringLiteral("dashboard"));
        if (!guard || m_loadGeneration != generation) {
            co_return;
        }

        resumeItems = ResumeItemResolver::preserveExistingResolvedCards(
            std::move(resumeItems), existingResumeItems,
            QStringLiteral("dashboard"));
        commitDashboardUiChange(generation, [this, resumeItems]() {
            m_resumeGallery->setItems(resumeItems);
            scheduleHeroPosterWallUpdate();
            if (m_resumeSection) {
                m_resumeSection->setVisible(!resumeItems.isEmpty());
            }
        });

        qDebug() << "[DashboardView] committed enriched resume items"
                 << "| generation=" << generation
                 << "| display=" << resumeItems.size();
    } catch (const std::exception& e) {
        if (!guard || m_loadGeneration != generation) {
            co_return;
        }

        qDebug() << "Dashboard failed to fetch resume items:" << e.what();
        if (m_resumeGallery) {
            m_resumeGallery->setLoading(false);
        }
        if (m_resumeSection) {
            m_resumeSection->setVisible(
                m_resumeGallery && m_resumeGallery->itemCount() > 0);
        }
    }
}

QCoro::Task<void> DashboardView::loadLatestSection(bool show, int generation)
{
    if (!show) {
        if (m_latestSection) {
            m_latestSection->setVisible(false);
        }
        co_return;
    }

    QPointer<DashboardView> guard(this);
    auto* mediaService = m_core->mediaService();
    const int requestLimit =
        DashboardRequestLimitUtils::homeSectionRequestLimit(
            currentServerId(), ConfigKeys::LatestMediaRequestLimit, 1000);

    try {
        QList<MediaItem> latestItems =
            co_await mediaService->getLatestItems(requestLimit);
        if (!guard || m_loadGeneration != generation) {
            co_return;
        }

        commitDashboardUiChange(generation, [this, latestItems]() {
            m_latestGallery->setItems(latestItems);
            scheduleHeroPosterWallUpdate();
            if (m_latestSection) {
                m_latestSection->setVisible(!latestItems.isEmpty());
            }
        });
    } catch (const std::exception& e) {
        if (!guard || m_loadGeneration != generation) {
            co_return;
        }

        qDebug() << "Dashboard failed to fetch latest items:" << e.what();
        if (m_latestGallery) {
            m_latestGallery->setLoading(false);
        }
        if (m_latestSection) {
            m_latestSection->setVisible(false);
        }
    }
}

QCoro::Task<void> DashboardView::loadRecommendedSection(bool show,
                                                        int generation)
{
    if (!show) {
        if (m_recommendSection) {
            m_recommendSection->setVisible(false);
        }
        co_return;
    }

    QPointer<DashboardView> guard(this);
    auto* mediaService = m_core->mediaService();
    const int requestLimit =
        DashboardRequestLimitUtils::homeSectionRequestLimit(
            currentServerId(), ConfigKeys::RecommendedRequestLimit, 1000);

    try {
        QList<MediaItem> recommendedItems =
            co_await mediaService->getRecommendedMovies(requestLimit);
        if (!guard || m_loadGeneration != generation) {
            co_return;
        }

        commitDashboardUiChange(generation, [this, recommendedItems]() {
            m_recommendGallery->setItems(recommendedItems);
            scheduleHeroPosterWallUpdate();
            if (m_recommendSection) {
                m_recommendSection->setVisible(!recommendedItems.isEmpty());
            }
        });
    } catch (const std::exception& e) {
        if (!guard || m_loadGeneration != generation) {
            co_return;
        }

        qDebug() << "Dashboard failed to fetch recommended items:" << e.what();
        if (m_recommendGallery) {
            m_recommendGallery->setLoading(false);
        }
        if (m_recommendSection) {
            m_recommendSection->setVisible(false);
        }
    }
}

QCoro::Task<void> DashboardView::loadCompletedSection(bool show,
                                                       int generation)
{
    if (!show) {
        if (m_completedSection) {
            m_completedSection->setVisible(false);
        }
        co_return;
    }

    QPointer<DashboardView> guard(this);
    auto* mediaService = m_core->mediaService();
    const int requestLimit =
        DashboardRequestLimitUtils::homeSectionRequestLimit(
            currentServerId(), ConfigKeys::CompletedWatchingRequestLimit, 0);

    try {
        QList<MediaItem> completedItems =
            co_await mediaService->getPlayedItems(requestLimit);
        if (!guard || m_loadGeneration != generation) {
            co_return;
        }

        commitDashboardUiChange(generation, [this, completedItems]() {
            m_completedGallery->setItems(completedItems);
            scheduleHeroPosterWallUpdate();
            if (m_completedSection) {
                m_completedSection->setVisible(!completedItems.isEmpty());
            }
        });
    } catch (const std::exception& e) {
        if (!guard || m_loadGeneration != generation) {
            co_return;
        }

        qDebug() << "Dashboard failed to fetch completed items:" << e.what();
        if (m_completedGallery) {
            m_completedGallery->setLoading(false);
        }
        if (m_completedSection) {
            m_completedSection->setVisible(false);
        }
    }
}

QCoro::Task<void> DashboardView::loadLibrarySections(bool showLibraries,
                                                     bool showEachLibrary,
                                                     int generation)
{
    if (!showLibraries && !showEachLibrary) {
        if (m_librarySection) {
            m_librarySection->setVisible(false);
        }
        clearDashboardGallery(m_libraryGallery);
        if (m_libraryGridSection) {
            m_libraryGridSection->setVisible(false);
        }
        if (m_libraryModel) {
            m_libraryModel->setItems(QList<MediaItem> {});
        }
        clearLibraryGallerySections();
        if (m_librarySectionsContainer) {
            m_librarySectionsContainer->setVisible(false);
        }
        adjustLibraryGridHeight();
        co_return;
    }

    QPointer<DashboardView> guard(this);
    auto* mediaService = m_core->mediaService();
    const QString sid = currentServerId();
    const int previousLibraryCount =
        m_libraryGallery ? m_libraryGallery->itemCount() : 0;
    const int previousEachLibrarySectionCount = m_libraryGalleries.size();
    bool hasReusableEachLibrarySections =
        previousEachLibrarySectionCount > 0;
    if (hasReusableEachLibrarySections) {
        for (MediaSectionWidget* section : std::as_const(m_libraryGalleries)) {
            if (!section || section->property("serverId").toString() != sid) {
                hasReusableEachLibrarySections = false;
                break;
            }
        }
    }

    try {
        const QList<MediaItem> userViews = co_await mediaService->getUserViews();
        if (!guard || m_loadGeneration != generation) {
            co_return;
        }

        qDebug() << "[DashboardView] loadLibrarySections fetched"
                 << "| generation=" << generation
                 << "| showLibraries=" << showLibraries
                 << "| showEachLibrary=" << showEachLibrary
                 << "| previousLibraryCount=" << previousLibraryCount
                 << "| previousEachLibrarySectionCount="
                 << previousEachLibrarySectionCount
                 << "| userViewCount=" << userViews.size();

        if (!showLibraries) {
            if (m_librarySection) {
                m_librarySection->setVisible(false);
            }
            clearDashboardGallery(m_libraryGallery);
            if (m_libraryGridSection) {
                m_libraryGridSection->setVisible(false);
            }
            if (m_libraryModel) {
                m_libraryModel->setItems(QList<MediaItem> {});
            }
            adjustLibraryGridHeight();
        } else {
            if (!userViews.isEmpty() || previousLibraryCount == 0) {
                m_libraryModel->setItems(userViews);
                m_libraryGallery->setItems(userViews);
            } else {
                qDebug()
                    << "[DashboardView] keeping previous library gallery items "
                       "because fetched views are unexpectedly empty";
            }
            if (m_librarySection) {
                m_librarySection->setVisible(!userViews.isEmpty() ||
                                             previousLibraryCount > 0);
            }
            if (m_libraryGridSection) {
                m_libraryGridSection->setVisible(false);
            }
            QTimer::singleShot(0, this, &DashboardView::adjustLibraryGridHeight);
        }

        if (!showEachLibrary) {
            clearLibraryGallerySections();
            if (m_librarySectionsContainer) {
                m_librarySectionsContainer->setVisible(false);
            }
            co_return;
        }

        if (userViews.isEmpty()) {
            if (hasReusableEachLibrarySections) {
                qDebug()
                    << "[DashboardView] keeping previous each-library sections "
                       "because fetched views are unexpectedly empty";
                if (m_librarySectionsContainer) {
                    m_librarySectionsContainer->setVisible(true);
                }
            } else {
                clearLibraryGallerySections();
                if (m_librarySectionsContainer) {
                    m_librarySectionsContainer->setVisible(false);
                }
            }
            co_return;
        }

        const MediaCardDelegate::CardStyle libGalleryStyle =
            dashboardGalleryStyle();
        const int libGalleryHeight = dashboardGalleryHeight();

        bool canReuse = (m_libraryGalleries.size() == userViews.size());
        if (canReuse) {
            for (int i = 0; i < userViews.size(); ++i) {
                if (!m_libraryGalleries[i] ||
                    m_libraryGalleries[i]->property("libraryId").toString() !=
                        userViews[i].id ||
                    m_libraryGalleries[i]->property("libraryName").toString() !=
                        userViews[i].name) {
                    canReuse = false;
                    break;
                }
            }
        }

        if (!canReuse) {
            clearLibraryGallerySections();

            for (const MediaItem& view : userViews) {
                auto* section = new MediaSectionWidget(view.name, m_core,
                                                       m_librarySectionsContainer);
                section->setProperty("serverId", sid);
                section->setProperty("libraryId", view.id);
                section->setProperty("libraryName", view.name);

                if (section->layout()) {
                    section->layout()->setContentsMargins(0, 20, 0, 0);
                }

                auto* seeAllBtn = new QPushButton(tr("查看全部 >"), section);
                seeAllBtn->setObjectName("section-more-btn");
                seeAllBtn->setCursor(Qt::PointingHandCursor);
                seeAllBtn->setSizePolicy(QSizePolicy::Fixed,
                                         QSizePolicy::Fixed);
                seeAllBtn->setMinimumWidth(
                    ButtonLayoutUtils::minimumSectionMoreButtonWidth(
                        seeAllBtn->text(), seeAllBtn->font()));

                const QString libraryId = view.id;
                const QString libraryName = view.name;
                connect(seeAllBtn, &QPushButton::clicked, this,
                        [this, libraryId, libraryName]() {
                            Q_EMIT navigateToLibrary(libraryId, libraryName);
                        });

                QWidget* headerContainer =
                    section->findChild<QWidget*>("section-header");
                if (headerContainer && headerContainer->layout()) {
                    headerContainer->layout()->addWidget(seeAllBtn);
                }

                if (section->gallery() && section->gallery()->listView()) {
                    section->gallery()->listView()->setProperty(
                        "isHorizontalListView", true);
                    section->gallery()->listView()->viewport()->installEventFilter(
                        this);
                }

                connect(section, &MediaSectionWidget::itemClicked, this,
                        [this](const MediaItem& item) {
                            if (isLibraryNavigationItem(item)) {
                                Q_EMIT navigateToLibrary(item.id, item.name);
                            } else {
                                Q_EMIT navigateToDetail(item.id, item.name, item);
                            }
                        });
                connect(section, &MediaSectionWidget::playRequested, this,
                        &BaseView::handlePlayRequested);
                connect(section, &MediaSectionWidget::favoriteRequested, this,
                        &BaseView::handleFavoriteRequested);
                connect(section, &MediaSectionWidget::moreMenuRequested, this,
                        &BaseView::handleMoreMenuRequested);

                m_librarySectionsLayout->addWidget(section);
                m_libraryGalleries.append(section);
            }
        }

        for (int i = 0; i < userViews.size(); ++i) {
            MediaSectionWidget* section = m_libraryGalleries.value(i, nullptr);
            if (!section) {
                continue;
            }

            const MediaItem& view = userViews[i];
            section->setProperty("serverId", sid);
            section->setProperty("libraryId", view.id);
            section->setProperty("libraryName", view.name);
            section->setTitle(view.name);
            section->setCardStyle(libGalleryStyle);
            section->setGalleryHeight(libGalleryHeight);

            const QString libraryId = view.id;
            QPointer<MediaService> sectionMediaService(mediaService);
            QCoro::connect(
                section->loadAsync(
                    [sectionMediaService, libraryId]()
                        -> QCoro::Task<QList<MediaItem>> {
                        if (!sectionMediaService) {
                            co_return {};
                        }

                        co_return co_await sectionMediaService->getLibraryItems(
                            libraryId, "DateCreated", "Descending", "",
                            "Movie,Series", 0, 20, true);
                    }),
                section, []() {});
        }

        if (m_librarySectionsContainer) {
            m_librarySectionsContainer->setVisible(true);
        }
        m_lastResponsiveContentWidth = -1;
        m_lastResponsiveHeroHeight = -1;
        applyResponsiveLayout();
    } catch (const std::exception& e) {
        if (!guard || m_loadGeneration != generation) {
            co_return;
        }

        qDebug() << "[DashboardView] loadLibrarySections failed"
                 << "| generation=" << generation
                 << "| showLibraries=" << showLibraries
                 << "| showEachLibrary=" << showEachLibrary
                 << "| previousLibraryCount=" << previousLibraryCount
                 << "| previousEachLibrarySectionCount="
                 << previousEachLibrarySectionCount
                 << "| error=" << e.what();

        if (showLibraries && previousLibraryCount > 0 && m_librarySection) {
            m_librarySection->setVisible(true);
            QTimer::singleShot(0, this, &DashboardView::adjustLibraryGridHeight);
        }
        if (showEachLibrary && hasReusableEachLibrarySections &&
            m_librarySectionsContainer) {
            m_librarySectionsContainer->setVisible(true);
        }
    }
}

void DashboardView::onMediaItemUpdated(const MediaItem& item)
{
    if (m_resumeGallery) {
        const bool canRemoveFromResume =
            MediaItemUtils::canRemoveFromResume(item);

        if (item.hasResumeContext && !canRemoveFromResume) {
            m_resumeGallery->removeItem(item.id);
        } else {
            m_resumeGallery->updateItem(item);
        }
    }

    if (m_latestGallery) {
        m_latestGallery->updateItem(item);
    }
    if (m_recommendGallery) {
        m_recommendGallery->updateItem(item);
    }
    if (m_completedGallery) {
        const QString sid = currentServerId();
        const bool showCompleted =
            !sid.isEmpty() &&
            ConfigStore::instance()->get<bool>(
                ConfigKeys::forServer(sid, ConfigKeys::ShowCompletedWatching),
                false);
        if (showCompleted && MediaItemUtils::isCompletedWatchingItem(item)) {
            const int completedMaxItems =
                DashboardRequestLimitUtils::homeSectionRequestLimit(
                    sid, ConfigKeys::CompletedWatchingRequestLimit, 0);
            m_completedGallery->prependOrUpdateItem(item, completedMaxItems);
            if (m_completedSection) {
                m_completedSection->setVisible(true);
            }
        } else {
            m_completedGallery->removeItem(item.id);
        }
    }
    if (m_libraryModel) {
        m_libraryModel->updateItem(item);
    }
    if (m_libraryGallery) {
        m_libraryGallery->updateItem(item);
    }

    for (MediaSectionWidget* section : std::as_const(m_libraryGalleries)) {
        if (section) {
            section->updateItem(item);
        }
    }

    scheduleHeroPosterWallUpdate();
}

void DashboardView::onMediaItemRemoved(const QString& itemId)
{
    if (m_resumeGallery) {
        m_resumeGallery->removeItem(itemId);
    }
    if (m_latestGallery) {
        m_latestGallery->removeItem(itemId);
    }
    if (m_recommendGallery) {
        m_recommendGallery->removeItem(itemId);
    }
    if (m_completedGallery) {
        m_completedGallery->removeItem(itemId);
    }
    if (m_libraryModel) {
        m_libraryModel->removeItem(itemId);
    }
    if (m_libraryGallery) {
        m_libraryGallery->removeItem(itemId);
    }

    for (MediaSectionWidget* section : std::as_const(m_libraryGalleries)) {
        if (section && section->gallery()) {
            section->gallery()->removeItem(itemId);
        }
    }

    if (m_recommendSection && m_recommendGallery) {
        m_recommendSection->setVisible(m_recommendGallery->itemCount() > 0);
    }
    if (m_completedSection && m_completedGallery) {
        m_completedSection->setVisible(m_completedGallery->itemCount() > 0);
    }

    scheduleHeroPosterWallUpdate();
}
