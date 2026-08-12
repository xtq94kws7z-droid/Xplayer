#include "taskssnapshotutils.h"

#include <QJsonDocument>

namespace
{

void appendField(QString &key, const QString &value)
{
    key.append(QString::number(value.size()));
    key.append(QLatin1Char(':'));
    key.append(value);
    key.append(QLatin1Char('|'));
}

void appendBool(QString &key, bool value)
{
    appendField(key, value ? QStringLiteral("1") : QStringLiteral("0"));
}

void appendDouble(QString &key, double value)
{
    appendField(key, QString::number(value, 'f', 3));
}

}

namespace TasksSnapshotUtils
{

QString structureKey(const QList<ScheduledTaskInfo> &tasks)
{
    QString key;
    key.reserve(tasks.size() * 96 + 16);
    appendField(key, QString::number(tasks.size()));

    for (const ScheduledTaskInfo &task : tasks)
    {
        appendField(key, task.id);
        appendField(key, task.name);
        appendField(key, task.description);
        appendField(key, task.category);
        appendField(key, task.key);
        appendField(key, QString::fromUtf8(
                            QJsonDocument(task.triggers).toJson(QJsonDocument::Compact)));
        appendBool(key, task.isHidden);
        appendBool(key, task.isRunning());
    }

    return key;
}

QString dynamicKey(const QList<ScheduledTaskInfo> &tasks)
{
    QString key;
    key.reserve(tasks.size() * 80 + 16);
    appendField(key, QString::number(tasks.size()));

    for (const ScheduledTaskInfo &task : tasks)
    {
        appendField(key, task.id);
        appendField(key, task.state);
        appendDouble(key, task.currentProgressPercentage);
        appendField(key, task.lastExecutionResult);
        appendField(key, task.lastStartTime);
        appendField(key, task.lastEndTime);
    }

    return key;
}

}
