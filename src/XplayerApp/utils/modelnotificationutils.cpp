#include "modelnotificationutils.h"

#include <algorithm>

namespace ModelNotificationUtils
{

QList<QPair<int, int>> coalescedRowRanges(QList<int> rows)
{
    for (int i = rows.size() - 1; i >= 0; --i) {
        if (rows.at(i) < 0) {
            rows.removeAt(i);
        }
    }
    std::sort(rows.begin(), rows.end());
    rows.erase(std::unique(rows.begin(), rows.end()), rows.end());

    QList<QPair<int, int>> ranges;
    if (rows.isEmpty()) {
        return ranges;
    }

    int first = rows.first();
    int last = first;
    for (int i = 1; i < rows.size(); ++i) {
        const int row = rows.at(i);
        if (row == last + 1) {
            last = row;
            continue;
        }

        ranges.append(QPair<int, int>(first, last));
        first = row;
        last = row;
    }

    ranges.append(QPair<int, int>(first, last));
    return ranges;
}

}
