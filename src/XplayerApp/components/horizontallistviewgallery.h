#ifndef HORIZONTALLISTVIEWGALLERY_H
#define HORIZONTALLISTVIEWGALLERY_H

#include <QWidget>
#include <QPoint>
#include <QList>
#include <QSize>
#include <models/media/mediaitem.h>

class ShimmerWidget;
#include "../views/media/mediacarddelegate.h"

class XplayerCore;
class QListView;
class QPushButton;
class QPropertyAnimation;
class QTimer;
class MediaListModel; 

class HorizontalListViewGallery : public QWidget
{
    Q_OBJECT
public:
    
    explicit HorizontalListViewGallery(XplayerCore* core, QWidget* parent = nullptr);
    ~HorizontalListViewGallery() override;

    
    QListView* listView() const { return m_listView; }
    QSize cardSize() const;

    
    void setItems(const QList<MediaItem>& items);
    void setVerticalScrollActive(bool active);
    void updateItem(const MediaItem& item);
    void prependOrUpdateItem(const MediaItem& item, int maxItems = 0);
    
    
    void removeItem(const QString& itemId);
    int itemCount() const;
    QList<MediaItem> items() const;
    void clearImageCache();
    void setForceNetworkImages(bool forceNetwork);
    
    
    void clearFailedImageItems();

    
    void setCardStyle(MediaCardDelegate::CardStyle style);

    
    void setTileSize(const QSize &size);
    void setTextPixelSizes(int titlePx, int subTitlePx);
    void setContentPadding(int padding);
    void setHoverControls(MediaCardDelegate::HoverControls controls);
    void scrollToItemId(const QString &itemId);

    
    void setHighlightedItemId(const QString &id);

    
    void setLoading(bool loading);

    
    QSize minimumSizeHint() const override {
        return QSize(1, QWidget::minimumSizeHint().height());
    }

Q_SIGNALS:
    
    void itemClicked(const MediaItem& item);
    void playRequested(const MediaItem& item);
    void favoriteRequested(const MediaItem& item);
    void moreMenuRequested(const MediaItem& item, const QPoint& globalPos);

protected:
    void resizeEvent(QResizeEvent* event) override;
    void hideEvent(QHideEvent* event) override;
    void showEvent(QShowEvent* event) override;
    bool eventFilter(QObject* obj, QEvent* event) override;

private:
    void updateButtonsVisibility();
    void updateButtonPositions();
    void updateVisibleImagePriority();
    void scheduleVisibleImagePriorityUpdate();
    void scheduleListGeometryRefresh();
    void refreshListGeometry();
    void updateImageRequestSize();
    void setHorizontalScrollActive(bool active);

    XplayerCore* m_core;
    QListView* m_listView;
    QPushButton* m_btnLeft;
    QPushButton* m_btnRight;

    
    MediaListModel* m_listModel;
    MediaCardDelegate* m_listDelegate;

    
    QPropertyAnimation* m_hScrollAnim;
    int m_hScrollTarget;

    
    MediaCardDelegate::CardStyle m_cardStyle;
    QSize m_tileSize;
    int m_titlePixelSize = -1;
    int m_subTitlePixelSize = -1;
    int m_contentPadding = -1;
    MediaCardDelegate::HoverControls m_hoverControls =
        MediaCardDelegate::HoverControlAll;

    
    ShimmerWidget* m_shimmer = nullptr;
    bool m_imageRequestsSuspendedForVisibility = false;
    bool m_loading = false;
    bool m_horizontalScrollActive = false;
    QTimer* m_visiblePriorityTimer = nullptr;
    QTimer* m_layoutRefreshTimer = nullptr;
    QTimer* m_scrollIdleTimer = nullptr;
};

#endif 
