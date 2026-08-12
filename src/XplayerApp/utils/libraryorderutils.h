#ifndef LIBRARYORDERUTILS_H
#define LIBRARYORDERUTILS_H

#include <models/admin/adminmodels.h>

namespace LibraryOrderUtils
{

QList<VirtualFolder> applyOrder(QList<VirtualFolder> folders,
                                const QStringList &orderedIds);

}

#endif
