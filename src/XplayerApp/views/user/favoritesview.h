#ifndef FAVORITESVIEW_H
#define FAVORITESVIEW_H

#include "../baseview.h"
#include "../../utils/favoritesrefreshpolicy.h"
#include <qcorotask.h>

class QListView;
class QLabel;
class QScrollArea;
class HorizontalListViewGallery;
class SmoothScrollController;

class FavoritesView : public BaseView
{
    Q_OBJECT
public:
    explicit FavoritesView(XplayerCore* core, QWidget *parent = nullptr);
    
    
    void requestFavoritesRefresh(bool force = true);

protected:
    
    void onMediaItemUpdated(const MediaItem& item) override;
    void onMediaItemRemoved(const QString& itemId) override;

    void showEvent(QShowEvent* event) override;
    
    bool eventFilter(QObject* obj, QEvent* event) override;

private:
    void setupUi();
    QCoro::Task<void> loadFavoritesData(int generation);

    
    QWidget* createSectionHeader(const QString& title, const QString& itemType = QString());

    QScrollArea* m_mainScrollArea;

    SmoothScrollController* m_vScrollController;

    
    QWidget* m_moviesHeader;
    HorizontalListViewGallery* m_moviesGallery;

    
    QWidget* m_seriesHeader;
    HorizontalListViewGallery* m_seriesGallery;

    
    QWidget* m_collectionsHeader;
    HorizontalListViewGallery* m_collectionsGallery;

    
    QWidget* m_playlistsHeader;
    HorizontalListViewGallery* m_playlistsGallery;

    
    QWidget* m_peopleHeader;
    HorizontalListViewGallery* m_peopleGallery;

    
    QWidget* m_foldersHeader;
    HorizontalListViewGallery* m_foldersGallery;
    FavoritesRefreshPolicy m_refreshPolicy;
};

#endif 
