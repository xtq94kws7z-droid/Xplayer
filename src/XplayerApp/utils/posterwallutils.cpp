#include "posterwallutils.h"

#include <functional>

#include <QRegularExpression>
#include <QRandomGenerator>
#include <QSet>
#include <QVector>

namespace PosterWallUtils
{

namespace {

constexpr int kPosterSignatureWidth = 12;
constexpr int kPosterSignatureHeight = 18;
constexpr int kPosterSimilarAverageDistance = 24;

QString normalizedTextKey(QString value)
{
    value = value.simplified().toCaseFolded();
    value.remove(QRegularExpression(QStringLiteral("[\\s\\p{P}\\p{S}]+")));
    return value;
}

QString providerKeyForItem(const MediaItem& item)
{
    QStringList parts;
    const QStringList keys = item.providerIds.keys();
    for (const QString& key : keys) {
        const QString value = item.providerIds.value(key).toString().trimmed();
        if (!value.isEmpty()) {
            parts << key.toCaseFolded() + QLatin1Char(':') + value.toCaseFolded();
        }
    }
    parts.sort();
    return parts.join(QLatin1Char('|'));
}

QString titleKeyForItem(const MediaItem& item)
{
    QString title = item.seriesName.trimmed();
    if (title.isEmpty()) {
        title = item.originalTitle.trimmed();
    }
    if (title.isEmpty()) {
        title = item.sortName.trimmed();
    }
    if (title.isEmpty()) {
        title = item.name.trimmed();
    }

    title = normalizedTextKey(title);
    if (title.isEmpty()) {
        return {};
    }

    if (item.productionYear > 0) {
        title += QLatin1Char('|') + QString::number(item.productionYear);
    }
    return title;
}

QString selectionKeyForItem(const MediaItem& item)
{
    QString key = item.seriesName.trimmed();
    if (key.isEmpty()) {
        key = item.originalTitle.trimmed();
    }
    if (key.isEmpty()) {
        key = item.sortName.trimmed();
    }
    if (key.isEmpty()) {
        key = item.name.trimmed();
    }
    if (item.productionYear > 0) {
        key += QLatin1Char('|') + QString::number(item.productionYear);
    }
    if (key.trimmed().isEmpty()) {
        key = item.id.trimmed();
    }
    return key.simplified().toCaseFolded();
}

QVector<QColor> posterSignature(const QImage& image)
{
    QVector<QColor> signature;
    if (image.isNull() || image.width() <= 0 || image.height() <= 0) {
        return signature;
    }

    const QImage sampled =
        image.scaled(kPosterSignatureWidth, kPosterSignatureHeight,
                     Qt::IgnoreAspectRatio, Qt::SmoothTransformation)
            .convertToFormat(QImage::Format_RGB32);

    signature.reserve(sampled.width() * sampled.height());
    for (int y = 0; y < sampled.height(); ++y) {
        const QRgb* line =
            reinterpret_cast<const QRgb*>(sampled.constScanLine(y));
        for (int x = 0; x < sampled.width(); ++x) {
            signature.append(QColor::fromRgb(line[x]));
        }
    }
    return signature;
}

int averageColorDistance(const QVector<QColor>& first,
                         const QVector<QColor>& second)
{
    if (first.isEmpty() || first.size() != second.size()) {
        return 255;
    }

    quint64 total = 0;
    for (int i = 0; i < first.size(); ++i) {
        total += static_cast<quint64>(qAbs(first.at(i).red() -
                                           second.at(i).red()));
        total += static_cast<quint64>(qAbs(first.at(i).green() -
                                           second.at(i).green()));
        total += static_cast<quint64>(qAbs(first.at(i).blue() -
                                           second.at(i).blue()));
    }

    return static_cast<int>(total /
                            static_cast<quint64>(first.size() * 3));
}

} // namespace

QList<MediaItem> mergeUniqueItems(const QList<MediaItem>& resume,
                                  const QList<MediaItem>& latest,
                                  const QList<MediaItem>& recommended,
                                  const QList<MediaItem>& completed,
                                  int maxItems)
{
    QList<MediaItem> result;
    if (maxItems <= 0) {
        return result;
    }

    QSet<QString> seenIds;
    QSet<QString> seenArtwork;
    QSet<QString> seenProviders;
    QSet<QString> seenTitles;
    const QList<QList<MediaItem>> sources = {
        resume, latest, recommended, completed};

    for (const QList<MediaItem>& source : sources) {
        for (const MediaItem& item : source) {
            const QString id = item.id.trimmed();
            const QString artworkItemId = item.getPrimaryImageId().trimmed();
            const QString artworkTag = !item.images.primaryTag.trimmed().isEmpty()
                                           ? item.images.primaryTag.trimmed()
                                           : item.images.parentPrimaryTag.trimmed();
            const QString artworkKey =
                QStringLiteral("%1|%2")
                    .arg(artworkItemId.isEmpty() ? id : artworkItemId,
                         artworkTag);
            const QString providerKey = providerKeyForItem(item);
            const QString titleKey = titleKeyForItem(item);

            if (id.isEmpty() || seenIds.contains(id) ||
                seenArtwork.contains(artworkKey) ||
                (!providerKey.isEmpty() && seenProviders.contains(providerKey)) ||
                (!titleKey.isEmpty() && seenTitles.contains(titleKey))) {
                continue;
            }

            seenIds.insert(id);
            seenArtwork.insert(artworkKey);
            if (!providerKey.isEmpty()) {
                seenProviders.insert(providerKey);
            }
            if (!titleKey.isEmpty()) {
                seenTitles.insert(titleKey);
            }
            result.append(item);
            if (result.size() >= maxItems) {
                return result;
            }
        }
    }

    return result;
}

QList<MediaItem> buildLibraryCandidates(const QList<MediaItem>& libraryItems,
                                        int maxItems)
{
    if (maxItems <= 0) {
        return {};
    }

    auto posterItemsOnly = [](const QList<MediaItem>& source) {
        QList<MediaItem> result;
        for (const MediaItem& item : source) {
            const QString type = item.type.trimmed().toCaseFolded();
            if (type.isEmpty() || type == QLatin1String("movie") ||
                type == QLatin1String("series")) {
                result.append(item);
            }
        }
        return result;
    };

    return mergeUniqueItems(posterItemsOnly(libraryItems), {}, {}, {},
                            maxItems);
}

QList<int> equalLibrarySampleQuotas(int libraryCount, int totalLimit,
                                    int perLibraryLimit)
{
    if (libraryCount <= 0 || totalLimit <= 0 || perLibraryLimit <= 0) {
        return {};
    }

    QList<int> quotas(libraryCount, 0);
    int remaining = qMin(totalLimit, libraryCount * perLibraryLimit);
    for (int index = 0; index < libraryCount && remaining > 0; ++index) {
        const int librariesRemaining = libraryCount - index;
        const int quota = qMin(
            perLibraryLimit,
            (remaining + librariesRemaining - 1) / librariesRemaining);
        quotas[index] = quota;
        remaining -= quota;
    }
    return quotas;
}

QList<MediaItem> selectRandomItems(const QList<MediaItem>& candidates,
                                   int maxItems,
                                   const QList<MediaItem>& previous,
                                   quint32 seed)
{
    if (candidates.isEmpty() || maxItems <= 0) {
        return {};
    }

    const int selectionSize = qMin(maxItems, candidates.size());
    QList<int> shuffledIndices;
    shuffledIndices.reserve(candidates.size());
    for (int index = 0; index < candidates.size(); ++index) {
        shuffledIndices.append(index);
    }

    auto shuffle = [](QList<int>& indices, quint32 shuffleSeed) {
        QRandomGenerator localGenerator(shuffleSeed);
        QRandomGenerator* generator =
            shuffleSeed == 0 ? QRandomGenerator::global() : &localGenerator;
        for (int index = indices.size() - 1; index > 0; --index) {
            const int swapIndex =
                static_cast<int>(generator->bounded(index + 1));
            indices.swapItemsAt(index, swapIndex);
        }
    };

    auto buildSelection = [&](const QList<int>& indices) {
        QList<MediaItem> selection;
        selection.reserve(selectionSize);
        for (int index = 0; index < selectionSize; ++index) {
            selection.append(candidates.at(indices.at(index)));
        }
        return selection;
    };

    auto sameSelection = [](const QList<MediaItem>& first,
                            const QList<MediaItem>& second) {
        if (first.size() != second.size()) {
            return false;
        }
        for (int index = 0; index < first.size(); ++index) {
            if (first.at(index).id != second.at(index).id) {
                return false;
            }
        }
        return true;
    };

    shuffle(shuffledIndices, seed);

    QList<int> prioritizedIndices;
    prioritizedIndices.reserve(shuffledIndices.size());
    for (const int index : std::as_const(shuffledIndices)) {
        if (!candidates.at(index).overview.trimmed().isEmpty()) {
            prioritizedIndices.append(index);
        }
    }
    for (const int index : std::as_const(shuffledIndices)) {
        if (candidates.at(index).overview.trimmed().isEmpty()) {
            prioritizedIndices.append(index);
        }
    }

    QList<MediaItem> selection = buildSelection(prioritizedIndices);
    if (sameSelection(selection, previous) && candidates.size() > selectionSize) {
        prioritizedIndices.swapItemsAt(0, selectionSize);
        selection = buildSelection(prioritizedIndices);
    }
    return selection;
}

QList<MediaItem> enrichItemDetails(const QList<MediaItem>& selected,
                                   const QList<MediaItem>& details)
{
    QHash<QString, MediaItem> detailsById;
    detailsById.reserve(details.size());
    for (const MediaItem& detail : details) {
        const QString id = detail.id.trimmed();
        if (!id.isEmpty()) {
            detailsById.insert(id, detail);
        }
    }

    QList<MediaItem> enriched = selected;
    for (MediaItem& item : enriched) {
        const auto detailIt = detailsById.constFind(item.id.trimmed());
        if (detailIt == detailsById.cend()) {
            continue;
        }

        const QString overview = detailIt->overview.trimmed();
        if (!overview.isEmpty()) {
            item.overview = overview;
        }
    }
    return enriched;
}

int nextIndex(int currentIndex, int itemCount, int direction)
{
    if (itemCount <= 0) {
        return -1;
    }

    const int normalizedCurrent =
        ((currentIndex % itemCount) + itemCount) % itemCount;
    const int step = direction < 0 ? -1 : (direction > 0 ? 1 : 0);
    return (normalizedCurrent + step + itemCount) % itemCount;
}

QList<int> visibleIndices(int currentIndex, int itemCount, int maxVisible)
{
    QList<int> indices;
    if (itemCount <= 0 || maxVisible <= 0) {
        return indices;
    }

    const int start = nextIndex(currentIndex, itemCount, 0);
    const int count = qMin(itemCount, maxVisible);
    for (int offset = 0; offset < count; ++offset) {
        indices.append((start + offset) % itemCount);
    }
    return indices;
}

QColor dominantColor(const QImage& image)
{
    if (image.isNull() || image.width() <= 0 || image.height() <= 0) {
        return QColor(128, 136, 148);
    }

    const int sampleWidth = qMin(image.width(), 24);
    const int sampleHeight = qMin(image.height(), 24);
    const QImage sampled =
        image.scaled(sampleWidth, sampleHeight, Qt::IgnoreAspectRatio,
                     Qt::FastTransformation)
            .convertToFormat(QImage::Format_RGB32);

    quint64 red = 0;
    quint64 green = 0;
    quint64 blue = 0;
    const int count = sampled.width() * sampled.height();
    for (int y = 0; y < sampled.height(); ++y) {
        const QRgb* line =
            reinterpret_cast<const QRgb*>(sampled.constScanLine(y));
        for (int x = 0; x < sampled.width(); ++x) {
            const QColor pixel = QColor::fromRgb(line[x]);
            red += pixel.red();
            green += pixel.green();
            blue += pixel.blue();
        }
    }

    if (count == 0) {
        return QColor(128, 136, 148);
    }

    const QColor average(static_cast<int>(red / count),
                         static_cast<int>(green / count),
                         static_cast<int>(blue / count));
    return average.lighter(112);
}

bool areImagesVisuallySimilar(const QImage& first, const QImage& second)
{
    const QVector<QColor> firstSignature = posterSignature(first);
    const QVector<QColor> secondSignature = posterSignature(second);
    return averageColorDistance(firstSignature, secondSignature) <=
           kPosterSimilarAverageDistance;
}

bool sameStageItems(const QList<MediaItem>& first,
                    const QList<MediaItem>& second)
{
    if (first.size() != second.size()) {
        return false;
    }

    for (int i = 0; i < first.size(); ++i) {
        const MediaItem& a = first.at(i);
        const MediaItem& b = second.at(i);
        if (a.id != b.id ||
            a.name != b.name ||
            a.originalTitle != b.originalTitle ||
            a.sortName != b.sortName ||
            a.seriesName != b.seriesName ||
            a.type != b.type ||
            a.mediaType != b.mediaType ||
            a.productionYear != b.productionYear ||
            a.overview != b.overview ||
            a.officialRating != b.officialRating ||
            a.premiereDate != b.premiereDate ||
            a.images.primaryTag != b.images.primaryTag ||
            a.images.thumbTag != b.images.thumbTag ||
            a.images.backdropTag != b.images.backdropTag ||
            a.images.logoTag != b.images.logoTag ||
            a.images.primaryImageItemId != b.images.primaryImageItemId ||
            a.images.parentPrimaryTag != b.images.parentPrimaryTag ||
            a.images.parentBackdropTag != b.images.parentBackdropTag ||
            a.images.parentThumbTag != b.images.parentThumbTag ||
            a.images.parentImageItemId != b.images.parentImageItemId) {
            return false;
        }
    }

    return true;
}

QList<int> posterStageIndices(
    const QList<MediaItem>& items, int currentIndex, int maxVisible,
    const std::function<QImage(const MediaItem&)>& imageProvider)
{
    QList<int> selected;
    if (items.isEmpty() || maxVisible <= 0) {
        return selected;
    }

    const int candidateCount = qMin(items.size(), maxVisible);
    const int firstOffset = -(candidateCount / 2);
    const int normalizedCurrent =
        ((currentIndex % items.size()) + items.size()) % items.size();
    QList<int> candidateIndices;
    candidateIndices.reserve(candidateCount);
    for (int offset = 0; offset < candidateCount; ++offset) {
        const int rawIndex =
            normalizedCurrent + firstOffset + offset;
        candidateIndices.append((rawIndex % items.size() + items.size()) %
                                items.size());
    }
    QSet<QString> strictKeys;
    QSet<QString> selectedKeys;
    QVector<QImage> selectedImages;

    auto addSelection = [&](int index, bool captureImage) {
        selected.append(index);
        const QString key = selectionKeyForItem(items.at(index));
        if (!key.isEmpty()) {
            selectedKeys.insert(key);
            strictKeys.insert(key);
        }
        if (captureImage && imageProvider) {
            const QImage image = imageProvider(items.at(index));
            if (!image.isNull()) {
                selectedImages.append(image);
            }
        }
    };

    for (const int index : candidateIndices) {
        if (selected.size() >= maxVisible) {
            return selected;
        }

        const QString key = selectionKeyForItem(items.at(index));
        if (key.isEmpty() || strictKeys.contains(key)) {
            continue;
        }

        const QImage image = imageProvider ? imageProvider(items.at(index))
                                           : QImage();
        bool duplicateImage = false;
        if (!image.isNull()) {
            for (const QImage& selectedImage : std::as_const(selectedImages)) {
                if (areImagesVisuallySimilar(selectedImage, image)) {
                    duplicateImage = true;
                    break;
                }
            }
        }

        if (duplicateImage) {
            continue;
        }

        addSelection(index, true);
    }

    for (const int index : candidateIndices) {
        if (selected.size() >= maxVisible) {
            return selected;
        }
        if (selected.contains(index)) {
            continue;
        }

        const QString key = selectionKeyForItem(items.at(index));
        if (!key.isEmpty() && selectedKeys.contains(key)) {
            continue;
        }

        addSelection(index, true);
    }

    for (const int index : candidateIndices) {
        if (selected.size() >= maxVisible) {
            break;
        }
        if (selected.contains(index)) {
            continue;
        }
        addSelection(index, true);
    }

    return selected;
}

} // namespace PosterWallUtils
