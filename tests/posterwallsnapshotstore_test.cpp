#include <QtTest>

#include "utils/posterwallsnapshotstore.h"

#include <QTemporaryDir>
#include <QDirIterator>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>

namespace {

MediaItem makeItem(const QString& id, const QString& name)
{
    MediaItem item;
    item.id = id;
    item.name = name;
    item.type = QStringLiteral("Movie");
    item.overview = QStringLiteral("Overview for %1").arg(name);
    item.productionYear = 2026;
    item.images.primaryTag = QStringLiteral("tag-%1").arg(id);
    return item;
}

QImage makeImage(const QColor& color)
{
    QImage image(32, 48, QImage::Format_ARGB32_Premultiplied);
    image.fill(color);
    return image;
}

QPair<QList<MediaItem>, QHash<QString, QImage>> makeSnapshot(
    const QString& prefix, int count = 18)
{
    QList<MediaItem> items;
    QHash<QString, QImage> images;
    for (int index = 0; index < count; ++index) {
        const QString id = QStringLiteral("%1-%2").arg(prefix).arg(index);
        items.append(makeItem(id, id));
        images.insert(id, makeImage(QColor::fromHsv((index * 31) % 360,
                                                    180, 220)));
    }
    return {items, images};
}

} // namespace

class PosterWallSnapshotStoreTest final : public QObject
{
    Q_OBJECT

private slots:
    void roundTripsCompleteSnapshot();
    void isolatesSnapshotsByUser();
    void rejectsSnapshotWithMissingImage();
    void failedSavePreservesPreviousSnapshot();
    void successfulReplacementRemovesOldGeneration();
    void loadRemovesOrphanGenerationWithoutTouchingActiveSnapshot();
    void loadRemovesAllGenerationsWhenActiveManifestIsMissing();
    void rejectsGenerationPathTraversal();
    void rejectsImagePathTraversal();
    void rejectsSnapshotWhenRootWouldExceedStorageLimit();
    void rejectsIncompleteSnapshot();
    void rejectsDuplicateItemIds();
    void maintenanceLimitsHistoricalAccounts();
};

void PosterWallSnapshotStoreTest::roundTripsCompleteSnapshot()
{
    QTemporaryDir root;
    QVERIFY(root.isValid());
    PosterWallSnapshotStore store(root.path());
    const auto [items, images] = makeSnapshot(QStringLiteral("roundtrip"));

    QString error;
    QVERIFY2(store.save("server", "user", items, images, &error),
             qPrintable(error));
    const auto snapshot = store.load("server", "user");

    QVERIFY(snapshot.has_value());
    QCOMPARE(snapshot->items.size(), 18);
    QCOMPARE(snapshot->items.at(0).overview,
             QStringLiteral("Overview for roundtrip-0"));
    QCOMPARE(snapshot->images.size(), 18);
}

void PosterWallSnapshotStoreTest::isolatesSnapshotsByUser()
{
    QTemporaryDir root;
    PosterWallSnapshotStore store(root.path());
    const auto [firstItems, firstImages] = makeSnapshot(QStringLiteral("first"));
    const auto [secondItems, secondImages] =
        makeSnapshot(QStringLiteral("second"));

    QVERIFY(store.save("server", "user-a", firstItems, firstImages));
    QVERIFY(store.save("server", "user-b", secondItems, secondImages));

    QCOMPARE(store.load("server", "user-a")->items.first().id,
             QStringLiteral("first-0"));
    QCOMPARE(store.load("server", "user-b")->items.first().id,
             QStringLiteral("second-0"));
}

void PosterWallSnapshotStoreTest::rejectsSnapshotWithMissingImage()
{
    QTemporaryDir root;
    PosterWallSnapshotStore store(root.path());
    const auto [items, images] = makeSnapshot(QStringLiteral("missing"));

    QVERIFY(store.save("server", "user", items, images));
    QDirIterator files(root.path(), {QStringLiteral("*.png")},
                       QDir::Files, QDirIterator::Subdirectories);
    QVERIFY(files.hasNext());
    QVERIFY(QFile::remove(files.next()));
    QVERIFY(!store.load("server", "user").has_value());
}

void PosterWallSnapshotStoreTest::failedSavePreservesPreviousSnapshot()
{
    QTemporaryDir root;
    PosterWallSnapshotStore store(root.path());
    const auto [previousItems, previousImages] =
        makeSnapshot(QStringLiteral("previous"));
    QVERIFY(store.save("server", "user", previousItems, previousImages));

    const MediaItem incomplete = makeItem("incomplete", "Incomplete");
    QVERIFY(!store.save("server", "user", {incomplete}, {}));

    const auto snapshot = store.load("server", "user");
    QVERIFY(snapshot.has_value());
    QCOMPARE(snapshot->items.first().id, QStringLiteral("previous-0"));
}

void PosterWallSnapshotStoreTest::successfulReplacementRemovesOldGeneration()
{
    QTemporaryDir root;
    PosterWallSnapshotStore store(root.path());
    const auto [firstItems, firstImages] = makeSnapshot(QStringLiteral("first"));
    const auto [secondItems, secondImages] =
        makeSnapshot(QStringLiteral("second"));
    QVERIFY(store.save("server", "user", firstItems, firstImages));
    QVERIFY(store.save("server", "user", secondItems, secondImages));

    int generationDirectories = 0;
    QDirIterator directories(root.path(), {QStringLiteral("generation-*")},
                             QDir::Dirs | QDir::NoDotAndDotDot,
                             QDirIterator::Subdirectories);
    while (directories.hasNext()) {
        directories.next();
        ++generationDirectories;
    }
    QCOMPARE(generationDirectories, 1);
    QCOMPARE(store.load("server", "user")->items.first().id,
             QStringLiteral("second-0"));
}

void PosterWallSnapshotStoreTest::loadRemovesOrphanGenerationWithoutTouchingActiveSnapshot()
{
    QTemporaryDir root;
    PosterWallSnapshotStore store(root.path());
    const auto [items, images] = makeSnapshot(QStringLiteral("active"));
    QVERIFY(store.save("server", "user", items, images));

    QDirIterator activeFiles(root.path(), {QStringLiteral("active.json")},
                             QDir::Files, QDirIterator::Subdirectories);
    QVERIFY(activeFiles.hasNext());
    const QString accountPath = QFileInfo(activeFiles.next()).absolutePath();
    const QString orphanPath =
        QDir(accountPath).filePath(QStringLiteral("generation-orphan"));
    QVERIFY(QDir().mkpath(orphanPath));
    QFile orphanFile(QDir(orphanPath).filePath(QStringLiteral("orphan.png")));
    QVERIFY(orphanFile.open(QIODevice::WriteOnly));
    QVERIFY(orphanFile.write("orphan") > 0);
    orphanFile.close();

    const auto snapshot = store.load("server", "user");

    QVERIFY(snapshot.has_value());
    QCOMPARE(snapshot->items.first().id, QStringLiteral("active-0"));
    QVERIFY(!QDir(orphanPath).exists());
    const QStringList generations = QDir(accountPath).entryList(
        {QStringLiteral("generation-*")}, QDir::Dirs | QDir::NoDotAndDotDot);
    QCOMPARE(generations.size(), 1);
}

void PosterWallSnapshotStoreTest::loadRemovesAllGenerationsWhenActiveManifestIsMissing()
{
    QTemporaryDir root;
    PosterWallSnapshotStore store(root.path());
    const auto [items, images] = makeSnapshot(QStringLiteral("abandoned"));
    QVERIFY(store.save("server", "user", items, images));

    QDirIterator activeFiles(root.path(), {QStringLiteral("active.json")},
                             QDir::Files, QDirIterator::Subdirectories);
    QVERIFY(activeFiles.hasNext());
    const QString activePath = activeFiles.next();
    const QString accountPath = QFileInfo(activePath).absolutePath();
    QVERIFY(QFile::remove(activePath));

    QVERIFY(!store.load("server", "user").has_value());
    const QStringList generations = QDir(accountPath).entryList(
        {QStringLiteral("generation-*")}, QDir::Dirs | QDir::NoDotAndDotDot);
    QVERIFY(generations.isEmpty());
}

void PosterWallSnapshotStoreTest::rejectsGenerationPathTraversal()
{
    QTemporaryDir root;
    PosterWallSnapshotStore store(root.path());
    const auto [items, images] = makeSnapshot(QStringLiteral("traversal"));
    QVERIFY(store.save("server", "user", items, images));

    QDirIterator activeFiles(root.path(), {QStringLiteral("active.json")},
                             QDir::Files, QDirIterator::Subdirectories);
    QVERIFY(activeFiles.hasNext());
    const QString activePath = activeFiles.next();
    const QString accountPath = QFileInfo(activePath).absolutePath();

    QFile activeFile(activePath);
    QVERIFY(activeFile.open(QIODevice::ReadOnly));
    QJsonObject active =
        QJsonDocument::fromJson(activeFile.readAll()).object();
    activeFile.close();
    const QString generation = active.value(QStringLiteral("generation"))
                                   .toString();
    QVERIFY(!generation.isEmpty());

    const QString externalPath = QDir(root.path()).filePath("external");
    QVERIFY(QDir().rename(QDir(accountPath).filePath(generation),
                          externalPath));
    active.insert(QStringLiteral("generation"),
                  QStringLiteral("../external"));
    QVERIFY(activeFile.open(QIODevice::WriteOnly | QIODevice::Truncate));
    QVERIFY(activeFile.write(QJsonDocument(active).toJson(
                QJsonDocument::Compact)) > 0);
    activeFile.close();

    QVERIFY(!store.load("server", "user").has_value());
    QVERIFY(QDir(externalPath).exists());
}

void PosterWallSnapshotStoreTest::rejectsImagePathTraversal()
{
    QTemporaryDir root;
    PosterWallSnapshotStore store(root.path());
    const auto [items, images] = makeSnapshot(QStringLiteral("image-path"));
    QVERIFY(store.save("server", "user", items, images));

    QDirIterator manifests(root.path(), {QStringLiteral("snapshot.json")},
                           QDir::Files, QDirIterator::Subdirectories);
    QVERIFY(manifests.hasNext());
    const QString manifestPath = manifests.next();
    QFile manifestFile(manifestPath);
    QVERIFY(manifestFile.open(QIODevice::ReadOnly));
    QJsonObject manifest =
        QJsonDocument::fromJson(manifestFile.readAll()).object();
    manifestFile.close();

    const QString generationPath = QFileInfo(manifestPath).absolutePath();
    const QString externalPath =
        QDir(QFileInfo(generationPath).absolutePath()).filePath("external.png");
    QVERIFY(makeImage(Qt::red).save(externalPath, "PNG"));
    QJsonArray entries = manifest.value(QStringLiteral("items")).toArray();
    QJsonObject first = entries.first().toObject();
    first.insert(QStringLiteral("imageFile"), QStringLiteral("../external.png"));
    entries[0] = first;
    manifest.insert(QStringLiteral("items"), entries);

    QVERIFY(manifestFile.open(QIODevice::WriteOnly | QIODevice::Truncate));
    QVERIFY(manifestFile.write(QJsonDocument(manifest).toJson(
                QJsonDocument::Compact)) > 0);
    manifestFile.close();

    QVERIFY(!store.load("server", "user").has_value());
    QVERIFY(QFileInfo::exists(externalPath));
}

void PosterWallSnapshotStoreTest::rejectsSnapshotWhenRootWouldExceedStorageLimit()
{
    QTemporaryDir root;
    PosterWallSnapshotStore store(root.path());
    const auto [items, images] = makeSnapshot(QStringLiteral("oversized"));

    const QString unrelatedAccount = QDir(root.path()).filePath("historical");
    QVERIFY(QDir().mkpath(unrelatedAccount));
    QFile sparseFile(QDir(unrelatedAccount).filePath("oversized.bin"));
    QVERIFY(sparseFile.open(QIODevice::WriteOnly));
    QVERIFY(sparseFile.resize(129LL * 1024 * 1024));
    sparseFile.close();

    QString error;
    QVERIFY(!store.save("server", "user", items, images, &error));
    QCOMPARE(error, QStringLiteral("snapshot storage limit exceeded"));
    QVERIFY(!store.load("server", "user").has_value());
}

void PosterWallSnapshotStoreTest::rejectsIncompleteSnapshot()
{
    QTemporaryDir root;
    PosterWallSnapshotStore store(root.path());
    const auto [items, images] = makeSnapshot(QStringLiteral("short"), 17);
    QVERIFY(!store.save("server", "user", items, images));
}

void PosterWallSnapshotStoreTest::rejectsDuplicateItemIds()
{
    QTemporaryDir root;
    PosterWallSnapshotStore store(root.path());
    auto [items, images] = makeSnapshot(QStringLiteral("duplicate"));
    items[17].id = items[0].id;
    QVERIFY(!store.save("server", "user", items, images));
}

void PosterWallSnapshotStoreTest::maintenanceLimitsHistoricalAccounts()
{
    QTemporaryDir root;
    PosterWallSnapshotStore store(root.path());
    for (int index = 0; index < 10; ++index) {
        const auto [items, images] =
            makeSnapshot(QStringLiteral("account-%1").arg(index));
        QVERIFY(store.save("server", QStringLiteral("user-%1").arg(index),
                           items, images));
    }

    const auto accountDirectories = QDir(root.path()).entryList(
        QDir::Dirs | QDir::NoDotAndDotDot);
    QVERIFY(accountDirectories.size() <= 8);
}

QTEST_MAIN(PosterWallSnapshotStoreTest)
#include "posterwallsnapshotstore_test.moc"
