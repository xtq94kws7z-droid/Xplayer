#ifndef CONFIGSTORE_H
#define CONFIGSTORE_H

#include <QObject>
#include <QVariant>
#include <QString>
#include <QStringList>
#include <QHash>
#include <QSettings>
#include <QMutex>
#include <QMutexLocker>


#include "../XplayerCore_global.h"

class XPLAYERCORE_EXPORT ConfigStore : public QObject {
    Q_OBJECT
public:
    
    static ConfigStore* instance();

    
    template<typename T>
    T get(const QString& key, const T& defaultValue = T()) const {
        QMutexLocker locker(&m_mutex);
        const QString storageKey = canonicalStorageKey(key);
        const QStringList legacyKeys = legacyStorageKeys(key);

        
        if (m_cache.contains(key)) {
            return m_cache.value(key).value<T>();
        }
        if (m_cache.contains(storageKey)) {
            return m_cache.value(storageKey).value<T>();
        }
        for (const QString& legacyKey : legacyKeys) {
            if (m_cache.contains(legacyKey)) {
                return m_cache.value(legacyKey).value<T>();
            }
        }

        
        QVariant val = m_settings->value(storageKey);
        if (!val.isValid()) {
            for (const QString& legacyKey : legacyKeys) {
                val = m_settings->value(legacyKey);
                if (val.isValid()) {
                    break;
                }
            }
        }
        if (!val.isValid()) {
            val = QVariant::fromValue(defaultValue);
        }
        return val.value<T>();
    }

    
    void set(const QString& key, const QVariant& value);
    void setDeferred(const QString& key, const QVariant& value);
    void sync();
    QString filePath() const;
    QStringList allKeys() const;

signals:
    
    void valueChanged(const QString& key, const QVariant& newValue);

private:
    explicit ConfigStore(QObject* parent = nullptr);
    ~ConfigStore() override;
    static QString canonicalStorageKey(const QString& key);
    static QStringList legacyStorageKeys(const QString& key);
    void migrateLegacyGeneralSettings();
    bool updateValueLocked(const QString& key, const QVariant& value);

    
    Q_DISABLE_COPY(ConfigStore)

    QSettings* m_settings;
    class QTimer* m_syncTimer = nullptr;
    mutable QMutex m_mutex;             
    QHash<QString, QVariant> m_cache;   
};

#endif 
