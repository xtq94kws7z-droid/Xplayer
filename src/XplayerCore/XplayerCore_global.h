#ifndef XPLAYERCORE_GLOBAL_H
#define XPLAYERCORE_GLOBAL_H

#include <QtCore/qglobal.h>

#if defined(XPLAYERCORE_LIBRARY)
#define XPLAYERCORE_EXPORT Q_DECL_EXPORT
#else
#define XPLAYERCORE_EXPORT Q_DECL_IMPORT
#endif

#endif 
