#ifndef DASHBOARDVIEW_H
#define DASHBOARDVIEW_H

#include "../baseview.h"
#include <QColor>
#include <QList>
#include <QStringList>
#include <functional>
#include <qcorotask.h>

class QListView;
class MediaListModel;
class MediaCardDelegate;
class QLabel;
class QWidget;
class QVBoxLayout;
class QScrollArea;
class QTimer;
class HorizontalListViewGallery;
class MediaSectionWidget;
class PosterStageWidget;
class SmoothScrollController;

class DashboardView : public BaseView
{
    Q_OBJECT
public:
    explicit DashboardView(XplayerCore* core, QWidget *parent = nullptr);
    
    
    QCoro::Task<void> loadDashboardData();

public Q_SLOTS:
    void scrollToTop() override;

Q_SIGNALS:
    
    void navigateToLibrary(const QString& libraryId, const QString& libraryName);

protected:
    
    void onMediaItemUpdated(const MediaItem& item) override;
    
    
    void onMediaItemRemoved(const QString& itemId) override;
    void paintEvent(QPaintEvent* event) override;
    CardContextMenuRequest showCardContextMenu(const MediaItem& item,
                                               const QPoint& globalPos) override;
    void dispatchCardContextMenuRequest(
        const MediaItem& item, const CardContextMenuRequest& request) override;

    void showEvent(QShowEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;
    
    bool eventFilter(QObject* obj, QEvent* event) override;

private:
    void launchDashboardTask(QCoro::Task<void>&& task);
    void scheduleDashboardReload(int delayMs = -1);
    void setupUi();
    void applyDashboardSectionOrder();
    QStringList dashboardSectionOrder() const;
    QString currentServerId() const;
    QString currentDashboardContextKey() const;
    QWidget* sectionWidgetForId(const QString& sectionId) const;
    void clearLibraryGallerySections();
    void clearDashboardState(bool resetScrollPositions);
    void resetDashboardScrollPositions();
    void clearDashboardGallery(HorizontalListViewGallery* gallery);
    void updateHeroPosterWall();
    void scheduleHeroPosterWallUpdate();
    QCoro::Task<void> loadHeroPosterWallCandidates(int generation);
    QCoro::Task<void> enrichHeroPosterWallItems(
        QString contextKey, QList<MediaItem> selectedItems);
    void commitDashboardUiChange(int generation, std::function<void()> commit);
    void flushDeferredDashboardUiCommits();
    bool isDashboardScrollActive() const;
    void scheduleDeferredDashboardStage(int generation,
                                        bool showLatest,
                                        bool showCompleted,
                                        bool showLibraries,
                                        bool showEachLibrary);
    void resetDashboardGalleryScrollPosition(
        HorizontalListViewGallery* gallery) const;
    void resetListViewScrollPosition(QListView* listView) const;
    bool isManageableDashboardLibraryCard(const MediaItem& item) const;
    void openDashboardLibraryImageEditor(const MediaItem& item);

    
    QWidget* createSectionHeader(const QString& title, const QString& type);
    
    QListView* createGridListView(MediaListModel** outModel);
    void applyResponsiveLayout();
    void adjustLibraryGridHeight();

    
    QCoro::Task<void> loadResumeSection(bool show, int generation);
    QCoro::Task<void> loadLatestSection(bool show, int generation);
    QCoro::Task<void> loadRecommendedSection(bool show, int generation);
    QCoro::Task<void> loadCompletedSection(bool show, int generation);
    QCoro::Task<void> loadLibrarySections(bool showLibraries, bool showEachLibrary, int generation);
    QCoro::Task<void> executeDashboardLibraryRefresh(
        MediaItem item, bool replaceAllMetadata, bool replaceAllImages,
        bool isMetadataRefresh);

    QScrollArea* m_mainScrollArea = nullptr; 
    QWidget* m_dashboardContainer = nullptr;

    SmoothScrollController* m_vScrollController = nullptr;

    PosterStageWidget* m_posterStage = nullptr;
    MediaListModel* m_heroPosterWallModel = nullptr;
    QColor m_dashboardAtmosphere = QColor(128, 136, 148);
    QTimer* m_heroPosterWallUpdateTimer = nullptr;
    QTimer* m_dashboardReloadTimer = nullptr;
    QTimer* m_dashboardDeferredLoadTimer = nullptr;
    QTimer* m_dashboardUiCommitTimer = nullptr;
    QList<MediaItem> m_heroPosterWallLibraryCandidates;
    bool m_heroPosterWallLibraryReady = false;
    bool m_heroPosterWallInitialized = false;

    
    QWidget* m_resumeSection = nullptr;
    QWidget* m_resumeHeader = nullptr;
    HorizontalListViewGallery* m_resumeGallery = nullptr;

    
    QWidget* m_latestSection = nullptr;
    QWidget* m_latestHeader = nullptr;
    HorizontalListViewGallery* m_latestGallery = nullptr;

    
    QWidget* m_recommendSection = nullptr;
    QWidget* m_recommendHeader = nullptr;
    HorizontalListViewGallery* m_recommendGallery = nullptr;

    
    QWidget* m_completedSection = nullptr;
    QWidget* m_completedHeader = nullptr;
    HorizontalListViewGallery* m_completedGallery = nullptr;

    
    QWidget* m_libraryGridSection = nullptr;
    QWidget* m_librarySection = nullptr;
    QWidget* m_librarySectionsContainer = nullptr;
    QLabel* m_libraryTitle = nullptr;
    QListView* m_libraryListView = nullptr;
    MediaListModel* m_libraryModel = nullptr;
    HorizontalListViewGallery* m_libraryGallery = nullptr;

    
    QList<MediaSectionWidget*> m_libraryGalleries;
    QVBoxLayout* m_containerLayout = nullptr;
    QVBoxLayout* m_librarySectionsLayout = nullptr;

    
    MediaCardDelegate* m_libraryDelegate = nullptr;

    
    int m_loadGeneration = 0;
    int m_deferredDashboardGeneration = -1;
    bool m_deferredShowLatest = false;
    bool m_deferredShowCompleted = false;
    bool m_deferredShowLibraries = false;
    bool m_deferredShowEachLibrary = false;
    QList<std::function<void()>> m_deferredDashboardUiCommits;
    int m_deferredUiCommitGeneration = -1;
    QString m_dashboardContextKey;
    int m_lastResponsiveContentWidth = -1;
    int m_lastResponsiveHeroHeight = -1;
    bool m_lastResponsiveTileStyle = false;
};

#endif 
