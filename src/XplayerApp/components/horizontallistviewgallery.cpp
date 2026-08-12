#include "horizontallistviewgallery.h"
#include "../utils/uianimationdefaults.h"
#include "shimmerwidget.h"
#include "../utils/textwraputils.h"
#include "../views/media/medialistmodel.h"
#include <QListView>
#include <QVBoxLayout>
#include <QPushButton>
#include <QAbstractAnimation>
#include <QScrollBar>
#include <QPropertyAnimation>
#include <QEvent>
#include <QHideEvent>
#include <QShowEvent>
#include <QWheelEvent>
#include <QCursor>
#include <QScroller>           
#include <QScrollerProperties> 
#include <QStyleOptionViewItem>
#include <QTimer>
#include <QDebug>
#include <QSet>
#include <algorithm>

namespace {

bool intersectsVisibleAncestorChain(const QWidget* widget)
{
    if (!widget || !widget->isVisible()) {
        return false;
    }

    QRect visibleRect = widget->rect();
    const QWidget* child = widget;
    const QWidget* parent = child->parentWidget();
    while (parent) {
        visibleRect.moveTopLeft(child->mapTo(parent, visibleRect.topLeft()));
        visibleRect = visibleRect.intersected(parent->rect());
        if (visibleRect.isEmpty()) {
            return false;
        }
        child = parent;
        parent = child->parentWidget();
    }
    return true;
}

} 

HorizontalListViewGallery::HorizontalListViewGallery(XplayerCore* core, QWidget* parent)
    : QWidget(parent), m_core(core), m_hScrollAnim(nullptr), m_hScrollTarget(0), m_cardStyle(MediaCardDelegate::Poster)
{
    setObjectName("horizontal-listview-gallery");
    setAttribute(Qt::WA_StyledBackground, true);
    setMouseTracking(true);

    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);

    m_listView = new QListView(this);
    m_listView->setViewMode(QListView::IconMode);
    m_listView->setFlow(QListView::LeftToRight);
    m_listView->setWrapping(false);
    m_listView->setSpacing(0);
    m_listView->setMovement(QListView::Static);
    m_listView->setResizeMode(QListView::Adjust);
    m_listView->setUniformItemSizes(true);

    m_listView->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_listView->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_listView->setFrameShape(QFrame::NoFrame);
    m_listView->setMouseTracking(true);
    m_listView->viewport()->setAttribute(Qt::WA_Hover);
    m_listView->setStyleSheet("QListView { background: transparent; outline: none; border: none; }");
    m_listView->setSelectionMode(QAbstractItemView::NoSelection);

    
    
    
    
    m_listView->setHorizontalScrollMode(QAbstractItemView::ScrollPerPixel);

    
    QScroller::grabGesture(m_listView->viewport(), QScroller::LeftMouseButtonGesture);
    QScroller* scroller = QScroller::scroller(m_listView->viewport());
    QScrollerProperties props = scroller->scrollerProperties();
    
    props.setScrollMetric(QScrollerProperties::VerticalOvershootPolicy, QScrollerProperties::OvershootAlwaysOff);
    props.setScrollMetric(QScrollerProperties::HorizontalOvershootPolicy, QScrollerProperties::OvershootAlwaysOff);
    
    props.setScrollMetric(QScrollerProperties::DragStartDistance, 0.001);
    scroller->setScrollerProperties(props);

    
    m_hScrollAnim = new QPropertyAnimation(m_listView->horizontalScrollBar(), "value", this);
    m_hScrollAnim->setEasingCurve(
        XplayerUi::easingCurve(XplayerUi::MotionCurve::Move));
    m_hScrollAnim->setDuration(
        XplayerUi::durationMs(XplayerUi::MotionDuration::Compact));
    connect(m_hScrollAnim, &QAbstractAnimation::stateChanged, this,
            [this](QAbstractAnimation::State newState,
                   QAbstractAnimation::State) {
                setHorizontalScrollActive(
                    newState == QAbstractAnimation::Running);
                if (newState == QAbstractAnimation::Stopped &&
                    m_scrollIdleTimer) {
                    m_scrollIdleTimer->start();
                }
            });

    m_visiblePriorityTimer = new QTimer(this);
    m_visiblePriorityTimer->setSingleShot(true);
    m_visiblePriorityTimer->setTimerType(Qt::PreciseTimer);
    m_visiblePriorityTimer->setInterval(16);
    connect(m_visiblePriorityTimer, &QTimer::timeout, this,
            &HorizontalListViewGallery::updateVisibleImagePriority);

    m_layoutRefreshTimer = new QTimer(this);
    m_layoutRefreshTimer->setSingleShot(true);
    m_layoutRefreshTimer->setTimerType(Qt::PreciseTimer);
    m_layoutRefreshTimer->setInterval(0);
    connect(m_layoutRefreshTimer, &QTimer::timeout, this,
            &HorizontalListViewGallery::refreshListGeometry);

    m_scrollIdleTimer = new QTimer(this);
    m_scrollIdleTimer->setSingleShot(true);
    m_scrollIdleTimer->setTimerType(Qt::CoarseTimer);
    m_scrollIdleTimer->setInterval(140);
    connect(m_scrollIdleTimer, &QTimer::timeout, this,
            [this]() { setHorizontalScrollActive(false); });

    mainLayout->addWidget(m_listView);

    
    m_shimmer = new ShimmerWidget(this);
    m_shimmer->hide();

    
    
    
    m_listModel = new MediaListModel(400, m_core, this);
    m_listDelegate = new MediaCardDelegate(MediaCardDelegate::Poster, this);

    m_listView->setModel(m_listModel);
    m_listView->setItemDelegate(m_listDelegate);
    m_listView->viewport()->installEventFilter(m_listDelegate);

    
    connect(m_listDelegate, &MediaCardDelegate::playRequested, this, &HorizontalListViewGallery::playRequested);
    connect(m_listDelegate, &MediaCardDelegate::favoriteRequested, this, &HorizontalListViewGallery::favoriteRequested);
    connect(m_listDelegate, &MediaCardDelegate::moreMenuRequested, this, &HorizontalListViewGallery::moreMenuRequested);

    
    connect(m_listView, &QListView::clicked, this, [this](const QModelIndex& index) {
        if (m_listModel) {
            Q_EMIT itemClicked(m_listModel->getItem(index));
        }
    });

    
    
    
    m_btnLeft = new QPushButton("❮", this);
    m_btnRight = new QPushButton("❯", this);
    m_btnLeft->setObjectName("gallery-nav-btn");
    m_btnRight->setObjectName("gallery-nav-btn");

    m_btnLeft->setFixedSize(44, 62);
    m_btnRight->setFixedSize(44, 62);
    m_btnLeft->setCursor(Qt::PointingHandCursor);
    m_btnRight->setCursor(Qt::PointingHandCursor);
    m_btnLeft->setFocusPolicy(Qt::NoFocus);
    m_btnRight->setFocusPolicy(Qt::NoFocus);

    m_btnLeft->hide();
    m_btnRight->hide();

    auto scrollAction = [this](int directionMultiplier) {
        QScrollBar* bar = m_listView->horizontalScrollBar();
        int step = this->width() / 2;
        int targetValue = bar->value() + directionMultiplier * step;
        targetValue = qBound(0, targetValue, bar->maximum());

        if (!m_hScrollAnim || !m_hScrollAnim->targetObject()) {
            return;
        }
        m_hScrollTarget = targetValue;
        setHorizontalScrollActive(true);
        m_hScrollAnim->stop();
        m_hScrollAnim->setStartValue(bar->value());
        m_hScrollAnim->setEndValue(m_hScrollTarget);
        m_hScrollAnim->start();
    };

    connect(m_btnLeft, &QPushButton::clicked, [scrollAction]() { scrollAction(-1); });
    connect(m_btnRight, &QPushButton::clicked, [scrollAction]() { scrollAction(1); });

    connect(m_listView->horizontalScrollBar(), &QScrollBar::valueChanged, this,
            [this]() {
                setHorizontalScrollActive(true);
                if (m_scrollIdleTimer) {
                    m_scrollIdleTimer->start();
                }
                updateButtonsVisibility();
                scheduleVisibleImagePriorityUpdate();
            });
    connect(m_listView->horizontalScrollBar(), &QScrollBar::rangeChanged, this, &HorizontalListViewGallery::updateButtonsVisibility);

    m_listView->viewport()->installEventFilter(this);
    this->installEventFilter(this);
}

HorizontalListViewGallery::~HorizontalListViewGallery()
{
    if (m_hScrollAnim) {
        m_hScrollAnim->stop();
    }
    if (m_listView && m_listView->viewport()) {
        m_listView->viewport()->removeEventFilter(this);
        QScroller::ungrabGesture(m_listView->viewport());
    }
    removeEventFilter(this);
}

QSize HorizontalListViewGallery::cardSize() const
{
    if (!m_listDelegate) {
        return {};
    }

    return m_listDelegate->sizeHint(QStyleOptionViewItem(), QModelIndex());
}


void HorizontalListViewGallery::setItems(const QList<MediaItem>& items)
{
    if (m_listModel) {
        m_listModel->setItems(items);
        QTimer::singleShot(0, this,
                           [this]() { scheduleVisibleImagePriorityUpdate(); });
    }
    
    
    
    if (m_shimmer && !items.isEmpty()) {
        m_shimmer->stopAnimation();
        m_shimmer->hide();
        m_loading = false;
    }
}

void HorizontalListViewGallery::setVerticalScrollActive(bool active)
{
    if (!m_listModel) {
        return;
    }
    m_listModel->setScrollActive(active);
    if (!active) {
        scheduleVisibleImagePriorityUpdate();
    }
}


void HorizontalListViewGallery::updateItem(const MediaItem& item)
{
    if (m_listModel) {
        m_listModel->updateItem(item);
    }
}

void HorizontalListViewGallery::prependOrUpdateItem(const MediaItem& item,
                                                    int maxItems)
{
    if (m_listModel) {
        m_listModel->prependOrUpdateItem(item, maxItems);
        QTimer::singleShot(0, this, [this]() {
            scheduleVisibleImagePriorityUpdate();
            updateButtonsVisibility();
        });
    }
}


void HorizontalListViewGallery::removeItem(const QString& itemId)
{
    if (m_listModel) {
        const int previousCount = m_listModel->rowCount();
        m_listModel->removeItem(itemId);
        if (m_listView && m_listModel->rowCount() < previousCount) {
            m_listView->doItemsLayout();
            m_listView->updateGeometry();
            m_listView->update();
            m_listView->viewport()->update();
            QTimer::singleShot(0, this, [this]() {
                if (!m_listView) {
                    return;
                }

                m_listView->doItemsLayout();
                m_listView->viewport()->update();
                updateButtonsVisibility();
            });
        }
    }
}

int HorizontalListViewGallery::itemCount() const
{
    return m_listModel ? m_listModel->rowCount() : 0;
}

QList<MediaItem> HorizontalListViewGallery::items() const
{
    return m_listModel ? m_listModel->items() : QList<MediaItem> {};
}

void HorizontalListViewGallery::clearImageCache()
{
    if (m_listModel) {
        m_listModel->clearImageCache();
    }
}

void HorizontalListViewGallery::setForceNetworkImages(bool forceNetwork)
{
    if (m_listModel) {
        m_listModel->setForceNetworkImages(forceNetwork);
    }
}

void HorizontalListViewGallery::clearFailedImageItems()
{
    if (m_listModel) {
        m_listModel->clearFailedImageItems();
    }
}

void HorizontalListViewGallery::setCardStyle(MediaCardDelegate::CardStyle style)
{
    if (style == m_cardStyle) {
        return;
    }

    m_cardStyle = style;
    if (m_listDelegate) {
        m_listDelegate->setStyle(style);

        
        if (style == MediaCardDelegate::LibraryTile) {
            int imgHeight = 160;
            int imgWidth = qRound(imgHeight * 16.0 / 9.0); 
            int cardWidth = imgWidth + 16;  
            const int hoverExpandH = qRound(imgHeight * 0.035);
            const int cardHeight =
                8 + imgHeight + hoverExpandH + 6 + 32 + 2 + 18 + 8;
            m_listDelegate->setTileSize(QSize(cardWidth, cardHeight));
        } else if (style == MediaCardDelegate::Poster) {
            m_listDelegate->setTileSize(QSize(160, 296));
        }

        scheduleListGeometryRefresh();
    }
    
    if (m_listModel) {
        m_listModel->setPreferThumb(style == MediaCardDelegate::LibraryTile || style == MediaCardDelegate::EpisodeList);
        updateImageRequestSize();
    }
    
    scheduleListGeometryRefresh();
}

void HorizontalListViewGallery::setTileSize(const QSize &size)
{
    if (size == m_tileSize) {
        return;
    }

    m_tileSize = size;
    if (m_listDelegate) {
        m_listDelegate->setTileSize(size);
        scheduleListGeometryRefresh();
    }
    updateImageRequestSize();
    scheduleListGeometryRefresh();
}

void HorizontalListViewGallery::setTextPixelSizes(int titlePx, int subTitlePx)
{
    if (titlePx == m_titlePixelSize &&
        subTitlePx == m_subTitlePixelSize) {
        return;
    }

    m_titlePixelSize = titlePx;
    m_subTitlePixelSize = subTitlePx;
    if (m_listDelegate) {
        m_listDelegate->setTextPixelSizes(titlePx, subTitlePx);
        scheduleListGeometryRefresh();
    }
}

void HorizontalListViewGallery::setContentPadding(int padding)
{
    const int normalizedPadding = qMax(0, padding);
    if (normalizedPadding == m_contentPadding) {
        return;
    }

    m_contentPadding = normalizedPadding;
    if (m_listDelegate) {
        m_listDelegate->setContentPadding(normalizedPadding);
        scheduleListGeometryRefresh();
    }
    updateImageRequestSize();
    scheduleListGeometryRefresh();
}

void HorizontalListViewGallery::setHoverControls(
    MediaCardDelegate::HoverControls controls)
{
    if (controls == m_hoverControls) {
        return;
    }

    m_hoverControls = controls;
    if (m_listDelegate) {
        m_listDelegate->setHoverControls(controls);
        m_listView->viewport()->update();
    }
}

void HorizontalListViewGallery::scrollToItemId(const QString &itemId)
{
    const QString targetItemId = itemId.trimmed();
    if (targetItemId.isEmpty() || !m_listView || !m_listModel) {
        return;
    }

    QTimer::singleShot(0, this, [this, targetItemId]() {
        if (!m_listView || !m_listModel) {
            return;
        }

        m_listView->doItemsLayout();
        const int rowCount = m_listModel->rowCount();
        for (int row = 0; row < rowCount; ++row) {
            const QModelIndex index = m_listModel->index(row, 0);
            const MediaItem item =
                index.data(MediaListModel::ItemDataRole).value<MediaItem>();
            if (item.id != targetItemId) {
                continue;
            }

            qDebug() << "[HorizontalListViewGallery] Scroll to item"
                     << "| itemId=" << targetItemId
                     << "| row=" << row;
            m_listView->scrollTo(index, QAbstractItemView::PositionAtCenter);
            return;
        }

        qDebug() << "[HorizontalListViewGallery] Scroll target not found"
                 << "| itemId=" << targetItemId
                 << "| rowCount=" << rowCount;
    });
}

void HorizontalListViewGallery::setHighlightedItemId(const QString &id)
{
    if (m_listDelegate) {
        m_listDelegate->setHighlightedItemId(id);
        m_listView->viewport()->update();
    }
}

void HorizontalListViewGallery::setLoading(bool loading)
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
        m_shimmer->setShowSubtitle(
            m_cardStyle == MediaCardDelegate::Poster ||
            m_cardStyle == MediaCardDelegate::Cast);
        m_shimmer->setGeometry(m_listView->geometry());
        m_shimmer->raise();
        m_shimmer->show();
        m_shimmer->startAnimation();
    } else {
        m_shimmer->stopAnimation();
        m_shimmer->hide();
    }
}

void HorizontalListViewGallery::resizeEvent(QResizeEvent* event)
{
    QWidget::resizeEvent(event);
    scheduleListGeometryRefresh();
    scheduleVisibleImagePriorityUpdate();

    
    if (m_shimmer && m_shimmer->isVisible()) {
        const QRect listGeometry = m_listView->geometry();
        if (m_shimmer->geometry() != listGeometry) {
            m_shimmer->setGeometry(listGeometry);
        }
    }
}

void HorizontalListViewGallery::hideEvent(QHideEvent* event)
{
    setHorizontalScrollActive(false);
    if (m_listModel && !m_imageRequestsSuspendedForVisibility) {
        m_listModel->suspendImageRequests();
        m_imageRequestsSuspendedForVisibility = true;
    }
    QWidget::hideEvent(event);
}

void HorizontalListViewGallery::showEvent(QShowEvent* event)
{
    QWidget::showEvent(event);
    if (m_listModel && m_imageRequestsSuspendedForVisibility) {
        m_listModel->resumeImageRequests();
        m_imageRequestsSuspendedForVisibility = false;
    }
    QTimer::singleShot(0, this, [this]() {
        scheduleVisibleImagePriorityUpdate();
        if (m_listView && m_listView->viewport()) {
            m_listView->viewport()->update();
        }
    });
}

void HorizontalListViewGallery::updateButtonPositions()
{
    int currentWidth = this->width();
    
    
    QStyleOptionViewItem dummyOption;
    QModelIndex dummyIndex;
    QSize itemSize = m_listDelegate->sizeHint(dummyOption, dummyIndex);
    
    int padding = m_listDelegate ? m_listDelegate->contentPadding() : 8;
    int imgWidth = itemSize.width() - padding * 2;
    int imgHeight = 0;
    
    
    if (m_cardStyle == MediaCardDelegate::LibraryTile) {
        imgHeight = qRound(imgWidth * 9.0 / 16.0);
    } else {
        
        imgHeight = qRound(imgWidth * 1.5);
    }
    
    
    int imageCenterY = padding + (imgHeight / 2);
    int btnY = imageCenterY - (m_btnLeft->height() / 2);

    m_btnLeft->move(10, btnY);
    m_btnRight->move(currentWidth - m_btnRight->width() - 10, btnY);

    m_btnLeft->raise();
    m_btnRight->raise();
}

bool HorizontalListViewGallery::eventFilter(QObject* obj, QEvent* event)
{
    if (event->type() == QEvent::Enter || event->type() == QEvent::MouseMove) {
        updateButtonsVisibility();
    } else if (event->type() == QEvent::Leave) {
        QPoint globalPos = QCursor::pos();
        if (!this->rect().contains(this->mapFromGlobal(globalPos))) {
            if (m_btnLeft->isVisible()) {
                m_btnLeft->hide();
            }
            if (m_btnRight->isVisible()) {
                m_btnRight->hide();
            }
        }
    } else if (obj == m_listView->viewport() && event->type() == QEvent::ToolTip) {
        return TextWrapUtils::showWrappedMediaItemToolTip(m_listView, event);
    } else if (event->type() == QEvent::Wheel) {
        
        QWheelEvent* wheelEvent = static_cast<QWheelEvent*>(event);
        const QPoint scrollDelta =
            !wheelEvent->pixelDelta().isNull()
                ? wheelEvent->pixelDelta()
                : wheelEvent->angleDelta();
        if (qAbs(scrollDelta.x()) > qAbs(scrollDelta.y())) {
            
            if (!m_listView || !m_hScrollAnim || !m_hScrollAnim->targetObject()) {
                wheelEvent->ignore();
                return false;
            }
            QScrollBar* hBar = m_listView->horizontalScrollBar();
            if (hBar) {
                int currentVal = hBar->value();
                if (m_hScrollAnim->state() == QAbstractAnimation::Running) {
                    currentVal = m_hScrollTarget;
                }
                int step = scrollDelta.x();
                int newTarget = currentVal - step;
                newTarget = qBound(hBar->minimum(), newTarget, hBar->maximum());

                if (newTarget != hBar->value()) {
                    m_hScrollTarget = newTarget;
                    setHorizontalScrollActive(true);
                    m_hScrollAnim->stop();
                    m_hScrollAnim->setStartValue(hBar->value());
                    m_hScrollAnim->setEndValue(m_hScrollTarget);
                    m_hScrollAnim->start();
                }
            }
            return true; 
        } else {
            
            wheelEvent->ignore();
            return false;
        }
    }
    return QWidget::eventFilter(obj, event);
}

void HorizontalListViewGallery::updateButtonsVisibility()
{
    QPoint globalPos = QCursor::pos();
    QPoint localPos = this->mapFromGlobal(globalPos);

    if (!this->rect().contains(localPos)) {
        if (m_btnLeft->isVisible()) {
            m_btnLeft->hide();
        }
        if (m_btnRight->isVisible()) {
            m_btnRight->hide();
        }
        return;
    }

    int currentWidth = this->width();
    QScrollBar* bar = m_listView->horizontalScrollBar();
    if (!bar) {
        if (m_btnLeft->isVisible()) {
            m_btnLeft->hide();
        }
        if (m_btnRight->isVisible()) {
            m_btnRight->hide();
        }
        return;
    }

    bool isLeftHalf = localPos.x() < (currentWidth / 2);

    const bool showLeft = isLeftHalf && bar->value() > 0;
    const bool showRight = !isLeftHalf && bar->value() < bar->maximum();
    if (m_btnLeft->isVisible() != showLeft) {
        m_btnLeft->setVisible(showLeft);
    }
    if (m_btnRight->isVisible() != showRight) {
        m_btnRight->setVisible(showRight);
    }
}

void HorizontalListViewGallery::updateVisibleImagePriority()
{
    if (!m_listView || !m_listModel) {
        return;
    }

    
    
    
    
    if (!intersectsVisibleAncestorChain(this)) {
        m_listModel->setPriorityRows({});
        return;
    }

    QWidget* viewport = m_listView->viewport();
    if (!viewport) {
        return;
    }

    QStyleOptionViewItem option;
    const QSize cellSize = m_listDelegate->sizeHint(option, QModelIndex());
    const int cellWidth = qMax(1, cellSize.width());
    const int rowCount = m_listModel->rowCount();
    if (rowCount <= 0) {
        m_listModel->setPriorityRows({});
        return;
    }
    const int scrollValue =
        m_listView->horizontalScrollBar()
            ? m_listView->horizontalScrollBar()->value()
            : 0;
    const int firstRow = qBound(0, scrollValue / cellWidth - 2,
                                qMax(0, rowCount - 1));
    const int visibleCount = viewport->width() / cellWidth + 5;
    const int lastRow = qBound(firstRow, firstRow + visibleCount,
                               qMax(0, rowCount - 1));

    QList<int> rows;
    rows.reserve(lastRow - firstRow + 1);
    for (int row = firstRow; row <= lastRow; ++row) {
        rows.append(row);
    }
    m_listModel->setPriorityRows(rows);
}

void HorizontalListViewGallery::scheduleVisibleImagePriorityUpdate()
{
    if (!m_visiblePriorityTimer || m_visiblePriorityTimer->isActive() ||
        !isVisible()) {
        return;
    }
    m_visiblePriorityTimer->start();
}

void HorizontalListViewGallery::scheduleListGeometryRefresh()
{
    if (!m_layoutRefreshTimer || m_layoutRefreshTimer->isActive()) {
        return;
    }
    m_layoutRefreshTimer->start();
}

void HorizontalListViewGallery::refreshListGeometry()
{
    if (!m_listView || !m_listDelegate) {
        return;
    }

    m_listView->doItemsLayout();
    if (m_listView->viewport()) {
        m_listView->viewport()->update();
    }
    updateButtonPositions();
    updateButtonsVisibility();
    scheduleVisibleImagePriorityUpdate();
}

void HorizontalListViewGallery::updateImageRequestSize()
{
    if (!m_listModel || !m_listDelegate) {
        return;
    }

    QStyleOptionViewItem option;
    const QSize cardSize = m_listDelegate->sizeHint(option, QModelIndex());
    const int displayWidth =
        qMax(1, cardSize.width() - m_listDelegate->contentPadding() * 2);
    const int requestWidth = qBound(
        160, qRound(displayWidth * devicePixelRatioF()), 768);
    m_listModel->setImageMaxWidth(requestWidth);
}

void HorizontalListViewGallery::setHorizontalScrollActive(bool active)
{
    if (m_horizontalScrollActive == active) {
        return;
    }

    m_horizontalScrollActive = active;
    if (active && m_scrollIdleTimer) {
        m_scrollIdleTimer->stop();
    }
    if (m_listModel) {
        m_listModel->setScrollActive(active);
    }
}
