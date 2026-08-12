#include "libraryorderutils.h"

#include <QHash>
#include <algorithm>

namespace LibraryOrderUtils
{

QList<VirtualFolder> applyOrder(QList<VirtualFolder> folders,
                                const QStringList &orderedIds)
{
    if (folders.isEmpty() || orderedIds.isEmpty())
    {
        return folders;
    }

    QHash<QString, int> orderIndex;
    orderIndex.reserve(orderedIds.size());
    for (int index = 0; index < orderedIds.size(); ++index)
    {
        if (!orderIndex.contains(orderedIds.at(index)))
        {
            orderIndex.insert(orderedIds.at(index), index);
        }
    }

    const int unmatchedRank = orderedIds.size();
    std::stable_sort(
        folders.begin(), folders.end(),
        [&orderIndex, unmatchedRank](const VirtualFolder &lhs,
                                     const VirtualFolder &rhs) {
            const int leftRank = orderIndex.value(lhs.itemId, unmatchedRank);
            const int rightRank = orderIndex.value(rhs.itemId, unmatchedRank);
            return leftRank < rightRank;
        });
    return folders;
}

}
