#include <QDateTime>
#include <QFile>
#include <QTemporaryDir>
#include <QTextStream>

#include "../src/XplayerApp/utils/diskcachemaintenanceutils.h"

namespace
{
bool check(bool condition, const char *message)
{
    if (condition)
        return true;
    QTextStream(stderr) << "diskcachemaintenanceutils_test failed: "
                        << message << Qt::endl;
    return false;
}

bool writeFile(const QString &path, int bytes, qint64 modifiedMs)
{
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly))
        return false;
    file.write(QByteArray(bytes, 'x'));
    const bool timestampSet = file.setFileTime(
        QDateTime::fromMSecsSinceEpoch(modifiedMs),
        QFileDevice::FileModificationTime);
    file.close();
    return timestampSet;
}
}

int main()
{
    QTemporaryDir temp;
    if (!check(temp.isValid(), "temporary directory is available"))
        return 1;

    const qint64 nowMs = QDateTime::currentMSecsSinceEpoch();
    const QString oldFile = temp.filePath(QStringLiteral("old.cache"));
    const QString recentA = temp.filePath(QStringLiteral("recent-a.cache"));
    const QString recentB = temp.filePath(QStringLiteral("recent-b.cache"));
    if (!check(writeFile(oldFile, 40, nowMs - 10'000) &&
                   writeFile(recentA, 70, nowMs - 2'000) &&
                   writeFile(recentB, 70, nowMs - 1'000),
               "cache fixtures are created"))
        return 1;

    const auto result = DiskCacheMaintenanceUtils::pruneDirectory(
        temp.path(), 100, 5'000, nowMs);
    if (!check(!QFile::exists(oldFile), "expired files are removed first"))
        return 1;
    if (!check(!QFile::exists(recentA) && QFile::exists(recentB),
               "oldest remaining files are removed to enforce budget"))
        return 1;
    if (!check(result.removedFiles == 2 && result.remainingBytes == 70,
               "prune result reports bounded cache size"))
        return 1;

    return 0;
}
