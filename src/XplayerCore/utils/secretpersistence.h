#ifndef SECRETPERSISTENCE_H
#define SECRETPERSISTENCE_H

#include "../XplayerCore_global.h"

#include <QString>

namespace SecretPersistence
{

XPLAYERCORE_EXPORT QString encryptToBase64(const QString &secret);
XPLAYERCORE_EXPORT QString decryptFromBase64(const QString &cipherBase64);

}

#endif
