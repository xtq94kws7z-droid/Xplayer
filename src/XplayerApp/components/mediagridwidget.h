#ifndef MEDIAGRIDWIDGET_H
#define MEDIAGRIDWIDGET_H

#include <QWidget>
#include <QPoint> 
#include <QSize>
#include <QString>
#include <models/media/mediaitem.h>
#include "../views/media/mediacarddelegate.h" 

class XplayerCore;
class QListView;
class MediaListModel;
class ShimmerWidget;
class SmoothScrollController;
class QTimer;

class MediaGridWidget : public QWidget {
    Q_OBJECT
public:
    explicit MediaGridWidget(XplayerCore* core, QWidget* parent = nullptr);
    void setItems(const QList<MediaItem>& items);
    void appendItems(const QList<MediaItem>& items);
    void setBasePadding(int padding);

    
    void setCardStyle(MediaCardDelegate::CardStyle style);

    
    void setLoading(bool loading);
    void suspendImageRequests();
    void resumeImageRequests();

    
    void updateItem(const MediaItem& item);
    void prependOrUpdateItem(const MediaItem& item, int maxItems = 0);
    
    
    void removeItem(const QString& itemId);

    
    int itemCount() const;

    
    int saveScrollPosition() const;
    void restoreScrollPosition(int pos);

Q_SIGNALS:
    void itemClicked(const MediaItem& item);
    void loadMoreRequested();
    
    
    
    
    void playRequested(const MediaItem& item);
    void favoriteRequested(const MediaItem& item);
    void moreMenuRequested(const MediaItem& item, const QPoint& globalPos);

protected:
    void resizeEvent(QResizeEvent* event) override;
    bool eventFilter(QObject* obj, QEvent* event) override;

private:
    void adjustGrid();
    void notifyLoadMoreIfNeeded();
    void updateVisibleImagePriority();
    void scheduleVisibleImagePriorityUpdate();
    void setScrollActive(bool active);
    void clearLastPriorityRange();

    int m_basePadding;
    MediaCardDelegate::CardStyle m_currentStyle;

    XplayerCore* m_core;
    QListView* m_listView;
    MediaListModel* m_listModel;
    MediaCardDelegate* m_listDelegate;

    SmoothScrollController* m_vScrollController;

    
    ShimmerWidget* m_shimmer = nullptr;
    QTimer* m_visiblePriorityTimer = nullptr;
    QTimer* m_scrollIdleTimer = nullptr;
    QString m_lastListViewStyleSheet;
    QSize m_lastTileSize;
    int m_lastImageRequestWidth = -1;
    bool m_scrollActive = false;
    bool m_priorityUpdatePending = false;
    bool m_loading = false;
    int m_lastLoadMoreRequestItemCount = -1;
    int m_lastPriorityFirstRow = -1;
    int m_lastPriorityLastRow = -1;
};

#endif 
