#include "mediasectionwidget.h"
#include "horizontallistviewgallery.h"
#include "../utils/gallerylayoututils.h"
#include <xplayercore.h>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPointer>
#include <QListView>

MediaSectionWidget::MediaSectionWidget(const QString& title, XplayerCore* core, QWidget* parent)
    : QWidget(parent), m_core(core)
{
    setObjectName("media-section");
    setStyleSheet("QWidget#media-section { background: transparent; }");

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(36, 24, 36, 0);
    layout->setSpacing(6);

    
    auto *headerContainer = new QWidget(this);
    headerContainer->setObjectName("section-header");
    headerContainer->setStyleSheet("#section-header { background: transparent; }");
    m_headerLayout = new QHBoxLayout(headerContainer);
    m_headerLayout->setContentsMargins(0, 0, 0, 0);
    m_headerLayout->setSpacing(10);

    m_titleLabel = new QLabel(title, headerContainer);
    m_titleLabel->setObjectName("detail-section-title");
    m_titleLabel->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
    m_titleLabel->setTextFormat(Qt::PlainText);
    m_headerLayout->addWidget(m_titleLabel);
    m_headerLayout->addStretch();

    m_gallery = new HorizontalListViewGallery(core, this);
    m_gallery->listView()->setProperty("isHorizontalListView", true);

    
    connect(m_gallery, &HorizontalListViewGallery::itemClicked, this, &MediaSectionWidget::itemClicked);
    connect(m_gallery, &HorizontalListViewGallery::playRequested, this, &MediaSectionWidget::playRequested);
    connect(m_gallery, &HorizontalListViewGallery::favoriteRequested, this, &MediaSectionWidget::favoriteRequested);
    connect(m_gallery, &HorizontalListViewGallery::moreMenuRequested, this, &MediaSectionWidget::moreMenuRequested);

    layout->addWidget(headerContainer);
    layout->addWidget(m_gallery);

    
    setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Maximum);

    
    hide();
}

HorizontalListViewGallery* MediaSectionWidget::gallery() const {
    return m_gallery;
}

void MediaSectionWidget::setTitle(const QString& title) {
    if (m_titleLabel->text() == title) {
        return;
    }
    m_titleLabel->setText(title);
}

void MediaSectionWidget::setHeaderWidget(QWidget* widget) {
    
    if (m_headerWidget && m_headerWidget != widget) {
        m_headerWidget->hide();
        m_headerWidget = nullptr;
    }
    if (widget) {
        m_headerWidget = widget;
        
        if (m_headerLayout->indexOf(m_headerWidget) < 0) {
            m_headerLayout->insertWidget(1, m_headerWidget);
        }
        m_headerWidget->show();
    }
}

void MediaSectionWidget::setCardStyle(MediaCardDelegate::CardStyle style) {
    m_gallery->setCardStyle(style);
    syncGalleryHeight();
}

void MediaSectionWidget::setGalleryHeight(int height) {
    if (m_requestedGalleryHeight == height) {
        return;
    }
    m_requestedGalleryHeight = height;
    syncGalleryHeight();
}

void MediaSectionWidget::setTileSize(const QSize &size) {
    m_gallery->setTileSize(size);
    syncGalleryHeight();
}

void MediaSectionWidget::syncGalleryHeight() {
    const int height = GalleryLayoutUtils::viewportHeightForCard(
        m_requestedGalleryHeight, m_gallery->cardSize().height());
    if (m_gallery->minimumHeight() == height &&
        m_gallery->maximumHeight() == height) {
        return;
    }
    m_gallery->setFixedHeight(height);
}

void MediaSectionWidget::clear() {
    ++m_loadGeneration;
    m_gallery->setItems(QList<MediaItem>());
    if (!isHidden()) {
        hide();
    }
}

void MediaSectionWidget::setItems(const QList<MediaItem>& items) {
    if (items.isEmpty()) {
        if (!isHidden()) {
            hide();
        }
    } else {
        m_gallery->setItems(items);
        if (isHidden()) {
            show();
        }
    }
}

void MediaSectionWidget::updateItem(const MediaItem& item) {
    m_gallery->updateItem(item);
}


QCoro::Task<void> MediaSectionWidget::loadAsync(FetchFunction fetcher) {
    if (!fetcher) co_return;

    
    QPointer<MediaSectionWidget> safeThis(this);
    const int generation = ++m_loadGeneration;

    try {
        QList<MediaItem> items = co_await fetcher();

        if (safeThis && safeThis->m_loadGeneration == generation) {
            safeThis->setItems(items);
            Q_EMIT safeThis->dataLoaded(items.size());
        }
    } catch(...) {
        
        if (safeThis && safeThis->m_loadGeneration == generation) {
            safeThis->hide();
        }
    }
}
