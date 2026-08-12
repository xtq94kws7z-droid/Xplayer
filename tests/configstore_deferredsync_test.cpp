#include <QFile>
#include <QSettings>
#include <QStandardPaths>
#include <QtTest/QtTest>

#include "config/configstore.h"

class ConfigStoreDeferredSyncTest : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void immediateValuePersistsWithoutBlocking();
    void deferredValueIsImmediatelyReadableAndEventuallyPersisted();
};

void ConfigStoreDeferredSyncTest::initTestCase()
{
    QStandardPaths::setTestModeEnabled(true);
    QCoreApplication::setOrganizationName(QStringLiteral("XplayerTests"));
    QCoreApplication::setApplicationName(
        QStringLiteral("configstore-deferred-%1")
            .arg(QCoreApplication::applicationPid()));
}

void ConfigStoreDeferredSyncTest::deferredValueIsImmediatelyReadableAndEventuallyPersisted()
{
    ConfigStore *store = ConfigStore::instance();
    const QString key = QStringLiteral("test/deferred-value");

    store->setDeferred(key, 42);
    QCOMPARE(store->get<int>(key), 42);

    QTRY_VERIFY_WITH_TIMEOUT(QFile::exists(store->filePath()), 1500);
    QTRY_COMPARE_WITH_TIMEOUT(
        QSettings(store->filePath(), QSettings::IniFormat).value(key).toInt(),
        42, 1500);
}

void ConfigStoreDeferredSyncTest::immediateValuePersistsWithoutBlocking()
{
    ConfigStore *store = ConfigStore::instance();
    const QString key = QStringLiteral("test/immediate-value");

    store->set(key, 17);

    QCOMPARE(store->get<int>(key), 17);
    QCOMPARE(QSettings(store->filePath(), QSettings::IniFormat)
                 .value(key).toInt(),
             17);
}

QTEST_MAIN(ConfigStoreDeferredSyncTest)
#include "configstore_deferredsync_test.moc"
