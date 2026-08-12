#ifndef POSTERSTAGEWIDGET_H
#define POSTERSTAGEWIDGET_H

#include <QColor>
#include <QHash>
#include <QList>
#include <QMetaObject>
#include <QPersistentModelIndex>
#include <QPixmap>
#include <QWidget>

#include <models/media/mediaitem.h>

class QAbstractItemModel;
class QGraphicsOpacityEffect;
class QHideEvent;
class QLabel;
class QPushButton;
class QPropertyAnimation;
class QShowEvent;
class QTimer;
class QVariantAnimation;

class PosterStageWidget final : public QWidget
{
    Q_OBJECT
    friend class PosterWallUtilsTest;

public:
    explicit PosterStageWidget(QWidget* parent = nullptr);

    void setModel(QAbstractItemModel* model);
    void setItems(const QList<MediaItem>& items);
    QList<MediaItem> items() const;
    bool hasContent() const;
    void setReducedMotion(bool enabled);

Q_SIGNALS:
    void itemActivated(const MediaItem& item);
    void atmosphereChanged(const QColor& color);

protected:
    void enterEvent(QEnterEvent* event) override;
    void leaveEvent(QEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void paintEvent(QPaintEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;
    void showEvent(QShowEvent* event) override;
    void hideEvent(QHideEvent* event) override;

private:
    void advance(int direction);
    void refreshCurrentItem();
    void updateOverlayGeometry();
    void updateControls();
    void updateAtmosphere();
    void updateBackdrop(bool animate = false);
    void updateVisiblePosterIndices();
    bool shouldRunRotation() const;
    void updateRotationTimer();
    void invalidatePosterCache(const QModelIndex& topLeft,
                               const QModelIndex& bottomRight);
    bool modelRangeContainsCurrentItem(const QModelIndex& topLeft,
                                       const QModelIndex& bottomRight) const;
    void updateModelConnection();
    void setCurrentIndex(int index, bool animated);
    QModelIndex modelIndexForItem(const MediaItem& item) const;
    QPixmap pixmapForItem(const MediaItem& item) const;

    QAbstractItemModel* m_model = nullptr;
    QMetaObject::Connection m_modelDataChangedConnection;
    QMetaObject::Connection m_modelResetConnection;
    QList<MediaItem> m_items;
    int m_currentIndex = -1;
    QColor m_atmosphere = QColor(128, 136, 148);
    bool m_reducedMotion = false;
    bool m_hovering = false;
    QPixmap m_backdropPixmap;
    QPixmap m_previousBackdropPixmap;
    QList<int> m_visiblePosterIndices;
    QHash<int, QRect> m_posterHitRects;
    mutable QHash<QString, QPixmap> m_posterPixmapCache;
    mutable QHash<QString, QPersistentModelIndex> m_modelIndexCache;
    qint64 m_backdropSourceCacheKey = 0;
    qreal m_backdropBlend = 1.0;
    qreal m_visualCenter = 0.0;
    qreal m_lastResponsiveScale = -1.0;
    int m_slideDirection = 0;

    QLabel* m_eyebrowLabel = nullptr;
    QWidget* m_infoPanel = nullptr;
    QLabel* m_titleLabel = nullptr;
    QLabel* m_metaLabel = nullptr;
    QLabel* m_overviewLabel = nullptr;
    QPushButton* m_previousButton = nullptr;
    QPushButton* m_nextButton = nullptr;
    QPushButton* m_playButton = nullptr;
    QWidget* m_dotsWidget = nullptr;

    QTimer* m_rotationTimer = nullptr;
    QTimer* m_backdropRefreshTimer = nullptr;
    QGraphicsOpacityEffect* m_overlayOpacity = nullptr;
    QPropertyAnimation* m_overlayAnimation = nullptr;
    QVariantAnimation* m_backdropAnimation = nullptr;
    QVariantAnimation* m_slideAnimation = nullptr;
};

#endif
