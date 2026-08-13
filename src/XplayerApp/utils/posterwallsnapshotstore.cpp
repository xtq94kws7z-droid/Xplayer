#include "posterwallsnapshotstore.h"

#include <QCryptographicHash>
#include <QDateTime>
#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMutex>
#include <QMutexLocker>
#include <QRegularExpression>
#include <QSaveFile>
#include <QStandardPaths>
#include <QUuid>
#include <QSet>

#include <algorithm>

namespace {

constexpr int kSnapshotVersion = 1;
constexpr int kSnapshotItemCount = 18;
constexpr int kMaxAccountDirectories = 8;
constexpr qint64 kMaxSnapshotBytes = 128LL * 1024 * 1024;
constexpr qint64 kAccountMaxAgeMs = 30LL * 24 * 60 * 60 * 1000;
QMutex g_snapshotStoreMutex;

QString safeKey(const QString& value)
{
    return QString::fromLatin1(QCryptographicHash::hash(
                                  value.trimmed().toUtf8(),
                                  QCryptographicHash::Sha256)
                                  .toHex());
}

QString accountPath(const QString& root, const QString& serverId,
                    const QString& userId)
{
    return QDir(root).filePath(safeKey(serverId + QLatin1Char('|') + userId));
}

QString activeManifestPath(const QString& account)
{
    return QDir(account).filePath(QStringLiteral("active.json"));
}

bool isDirectChildDirectory(const QDir& root, const QString& path);
bool isDirectChildFile(const QDir& root, const QString& path);

bool isSafeAccountDirectory(const QString& rootPath, const QString& account)
{
    return isDirectChildDirectory(QDir(rootPath), account);
}

QJsonObject itemToJson(const MediaItem& item)
{
    QJsonObject imageTags;
    imageTags.insert(QStringLiteral("Primary"), item.images.primaryTag);
    imageTags.insert(QStringLiteral("Thumb"), item.images.thumbTag);
    imageTags.insert(QStringLiteral("Backdrop"), item.images.backdropTag);

    QJsonObject object;
    object.insert(QStringLiteral("Id"), item.id);
    object.insert(QStringLiteral("Name"), item.name);
    object.insert(QStringLiteral("Type"), item.type);
    object.insert(QStringLiteral("MediaType"), item.mediaType);
    object.insert(QStringLiteral("ProductionYear"), item.productionYear);
    object.insert(QStringLiteral("Overview"), item.overview);
    object.insert(QStringLiteral("OfficialRating"), item.officialRating);
    object.insert(QStringLiteral("RunTimeTicks"),
                  QString::number(item.runTimeTicks));
    object.insert(QStringLiteral("CommunityRating"), item.communityRating);
    object.insert(QStringLiteral("ImageTags"), imageTags);
    object.insert(QStringLiteral("PrimaryImageItemId"),
                  item.images.primaryImageItemId);
    object.insert(QStringLiteral("PrimaryImageAspectRatio"),
                  item.images.primaryImageAspectRatio);

    QJsonArray genres;
    for (const QString& genre : item.genres) {
        genres.append(genre);
    }
    object.insert(QStringLiteral("Genres"), genres);
    return object;
}

bool writeJson(const QString& path, const QJsonObject& object,
               QString* errorString)
{
    QSaveFile file(path);
    if (!file.open(QIODevice::WriteOnly)) {
        if (errorString) {
            *errorString = file.errorString();
        }
        return false;
    }
    if (file.write(QJsonDocument(object).toJson(QJsonDocument::Compact)) < 0 ||
        !file.commit()) {
        if (errorString) {
            *errorString = file.errorString();
        }
        return false;
    }
    return true;
}

std::optional<QJsonObject> readJson(const QString& path)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        return std::nullopt;
    }
    QJsonParseError error;
    const QJsonDocument document = QJsonDocument::fromJson(file.readAll(), &error);
    if (error.error != QJsonParseError::NoError || !document.isObject()) {
        return std::nullopt;
    }
    return document.object();
}

bool isDirectChildDirectory(const QDir& root, const QString& path)
{
    const QFileInfo info(path);
    return info.absolutePath() == root.absolutePath() && info.isDir() &&
           !info.isSymLink();
}

bool isValidGenerationName(const QString& generation)
{
    static const QRegularExpression pattern(
        QStringLiteral("^generation-[0-9]+-[0-9a-fA-F-]{36}$"));
    return pattern.match(generation).hasMatch();
}

bool isDirectChildFile(const QDir& root, const QString& path)
{
    const QFileInfo info(path);
    return info.absolutePath() == root.absolutePath() && info.isFile() &&
           !info.isSymLink();
}

bool isValidImageFileName(const QString& imageFile)
{
    static const QRegularExpression pattern(
        QStringLiteral("^poster-[0-9]+\\.png$"));
    return pattern.match(imageFile).hasMatch();
}

qint64 directorySize(const QString& path)
{
    qint64 total = 0;
    QDirIterator files(path, QDir::Files, QDirIterator::Subdirectories);
    while (files.hasNext()) {
        files.next();
        total += files.fileInfo().size();
    }
    return total;
}

void maintainSnapshotRoot(const QString& rootPath, const QString& activeAccount)
{
    QDir root(rootPath);
    if (!root.exists()) {
        return;
    }

    struct AccountEntry {
        QString path;
        qint64 lastModifiedMs = 0;
        qint64 bytes = 0;
    };
    QList<AccountEntry> accounts;
    const QFileInfoList entries = root.entryInfoList(
        QDir::Dirs | QDir::NoDotAndDotDot, QDir::Time);
    const qint64 now = QDateTime::currentMSecsSinceEpoch();
    for (const QFileInfo& entry : entries) {
        if (!isDirectChildDirectory(root, entry.absoluteFilePath())) {
            continue;
        }
        const qint64 modified = entry.lastModified().toMSecsSinceEpoch();
        if (entry.absoluteFilePath() != activeAccount &&
            now - modified > kAccountMaxAgeMs) {
            QDir(entry.absoluteFilePath()).removeRecursively();
            continue;
        }
        accounts.append({entry.absoluteFilePath(), modified,
                         directorySize(entry.absoluteFilePath())});
    }

    std::sort(accounts.begin(), accounts.end(),
              [&activeAccount](const AccountEntry& left,
                               const AccountEntry& right) {
                  if ((left.path == activeAccount) !=
                      (right.path == activeAccount)) {
                      return left.path == activeAccount;
                  }
                  return left.lastModifiedMs > right.lastModifiedMs;
              });
    qint64 keptBytes = 0;
    int keptAccounts = 0;
    for (const AccountEntry& account : std::as_const(accounts)) {
        const bool active = account.path == activeAccount;
        if (active && account.bytes > kMaxSnapshotBytes) {
            QDir(account.path).removeRecursively();
            continue;
        }
        const bool overCount = keptAccounts >= kMaxAccountDirectories;
        const bool overBytes = keptBytes + account.bytes > kMaxSnapshotBytes;
        if (!active && (overCount || overBytes)) {
            QDir(account.path).removeRecursively();
            continue;
        }
        keptBytes += account.bytes;
        ++keptAccounts;
    }
}

void removeInactiveGenerations(const QString& accountPath,
                               const QString& activeGeneration)
{
    QDir account(accountPath);
    if (!account.exists()) {
        return;
    }

    const QStringList generations = account.entryList(
        {QStringLiteral("generation-*")}, QDir::Dirs | QDir::NoDotAndDotDot);
    for (const QString& generation : generations) {
        if (generation == activeGeneration) {
            continue;
        }
        const QString path = account.filePath(generation);
        if (isDirectChildDirectory(account, path)) {
            QDir(path).removeRecursively();
        }
    }
}

} // namespace

PosterWallSnapshotStore::PosterWallSnapshotStore(QString rootPath)
    : m_rootPath(rootPath.trimmed().isEmpty()
                     ? QStandardPaths::writableLocation(
                           QStandardPaths::CacheLocation) +
                           QStringLiteral("/poster-wall-snapshots")
                     : std::move(rootPath))
{
}

std::optional<PosterWallSnapshot> PosterWallSnapshotStore::load(
    const QString& serverId, const QString& userId) const
{
    const QMutexLocker locker(&g_snapshotStoreMutex);
    if (serverId.trimmed().isEmpty() || userId.trimmed().isEmpty()) {
        return std::nullopt;
    }

    const QString account = accountPath(m_rootPath, serverId, userId);
    const QDir root(m_rootPath);
    const QString activePath = activeManifestPath(account);
    if ((root.exists() && QFileInfo(account).exists() &&
         !isSafeAccountDirectory(m_rootPath, account)) ||
        (QFileInfo(activePath).exists() &&
         !isDirectChildFile(QDir(account), activePath))) {
        return std::nullopt;
    }
    const auto active = readJson(activeManifestPath(account));
    if (!active || active->value(QStringLiteral("version")).toInt() !=
                       kSnapshotVersion) {
        removeInactiveGenerations(account, {});
        maintainSnapshotRoot(m_rootPath, account);
        return std::nullopt;
    }
    const QString generation =
        active->value(QStringLiteral("generation")).toString();
    if (!isValidGenerationName(generation)) {
        removeInactiveGenerations(account, {});
        maintainSnapshotRoot(m_rootPath, account);
        return std::nullopt;
    }

    const QString generationPath = QDir(account).filePath(generation);
    if (!isDirectChildDirectory(QDir(account), generationPath)) {
        removeInactiveGenerations(account, {});
        maintainSnapshotRoot(m_rootPath, account);
        return std::nullopt;
    }
    const QString manifestPath =
        QDir(generationPath).filePath(QStringLiteral("snapshot.json"));
    if (!isDirectChildFile(QDir(generationPath), manifestPath)) {
        return std::nullopt;
    }
    const auto manifest = readJson(manifestPath);
    if (!manifest || manifest->value(QStringLiteral("serverId")).toString() !=
                         serverId ||
        manifest->value(QStringLiteral("userId")).toString() != userId) {
        return std::nullopt;
    }

    PosterWallSnapshot snapshot;
    const QJsonArray items = manifest->value(QStringLiteral("items")).toArray();
    if (items.size() != kSnapshotItemCount) {
        return std::nullopt;
    }
    QSet<QString> loadedIds;
    for (const QJsonValue& value : items) {
        const QJsonObject entry = value.toObject();
        const MediaItem item =
            MediaItem::fromJson(entry.value(QStringLiteral("item")).toObject());
        const QString imageFile =
            entry.value(QStringLiteral("imageFile")).toString();
        const QString imagePath = QDir(generationPath).filePath(imageFile);
        if (item.id.trimmed().isEmpty() ||
            !isValidImageFileName(imageFile) ||
            !isDirectChildFile(QDir(generationPath), imagePath)) {
            return std::nullopt;
        }
        const QImage image(imagePath);
        if (image.isNull() || loadedIds.contains(item.id)) {
            return std::nullopt;
        }
        loadedIds.insert(item.id);
        snapshot.items.append(item);
        snapshot.images.insert(item.id, image);
    }
    removeInactiveGenerations(account, generation);
    maintainSnapshotRoot(m_rootPath, account);
    return snapshot;
}

bool PosterWallSnapshotStore::save(const QString& serverId,
                                   const QString& userId,
                                   const QList<MediaItem>& items,
                                   const QHash<QString, QImage>& images,
                                   QString* errorString) const
{
    const QMutexLocker locker(&g_snapshotStoreMutex);
    if (serverId.trimmed().isEmpty() || userId.trimmed().isEmpty() ||
        items.size() != kSnapshotItemCount) {
        if (errorString) {
            *errorString = QStringLiteral("snapshot requires exactly 18 items");
        }
        return false;
    }
    QSet<QString> itemIds;
    for (const MediaItem& item : items) {
        if (item.id.trimmed().isEmpty() || !images.contains(item.id) ||
            images.value(item.id).isNull() || itemIds.contains(item.id)) {
            if (errorString) {
                *errorString = QStringLiteral("snapshot image set is incomplete");
            }
            return false;
        }
        itemIds.insert(item.id);
    }

    const QString account = accountPath(m_rootPath, serverId, userId);
    if (QFileInfo(account).exists() &&
        !isSafeAccountDirectory(m_rootPath, account)) {
        if (errorString) {
            *errorString = QStringLiteral("unsafe snapshot account path");
        }
        return false;
    }
    const QString generation =
        QStringLiteral("generation-%1-%2")
            .arg(QDateTime::currentMSecsSinceEpoch())
            .arg(QUuid::createUuid().toString(QUuid::WithoutBraces));
    const QString generationPath = QDir(account).filePath(generation);
    if (!QDir().mkpath(generationPath)) {
        if (errorString) {
            *errorString = QStringLiteral("failed to create snapshot directory");
        }
        return false;
    }

    QJsonArray entries;
    for (int index = 0; index < items.size(); ++index) {
        const MediaItem& item = items.at(index);
        const QString imageFile = QStringLiteral("poster-%1.png").arg(index);
        if (!images.value(item.id).save(QDir(generationPath).filePath(imageFile),
                                        "PNG")) {
            QDir(generationPath).removeRecursively();
            if (errorString) {
                *errorString = QStringLiteral("failed to save snapshot image");
            }
            return false;
        }
        QJsonObject entry;
        entry.insert(QStringLiteral("item"), itemToJson(item));
        entry.insert(QStringLiteral("imageFile"), imageFile);
        entries.append(entry);
    }

    QJsonObject manifest;
    manifest.insert(QStringLiteral("version"), kSnapshotVersion);
    manifest.insert(QStringLiteral("serverId"), serverId);
    manifest.insert(QStringLiteral("userId"), userId);
    manifest.insert(QStringLiteral("items"), entries);
    if (!writeJson(QDir(generationPath).filePath(QStringLiteral("snapshot.json")),
                   manifest, errorString)) {
        QDir(generationPath).removeRecursively();
        return false;
    }

    qint64 prospectiveRootBytes = directorySize(m_rootPath);
    const QDir accountDirectory(account);
    const QStringList generations = accountDirectory.entryList(
        {QStringLiteral("generation-*")}, QDir::Dirs | QDir::NoDotAndDotDot);
    for (const QString& oldGeneration : generations) {
        if (oldGeneration == generation) {
            continue;
        }
        const QString oldPath = accountDirectory.filePath(oldGeneration);
        if (isDirectChildDirectory(accountDirectory, oldPath)) {
            prospectiveRootBytes -= directorySize(oldPath);
        }
    }
    if (prospectiveRootBytes > kMaxSnapshotBytes) {
        QDir(generationPath).removeRecursively();
        if (errorString) {
            *errorString = QStringLiteral("snapshot storage limit exceeded");
        }
        return false;
    }

    for (int index = 0; index < items.size(); ++index) {
        const QImage verifiedImage(QDir(generationPath).filePath(
            QStringLiteral("poster-%1.png").arg(index)));
        if (verifiedImage.isNull()) {
            QDir(generationPath).removeRecursively();
            if (errorString) {
                *errorString = QStringLiteral("snapshot image verification failed");
            }
            return false;
        }
    }

    QJsonObject active;
    active.insert(QStringLiteral("version"), kSnapshotVersion);
    active.insert(QStringLiteral("generation"), generation);
    if (!QDir().mkpath(account) ||
        !writeJson(activeManifestPath(account), active, errorString)) {
        QDir(generationPath).removeRecursively();
        return false;
    }

    removeInactiveGenerations(account, generation);
    maintainSnapshotRoot(m_rootPath, account);
    return true;
}
