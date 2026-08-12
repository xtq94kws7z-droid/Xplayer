#pragma once

#include <QtGlobal>

class QString;

namespace DiskCacheMaintenanceUtils
{
struct PruneResult
{
    qsizetype removedFiles = 0;
    qint64 removedBytes = 0;
    qint64 remainingBytes = 0;
};

PruneResult pruneDirectory(const QString &rootPath, qint64 maximumBytes,
                           qint64 maximumAgeMs, qint64 nowMs);
}
