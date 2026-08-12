#include "mediaimagecandidateutils.h"

#include <QSet>

namespace MediaImageCandidateUtils {

QList<ImageCandidateDescriptor> buildCandidates(const MediaItem& item,
                                                bool preferThumb,
                                                bool adaptiveImages)
{
    QList<ImageCandidateDescriptor> candidates;
    QSet<QString> seenCandidates;

    auto appendCandidate = [&](QString targetImageId, QString imageType,
                               QString imageTag) {
        targetImageId = targetImageId.trimmed();
        imageType = imageType.trimmed();
        imageTag = imageTag.trimmed();
        if (targetImageId.isEmpty() || imageType.isEmpty()) {
            return;
        }

        const QString candidateKey =
            QStringLiteral("%1|%2|%3").arg(targetImageId, imageType, imageTag);
        if (seenCandidates.contains(candidateKey)) {
            return;
        }
        seenCandidates.insert(candidateKey);

        candidates.append({targetImageId, imageType, imageTag});
    };

    auto appendTaggedCandidate = [&](const QString& tag,
                                     const QString& imageType) {
        const QString trimmedTag = tag.trimmed();
        if (trimmedTag.isEmpty()) {
            return;
        }

        QString targetImageId = item.getPrimaryImageId();
        if (item.images.isParentTag(trimmedTag) &&
            !item.images.parentImageItemId.isEmpty()) {
            targetImageId = item.images.parentImageItemId;
        }

        appendCandidate(targetImageId, imageType, trimmedTag);
    };

    auto appendUntaggedFallback = [&](const QString& imageType) {
        QString targetImageId = item.id;
        if (imageType == QStringLiteral("Primary") &&
            !item.images.primaryImageItemId.trimmed().isEmpty()) {
            targetImageId = item.images.primaryImageItemId;
        }
        appendCandidate(targetImageId, imageType, QString());
    };

    if (preferThumb) {
        if (adaptiveImages) {
            appendTaggedCandidate(item.images.thumbTag, QStringLiteral("Thumb"));
            appendTaggedCandidate(item.images.backdropTag,
                                  QStringLiteral("Backdrop"));
            appendTaggedCandidate(item.images.primaryTag,
                                  QStringLiteral("Primary"));
            appendTaggedCandidate(item.images.parentBackdropTag,
                                  QStringLiteral("Backdrop"));
            appendTaggedCandidate(item.images.parentThumbTag,
                                  QStringLiteral("Thumb"));
            appendTaggedCandidate(item.images.parentPrimaryTag,
                                  QStringLiteral("Primary"));
        } else {
            appendTaggedCandidate(item.images.thumbTag, QStringLiteral("Thumb"));
            appendTaggedCandidate(item.images.backdropTag,
                                  QStringLiteral("Backdrop"));
        }

        appendUntaggedFallback(QStringLiteral("Thumb"));
        appendUntaggedFallback(QStringLiteral("Backdrop"));
        appendUntaggedFallback(QStringLiteral("Primary"));
        return candidates;
    }

    if (adaptiveImages) {
        appendTaggedCandidate(item.images.primaryTag, QStringLiteral("Primary"));
        appendTaggedCandidate(item.images.thumbTag, QStringLiteral("Thumb"));
        appendTaggedCandidate(item.images.backdropTag,
                              QStringLiteral("Backdrop"));
        appendTaggedCandidate(item.images.parentPrimaryTag,
                              QStringLiteral("Primary"));
        appendTaggedCandidate(item.images.parentBackdropTag,
                              QStringLiteral("Backdrop"));

        appendUntaggedFallback(QStringLiteral("Primary"));
        appendUntaggedFallback(QStringLiteral("Thumb"));
        appendUntaggedFallback(QStringLiteral("Backdrop"));
    } else {
        appendTaggedCandidate(item.images.primaryTag, QStringLiteral("Primary"));
        appendUntaggedFallback(QStringLiteral("Primary"));
    }

    return candidates;
}

QList<ImageCandidateDescriptor> buildDetailPosterCandidates(
    const MediaItem& item, bool adaptiveImages)
{
    return buildCandidates(item, false, adaptiveImages);
}

} // namespace MediaImageCandidateUtils
