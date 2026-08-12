#pragma once

#include <QList>
#include <QPair>

namespace ModelNotificationUtils
{
QList<QPair<int, int>> coalescedRowRanges(QList<int> rows);
}
