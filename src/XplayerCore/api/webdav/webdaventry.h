#ifndef WEBDAVENTRY_H
#define WEBDAVENTRY_H

#include "../../XplayerCore_global.h"

#include <QDateTime>
#include <QString>







struct XPLAYERCORE_EXPORT WebdavEntry
{
    
    
    
    QString href;

    
    QString displayName;

    
    
    QString parentRelPath;

    
    
    bool isCollection = false;

    
    qint64 contentLength = -1;

    
    QDateTime lastModified;

    
    QString contentType;

    
    QString etag;
};

#endif 
