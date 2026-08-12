#ifndef MEDIALISTMODEL_H
#define MEDIALISTMODEL_H

#include <QAbstractListModel>
#include <models/media/mediaitem.h>
#include <QPixmap>
#include <QHash>
#include <QSet>
#include <QPointer>
#include <QStringList>
#include <QTimer>
#include <qcorotask.h>

class XplayerCore;

class MediaListModel : public QAbstractListModel {
    Q_OBJECT
    friend class MediaListModelTest;
public:
    enum MediaRoles {
        ItemDataRole = Qt::UserRole + 1,
        PosterPixmapRole
    };

    explicit MediaListModel(int imageMaxWidth, XplayerCore* core, QObject *parent = nullptr);
    ~MediaListModel() override;

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;

    void setItems(const QList<MediaItem>& items);
    void appendItems(const QList<MediaItem>& items);
    MediaItem getItem(const QModelIndex& index) const;
    const MediaItem* itemAt(int row) const;
    QList<MediaItem> items() const { return m_items; }

    
    void setPreferThumb(bool prefer);
    void setImageMaxWidth(int maxWidth);
    void setForceNetworkImages(bool forceNetwork);

    
    
    void updateItem(const MediaItem& updatedItem);
    void prependOrUpdateItem(const MediaItem& item, int maxItems = 0);

    
    
    
    void removeItem(const QString& itemId);
    void setPriorityRows(const QList<int>& rows);
    void suspendImageRequests();
    void resumeImageRequests();
    void setScrollActive(bool active);

    
    void clearImageCache();

    
    
    
    
    
    void clearFailedImageItems();

private:
    struct ImageCandidate {
        QString targetImageId;
        QString imageType;
        QString imageTag;
        int maxWidth = 0;
    };

    struct PendingImageRequest {
        QList<ImageCandidate> candidates;
        int candidateIndex = 0;
        int transientRetryCount = 0;
        bool highPriority = false;
        QString imageIdentity;
    };

    QString buildTooltipText(const MediaItem &item) const;
    void ensureImageRequested(const MediaItem& item,
                              bool highPriority = false);
    void enqueueImageFetch(const QString& itemId,
                           const PendingImageRequest& request);
    void enqueueImageRetry(const QString& itemId,
                           const PendingImageRequest& request);
    void scheduleImageFetches();
    QString takeNextPendingImageId();
    void queueImageDataChanged(const QString& itemId);
    void flushPendingImageDataChanges();
    void touchCachedImage(const QString& itemId);
    void removeCachedImage(const QString& itemId);
    void trimImageCache();
    void retryNextImageCandidate(const QString& itemId,
                                 PendingImageRequest request, int generation);
    bool isImageRequestCurrent(const QString& itemId,
                               const PendingImageRequest& request) const;
    void invalidateItemImageRequest(const QString& itemId);

    
    static QCoro::Task<void> executeImageFetch(
        QPointer<MediaListModel> safeThis, QString itemId,
        PendingImageRequest request, int generation, XplayerCore* core);

    bool m_preferThumb = false;
    bool m_forceNetworkImages = false;
    int m_imageMaxWidth;
    XplayerCore* m_core;
    QList<MediaItem> m_items;

    mutable QHash<QString, QPixmap> m_imageCache;
    mutable qsizetype m_imageCacheCostBytes = 0;
    mutable QHash<QString, quint64> m_imageCacheLastUsed;
    mutable quint64 m_imageCacheUseTick = 0;
    mutable QSet<QString> m_loadingImages;
    mutable QSet<QString> m_failedImageItems;
    QHash<QString, PendingImageRequest> m_pendingImageRequests;
    QStringList m_pendingImageOrder;
    QStringList m_priorityImageIds;
    QSet<QString> m_priorityImageIdSet;
    QSet<QString> m_pendingImageNotifyIds;
    QTimer* m_imageNotifyTimer = nullptr;
    int m_activeImageFetches = 0;
    int m_imageRequestGeneration = 0;
    QObject* m_imageRequestContext = nullptr;
    bool m_imageRequestsSuspended = false;
    bool m_scrollActive = false;
};

#endif 
