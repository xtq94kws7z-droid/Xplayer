#include "utils/taskssnapshotutils.h"

#include <QtTest>

class TasksSnapshotUtilsTest final : public QObject
{
    Q_OBJECT

private slots:
    void progressChangesOnlyDynamicKey();
    void triggerChangesStructureKey();
};

namespace
{

ScheduledTaskInfo makeTask()
{
    ScheduledTaskInfo task;
    task.id = QStringLiteral("task-1");
    task.name = QStringLiteral("Library scan");
    task.description = QStringLiteral("Scan media");
    task.category = QStringLiteral("Library");
    task.key = QStringLiteral("scan");
    task.state = QStringLiteral("Running");
    task.currentProgressPercentage = 25.0;
    task.triggers = QJsonArray{QJsonObject{{QStringLiteral("Type"), QStringLiteral("Daily")}}};
    return task;
}

}

void TasksSnapshotUtilsTest::progressChangesOnlyDynamicKey()
{
    const QList<ScheduledTaskInfo> first{makeTask()};
    QList<ScheduledTaskInfo> second = first;
    second[0].currentProgressPercentage = 26.0;

    QCOMPARE(TasksSnapshotUtils::structureKey(first),
             TasksSnapshotUtils::structureKey(second));
    QVERIFY(TasksSnapshotUtils::dynamicKey(first) !=
            TasksSnapshotUtils::dynamicKey(second));
}

void TasksSnapshotUtilsTest::triggerChangesStructureKey()
{
    const QList<ScheduledTaskInfo> first{makeTask()};
    QList<ScheduledTaskInfo> second = first;
    second[0].triggers = QJsonArray{
        QJsonObject{{QStringLiteral("Type"), QStringLiteral("Hourly")}}};

    QVERIFY(TasksSnapshotUtils::structureKey(first) !=
            TasksSnapshotUtils::structureKey(second));
}

QTEST_MAIN(TasksSnapshotUtilsTest)
#include "taskssnapshotutils_test.moc"
