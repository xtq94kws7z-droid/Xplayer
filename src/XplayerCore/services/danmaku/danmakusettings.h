#ifndef DANMAKUSETTINGS_H
#define DANMAKUSETTINGS_H

#include "../../models/danmaku/danmakumodels.h"

#include <QList>
#include <QString>

class XPLAYERCORE_EXPORT DanmakuSettings final
{
public:
    static DanmakuServerDefinition builtInOfficialDandanplayServer();
    static QList<DanmakuServerDefinition> loadServers(QString serverId);
    static DanmakuServerDefinition selectedServer(QString serverId);
    static QString selectedServerId(QString serverId);
    static void saveServers(QString serverId,
                            QList<DanmakuServerDefinition> servers,
                            QString selectedServerId);
};

#endif 
