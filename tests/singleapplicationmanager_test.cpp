#include "managers/singleapplicationmanager.h"

#include <QCoreApplication>
#include <QDir>
#include <QFuture>
#include <QSignalSpy>
#include <QtConcurrent>
#include <QtTest>

class SingleApplicationManagerTest final : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void forwardsArgumentsToPrimaryInstance();
};

void SingleApplicationManagerTest::initTestCase()
{
    QCoreApplication::setOrganizationName(QStringLiteral("XplayerTests"));
    QCoreApplication::setApplicationName(
        QStringLiteral("single-application-%1")
            .arg(QCoreApplication::applicationPid()));
}

void SingleApplicationManagerTest::forwardsArgumentsToPrimaryInstance()
{
    SingleApplicationManager primary;
    QCOMPARE(primary.start({QStringLiteral("Xplayer.exe")}, QDir::currentPath()),
             SingleApplicationManager::StartResult::PrimaryInstance);

    QSignalSpy activationSpy(
        &primary, &SingleApplicationManager::activationRequested);
    QVERIFY(activationSpy.isValid());

    const QStringList forwardedArguments = {
        QStringLiteral("Xplayer.exe"),
        QStringLiteral("D:/Media/My Movie.mkv"),
        QStringLiteral("--fullscreen")};
    const QString forwardedWorkingDirectory =
        QStringLiteral("D:/Media");

    QFuture<SingleApplicationManager::StartResult> secondaryResult =
        QtConcurrent::run([forwardedArguments, forwardedWorkingDirectory]() {
            SingleApplicationManager secondary;
            return secondary.start(forwardedArguments,
                                   forwardedWorkingDirectory);
        });

    QTRY_VERIFY_WITH_TIMEOUT(secondaryResult.isFinished(), 3000);
    QCOMPARE(secondaryResult.result(),
             SingleApplicationManager::StartResult::SecondaryInstance);
    QTRY_COMPARE_WITH_TIMEOUT(activationSpy.size(), 1, 3000);

    const QList<QVariant> request = activationSpy.takeFirst();
    QCOMPARE(request.at(0).toStringList(), forwardedArguments);
    QCOMPARE(request.at(1).toString(), forwardedWorkingDirectory);
}

QTEST_GUILESS_MAIN(SingleApplicationManagerTest)
#include "singleapplicationmanager_test.moc"
