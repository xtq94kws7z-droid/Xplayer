#include "imageconcurrencyutils.h"

namespace ImageConcurrencyUtils
{
int maxConcurrentRequests(qint64 startupElapsedMs)
{
    if (startupElapsedMs < kStartupImageBurstDurationMs) {
        return kStartupImageConcurrency;
    }

    return kNormalImageConcurrency;
}
}
