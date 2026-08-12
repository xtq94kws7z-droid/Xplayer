#include "mediagridwidget.h"
#include "shimmerwidget.h"
#include "../utils/smoothscrollcontroller.h"
#include "../utils/textwraputils.h"
#include "../utils/uianimationdefaults.h"
#include "../utils/xplayerresponsiveutils.h"
#include "../views/media/medialistmodel.h"
#include <QVBoxLayout>
#include <QListView>
#include <QResizeEvent>
#include <QApplication>
#include <QStyle>
#include <QScroller>
#include <QScrollerProperties>
#include <QWheelEvent>
#include <QScrollBar>
#include <QSet>
#include <QStyleOptionViewItem>
#include <QTimer>
#include <algorithm>

namespace {
constexpr int kActiveScrollPriorityUpdateMs = 48;
}

MediaGridWidget::MediaGridWidget(XplayerCore* core, QWidget* parent)
    : QWidget(parent), m_core(core), m_basePadding(20), m_currentStyle(MediaCardDelegate::Poster),
    m_vScrollController(nullptr)
{
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);

    m_listView = new QListView(this);
    m_listView->setSelectionMode(QAbstractItemView::NoSelection);
    m_listView->setViewMode(QListView::IconMode);
    m_listView->setResizeMode(QListView::Adjust);
    m_listView->setMovement(QListView::Static);
    m_listView->setSpacing(0);
    m_listView->setUniformItemSizes(true);
    m_listView->setWrapping(true);
    m_listView->setFrameShape(QFrame::NoFrame);
    m_listView->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_listView->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    m_listView->setMouseTracking(true);
    
    
    m_listView->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    m_listView->viewport()->setAttribute(Qt::WA_Hover);

    
    
    
    
    m_listView->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);

    
    QScroller::grabGesture(m_listView->viewport(), QScroller::LeftMouseButtonGesture);
    QScroller* scroller = QScroller::scroller(m_listView->viewport());
    QScrollerProperties props = scroller->scrollerProperties();
    
    props.setScrollMetric(QScrollerProperties::VerticalOvershootPolicy, QScrollerProperties::OvershootAlwaysOff);
    props.setScrollMetric(QScrollerProperties::HorizontalOvershootPolicy, QScrollerProperties::OvershootAlwaysOff);
    
    props.setScrollMetric(QScrollerProperties::DragStartDistance, 0.001);
    scroller->setScrollerProperties(props);

    m_vScrollController =
        new SmoothScrollController(m_listView->verticalScrollBar(), this);
    m_vScrollController->setDuration(XplayerUi::kScrollAnimationMs);

    m_visiblePriorityTimer = new QTimer(this);
    m_visiblePriorityTimer->setSingleShot(true);
    m_visiblePriorityTimer->setTimerType(Qt::PreciseTimer);
    m_visiblePriorityTimer->setInterval(16);
    connect(m_visiblePriorityTimer, &QTimer::timeout, this,
            &MediaGridWidget::updateVisibleImagePriority);

    m_scrollIdleTimer = new QTimer(this);
    m_scrollIdleTimer->setSingleShot(true);
    m_scrollIdleTimer->setTimerType(Qt::CoarseTimer);
    m_scrollIdleTimer->setInterval(140);
    connect(m_scrollIdleTimer, &QTimer::timeout, this, [this]() {
        setScrollActive(false);
        m_priorityUpdatePending = false;
        scheduleVisibleImagePriorityUpdate();
    });

    
    m_listView->viewport()->installEventFilter(this);

    m_listModel = new MediaListModel(400, m_core, this);
    m_listDelegate = new MediaCardDelegate(m_currentStyle, this);

    m_listView->setModel(m_listModel);
    m_listView->setItemDelegate(m_listDelegate);
    m_listView->viewport()->installEventFilter(m_listDelegate);
    connect(m_vScrollController, &SmoothScrollController::scrollingChanged,
            this, [this](bool active) {
                setScrollActive(active);
                if (active) {
                    if (m_scrollIdleTimer) {
                        m_scrollIdleTimer->start();
                    }
                } else {
                    m_priorityUpdatePending = false;
                    scheduleVisibleImagePriorityUpdate();
                }
            });
    layout->addWidget(m_listView);

    
    m_shimmer = new ShimmerWidget(this);
    m_shimmer->hide();

    connect(m_listView, &QListView::clicked, this, [this](const QModelIndex& index) {
        Q_EMIT itemClicked(m_listModel->getItem(index));
    });
    connect(m_listView->verticalScrollBar(), &QScrollBar::valueChanged, this,
            [this](int) {
                setScrollActive(true);
                if (m_scrollIdleTimer) {
                    m_scrollIdleTimer->start();
                }
                notifyLoadMoreIfNeeded();
                scheduleVisibleImagePriorityUpdate();
            });

    
    
    
    connect(m_listDelegate, &MediaCardDelegate::playRequested, this, &MediaGridWidget::playRequested);
    connect(m_listDelegate, &MediaCardDelegate::favoriteRequested, this, &MediaGridWidget::favoriteRequested);
    connect(m_listDelegate, &MediaCardDelegate::moreMenuRequested, this, &MediaGridWidget::moreMenuRequested);
    
}

void MediaGridWidget::setBasePadding(int padding) {
    if (m_basePadding == padding) {
        return;
    }
    m_basePadding = padding;
    adjustGrid();
}

void MediaGridWidget::setCardStyle(MediaCardDelegate::CardStyle style) {
    if (m_currentStyle == style) return;
    m_currentStyle = style;
    m_lastListViewStyleSheet.clear();
    m_lastTileSize = QSize();
    m_lastImageRequestWidth = -1;

    
    m_listDelegate->setStyle(style);
    const bool preferThumb =
        style == MediaCardDelegate::LibraryTile ||
        style == MediaCardDelegate::EpisodeList;
    m_listModel->setPreferThumb(preferThumb);
    m_listModel->clearImageCache();
    clearLastPriorityRange();

    
    if (style == MediaCardDelegate::EpisodeList) {
        m_listView->setViewMode(QListView::ListMode);
        m_listView->setFlow(QListView::TopToBottom);
        m_listView->setWrapping(false);
        m_listView->setSpacing(5); 
    } else {
        m_listView->setViewMode(QListView::IconMode);
        m_listView->setFlow(QListView::LeftToRight);
        m_listView->setWrapping(true);
        m_listView->setSpacing(0);
    }

    adjustGrid();
    scheduleVisibleImagePriorityUpdate();
}

void MediaGridWidget::setLoading(bool loading)
{
    if (!m_shimmer) {
        return;
    }

    if (loading == m_loading) {
        if (loading && m_shimmer->geometry() != m_listView->geometry()) {
            m_shimmer->setGeometry(m_listView->geometry());
        }
        return;
    }
    m_loading = loading;

    if (loading) {
        
        QStyleOptionViewItem opt;
        const QSize cardSize = m_listDelegate->sizeHint(opt, QModelIndex());
        m_shimmer->setCardSize(cardSize);
        m_shimmer->setShowSubtitle(false);
        m_shimmer->setGeometry(m_listView->geometry());
        m_shimmer->raise();
        m_shimmer->show();
        m_shimmer->startAnimation();
    } else {
        m_shimmer->stopAnimation();
        m_shimmer->hide();
    }
}

void MediaGridWidget::suspendImageRequests()
{
    if (m_listModel) {
        m_listModel->suspendImageRequests();
    }
}

void MediaGridWidget::resumeImageRequests()
{
    if (m_listModel) {
        m_listModel->resumeImageRequests();
    }
}

void MediaGridWidget::setItems(const QList<MediaItem>& items) {
    m_listModel->setItems(items);
    clearLastPriorityRange();
    m_lastLoadMoreRequestItemCount = -1;
    adjustGrid();
    
    
    if (m_shimmer && !items.isEmpty()) {
        m_shimmer->stopAnimation();
        m_shimmer->hide();
        m_loading = false;
    }
    if (!items.isEmpty()) {
        QMetaObject::invokeMethod(
            this,
            [this]() {
                notifyLoadMoreIfNeeded();
                scheduleVisibleImagePriorityUpdate();
            },
            Qt::QueuedConnection);
    }
}

void MediaGridWidget::appendItems(const QList<MediaItem>& items)
{
    if (items.isEmpty() || !m_listModel) {
        return;
    }

    m_listModel->appendItems(items);
    clearLastPriorityRange();
    m_lastLoadMoreRequestItemCount = -1;
    if (m_shimmer) {
        m_shimmer->stopAnimation();
        m_shimmer->hide();
        m_loading = false;
    }

    QMetaObject::invokeMethod(
        this,
        [this]() {
            notifyLoadMoreIfNeeded();
            scheduleVisibleImagePriorityUpdate();
        },
        Qt::QueuedConnection);
}




void MediaGridWidget::updateItem(const MediaItem& item) {
    if (m_listModel) {
        m_listModel->updateItem(item);
    }
}

void MediaGridWidget::prependOrUpdateItem(const MediaItem& item, int maxItems) {
    if (m_listModel) {
        m_listModel->prependOrUpdateItem(item, maxItems);
        clearLastPriorityRange();
        m_lastLoadMoreRequestItemCount = -1;
        QMetaObject::invokeMethod(
            this,
            [this]() {
                notifyLoadMoreIfNeeded();
                scheduleVisibleImagePriorityUpdate();
            },
            Qt::QueuedConnection);
    }
}


void MediaGridWidget::removeItem(const QString& itemId) {
    if (m_listModel) {
        m_listModel->removeItem(itemId);
        clearLastPriorityRange();
        m_lastLoadMoreRequestItemCount = -1;
    }
}


int MediaGridWidget::itemCount() const {
    return m_listModel ? m_listModel->rowCount() : 0;
}


int MediaGridWidget::saveScrollPosition() const {
    if (m_listView && m_listView->verticalScrollBar())
        return m_listView->verticalScrollBar()->value();
    return 0;
}


void MediaGridWidget::restoreScrollPosition(int pos) {
    if (m_listView && m_listView->verticalScrollBar()) {
        if (m_vScrollController) {
            m_vScrollController->scrollTo(pos, false);
        } else {
            m_listView->verticalScrollBar()->setValue(pos);
        }
    }
}

void MediaGridWidget::resizeEvent(QResizeEvent *event) {
    QWidget::resizeEvent(event);
    adjustGrid();
    
    if (m_shimmer && m_shimmer->isVisible()) {
        const QRect listGeometry = m_listView->geometry();
        if (m_shimmer->geometry() != listGeometry) {
            m_shimmer->setGeometry(listGeometry);
        }
    }
    notifyLoadMoreIfNeeded();
    scheduleVisibleImagePriorityUpdate();
}

bool MediaGridWidget::eventFilter(QObject* obj, QEvent* event) {
    if (obj == m_listView->viewport() && event->type() == QEvent::ToolTip) {
        return TextWrapUtils::showWrappedMediaItemToolTip(m_listView, event);
    }

    
    if (event->type() == QEvent::Wheel && obj == m_listView->viewport()) {
        QWheelEvent* we = static_cast<QWheelEvent*>(event);
        if (qAbs(we->angleDelta().y()) >= qAbs(we->angleDelta().x())) {
            if (m_vScrollController) {
                m_vScrollController->scrollByWheelEvent(we, Qt::Vertical);
            }
            return true;
        }
    }
    return QWidget::eventFilter(obj, event);
}

void MediaGridWidget::notifyLoadMoreIfNeeded()
{
    if (!m_listView || !m_listModel || m_listModel->rowCount() <= 0) {
        return;
    }

    QScrollBar* vBar = m_listView->verticalScrollBar();
    if (!vBar) {
        return;
    }

    constexpr int kLoadMoreThreshold = 160;
    const int remaining = vBar->maximum() - vBar->value();
    const int rowCount = m_listModel->rowCount();
    if ((vBar->maximum() <= 0 || remaining <= kLoadMoreThreshold) &&
        m_lastLoadMoreRequestItemCount != rowCount) {
        m_lastLoadMoreRequestItemCount = rowCount;
        Q_EMIT loadMoreRequested();
    }
}

void MediaGridWidget::updateVisibleImagePriority()
{
    m_priorityUpdatePending = false;

    if (!m_listView || !m_listModel) {
        return;
    }

    if (!isVisible() || visibleRegion().isEmpty()) {
        m_listModel->setPriorityRows({});
        clearLastPriorityRange();
        return;
    }

    QWidget* viewport = m_listView->viewport();
    if (!viewport) {
        return;
    }

    const int rowCount = m_listModel->rowCount();
    if (rowCount <= 0) {
        m_listModel->setPriorityRows({});
        clearLastPriorityRange();
        return;
    }

    const QRect rect = viewport->rect();

    QStyleOptionViewItem option;
    const QSize cellSize = m_listDelegate->sizeHint(option, QModelIndex());
    const int columns = qMax(1, rect.width() / qMax(1, cellSize.width()));

    int firstRow = rowCount;
    int lastRow = -1;
    const QList<QPoint> samplePoints = {
        QPoint(rect.left() + 1, rect.top() + 1),
        QPoint(rect.center().x(), rect.top() + 1),
        QPoint(rect.right() - 1, rect.top() + 1),
        QPoint(rect.left() + 1, rect.bottom() - 1),
        QPoint(rect.center().x(), rect.bottom() - 1),
        QPoint(rect.right() - 1, rect.bottom() - 1),
        rect.center()
    };

    for (const QPoint& point : samplePoints) {
        const QModelIndex idx = m_listView->indexAt(point);
        if (idx.isValid()) {
            firstRow = qMin(firstRow, idx.row());
            lastRow = qMax(lastRow, idx.row());
        }
    }

    if (lastRow < 0) {
        m_listModel->setPriorityRows({});
        clearLastPriorityRange();
        return;
    }

    const int prefetchRows = columns * 2;
    firstRow = qMax(0, firstRow - prefetchRows);
    lastRow = qMin(rowCount - 1, lastRow + prefetchRows);
    if (firstRow == m_lastPriorityFirstRow &&
        lastRow == m_lastPriorityLastRow) {
        return;
    }
    m_lastPriorityFirstRow = firstRow;
    m_lastPriorityLastRow = lastRow;

    QList<int> rows;
    rows.reserve(lastRow - firstRow + 1);
    for (int row = firstRow; row <= lastRow; ++row) {
        rows.append(row);
    }
    m_listModel->setPriorityRows(rows);
}

void MediaGridWidget::scheduleVisibleImagePriorityUpdate()
{
    if (!m_visiblePriorityTimer || !isVisible()) {
        return;
    }
    if (m_scrollActive) {
        m_priorityUpdatePending = true;
        if (!m_visiblePriorityTimer->isActive()) {
            m_visiblePriorityTimer->start(kActiveScrollPriorityUpdateMs);
        }
        return;
    }
    if (m_visiblePriorityTimer->isActive()) {
        return;
    }
    m_visiblePriorityTimer->setInterval(16);
    m_visiblePriorityTimer->start();
}

void MediaGridWidget::setScrollActive(bool active)
{
    if (m_scrollActive == active) {
        return;
    }

    m_scrollActive = active;
    if (m_listModel) {
        m_listModel->setScrollActive(active);
    }
}

void MediaGridWidget::clearLastPriorityRange()
{
    m_lastPriorityFirstRow = -1;
    m_lastPriorityLastRow = -1;
}

void MediaGridWidget::adjustGrid() {
    if (!m_listView || !m_listModel) return;
    const qreal scale = XplayerResponsiveUtils::scaleForViewport(size());

    int scrollBarWidth = qApp->style()->pixelMetric(QStyle::PM_ScrollBarExtent);
    int availableWidth = this->width() - scrollBarWidth;
    if (availableWidth < 100) availableWidth = 100;

    
    if (m_currentStyle == MediaCardDelegate::EpisodeList) {
        int padding = XplayerResponsiveUtils::scaled(m_basePadding, scale, 12, 42);
        const QString listViewStyle =
            QString("QListView { background: transparent; border: none; outline: none; padding-left: %1px; padding-right: 0px; }").arg(padding);
        
        
        
        
        int cellWidth = availableWidth - padding - padding; 
        if (cellWidth < 100) cellWidth = 100;
        
        const QSize tileSize(
            cellWidth, XplayerResponsiveUtils::scaled(160, scale, 126, 232));
        const int requestWidth = qBound(
            160, qRound(250 * devicePixelRatioF()), 768);
        bool layoutChanged = false;
        if (m_lastListViewStyleSheet != listViewStyle) {
            m_listView->setStyleSheet(listViewStyle);
            m_lastListViewStyleSheet = listViewStyle;
            layoutChanged = true;
        }
        if (m_lastTileSize != tileSize) {
            m_listDelegate->setTileSize(tileSize);
            m_lastTileSize = tileSize;
            layoutChanged = true;
        }
        if (m_lastImageRequestWidth != requestWidth) {
            m_listModel->setImageMaxWidth(requestWidth);
            m_lastImageRequestWidth = requestWidth;
        }
        if (layoutChanged) {
            m_listView->doItemsLayout();
        }
    } else {
        const int effectivePadding =
            XplayerResponsiveUtils::scaled(m_basePadding, scale, 12, 42);
        availableWidth -= (effectivePadding * 2);
        int defaultCellWidth =
            XplayerResponsiveUtils::scaled(150, scale, 122, 218);
        if (m_currentStyle == MediaCardDelegate::LibraryTile) {
            defaultCellWidth =
                XplayerResponsiveUtils::scaled(250, scale, 198, 362);
        } else if (m_currentStyle == MediaCardDelegate::Cast) {
            defaultCellWidth =
                XplayerResponsiveUtils::scaled(140, scale, 112, 203);
        }

        int tolerance = 5;
        int cols = (availableWidth + tolerance) / defaultCellWidth;
        if (cols < 1) cols = 1;

        int cellWidth = availableWidth / cols;
        int remainder = availableWidth - (cols * cellWidth);
        int leftPad = effectivePadding + remainder / 2;

        const QString listViewStyle =
            QString("QListView { background: transparent; border: none; outline: none; padding-left: %1px; padding-right: 0px; }").arg(leftPad);

        int imgWidth = cellWidth - 16;
        int imgHeight = imgWidth;

        if (m_currentStyle == MediaCardDelegate::Poster || m_currentStyle == MediaCardDelegate::Cast) {
            imgHeight = qRound(imgWidth * 1.5);        
        } else if (m_currentStyle == MediaCardDelegate::LibraryTile) {
            imgHeight = qRound(imgWidth * 9.0 / 16.0); 
        }

        int cellHeight = imgHeight + XplayerResponsiveUtils::scaled(60, scale, 48, 86);
        const QSize tileSize(cellWidth, cellHeight);
        const int requestWidth = qBound(
            160, qRound(imgWidth * devicePixelRatioF()), 768);
        bool layoutChanged = false;
        if (m_lastListViewStyleSheet != listViewStyle) {
            m_listView->setStyleSheet(listViewStyle);
            m_lastListViewStyleSheet = listViewStyle;
            layoutChanged = true;
        }
        if (m_lastTileSize != tileSize) {
            m_listDelegate->setTileSize(tileSize);
            m_lastTileSize = tileSize;
            layoutChanged = true;
        }
        if (m_lastImageRequestWidth != requestWidth) {
            m_listModel->setImageMaxWidth(requestWidth);
            m_lastImageRequestWidth = requestWidth;
        }
        if (layoutChanged) {
            m_listView->doItemsLayout();
        }
    }
}




























