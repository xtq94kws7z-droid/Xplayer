#include "secretpersistence.h"

#include "securesecretbox.h"

#include <QByteArray>

namespace SecretPersistence
{

QString encryptToBase64(const QString &secret)
{
    if (secret.isEmpty())
    {
        return {};
    }

    QByteArray plain = secret.toUtf8();
    const QByteArray cipher = SecureSecretBox::encryptLocalSecret(plain);
    SecureSecretBox::secureZero(plain);

    if (cipher.isEmpty())
    {
        return {};
    }
    return QString::fromLatin1(cipher.toBase64(QByteArray::Base64Encoding));
}

QString decryptFromBase64(const QString &cipherBase64)
{
    if (cipherBase64.isEmpty())
    {
        return {};
    }

    const QByteArray cipher = QByteArray::fromBase64(
        cipherBase64.toLatin1(), QByteArray::Base64Encoding);
    if (cipher.isEmpty())
    {
        return {};
    }

    auto plain = SecureSecretBox::decryptLocalSecret(cipher);
    if (!plain.has_value())
    {
        return {};
    }

    QString secret = QString::fromUtf8(plain.value());
    SecureSecretBox::secureZero(plain.value());
    return secret;
}

}
