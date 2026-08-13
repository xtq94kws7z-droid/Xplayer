#include "views/media/medialistmodel.h"

#include <QPixmap>
#include <QtTest>

class MediaListModelTest final : public QObject
{
    Q_OBJECT

private slots:
    void cacheHitUpdatesUsageWithoutReorderingStorage();
    void scrollingDefersNonPriorityImageNotificationsUntilIdle();
    void setItemsSkipsVisualRefreshWhenDisplayDataIsUnchanged();
    void setItemsRefreshesOnlyChangedDisplayRows();
    void preloadedPosterIsAvailableOnFirstRead();
};

void MediaListModelTest::cacheHitUpdatesUsageWithoutReorderingStorage()
{
    MediaListModel model(400, nullptr);
    model.m_imageCache.insert(QStringLiteral("first"), QPixmap(8, 8));
    model.m_imageCache.insert(QStringLiteral("second"), QPixmap(8, 8));
    model.m_imageCacheLastUsed.insert(QStringLiteral("first"), 1);
    model.m_imageCacheLastUsed.insert(QStringLiteral("second"), 2);
    model.m_imageCacheUseTick = 2;

    model.touchCachedImage(QStringLiteral("first"));

    QCOMPARE(model.m_imageCacheLastUsed.value(QStringLiteral("first")), quint64(3));
    QCOMPARE(model.m_imageCacheLastUsed.value(QStringLiteral("second")), quint64(2));
    QCOMPARE(model.m_imageCache.size(), 2);
}

void MediaListModelTest::scrollingDefersNonPriorityImageNotificationsUntilIdle()
{
    MediaListModel model(400, nullptr);

    MediaItem first;
    first.id = QStringLiteral("first");
    MediaItem second;
    second.id = QStringLiteral("second");
    model.setItems({first, second});
    model.setPriorityRows({0});
    model.setScrollActive(true);

    QSignalSpy dataChangedSpy(
        &model,
        &QAbstractItemModel::dataChanged);

    model.queueImageDataChanged(QStringLiteral("second"));
    model.flushPendingImageDataChanges();
    QCOMPARE(dataChangedSpy.count(), 0);

    model.queueImageDataChanged(QStringLiteral("first"));
    model.flushPendingImageDataChanges();
    QCOMPARE(dataChangedSpy.count(), 1);
    QCOMPARE(dataChangedSpy.takeFirst().at(0).toModelIndex().row(), 0);

    model.setScrollActive(false);
    QCOMPARE(dataChangedSpy.count(), 1);
    QCOMPARE(dataChangedSpy.takeFirst().at(0).toModelIndex().row(), 1);
}

void MediaListModelTest::setItemsSkipsVisualRefreshWhenDisplayDataIsUnchanged()
{
    MediaListModel model(400, nullptr);
    MediaItem first;
    first.id = QStringLiteral("first");
    first.name = QStringLiteral("First");
    first.type = QStringLiteral("Movie");
    first.mediaType = QStringLiteral("Video");
    first.images.primaryTag = QStringLiteral("poster-first");
    MediaItem second;
    second.id = QStringLiteral("second");
    second.name = QStringLiteral("Second");
    second.type = QStringLiteral("Movie");
    second.mediaType = QStringLiteral("Video");
    second.images.primaryTag = QStringLiteral("poster-second");
    QList<MediaItem> items;
    items << first << second;
    model.setItems(items);

    QSignalSpy dataChangedSpy(&model, &QAbstractItemModel::dataChanged);

    MediaItem updatedFirst = first;
    updatedFirst.providerIds.insert(QStringLiteral("Imdb"),
                                    QStringLiteral("tt123"));
    MediaItem updatedSecond = second;
    updatedSecond.path = QStringLiteral("D:/media/second.mkv");

    QList<MediaItem> updatedItems;
    updatedItems << updatedFirst << updatedSecond;
    model.setItems(updatedItems);

    QCOMPARE(dataChangedSpy.count(), 0);
}

void MediaListModelTest::setItemsRefreshesOnlyChangedDisplayRows()
{
    MediaListModel model(400, nullptr);
    MediaItem first;
    first.id = QStringLiteral("first");
    first.name = QStringLiteral("First");
    first.type = QStringLiteral("Movie");
    first.mediaType = QStringLiteral("Video");
    first.images.primaryTag = QStringLiteral("poster-first");
    MediaItem second;
    second.id = QStringLiteral("second");
    second.name = QStringLiteral("Second");
    second.type = QStringLiteral("Movie");
    second.mediaType = QStringLiteral("Video");
    second.images.primaryTag = QStringLiteral("poster-second");
    QList<MediaItem> items;
    items << first << second;
    model.setItems(items);

    QSignalSpy dataChangedSpy(&model, &QAbstractItemModel::dataChanged);

    MediaItem updatedFirst = first;
    MediaItem updatedSecond = second;
    updatedSecond.name = QStringLiteral("Second Updated");

    QList<MediaItem> updatedItems;
    updatedItems << updatedFirst << updatedSecond;
    model.setItems(updatedItems);

    QCOMPARE(dataChangedSpy.count(), 1);
    const QList<QVariant> signal = dataChangedSpy.takeFirst();
    QCOMPARE(signal.at(0).toModelIndex().row(), 1);
    QCOMPARE(signal.at(1).toModelIndex().row(), 1);
}

void MediaListModelTest::preloadedPosterIsAvailableOnFirstRead()
{
    MediaListModel model(400, nullptr);
    MediaItem item;
    item.id = QStringLiteral("cached-poster");
    item.images.primaryTag = QStringLiteral("poster-tag");
    model.setItems({item});

    QPixmap poster(24, 36);
    poster.fill(Qt::magenta);
    model.setPreloadedPosterPixmaps({{item.id, poster}});

    const QPixmap firstRead =
        model.data(model.index(0, 0), MediaListModel::PosterPixmapRole)
            .value<QPixmap>();
    QVERIFY(!firstRead.isNull());
    QCOMPARE(firstRead.toImage().pixelColor(0, 0), QColor(Qt::magenta));
    QVERIFY(model.m_pendingImageRequests.isEmpty());
}

QTEST_MAIN(MediaListModelTest)
#include "medialistmodel_test.moc"
