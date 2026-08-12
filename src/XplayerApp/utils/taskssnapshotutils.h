#ifndef TASKSSNAPSHOTUTILS_H
#define TASKSSNAPSHOTUTILS_H

#include <models/admin/adminmodels.h>

namespace TasksSnapshotUtils
{

QString structureKey(const QList<ScheduledTaskInfo> &tasks);
QString dynamicKey(const QList<ScheduledTaskInfo> &tasks);

}

#endif
