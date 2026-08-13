#ifndef POSTERWALLSNAPSHOTSTORE_H
#define POSTERWALLSNAPSHOTSTORE_H

#include <QHash>
#include <QImage>
#include <QList>
#include <QString>
#include <models/media/mediaitem.h>
#include <optional>

struct PosterWallSnapshot
{
    QList<MediaItem> items;
    QHash<QString, QImage> images;
};

class PosterWallSnapshotStore
{
public:
    explicit PosterWallSnapshotStore(QString rootPath = {});

    std::optional<PosterWallSnapshot> load(const QString& serverId,
                                           const QString& userId) const;
    bool save(const QString& serverId, const QString& userId,
              const QList<MediaItem>& items,
              const QHash<QString, QImage>& images,
              QString* errorString = nullptr) const;

private:
    QString m_rootPath;
};

#endif
