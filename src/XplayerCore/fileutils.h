#ifndef FILE_UTILS_H
#define FILE_UTILS_H

#include <QString>
#include <QtGlobal>
#include "XplayerCore_global.h"


namespace FileUtils {


XPLAYERCORE_EXPORT QString formatSize(qint64 bytes);


XPLAYERCORE_EXPORT qint64 calcDirSize(const QString &path);

} 

#endif 
