#include "diskcachemaintenanceutils.h"

#include <QDirIterator>
#include <QFile>
#include <QFileInfo>

#include <algorithm>

namespace DiskCacheMaintenanceUtils
{
PruneResult pruneDirectory(const QString &rootPath, qint64 maximumBytes,
                           qint64 maximumAgeMs, qint64 nowMs)
{
    struct CacheFile
    {
        QString path;
        qint64 bytes = 0;
        qint64 modifiedMs = 0;
    };

    PruneResult result;
    QList<CacheFile> retainedFiles;
    QDirIterator iterator(rootPath, QDir::Files | QDir::NoSymLinks,
                          QDirIterator::Subdirectories);
    while (iterator.hasNext())
    {
        const QFileInfo info(iterator.next());
        const CacheFile cacheFile{
            info.absoluteFilePath(), info.size(),
            info.lastModified().toMSecsSinceEpoch()};
        const bool expired = maximumAgeMs > 0 && nowMs > 0 &&
                             cacheFile.modifiedMs > 0 &&
                             nowMs - cacheFile.modifiedMs > maximumAgeMs;
        if (expired && QFile::remove(cacheFile.path))
        {
            ++result.removedFiles;
            result.removedBytes += cacheFile.bytes;
            continue;
        }

        retainedFiles.append(cacheFile);
        result.remainingBytes += cacheFile.bytes;
    }

    if (maximumBytes <= 0 || result.remainingBytes <= maximumBytes)
        return result;

    std::sort(retainedFiles.begin(), retainedFiles.end(),
              [](const CacheFile &left, const CacheFile &right)
              {
                  return left.modifiedMs < right.modifiedMs;
              });
    for (const CacheFile &cacheFile : retainedFiles)
    {
        if (result.remainingBytes <= maximumBytes)
            break;
        if (!QFile::remove(cacheFile.path))
            continue;

        ++result.removedFiles;
        result.removedBytes += cacheFile.bytes;
        result.remainingBytes -= cacheFile.bytes;
    }

    return result;
}
}
