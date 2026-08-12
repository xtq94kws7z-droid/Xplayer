#ifndef CATEGORYVIEW_H
#define CATEGORYVIEW_H

#include "../baseview.h" 
#include <models/media/mediaitem.h>
#include <QList>
#include <QPixmap>
#include <QString>
#include <qcorotask.h>

class MediaGridWidget;
class ElidedLabel;
class ModernSortButton;
class QPushButton;
class QLabel;
class QHBoxLayout;
class QScrollArea;
class QResizeEvent;

class QVariantAnimation;

class CategoryView : public BaseView {
    Q_OBJECT
public:
    explicit CategoryView(XplayerCore* core, QWidget *parent = nullptr);
    
    void prepareCategory(const QString& categoryType, const QString& title);
    QCoro::Task<void> loadPreparedCategory();
    
    QCoro::Task<void> loadCategory(const QString& categoryType, const QString& title);

protected:
    void resizeEvent(QResizeEvent* event) override;
    void prepareForStackLeave() override;
    
    void onMediaItemUpdated(const MediaItem& item) override;
    
    
    void onMediaItemRemoved(const QString& itemId) override;

private slots:
    
    QCoro::Task<void> onFilterChanged();

private:
    struct DashboardCategoryQuery {
        QString category;
        QString sortBy;
        QString sortOrder;
        int requestLimit = 0;
        int firstPageSize = 100;
        int pageSize = 300;
    };

    struct DashboardCategoryPage {
        QList<MediaItem> items;
        QString fingerprint;
        int rawItemCount = 0;
        int totalRecordCount = 0;
        bool hasTotalRecordCount = false;
    };

    void setupTopBar(QHBoxLayout* headerLayout);
    void applyResponsiveLayout();
    bool isCastStyleCategory(const QString& categoryType) const;
    bool isProgressiveDashboardCategory(const QString& categoryType) const;
    QString currentViewPreferenceCategoryId() const;
    void applyViewMode(bool isTile);
    void saveViewPreference();
    void restoreViewPreference();
    int dashboardCategoryRequestLimit(const QString& categoryType) const;
    DashboardCategoryQuery buildDashboardCategoryQuery(const QString& sortBy,
                                                       const QString& sortOrder) const;
    QCoro::Task<void> loadDashboardCategoryProgressively(DashboardCategoryQuery query);
    QCoro::Task<DashboardCategoryPage> fetchDashboardCategoryPage(DashboardCategoryQuery query,
                                                                  int startIndex,
                                                                  int limit);
    void appendUniqueLoadedItems(const QList<MediaItem>& items);
    void setLoadedItems(const QList<MediaItem>& items);

    
    QCoro::Task<void> refreshData();

    QString m_currentCategory;
    
    ElidedLabel* m_titleLabel;
    ModernSortButton* m_sortButton; 
    QPushButton* m_viewSwitchBtn;
    QPushButton* m_refreshBtn;   
    QLabel* m_statsLabel;
    QScrollArea* m_headerScrollArea = nullptr;
    QHBoxLayout* m_headerLayout = nullptr;
    
    MediaGridWidget* m_mediaGrid;
    QList<MediaItem> m_loadedItems;
    int m_requestGeneration = 0;

    
    QVariantAnimation* m_refreshAnimation = nullptr;
    QPixmap m_refreshBasePixmap;
    qreal m_refreshIconAngle = -1.0;
};

#endif 
