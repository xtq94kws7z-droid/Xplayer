#ifndef SEARCHVIEW_H
#define SEARCHVIEW_H

#include "../baseview.h"
#include <models/media/mediaitem.h>
#include <qcorotask.h>

class MediaGridWidget;
class QLabel;
class QButtonGroup;
class QPushButton;
class QHBoxLayout;
class QScrollArea;
class QResizeEvent;

class SearchView : public BaseView
{
    Q_OBJECT
public:
    explicit SearchView(XplayerCore* core, QWidget *parent = nullptr);

    void prepareSearch(QString query);
    QCoro::Task<void> loadPreparedSearch();
    
    QCoro::Task<void> performSearch(QString query);

protected:
    void resizeEvent(QResizeEvent* event) override;
    void prepareForStackLeave() override;
    
    void onMediaItemUpdated(const MediaItem& item) override;

private slots:
    
    QCoro::Task<void> onTabChanged(int tabId);

    
    void onViewModeChanged();

private:
    void setupUi();
    void setupTopBar(class QHBoxLayout* headerLayout);
    void applyResponsiveLayout();

    QString m_currentQuery;

    
    QList<MediaItem> m_currentItems;

    QLabel* m_titleLabel;
    QLabel* m_statsLabel;

    QButtonGroup* m_tabGroup;
    MediaGridWidget* m_mediaGrid;

    QPushButton* m_btnViewToggle;
    QScrollArea* m_headerScrollArea = nullptr;
    QHBoxLayout* m_headerLayout = nullptr;
    int m_requestGeneration = 0;
};

#endif 
