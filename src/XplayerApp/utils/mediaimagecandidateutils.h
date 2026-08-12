#ifndef MEDIAIMAGECANDIDATEUTILS_H
#define MEDIAIMAGECANDIDATEUTILS_H

#include <QList>
#include <QString>
#include <models/media/mediaitem.h>

namespace MediaImageCandidateUtils {

struct ImageCandidateDescriptor {
    QString targetImageId;
    QString imageType;
    QString imageTag;
};

QList<ImageCandidateDescriptor> buildCandidates(const MediaItem& item,
                                                bool preferThumb,
                                                bool adaptiveImages);

QList<ImageCandidateDescriptor> buildDetailPosterCandidates(
    const MediaItem& item, bool adaptiveImages);

} // namespace MediaImageCandidateUtils

#endif
