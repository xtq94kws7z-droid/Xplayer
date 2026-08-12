#include "medialistmodel.h"
#include "../../utils/mediaimagecandidateutils.h"
#include "../../utils/modelnotificationutils.h"
#include "../../utils/pixmapcachebudgetutils.h"
#include <xplayercore.h>
#include <services/media/mediaservice.h>
#include <config/configstore.h>
#include <config/config_keys.h>
#include <QSet>
#include <QVector>
#include <QMutableHashIterator>
#include <QMutableSetIterator>
#include <algorithm>
#include <exception>
#include <limits>
#include <utility>

namespace {




constexpr int kMaxConcurrentImageFetches = 4;
constexpr int kMaxConcurrentScrollingImageFetches = 2;
constexpr int kMaxCachedImages = 256;
constexpr qsizetype kMaxCachedImageBytes = 64 * 1024 * 1024;

QString buildImageIdentity(const MediaItem& item)
{
    return QStringLiteral("%1|%2|%3|%4|%5|%6|%7|%8|%9")
        .arg(item.getPrimaryImageId())
        .arg(item.images.primaryTag)
        .arg(item.images.thumbTag)
        .arg(item.images.backdropTag)
        .arg(item.images.logoTag)
        .arg(item.images.parentPrimaryTag)
        .arg(item.images.parentThumbTag)
        .arg(item.images.parentBackdropTag)
        .arg(item.images.parentImageItemId);
}

bool isResumeSourceForItem(const MediaItem& item, const QString& sourceItemId)
{
    const QString trimmedSourceItemId = sourceItemId.trimmed();
    if (trimmedSourceItemId.isEmpty()) {
        return false;
    }

    const QString trimmedResumeItemId = item.resumeItemId.trimmed();
    return item.hasResumeContext && !trimmedResumeItemId.isEmpty() &&
           trimmedResumeItemId == trimmedSourceItemId;
}

bool matchesItemOrResumeSource(const MediaItem& item, const QString& itemId)
{
    const QString trimmedItemId = itemId.trimmed();
    if (trimmedItemId.isEmpty()) {
        return false;
    }

    return item.id.trimmed() == trimmedItemId ||
           isResumeSourceForItem(item, trimmedItemId);
}

bool isDisplayEquivalent(const MediaItem& currentItem,
                         const MediaItem& newItem)
{
    return currentItem.id == newItem.id &&
           currentItem.name == newItem.name &&
           currentItem.type == newItem.type &&
           currentItem.mediaType == newItem.mediaType &&
           currentItem.collectionType == newItem.collectionType &&
           currentItem.seriesName == newItem.seriesName &&
           currentItem.productionYear == newItem.productionYear &&
           currentItem.overview == newItem.overview &&
           currentItem.premiereDate == newItem.premiereDate &&
           currentItem.runTimeTicks == newItem.runTimeTicks &&
           currentItem.recursiveItemCount == newItem.recursiveItemCount &&
           currentItem.parentIndexNumber == newItem.parentIndexNumber &&
           currentItem.indexNumber == newItem.indexNumber &&
           currentItem.userData.isFavorite == newItem.userData.isFavorite &&
           currentItem.isResumeDisplayFallback ==
               newItem.isResumeDisplayFallback &&
           currentItem.hasResumeContext == newItem.hasResumeContext &&
           currentItem.resumeItemId == newItem.resumeItemId &&
           currentItem.resumeUserData.isFavorite ==
               newItem.resumeUserData.isFavorite &&
           currentItem.resumeUserData.playedPercentage ==
               newItem.resumeUserData.playedPercentage &&
           currentItem.resumeUserData.playbackPositionTicks ==
               newItem.resumeUserData.playbackPositionTicks &&
           currentItem.resumeUserData.played == newItem.resumeUserData.played;
}

void preserveResumeContext(MediaItem& target, const MediaItem& source)
{
    if (target.hasResumeContext || !source.hasResumeContext) {
        return;
    }

    target.hasResumeContext = true;
    target.resumeItemId = source.resumeItemId;
    target.resumeUserData = source.resumeUserData;
}

MediaItem mergeItemUpdate(const MediaItem& currentItem,
                          const MediaItem& updatedItem)
{
    if (currentItem.id == updatedItem.id) {
        MediaItem mergedItem = updatedItem;
        preserveResumeContext(mergedItem, currentItem);
        return mergedItem;
    }

    if (isResumeSourceForItem(currentItem, updatedItem.id)) {
        MediaItem mergedItem = currentItem;
        mergedItem.resumeUserData = updatedItem.userData;
        return mergedItem;
    }

    return updatedItem;
}

} 

MediaListModel::MediaListModel(int imageMaxWidth, XplayerCore* core, QObject *parent)
    : QAbstractListModel(parent), m_imageMaxWidth(imageMaxWidth), m_core(core)
{
    m_imageRequestContext = new QObject(this);
    m_imageNotifyTimer = new QTimer(this);
    m_imageNotifyTimer->setSingleShot(true);
    m_imageNotifyTimer->setTimerType(Qt::PreciseTimer);
    connect(m_imageNotifyTimer, &QTimer::timeout, this,
            &MediaListModel::flushPendingImageDataChanges);
}

MediaListModel::~MediaListModel()
{
    ++m_imageRequestGeneration;
    m_loadingImages.clear();
    m_pendingImageRequests.clear();
    m_pendingImageOrder.clear();
    m_priorityImageIds.clear();
    m_priorityImageIdSet.clear();
    m_activeImageFetches = 0;
    QObject* requestContext = m_imageRequestContext;
    m_imageRequestContext = nullptr;
    delete requestContext;
}

void MediaListModel::suspendImageRequests()
{
    const int activeCount = m_activeImageFetches;
    const int pendingCount = m_pendingImageRequests.size();
    m_imageRequestsSuspended = true;
    ++m_imageRequestGeneration;

    
    
    
    QObject* previousRequestContext = m_imageRequestContext;
    m_imageRequestContext = new QObject(this);
    delete previousRequestContext;

    m_loadingImages.clear();
    m_pendingImageRequests.clear();
    m_pendingImageOrder.clear();
    m_priorityImageIds.clear();
    m_priorityImageIdSet.clear();
    m_pendingImageNotifyIds.clear();
    m_activeImageFetches = 0;
    if (m_imageNotifyTimer) {
        m_imageNotifyTimer->stop();
    }

    if (activeCount > 0 || pendingCount > 0) {
        qDebug() << "[MediaListModel] cancelled image request generation"
                 << "| active=" << activeCount
                 << "| pending=" << pendingCount
                 << "| cached=" << m_imageCache.size()
                 << "| generation=" << m_imageRequestGeneration;
    }
}

void MediaListModel::resumeImageRequests()
{
    m_imageRequestsSuspended = false;
}

void MediaListModel::clearImageCache()
{
    if (m_imageCache.isEmpty() && m_loadingImages.isEmpty() &&
        m_pendingImageRequests.isEmpty() && m_failedImageItems.isEmpty() &&
        m_activeImageFetches == 0 && m_imageCacheCostBytes == 0 &&
        m_imageCacheLastUsed.isEmpty()) {
        return;
    }

    const bool hasRequests = !m_loadingImages.isEmpty() ||
                             !m_pendingImageRequests.isEmpty() ||
                             m_activeImageFetches > 0;
    if (hasRequests) {
        const bool wasSuspended = m_imageRequestsSuspended;
        suspendImageRequests();
        m_imageRequestsSuspended = wasSuspended;
    }
    m_imageCache.clear();
    m_imageCacheCostBytes = 0;
    m_imageCacheLastUsed.clear();
    m_imageCacheUseTick = 0;
    m_failedImageItems.clear();
}

void MediaListModel::setPreferThumb(bool prefer)
{
    if (m_preferThumb == prefer) {
        return;
    }

    m_preferThumb = prefer;
    if (!m_items.isEmpty()) {
        clearImageCache();
    }
}

void MediaListModel::clearFailedImageItems()
{
    m_failedImageItems.clear();
}

int MediaListModel::rowCount(const QModelIndex &parent) const {
    if (parent.isValid()) {
        return 0;
    }
    return m_items.size();
}

QVariant MediaListModel::data(const QModelIndex &index, int role) const {
    if (!index.isValid() || index.row() >= m_items.size()) {
        return QVariant();
    }
    
    const MediaItem& item = m_items.at(index.row());

    if (role == ItemDataRole) {
        return QVariant::fromValue(item);
    }
    else if (role == Qt::ToolTipRole) {
        if (!ConfigStore::instance()->get<bool>(ConfigKeys::ShowMediaTooltips,
                                                true)) {
            return QVariant();
        }
        return buildTooltipText(item);
    }
    else if (role == PosterPixmapRole) {
        if (m_imageCache.contains(item.id)) {
            const_cast<MediaListModel*>(this)->touchCachedImage(item.id);
            return m_imageCache.value(item.id);
        }

        if (m_scrollActive && !m_priorityImageIds.contains(item.id)) {
            return QVariant();
        }
        const_cast<MediaListModel*>(this)->ensureImageRequested(item, true);
        return QVariant(); 
    }
    return QVariant();
}

void MediaListModel::setImageMaxWidth(int maxWidth)
{
    const int normalizedWidth = qBound(96, maxWidth, 1024);
    if (normalizedWidth == m_imageMaxWidth) {
        return;
    }

    if (normalizedWidth > m_imageMaxWidth &&
        normalizedWidth <= m_imageMaxWidth * 6 / 5) {
        return;
    }

    const bool requiresHigherResolution = normalizedWidth > m_imageMaxWidth;
    m_imageMaxWidth = normalizedWidth;
    if (requiresHigherResolution && !m_items.isEmpty()) {
        clearImageCache();
    }
}

void MediaListModel::setForceNetworkImages(bool forceNetwork)
{
    if (m_forceNetworkImages == forceNetwork) {
        return;
    }

    m_forceNetworkImages = forceNetwork;
    if (!m_items.isEmpty()) {
        clearImageCache();
    }
}

QString MediaListModel::buildTooltipText(const MediaItem &item) const {
    QStringList lines;

    if ((item.type == QStringLiteral("Episode") ||
         item.type == QStringLiteral("Season")) &&
        !item.seriesName.isEmpty()) {
        lines << item.seriesName;
    }

    QString title = item.name.trimmed();
    if (item.type == QStringLiteral("Episode") && item.parentIndexNumber >= 0 &&
        item.indexNumber >= 0) {
        title = QStringLiteral("S%1E%2  %3")
                    .arg(item.parentIndexNumber, 2, 10, QChar('0'))
                    .arg(item.indexNumber, 2, 10, QChar('0'))
                    .arg(item.name);
    }
    lines << title;

    QStringList metaParts;
    if (item.type == QStringLiteral("Season") && item.parentIndexNumber >= 0) {
        metaParts << tr("Season %1").arg(item.parentIndexNumber);
    }
    if (item.productionYear > 0) {
        metaParts << QString::number(item.productionYear);
    }
    if (!item.premiereDate.trimmed().isEmpty()) {
        const QString trimmedDate = item.premiereDate.trimmed();
        const int len = trimmedDate.length();
        if (len > 0) metaParts << trimmedDate.left(qMin(len, 10));
    }
    if (item.runTimeTicks > 0) {
        const long long minutes = item.runTimeTicks / 10000000 / 60;
        if (minutes > 0) {
            metaParts << tr("%1 min").arg(minutes);
        }
    }
    if (item.type == QStringLiteral("Season") && item.recursiveItemCount > 0) {
        metaParts << tr("%1 Episodes").arg(item.recursiveItemCount);
    }
    if (!metaParts.isEmpty()) {
        lines << metaParts.join(QStringLiteral(" • "));
    }

    const QString overview = item.overview.simplified();
    if (!overview.isEmpty()) {
        lines << overview;
    }

    return lines.join(QLatin1Char('\n'));
}


void MediaListModel::setItems(const QList<MediaItem>& newItems) {
    if (m_items.isEmpty() || newItems.isEmpty()) {
        beginResetModel();
        m_items = newItems;
        if (newItems.isEmpty()) {
            clearImageCache();
        }
        endResetModel();
        return;
    }

    QSet<QString> newIds;
    newIds.reserve(newItems.size());
    for (const auto& item : newItems) {
        newIds.insert(item.id);
    }

    const bool pureAppend = newItems.size() > m_items.size();
    bool existingPrefixUnchanged = pureAppend;
    if (pureAppend) {
        for (int i = 0; i < m_items.size(); ++i) {
            if (m_items.at(i).id != newItems.at(i).id) {
                existingPrefixUnchanged = false;
                break;
            }
        }
    }

    if (existingPrefixUnchanged) {
        for (int i = 0; i < m_items.size(); ++i) {
            const MediaItem& newItem = newItems.at(i);
            const bool imageChanged =
                buildImageIdentity(m_items.at(i)) != buildImageIdentity(newItem);
            const bool displayChanged =
                !isDisplayEquivalent(m_items.at(i), newItem);
            m_items[i] = newItem;
            QVector<int> roles;
            if (displayChanged) {
                roles.append(ItemDataRole);
            }
            if (imageChanged) {
                invalidateItemImageRequest(newItem.id);
                roles.append(PosterPixmapRole);
            }
            if (!roles.isEmpty()) {
                Q_EMIT dataChanged(index(i, 0), index(i, 0), roles);
            }
        }
        appendItems(newItems.mid(m_items.size()));
        return;
    }

    const bool sameShape = newItems.size() == m_items.size();
    bool sameOrder = sameShape;
    if (sameOrder) {
        for (int i = 0; i < m_items.size(); ++i) {
            if (m_items.at(i).id != newItems.at(i).id) {
                sameOrder = false;
                break;
            }
        }
    }

    if (sameOrder) {
        for (int i = 0; i < newItems.size(); ++i) {
            const MediaItem& newItem = newItems.at(i);
            const bool imageChanged =
                buildImageIdentity(m_items.at(i)) != buildImageIdentity(newItem);
            const bool displayChanged =
                !isDisplayEquivalent(m_items.at(i), newItem);
            m_items[i] = newItem;
            QVector<int> roles;
            if (displayChanged) {
                roles.append(ItemDataRole);
            }
            if (imageChanged) {
                invalidateItemImageRequest(newItem.id);
                roles.append(PosterPixmapRole);
            }
            if (!roles.isEmpty()) {
                Q_EMIT dataChanged(index(i, 0), index(i, 0), roles);
            }
        }
        return;
    }

    QHash<QString, QString> previousImageIdentityById;
    previousImageIdentityById.reserve(m_items.size());
    for (const MediaItem& item : std::as_const(m_items)) {
        previousImageIdentityById.insert(item.id, buildImageIdentity(item));
    }
    QHash<QString, QString> newImageIdentityById;
    newImageIdentityById.reserve(newItems.size());
    for (const MediaItem& item : std::as_const(newItems)) {
        newImageIdentityById.insert(item.id, buildImageIdentity(item));
    }

    beginResetModel();
    m_items = newItems;

    QMutableHashIterator<QString, QPixmap> cacheIt(m_imageCache);
    while (cacheIt.hasNext()) {
        cacheIt.next();
        const auto previousIdentityIt =
            previousImageIdentityById.constFind(cacheIt.key());
        const auto newIdentityIt = newImageIdentityById.constFind(cacheIt.key());
        const bool stillPresent = newIdentityIt != newImageIdentityById.constEnd();
        bool imageChanged = false;
        if (stillPresent && previousIdentityIt != previousImageIdentityById.constEnd()) {
            imageChanged = previousIdentityIt.value() != newIdentityIt.value();
        }
        if (!stillPresent || imageChanged) {
            m_imageCacheCostBytes = qMax<qsizetype>(
                0, m_imageCacheCostBytes -
                       PixmapCacheBudgetUtils::costBytes(cacheIt.value()));
            m_imageCacheLastUsed.remove(cacheIt.key());
            cacheIt.remove();
        }
    }

    QMutableSetIterator<QString> failedIt(m_failedImageItems);
    while (failedIt.hasNext()) {
        if (!newIds.contains(failedIt.next())) {
            failedIt.remove();
        }
    }

    for (auto it = m_pendingImageRequests.begin();
         it != m_pendingImageRequests.end();) {
        if (!newIds.contains(it.key())) {
            m_pendingImageOrder.removeAll(it.key());
            m_loadingImages.remove(it.key());
            it = m_pendingImageRequests.erase(it);
        } else {
            ++it;
        }
    }
    m_priorityImageIds.removeIf(
        [&newIds](const QString& itemId) { return !newIds.contains(itemId); });
    endResetModel();
}

void MediaListModel::appendItems(const QList<MediaItem>& items)
{
    if (items.isEmpty()) {
        return;
    }

    QSet<QString> existingIds;
    existingIds.reserve(m_items.size());
    for (const MediaItem& item : std::as_const(m_items)) {
        existingIds.insert(item.id);
    }

    QList<MediaItem> appendItems;
    appendItems.reserve(items.size());
    for (const MediaItem& item : items) {
        if (item.id.isEmpty() || existingIds.contains(item.id)) {
            continue;
        }
        existingIds.insert(item.id);
        appendItems.append(item);
    }

    if (appendItems.isEmpty()) {
        return;
    }

    const int firstRow = m_items.size();
    const int lastRow = firstRow + appendItems.size() - 1;
    beginInsertRows(QModelIndex(), firstRow, lastRow);
    m_items.append(appendItems);
    endInsertRows();
}

MediaItem MediaListModel::getItem(const QModelIndex& index) const {
    if (!index.isValid() || index.row() >= m_items.size()) {
        return MediaItem();
    }
    return m_items.at(index.row());
}

const MediaItem* MediaListModel::itemAt(int row) const
{
    if (row < 0 || row >= m_items.size()) {
        return nullptr;
    }
    return &m_items[row];
}

void MediaListModel::setScrollActive(bool active)
{
    if (m_scrollActive == active) {
        return;
    }

    m_scrollActive = active;
    if (!m_scrollActive) {
        if (m_imageNotifyTimer) {
            m_imageNotifyTimer->stop();
        }
        flushPendingImageDataChanges();
        scheduleImageFetches();
    }
}

void MediaListModel::updateItem(const MediaItem& updatedItem) {
    for (int i = 0; i < m_items.size(); ++i) {
        if (matchesItemOrResumeSource(m_items[i], updatedItem.id)) {
            const QString cachedItemId = m_items[i].id;
            const MediaItem mergedItem = mergeItemUpdate(m_items[i], updatedItem);
            const bool imageChanged =
                buildImageIdentity(m_items[i]) != buildImageIdentity(mergedItem);

            
            m_items[i] = mergedItem;

            if (imageChanged) {
                invalidateItemImageRequest(cachedItemId);
            }
            
            
            QModelIndex idx = index(i, 0);
            QVector<int> roles = {ItemDataRole};
            if (imageChanged) {
                roles.append(PosterPixmapRole);
            }
            Q_EMIT dataChanged(idx, idx, roles);
            break; 
        }
    }
}

void MediaListModel::prependOrUpdateItem(const MediaItem& item, int maxItems)
{
    const QString itemId = item.id.trimmed();
    if (itemId.isEmpty()) {
        return;
    }

    for (int i = 0; i < m_items.size(); ++i) {
        if (matchesItemOrResumeSource(m_items[i], itemId)) {
            updateItem(item);
            return;
        }
    }

    beginInsertRows(QModelIndex(), 0, 0);
    m_items.prepend(item);
    endInsertRows();

    if (maxItems > 0) {
        while (m_items.size() > maxItems) {
            const int removeIndex = m_items.size() - 1;
            const QString cachedItemId = m_items.at(removeIndex).id;
            beginRemoveRows(QModelIndex(), removeIndex, removeIndex);
            m_items.removeAt(removeIndex);
            endRemoveRows();
            removeCachedImage(cachedItemId);
            m_loadingImages.remove(cachedItemId);
        }
    }
}




void MediaListModel::removeItem(const QString& itemId) {
    for (int i = 0; i < m_items.size(); ++i) {
        if (matchesItemOrResumeSource(m_items[i], itemId)) {
            const QString cachedItemId = m_items[i].id;
            
            beginRemoveRows(QModelIndex(), i, i);
            
            
            m_items.removeAt(i);
            
            
            endRemoveRows();

            
            removeCachedImage(cachedItemId);
            m_loadingImages.remove(cachedItemId);
            m_failedImageItems.remove(cachedItemId);
            removeCachedImage(itemId);
            m_loadingImages.remove(itemId);
            m_failedImageItems.remove(itemId);
            
            break; 
        }
    }
}

void MediaListModel::setPriorityRows(const QList<int>& rows)
{
    QStringList priorityIds;
    priorityIds.reserve(rows.size());
    QSet<QString> priorityIdSet;
    priorityIdSet.reserve(rows.size());

    for (const int row : rows) {
        if (row < 0 || row >= m_items.size()) {
            continue;
        }

        const MediaItem& item = m_items.at(row);
        if (item.id.isEmpty() || priorityIdSet.contains(item.id)) {
            continue;
        }

        priorityIds.append(item.id);
        priorityIdSet.insert(item.id);
    }

    if (priorityIds == m_priorityImageIds) {
        return;
    }

    m_priorityImageIds = priorityIds;
    m_priorityImageIdSet = priorityIdSet;

    for (auto it = m_pendingImageRequests.begin();
         it != m_pendingImageRequests.end(); ++it) {
        it->highPriority = priorityIdSet.contains(it.key());
    }

    for (const int row : rows) {
        if (row < 0 || row >= m_items.size()) {
            continue;
        }
        ensureImageRequested(m_items.at(row), true);
    }

    scheduleImageFetches();
}

void MediaListModel::ensureImageRequested(const MediaItem& item,
                                          bool highPriority)
{
    if (m_imageRequestsSuspended) {
        return;
    }

    if (m_scrollActive && !highPriority) {
        return;
    }

    if (highPriority && m_pendingImageRequests.contains(item.id)) {
        m_pendingImageRequests[item.id].highPriority = true;
    }

    if (item.id.isEmpty() || m_imageCache.contains(item.id) ||
        m_loadingImages.contains(item.id) ||
        m_failedImageItems.contains(item.id)) {
        return;
    }

    PendingImageRequest request;
    request.highPriority = highPriority;
    request.imageIdentity = buildImageIdentity(item);

    const bool adaptiveImages =
        ConfigStore::instance()->get<bool>(ConfigKeys::AdaptiveImages, true);
    const QList<MediaImageCandidateUtils::ImageCandidateDescriptor>
        imageCandidates =
            MediaImageCandidateUtils::buildCandidates(item, m_preferThumb,
                                                      adaptiveImages);
    request.candidates.reserve(imageCandidates.size());
    for (const auto& imageCandidate : imageCandidates) {
        ImageCandidate candidate;
        candidate.targetImageId = imageCandidate.targetImageId;
        candidate.imageType = imageCandidate.imageType;
        candidate.imageTag = imageCandidate.imageTag;
        candidate.maxWidth = m_imageMaxWidth;
        request.candidates.append(candidate);
    }

    if (request.candidates.isEmpty()) {
        m_failedImageItems.insert(item.id);
        return;
    }

    enqueueImageFetch(item.id, request);
}

void MediaListModel::enqueueImageFetch(const QString& itemId,
                                       const PendingImageRequest& request)
{
    if (itemId.isEmpty() || request.candidates.isEmpty() ||
        m_imageCache.contains(itemId) || m_loadingImages.contains(itemId)) {
        return;
    }

    m_loadingImages.insert(itemId);
    m_pendingImageRequests.insert(itemId, request);
    m_pendingImageOrder.append(itemId);
    scheduleImageFetches();
}

void MediaListModel::enqueueImageRetry(const QString& itemId,
                                       const PendingImageRequest& request)
{
    if (itemId.isEmpty() || request.candidates.isEmpty() ||
        request.candidateIndex < 0 ||
        request.candidateIndex >= request.candidates.size() ||
        m_imageCache.contains(itemId)) {
        return;
    }

    m_loadingImages.insert(itemId);
    m_pendingImageRequests.insert(itemId, request);
    m_pendingImageOrder.removeAll(itemId);
    m_pendingImageOrder.prepend(itemId);
    scheduleImageFetches();
}

void MediaListModel::scheduleImageFetches()
{
    const int maxConcurrentFetches =
        m_scrollActive ? kMaxConcurrentScrollingImageFetches
                       : kMaxConcurrentImageFetches;
    while (m_activeImageFetches < maxConcurrentFetches &&
           !m_pendingImageRequests.isEmpty()) {
        const QString itemId = takeNextPendingImageId();
        if (itemId.isEmpty()) {
            return;
        }

        const PendingImageRequest request = m_pendingImageRequests.take(itemId);
        m_pendingImageOrder.removeAll(itemId);
        ++m_activeImageFetches;

        QPointer<MediaListModel> safeThis(this);
        QCoro::connect(
            executeImageFetch(safeThis, itemId, request,
                              m_imageRequestGeneration, m_core),
            this, []() {});
    }
}

QString MediaListModel::takeNextPendingImageId()
{
    for (const QString& itemId : std::as_const(m_priorityImageIds)) {
        if (m_pendingImageRequests.contains(itemId)) {
            return itemId;
        }
    }

    while (!m_pendingImageOrder.isEmpty()) {
        const QString itemId = m_pendingImageOrder.takeFirst();
        if (m_pendingImageRequests.contains(itemId)) {
            return itemId;
        }
    }

    if (!m_pendingImageRequests.isEmpty()) {
        return m_pendingImageRequests.constBegin().key();
    }

    return {};
}


QCoro::Task<void> MediaListModel::executeImageFetch(
    QPointer<MediaListModel> safeThis, QString itemId,
    PendingImageRequest request, int generation, XplayerCore* core) {
    try {
        if (request.candidateIndex < 0 ||
            request.candidateIndex >= request.candidates.size() || !safeThis ||
            !core ||
            !core->mediaService()) {
            if (safeThis && generation == safeThis->m_imageRequestGeneration) {
                safeThis->m_loadingImages.remove(itemId);
                safeThis->m_activeImageFetches =
                    qMax(0, safeThis->m_activeImageFetches - 1);
                safeThis->scheduleImageFetches();
            }
            co_return;
        }

        const ImageCandidate candidate =
            request.candidates.at(request.candidateIndex);
        QPointer<QObject> requestContext(safeThis->m_imageRequestContext);
        const ImageFetchPolicy fetchPolicy =
            safeThis->m_forceNetworkImages
                ? ImageFetchPolicy::NetworkOnly
                : ImageFetchPolicy::CachePreferred;

        
        QPixmap pix = co_await core->mediaService()->fetchImage(
            candidate.targetImageId, candidate.imageType, candidate.imageTag,
            candidate.maxWidth, -1,
            request.highPriority ? ImageRequestPriority::High
                                 : ImageRequestPriority::Normal,
            requestContext.data(), fetchPolicy);
        
        
        if (!safeThis) {
            co_return; 
        }

        auto finishFetch = [safeThis, generation]() {
            if (!safeThis) {
                return;
            }
            if (generation == safeThis->m_imageRequestGeneration) {
                safeThis->m_activeImageFetches =
                    qMax(0, safeThis->m_activeImageFetches - 1);
            }
            safeThis->scheduleImageFetches();
        };

        if (generation != safeThis->m_imageRequestGeneration) {
            finishFetch();
            co_return;
        }

        if (!safeThis->isImageRequestCurrent(itemId, request)) {
            finishFetch();
            co_return;
        }

        if (pix.isNull() && request.transientRetryCount == 0) {
            
            
            
            
            
            PendingImageRequest retryRequest = request;
            ++retryRequest.transientRetryCount;
            QTimer::singleShot(
                250, safeThis.data(),
                [safeThis, itemId, retryRequest, generation]() {
                    if (!safeThis ||
                        generation != safeThis->m_imageRequestGeneration ||
                        safeThis->m_imageRequestsSuspended ||
                        !safeThis->isImageRequestCurrent(itemId,
                                                         retryRequest)) {
                        if (safeThis &&
                            generation == safeThis->m_imageRequestGeneration) {
                            safeThis->m_loadingImages.remove(itemId);
                        }
                        return;
                    }
                    safeThis->enqueueImageRetry(itemId, retryRequest);
                });
            finishFetch();
            co_return;
        }

        if (pix.isNull()) {
            safeThis->retryNextImageCandidate(itemId, request, generation);
            finishFetch();
            co_return;
        }

        
        safeThis->removeCachedImage(itemId);
        safeThis->m_imageCache.insert(itemId, pix);
        safeThis->m_imageCacheCostBytes +=
            PixmapCacheBudgetUtils::costBytes(pix);
        safeThis->touchCachedImage(itemId);
        safeThis->trimImageCache();
        safeThis->m_loadingImages.remove(itemId);

        
        int currentRow = -1;
        for (int i = 0; i < safeThis->m_items.size(); ++i) {
            if (safeThis->m_items.at(i).id == itemId) {
                currentRow = i;
                break;
            }
        }

        
        
        if (currentRow >= 0) {
            safeThis->queueImageDataChanged(itemId);
        }
        finishFetch();
    } catch (const std::exception& e) {
        qWarning() << "[MediaListModel] image request threw exception"
                   << "| itemId=" << itemId
                   << "| candidateIndex=" << request.candidateIndex
                   << "| error=" << e.what();
        if (safeThis) {
            if (generation == safeThis->m_imageRequestGeneration) {
                if (safeThis->isImageRequestCurrent(itemId, request)) {
                    safeThis->retryNextImageCandidate(itemId, request,
                                                      generation);
                }
                safeThis->m_activeImageFetches =
                    qMax(0, safeThis->m_activeImageFetches - 1);
            }
            safeThis->scheduleImageFetches();
        }
    } catch (...) {
        qWarning() << "[MediaListModel] image request threw unknown exception"
                   << "| itemId=" << itemId
                   << "| candidateIndex=" << request.candidateIndex;
        if (safeThis) {
            if (generation == safeThis->m_imageRequestGeneration) {
                if (safeThis->isImageRequestCurrent(itemId, request)) {
                    safeThis->retryNextImageCandidate(itemId, request,
                                                      generation);
                }
                safeThis->m_activeImageFetches =
                    qMax(0, safeThis->m_activeImageFetches - 1);
            }
            safeThis->scheduleImageFetches();
        }
    }
}

void MediaListModel::retryNextImageCandidate(const QString& itemId,
                                             PendingImageRequest request,
                                             int generation)
{
    if (generation != m_imageRequestGeneration ||
        !isImageRequestCurrent(itemId, request)) {
        return;
    }

    ++request.candidateIndex;
    if (request.candidateIndex >= request.candidates.size()) {
        m_loadingImages.remove(itemId);
        m_failedImageItems.insert(itemId);
        return;
    }

    enqueueImageRetry(itemId, request);
}

bool MediaListModel::isImageRequestCurrent(
    const QString& itemId, const PendingImageRequest& request) const
{
    for (const MediaItem& item : m_items) {
        if (item.id == itemId) {
            return buildImageIdentity(item) == request.imageIdentity;
        }
    }
    return false;
}

void MediaListModel::invalidateItemImageRequest(const QString& itemId)
{
    removeCachedImage(itemId);
    m_loadingImages.remove(itemId);
    m_failedImageItems.remove(itemId);
    m_pendingImageRequests.remove(itemId);
    m_pendingImageOrder.removeAll(itemId);
    m_pendingImageNotifyIds.remove(itemId);
}

void MediaListModel::touchCachedImage(const QString& itemId)
{
    if (itemId.isEmpty() || !m_imageCache.contains(itemId)) {
        return;
    }

    m_imageCacheLastUsed.insert(itemId, ++m_imageCacheUseTick);
}

void MediaListModel::removeCachedImage(const QString& itemId)
{
    if (itemId.isEmpty()) {
        return;
    }

    const auto cacheIt = m_imageCache.constFind(itemId);
    if (cacheIt != m_imageCache.constEnd()) {
        m_imageCacheCostBytes = qMax<qsizetype>(
            0, m_imageCacheCostBytes -
                   PixmapCacheBudgetUtils::costBytes(cacheIt.value()));
    }
    m_imageCache.remove(itemId);
    m_imageCacheLastUsed.remove(itemId);
}

void MediaListModel::trimImageCache()
{
    while (m_imageCache.size() > kMaxCachedImages ||
           PixmapCacheBudgetUtils::exceedsBudget(
               m_imageCacheCostBytes, kMaxCachedImageBytes)) {
        QString oldestId;
        quint64 oldestTick = std::numeric_limits<quint64>::max();
        for (auto it = m_imageCache.constBegin();
             it != m_imageCache.constEnd(); ++it) {
            const quint64 lastUsed =
                m_imageCacheLastUsed.value(it.key(), 0);
            if (lastUsed < oldestTick) {
                oldestTick = lastUsed;
                oldestId = it.key();
            }
        }

        if (oldestId.isEmpty()) {
            break;
        }
        removeCachedImage(oldestId);
    }

    for (auto it = m_imageCacheLastUsed.begin();
         it != m_imageCacheLastUsed.end();) {
        if (!m_imageCache.contains(it.key())) {
            it = m_imageCacheLastUsed.erase(it);
        } else {
            ++it;
        }
    }
}

void MediaListModel::queueImageDataChanged(const QString& itemId)
{
    if (itemId.isEmpty()) {
        return;
    }

    m_pendingImageNotifyIds.insert(itemId);
    if (m_imageNotifyTimer && !m_imageNotifyTimer->isActive()) {
        m_imageNotifyTimer->start(16);
    }
}

void MediaListModel::flushPendingImageDataChanges()
{
    if (m_pendingImageNotifyIds.isEmpty()) {
        return;
    }

    QStringList itemIds;
    itemIds.reserve(m_pendingImageNotifyIds.size());
    if (m_scrollActive) {
        for (const QString& itemId : std::as_const(m_pendingImageNotifyIds)) {
            if (m_priorityImageIdSet.contains(itemId)) {
                itemIds.append(itemId);
            }
        }
        for (const QString& itemId : std::as_const(itemIds)) {
            m_pendingImageNotifyIds.remove(itemId);
        }
    } else {
        itemIds = m_pendingImageNotifyIds.values();
        m_pendingImageNotifyIds.clear();
    }

    if (itemIds.isEmpty()) {
        return;
    }

    QList<int> rows;
    rows.reserve(itemIds.size());
    QHash<QString, int> rowByItemId;
    rowByItemId.reserve(m_items.size());
    for (int row = 0; row < m_items.size(); ++row) {
        rowByItemId.insert(m_items.at(row).id, row);
    }
    for (const QString& itemId : std::as_const(itemIds)) {
        const auto rowIt = rowByItemId.constFind(itemId);
        if (rowIt != rowByItemId.constEnd()) {
            rows.append(rowIt.value());
        }
    }

    const QList<QPair<int, int>> ranges =
        ModelNotificationUtils::coalescedRowRanges(rows);
    for (const auto& range : ranges) {
        if (range.first < 0 || range.second >= m_items.size()) {
            continue;
        }
        Q_EMIT dataChanged(index(range.first, 0), index(range.second, 0),
                           {PosterPixmapRole});
    }
}
